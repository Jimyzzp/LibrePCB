/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "commandlineinterface.h"

#include <librepcb/core/application.h>
#include <librepcb/core/attribute/attributesubstitutor.h>
#include <librepcb/core/debug.h>
#include <librepcb/core/export/bom.h>
#include <librepcb/core/export/bomcsvwriter.h>
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/export/pickplacecsvwriter.h>
#include <librepcb/core/fileio/csvfile.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardd356netlistexport.h>
#include <librepcb/core/project/board/boardfabricationoutputsettings.h>
#include <librepcb/core/project/board/boardgerberexport.h>
#include <librepcb/core/project/board/boardpickplacegenerator.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/bomgenerator.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace cli {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CommandLineInterface::CommandLineInterface() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int CommandLineInterface::execute(const QStringList& args) noexcept {
  QStringList positionalArgNames;
  QMap<QString, QPair<QString, QString>> commands = {
      {"open-project",
       {tr("Open a project to execute project-related tasks."),
        tr("open-project [command_options]")}},
      {"open-library",
       {tr("Open a library to execute library-related tasks."),
        tr("open-library [command_options]")}},
  };

  // Add global options
  QCommandLineParser parser;
  parser.setApplicationDescription(tr("LibrePCB Command Line Interface"));
  // Don't use the built-in addHelpOption() since it also adds the "--help-all"
  // option which we don't need, and the OS-dependent option "-?".
  const QCommandLineOption helpOption({"h", "help"}, tr("Print this message."));
  parser.addOption(helpOption);
  const QCommandLineOption versionOption({"V", "version"},
                                         tr("Displays version information."));
  parser.addOption(versionOption);
  QCommandLineOption verboseOption({"v", "verbose"}, tr("Verbose output."));
  parser.addOption(verboseOption);
  parser.addPositionalArgument("command",
                               tr("The command to execute (see list below)."));
  positionalArgNames.append("command");

  // Define options for "open-project"
  QCommandLineOption ercOption(
      "erc",
      tr("Run the electrical rule check, print all non-approved "
         "warnings/errors and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption drcOption(
      "drc",
      tr("Run the design rule check, print all non-approved warnings/errors "
         "and report failure (exit code = 1) if there are non-approved "
         "messages."));
  QCommandLineOption drcSettingsOption(
      "drc-settings",
      tr("Override DRC settings by providing a *.lp file containing custom "
         "settings. If not set, the settings from the boards will be used "
         "instead."),
      tr("file"));
  QCommandLineOption exportSchematicsOption(
      "export-schematics",
      tr("Export schematics to given file(s). Existing files will be "
         "overwritten. Supported file extensions: %1")
          .arg(GraphicsExport::getSupportedExtensions().join(", ")),
      tr("file"));
  QCommandLineOption exportBomOption(
      "export-bom",
      tr("Export generic BOM to given file(s). Existing files will be "
         "overwritten. Supported file extensions: %1")
          .arg("csv"),
      tr("file"));
  QCommandLineOption exportBoardBomOption(
      "export-board-bom",
      tr("Export board-specific BOM to given file(s). Existing files "
         "will be overwritten. Supported file extensions: %1")
          .arg("csv"),
      tr("file"));
  QCommandLineOption bomAttributesOption(
      "bom-attributes",
      tr("Comma-separated list of additional attributes to be exported "
         "to the BOM. Example: \"%1\"")
          .arg("MANUFACTURER, MPN"),
      tr("attributes"));
  QCommandLineOption exportPcbFabricationDataOption(
      "export-pcb-fabrication-data",
      tr("Export PCB fabrication data (Gerber/Excellon) according the "
         "fabrication "
         "output settings of boards. Existing files will be overwritten."));
  QCommandLineOption pcbFabricationSettingsOption(
      "pcb-fabrication-settings",
      tr("Override PCB fabrication output settings by providing a *.lp file "
         "containing custom settings. If not set, the settings from the boards "
         "will be used instead."),
      tr("file"));
  QCommandLineOption exportPnpTopOption(
      "export-pnp-top",
      tr("Export pick&place file for automated assembly of the top board side. "
         "Existing files will be overwritten. Supported file extensions: %1")
          .arg("csv, gbr"),
      tr("file"));
  QCommandLineOption exportPnpBottomOption(
      "export-pnp-bottom",
      tr("Export pick&place file for automated assembly of the bottom board "
         "side. Existing files will be overwritten. Supported file extensions: "
         "%1")
          .arg("csv, gbr"),
      tr("file"));
  QCommandLineOption exportNetlistOption(
      "export-netlist",
      tr("Export netlist file for automated PCB testing. Existing files will "
         "be overwritten. Supported file extensions: %1")
          .arg("d356"),
      tr("file"));
  QCommandLineOption boardOption("board",
                                 tr("The name of the board(s) to export. Can "
                                    "be given multiple times. If not set, "
                                    "all boards are exported."),
                                 tr("name"));
  QCommandLineOption boardIndexOption("board-index",
                                      tr("Same as '%1', but allows to specify "
                                         "boards by index instead of by name.")
                                          .arg("--board"),
                                      tr("index"));
  QCommandLineOption removeOtherBoardsOption(
      "remove-other-boards",
      tr("Remove all boards not specified with '%1' from the project before "
         "executing all the other actions. If '%1' is not passed, all boards "
         "will be removed. Pass '%2' to save the modified project to disk.")
          .arg("--board[-index]")
          .arg("--save"));
  QCommandLineOption saveOption(
      "save",
      tr("Save project before closing it (useful to upgrade file format)."));
  QCommandLineOption prjStrictOption(
      "strict",
      tr("Fail if the project files are not strictly canonical, i.e. "
         "there would be changes when saving the project. Note that "
         "this option is not available for *.lppz files."));

  // Define options for "open-library"
  QCommandLineOption libAllOption(
      "all",
      tr("Perform the selected action(s) on all elements contained in "
         "the opened library."));
  QCommandLineOption libCheckOption(
      "check",
      tr("Run the library element check, print all non-approved messages and "
         "report failure (exit code = 1) if there are non-approved messages."));
  QCommandLineOption libSaveOption(
      "save",
      tr("Save library (and contained elements if '--all' is given) "
         "before closing them (useful to upgrade file format)."));
  QCommandLineOption libStrictOption(
      "strict",
      tr("Fail if the opened files are not strictly canonical, i.e. "
         "there would be changes when saving the library elements."));

  // Build help text.
  const QString executable = args.value(0);
  QString helpText = parser.helpText() % "\n" % tr("Commands:") % "\n";
  for (auto it = commands.constBegin(); it != commands.constEnd(); ++it) {
    helpText += "  " % it.key().leftJustified(15) % it.value().first % "\n";
  }
  helpText += "\n" % tr("List command-specific options:") % "\n  " %
      executable % " <command> --help";
  QString usageHelpText = helpText.split("\n").value(0);
  const QString helpCommandTextPrefix = tr("Help:") % " ";
  QString helpCommandText = helpCommandTextPrefix % executable % " --help";

  // First parse to get the supplied command (ignoring errors because the parser
  // does not yet know the command-dependent options).
  parser.parse(args);

  // Add command-dependent options
  const QString command = parser.positionalArguments().value(0);
  parser.clearPositionalArguments();
  if (command == "open-project") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("project",
                                 tr("Path to project file (*.lpp[z])."));
    positionalArgNames.append("project");
    parser.addOption(ercOption);
    parser.addOption(drcOption);
    parser.addOption(drcSettingsOption);
    parser.addOption(exportSchematicsOption);
    parser.addOption(exportBomOption);
    parser.addOption(exportBoardBomOption);
    parser.addOption(bomAttributesOption);
    parser.addOption(exportPcbFabricationDataOption);
    parser.addOption(pcbFabricationSettingsOption);
    parser.addOption(exportPnpTopOption);
    parser.addOption(exportPnpBottomOption);
    parser.addOption(exportNetlistOption);
    parser.addOption(boardOption);
    parser.addOption(boardIndexOption);
    parser.addOption(removeOtherBoardsOption);
    parser.addOption(saveOption);
    parser.addOption(prjStrictOption);
  } else if (command == "open-library") {
    parser.addPositionalArgument(command, commands[command].first,
                                 commands[command].second);
    parser.addPositionalArgument("library",
                                 tr("Path to library directory (*.lplib)."));
    positionalArgNames.append("library");
    parser.addOption(libAllOption);
    parser.addOption(libCheckOption);
    parser.addOption(libSaveOption);
    parser.addOption(libStrictOption);
  } else if (!command.isEmpty()) {
    printErr(tr("Unknown command '%1'.").arg(command));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // If a command is given, make the help texts command-specific now.
  if (!command.isEmpty()) {
    helpText = parser.helpText().trimmed();  // Remove the list of commands.
    usageHelpText = helpText.split("\n").value(0);
    helpCommandText =
        helpCommandTextPrefix % executable % " " % command % " --help";
  }

  // Parse the actual command line arguments given by the user
  if (!parser.parse(args)) {
    printErr(parser.errorText());
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // --verbose
  if (parser.isSet(verboseOption)) {
    Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::All);
  }

  // --help (also shown if no arguments supplied)
  if (parser.isSet(helpOption) || (args.count() <= 1)) {
    print(helpText);
    return 0;
  }

  // --version
  if (parser.isSet(versionOption)) {
    print(tr("LibrePCB CLI Version %1").arg(Application::getVersion()));
    print(
        tr("File Format %1").arg(Application::getFileFormatVersion().toStr()) %
        " " %
        (Application::isFileFormatStable() ? tr("(stable)")
                                           : tr("(unstable)")));
    print(tr("Git Revision %1").arg(Application::getGitRevision()));
    print(tr("Qt Version %1 (compiled against %2)")
              .arg(qVersion(), QT_VERSION_STR));
    print(tr("Built at %1")
              .arg(Application::getBuildDate().toString(Qt::LocalDate)));
    return 0;
  }

  // Check number of passed positional command arguments.
  const QStringList positionalArgs = parser.positionalArguments();
  if (positionalArgs.count() < positionalArgNames.count()) {
    const QStringList names = positionalArgNames.mid(positionalArgs.count());
    printErr(tr("Missing arguments:") % " " % names.join(" "));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  } else if (positionalArgs.count() > positionalArgNames.count()) {
    const QStringList args = positionalArgs.mid(positionalArgNames.count());
    printErr(tr("Unknown arguments:") % " " % args.join(" "));
    printErr(usageHelpText);
    printErr(helpCommandText);
    return 1;
  }

  // Execute command
  bool cmdSuccess = false;
  if (command == "open-project") {
    cmdSuccess = openProject(
        positionalArgs.value(1),  // project filepath
        parser.isSet(ercOption),  // run ERC
        parser.isSet(drcOption),  // run DRC
        parser.value(drcSettingsOption),  // DRC settings
        parser.values(exportSchematicsOption),  // export schematics
        parser.values(exportBomOption),  // export generic BOM
        parser.values(exportBoardBomOption),  // export board BOM
        parser.value(bomAttributesOption),  // BOM attributes
        parser.isSet(exportPcbFabricationDataOption),  // export PCB fab. data
        parser.value(pcbFabricationSettingsOption),  // PCB fab. settings
        parser.values(exportPnpTopOption),  // export PnP top
        parser.values(exportPnpBottomOption),  // export PnP bottom
        parser.values(exportNetlistOption),  // export netlist
        parser.values(boardOption),  // board names
        parser.values(boardIndexOption),  // board indices
        parser.isSet(removeOtherBoardsOption),  // remove other boards
        parser.isSet(saveOption),  // save project
        parser.isSet(prjStrictOption)  // strict mode
    );
  } else if (command == "open-library") {
    cmdSuccess = openLibrary(positionalArgs.value(1),  // library directory
                             parser.isSet(libAllOption),  // all elements
                             parser.isSet(libCheckOption),  // run check
                             parser.isSet(libSaveOption),  // save
                             parser.isSet(libStrictOption)  // strict mode
    );
  } else {
    printErr("Internal failure.");  // No tr() because this cannot occur.
  }
  if (cmdSuccess) {
    print(tr("SUCCESS"));
    return 0;
  } else {
    print(tr("Finished with errors!"));
    return 1;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool CommandLineInterface::openProject(
    const QString& projectFile, bool runErc, bool runDrc,
    const QString& drcSettingsPath, const QStringList& exportSchematicsFiles,
    const QStringList& exportBomFiles, const QStringList& exportBoardBomFiles,
    const QString& bomAttributes, bool exportPcbFabricationData,
    const QString& pcbFabricationSettingsPath,
    const QStringList& exportPnpTopFiles,
    const QStringList& exportPnpBottomFiles,
    const QStringList& exportNetlistFiles, const QStringList& boardNames,
    const QStringList& boardIndices, bool removeOtherBoards, bool save,
    bool strict) const noexcept {
  try {
    bool success = true;
    QMap<FilePath, int> writtenFilesCounter;

    // Open project
    FilePath projectFp(QFileInfo(projectFile).absoluteFilePath());
    print(tr("Open project '%1'...").arg(prettyPath(projectFp, projectFile)));
    std::shared_ptr<TransactionalFileSystem> projectFs;
    QString projectFileName;
    if (projectFp.getSuffix() == "lppz") {
      projectFs = TransactionalFileSystem::openRO(projectFp.getParentDir());
      projectFs->removeDirRecursively();  // 1) get a clean initial state
      projectFs->loadFromZip(projectFp);  // 2) load files from ZIP
      foreach (const QString& fn, projectFs->getFiles()) {
        if (fn.endsWith(".lpp")) {
          projectFileName = fn;
        }
      }
    } else {
      projectFs = TransactionalFileSystem::open(projectFp.getParentDir(), save);
      projectFileName = projectFp.getFilename();
    }
    ProjectLoader loader;
    std::unique_ptr<Project> project =
        loader.open(std::unique_ptr<TransactionalDirectory>(
                        new TransactionalDirectory(projectFs)),
                    projectFileName);  // can throw
    if (auto messages = loader.getUpgradeMessages()) {
      print(tr("Attention: Project has been upgraded to a newer file format!"));
      std::sort(messages->begin(), messages->end(),
                [](const FileFormatMigration::Message& a,
                   const FileFormatMigration::Message& b) {
                  if (a.severity > b.severity) return true;
                  if (a.message < b.message) return true;
                  return false;
                });
      foreach (const auto& msg, *messages) {
        const QString multiplier = msg.affectedItems > 0
            ? QString(" (%1x)").arg(msg.affectedItems)
            : "";
        print(QString(" - %1%2: %3")
                  .arg(msg.getSeverityStrTr())
                  .arg(multiplier)
                  .arg(msg.message));
      }
    }

    // Parse list of boards.
    QList<Board*> boards;
    foreach (const QString& boardName, boardNames) {
      if (Board* board = project->getBoardByName(boardName)) {
        if (!boards.contains(board)) {
          boards.append(board);
        }
      } else {
        printErr(
            tr("ERROR: No board with the name '%1' found.").arg(boardName));
        success = false;
      }
    }
    foreach (const QString& boardIndex, boardIndices) {
      bool ok = false;
      const int index = boardIndex.trimmed().toInt(&ok);
      Board* board = project->getBoardByIndex(index);
      if (ok && board) {
        if (!boards.contains(board)) {
          boards.append(board);
        }
      } else {
        printErr(tr("ERROR: Board index '%1' is invalid.").arg(boardIndex));
        success = false;
      }
    }

    // Remove other boards (note: do this at the very beginning to make all
    // the other commands, e.g. the ERC, working without the removed boards).
    if (removeOtherBoards) {
      print(tr("Remove other boards..."));
      foreach (Board* board, project->getBoards()) {
        if (!boards.contains(board)) {
          print(QString("  - '%1'").arg(*board->getName()));
          project->removeBoard(*board);
        }
      }
    }

    // If no boards are specified, export all boards.
    if (boardNames.isEmpty() && boardIndices.isEmpty()) {
      boards = project->getBoards();
    }

    // Check for non-canonical files (strict mode)
    if (strict) {
      print(tr("Check for non-canonical files..."));
      if (projectFp.getSuffix() == "lppz") {
        printErr("  " %
                 tr("ERROR: The option '--strict' is not available for "
                    "*.lppz files!"));
        success = false;
      } else {
        project->save();  // can throw
        QStringList paths = projectFs->checkForModifications();  // can throw
        // ignore user config files
        paths = paths.filter(QRegularExpression("^((?!\\.user\\.lp).)*$"));
        // sort file paths to increases readability of console output
        std::sort(paths.begin(), paths.end());
        foreach (const QString& path, paths) {
          printErr(
              QString("    - Non-canonical file: '%1'")
                  .arg(prettyPath(projectFs->getAbsPath(path), projectFile)));
        }
        if (paths.count() > 0) {
          success = false;
        }
      }
    }

    // ERC
    if (runErc) {
      print(tr("Run ERC..."));
      ElectricalRuleCheck erc(*project);
      int approvedMsgCount = 0;
      const RuleCheckMessageList messages = erc.runChecks();
      const QStringList nonApproved = prepareRuleCheckMessages(
          messages, project->getErcMessageApprovals(), approvedMsgCount);
      print("  " % tr("Approved messages: %1").arg(approvedMsgCount));
      print("  " % tr("Non-approved messages: %1").arg(nonApproved.count()));
      foreach (const QString& msg, nonApproved) {
        printErr("    - " % msg);
        success = false;
      }
    }

    // DRC
    if (runDrc) {
      print(tr("Run DRC..."));
      tl::optional<BoardDesignRuleCheckSettings> customSettings;
      QList<Board*> boardsToCheck = boards;
      if (!drcSettingsPath.isEmpty()) {
        try {
          qDebug() << "Load custom DRC settings:" << drcSettingsPath;
          const FilePath fp(QFileInfo(drcSettingsPath).absoluteFilePath());
          const SExpression root =
              SExpression::parse(FileUtils::readFile(fp), fp);
          customSettings = BoardDesignRuleCheckSettings(root);  // can throw
        } catch (const Exception& e) {
          printErr(
              tr("ERROR: Failed to load custom settings: %1").arg(e.getMsg()));
          success = false;
          boardsToCheck.clear();  // avoid exporting any boards
        }
      }
      foreach (Board* board, boardsToCheck) {
        print("  " % tr("Board '%1':").arg(*board->getName()));
        BoardDesignRuleCheck drc(
            *board, customSettings ? *customSettings : board->getDrcSettings());
        drc.execute(false);
        int approvedMsgCount = 0;
        const QStringList nonApproved = prepareRuleCheckMessages(
            drc.getMessages(), board->getDrcMessageApprovals(),
            approvedMsgCount);
        print("    " % tr("Approved messages: %1").arg(approvedMsgCount));
        print("    " %
              tr("Non-approved messages: %1").arg(nonApproved.count()));
        foreach (const QString& msg, nonApproved) {
          printErr("      - " % msg);
          success = false;
        }
      }
    }

    // Export schematics
    foreach (const QString& destStr, exportSchematicsFiles) {
      print(tr("Export schematics to '%1'...").arg(destStr));
      QString destPathStr = AttributeSubstitutor::substitute(
          destStr, project.get(), [&](const QString& str) {
            return FilePath::cleanFileName(
                str, FilePath::ReplaceSpaces | FilePath::KeepCase);
          });
      FilePath destPath(QFileInfo(destPathStr).absoluteFilePath());
      GraphicsExport graphicsExport;
      graphicsExport.setDocumentName(*project->getName());
      QObject::connect(
          &graphicsExport, &GraphicsExport::savingFile,
          [&destPathStr, &writtenFilesCounter](const FilePath& fp) {
            print(QString("  => '%1'").arg(prettyPath(fp, destPathStr)));
            writtenFilesCounter[fp]++;
          });
      std::shared_ptr<GraphicsExportSettings> settings =
          std::make_shared<GraphicsExportSettings>();
      GraphicsExport::Pages pages;
      foreach (const Schematic* schematic, project->getSchematics()) {
        pages.append(std::make_pair(
            std::make_shared<SchematicPainter>(*schematic), settings));
      }
      graphicsExport.startExport(pages, destPath);
      const QString errorMsg = graphicsExport.waitForFinished();
      if (!errorMsg.isEmpty()) {
        printErr("  " % tr("ERROR") % ": " % errorMsg);
        success = false;
      }
    }

    // Export BOM
    if (exportBomFiles.count() + exportBoardBomFiles.count() > 0) {
      QList<QPair<QString, bool>> jobs;  // <OutputPath, BoardSpecific>
      foreach (const QString& fp, exportBomFiles) {
        jobs.append(qMakePair(fp, false));
      }
      foreach (const QString& fp, exportBoardBomFiles) {
        jobs.append(qMakePair(fp, true));
      }
      QStringList attributes;
      if (bomAttributes.isEmpty()) {
        attributes = project->getCustomBomAttributes();
      } else {
        foreach (const QString str, bomAttributes.simplified().split(',')) {
          if (!str.trimmed().isEmpty()) {
            attributes.append(str.trimmed());
          }
        }
      }
      foreach (const auto& job, jobs) {
        QList<Board*> boardsToExport;
        const QString& destStr = job.first;
        bool boardSpecific = job.second;
        if (boardSpecific) {
          print(tr("Export board-specific BOM to '%1'...").arg(destStr));
          boardsToExport = boards;
        } else {
          print(tr("Export generic BOM to '%1'...").arg(destStr));
          boardsToExport = {nullptr};
        }
        foreach (const Board* board, boardsToExport) {
          const AttributeProvider* attrProvider = board;
          if (!board) {
            attrProvider = project.get();
          }
          QString destPathStr = AttributeSubstitutor::substitute(
              destStr, attrProvider, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              });
          FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
          BomGenerator gen(*project);
          gen.setAdditionalAttributes(attributes);
          std::shared_ptr<Bom> bom = gen.generate(board);
          if (board) {
            print(QString("  - '%1' => '%2'")
                      .arg(*board->getName(), prettyPath(fp, destPathStr)));
          } else {
            print(QString("  => '%1'").arg(prettyPath(fp, destPathStr)));
          }
          QString suffix = destStr.split('.').last().toLower();
          if (suffix == "csv") {
            BomCsvWriter writer(*bom);
            std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
            csv->saveToFile(fp);  // can throw
            writtenFilesCounter[fp]++;
          } else {
            printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
            success = false;
          }
        }
      }
    }

    // Export PCB fabrication data
    if (exportPcbFabricationData) {
      print(tr("Export PCB fabrication data..."));
      tl::optional<BoardFabricationOutputSettings> customSettings;
      QList<Board*> boardsToExport = boards;
      if (!pcbFabricationSettingsPath.isEmpty()) {
        try {
          qDebug() << "Load custom fabrication output settings:"
                   << pcbFabricationSettingsPath;
          const FilePath fp(
              QFileInfo(pcbFabricationSettingsPath).absoluteFilePath());
          const SExpression root =
              SExpression::parse(FileUtils::readFile(fp), fp);
          customSettings = BoardFabricationOutputSettings(root);  // can throw
        } catch (const Exception& e) {
          printErr(
              tr("ERROR: Failed to load custom settings: %1").arg(e.getMsg()));
          success = false;
          boardsToExport.clear();  // avoid exporting any boards
        }
      }
      foreach (const Board* board, boardsToExport) {
        print("  " % tr("Board '%1':").arg(*board->getName()));
        BoardGerberExport grbExport(*board);
        grbExport.exportPcbLayers(
            customSettings
                ? *customSettings
                : board->getFabricationOutputSettings());  // can throw
        foreach (const FilePath& fp, grbExport.getWrittenFiles()) {
          print(QString("    => '%1'").arg(prettyPath(fp, projectFile)));
          writtenFilesCounter[fp]++;
        }
      }
    }

    // Export pick&place files
    if ((exportPnpTopFiles.count() + exportPnpBottomFiles.count()) > 0) {
      struct Job {
        QString boardSideStr;
        PickPlaceCsvWriter::BoardSide boardSideCsv;
        BoardGerberExport::BoardSide boardSideGbr;
        QString destStr;
      };
      QVector<Job> jobs;
      foreach (const QString& fp, exportPnpTopFiles) {
        jobs.append(Job{tr("top"), PickPlaceCsvWriter::BoardSide::TOP,
                        BoardGerberExport::BoardSide::Top, fp});
      }
      foreach (const QString& fp, exportPnpBottomFiles) {
        jobs.append(Job{tr("bottom"), PickPlaceCsvWriter::BoardSide::BOTTOM,
                        BoardGerberExport::BoardSide::Bottom, fp});
      }
      foreach (const auto& job, jobs) {
        print(tr("Export %1 assembly data to '%2'...")
                  .arg(job.boardSideStr)
                  .arg(job.destStr));
        foreach (const Board* board, boards) {
          const QString destPathStr = AttributeSubstitutor::substitute(
              job.destStr, board, [&](const QString& str) {
                return FilePath::cleanFileName(
                    str, FilePath::ReplaceSpaces | FilePath::KeepCase);
              });
          const FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
          print(QString("  - '%1' => '%2'")
                    .arg(*board->getName(), prettyPath(fp, destPathStr)));
          const QString suffix = job.destStr.split('.').last().toLower();
          if (suffix == "csv") {
            BoardPickPlaceGenerator gen(*board);
            std::shared_ptr<PickPlaceData> data = gen.generate();
            PickPlaceCsvWriter writer(*data);
            writer.setIncludeMetadataComment(true);
            writer.setBoardSide(job.boardSideCsv);
            std::shared_ptr<CsvFile> csv = writer.generateCsv();  // can throw
            csv->saveToFile(fp);  // can throw
            writtenFilesCounter[fp]++;
          } else if (suffix == "gbr") {
            BoardGerberExport gen(*board);
            gen.exportComponentLayer(job.boardSideGbr, fp);  // can throw
            writtenFilesCounter[fp]++;
          } else {
            printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
            success = false;
          }
        }
      }
    }

    // Export netlist files
    foreach (const QString& destStr, exportNetlistFiles) {
      print(tr("Export netlist to '%1'...").arg(destStr));
      foreach (const Board* board, boards) {
        QString destPathStr = AttributeSubstitutor::substitute(
            destStr, board, [&](const QString& str) {
              return FilePath::cleanFileName(
                  str, FilePath::ReplaceSpaces | FilePath::KeepCase);
            });
        const FilePath fp(QFileInfo(destPathStr).absoluteFilePath());
        print(QString("  - '%1' => '%2'")
                  .arg(*board->getName(), prettyPath(fp, destPathStr)));
        const QString suffix = destStr.split('.').last().toLower();
        if (suffix == "d356") {
          BoardD356NetlistExport exp(*board);
          FileUtils::writeFile(fp, exp.generate());  // can throw
          writtenFilesCounter[fp]++;
        } else {
          printErr("  " % tr("ERROR: Unknown extension '%1'.").arg(suffix));
          success = false;
        }
      }
    }

    // Save project
    if (save) {
      print(tr("Save project..."));
      if (failIfFileFormatUnstable()) {
        success = false;
      } else {
        project->save();  // can throw
        if (projectFp.getSuffix() == "lppz") {
          projectFs->exportToZip(projectFp);  // can throw
        } else {
          projectFs->save();  // can throw
        }
      }
    }

    // Fail if some files were written multiple times
    bool filesOverwritten = false;
    QMapIterator<FilePath, int> writtenFilesIterator(writtenFilesCounter);
    while (writtenFilesIterator.hasNext()) {
      writtenFilesIterator.next();
      if (writtenFilesIterator.value() > 1) {
        filesOverwritten = true;
        printErr(tr("ERROR: The file '%1' was written multiple times!")
                     .arg(prettyPath(writtenFilesIterator.key(), projectFile)));
      }
    }
    if (filesOverwritten) {
      printErr(tr("NOTE: To avoid writing files multiple times, make "
                  "sure to pass unique filepaths to all export "
                  "functions. For board output files, you could either "
                  "add the placeholder '%1' to the path or specify the "
                  "boards to export with the '%2' argument.")
                   .arg("{{BOARD}}", "--board"));
      success = false;
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

bool CommandLineInterface::openLibrary(const QString& libDir, bool all,
                                       bool runCheck, bool save,
                                       bool strict) const noexcept {
  try {
    bool success = true;

    // Open library
    FilePath libFp(QFileInfo(libDir).absoluteFilePath());
    print(tr("Open library '%1'...").arg(prettyPath(libFp, libDir)));

    std::shared_ptr<TransactionalFileSystem> libFs =
        TransactionalFileSystem::open(libFp, save);  // can throw
    std::unique_ptr<Library> lib =
        Library::open(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(libFs)));  // can throw
    processLibraryElement(libDir, *libFs, *lib, runCheck, save, strict,
                          success);  // can throw

    // Open all component categories
    if (all) {
      QStringList elements = lib->searchForElements<ComponentCategory>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 component categories...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<ComponentCategory> element =
            ComponentCategory::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    // Open all package categories
    if (all) {
      QStringList elements = lib->searchForElements<PackageCategory>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 package categories...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<PackageCategory> element =
            PackageCategory::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    // Open all symbols
    if (all) {
      QStringList elements = lib->searchForElements<Symbol>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 symbols...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Symbol> element =
            Symbol::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    // Open all packages
    if (all) {
      QStringList elements = lib->searchForElements<Package>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 packages...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Package> element =
            Package::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    // Open all components
    if (all) {
      QStringList elements = lib->searchForElements<Component>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 components...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Component> element =
            Component::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    // Open all devices
    if (all) {
      QStringList elements = lib->searchForElements<Device>();
      elements.sort();  // For deterministic console output.
      print(tr("Process %1 devices...").arg(elements.count()));
      foreach (const QString& dir, elements) {
        FilePath fp = libFp.getPathTo(dir);
        qInfo().noquote() << tr("Open '%1'...").arg(prettyPath(fp, libDir));
        std::shared_ptr<TransactionalFileSystem> fs =
            TransactionalFileSystem::open(fp, save);  // can throw
        std::unique_ptr<Device> element =
            Device::open(std::unique_ptr<TransactionalDirectory>(
                new TransactionalDirectory(fs)));  // can throw
        processLibraryElement(libDir, *fs, *element, runCheck, save, strict,
                              success);  // can throw
      }
    }

    return success;
  } catch (const Exception& e) {
    printErr(tr("ERROR: %1").arg(e.getMsg()));
    return false;
  }
}

void CommandLineInterface::processLibraryElement(const QString& libDir,
                                                 TransactionalFileSystem& fs,
                                                 LibraryBaseElement& element,
                                                 bool runCheck, bool save,
                                                 bool strict,
                                                 bool& success) const {
  // Helper function to print an error header to console only once, if
  // there is at least one error.
  bool errorHeaderPrinted = false;
  auto printErrorHeaderOnce = [&errorHeaderPrinted, &element]() {
    if (!errorHeaderPrinted) {
      printErr(QString("  - %1 (%2):")
                   .arg(*element.getNames().getDefaultValue(),
                        element.getUuid().toStr()));
      errorHeaderPrinted = true;
    }
  };

  // Save element to transactional file system, if needed
  if (strict || save) {
    element.save();  // can throw
  }

  // Check for non-canonical files (strict mode)
  if (strict) {
    qInfo().noquote() << tr("Check '%1' for non-canonical files...")
                             .arg(prettyPath(fs.getPath(), libDir));

    QStringList paths = fs.checkForModifications();  // can throw
    if (!paths.isEmpty()) {
      // sort file paths to increases readability of console output
      std::sort(paths.begin(), paths.end());
      printErrorHeaderOnce();
      foreach (const QString& path, paths) {
        printErr(QString("    - Non-canonical file: '%1'")
                     .arg(prettyPath(fs.getAbsPath(path), libDir)));
      }
      success = false;
    }
  }

  // Run library element check, if needed.
  if (runCheck) {
    qInfo().noquote() << tr("Check '%1' for non-approved messages...")
                             .arg(prettyPath(fs.getPath(), libDir));
    int approvedMsgCount = 0;
    const RuleCheckMessageList messages = element.runChecks();
    const QStringList nonApproved = prepareRuleCheckMessages(
        messages, element.getMessageApprovals(), approvedMsgCount);
    qInfo().noquote() << "  " %
            tr("Approved messages: %1").arg(approvedMsgCount);
    qInfo().noquote() << "  " %
            tr("Non-approved messages: %1").arg(nonApproved.count());
    foreach (const QString& msg, nonApproved) {
      printErrorHeaderOnce();
      printErr("    - " % msg);
      success = false;
    }
  }

  // Save element to file system, if needed
  if (save) {
    qInfo().noquote()
        << tr("Save '%1'...").arg(prettyPath(fs.getPath(), libDir));
    if (failIfFileFormatUnstable()) {
      success = false;
    } else {
      fs.save();  // can throw
    }
  }

  // Do not propagate changes in the transactional file system to the
  // following checks
  fs.discardChanges();
}

QStringList CommandLineInterface::prepareRuleCheckMessages(
    RuleCheckMessageList messages, const QSet<SExpression>& approvals,
    int& approvedMsgCount) noexcept {
  // Sort messages to increases readability of console output.
  Toolbox::sortNumeric(messages,
                       [](const QCollator& cmp,
                          const std::shared_ptr<const RuleCheckMessage>& lhs,
                          const std::shared_ptr<const RuleCheckMessage>& rhs) {
                         if (lhs->getSeverity() != rhs->getSeverity()) {
                           return lhs->getSeverity() > rhs->getSeverity();
                         } else {
                           return cmp(lhs->getMessage(), rhs->getMessage());
                         }
                       },
                       Qt::CaseInsensitive, false);
  approvedMsgCount = 0;
  QStringList printedMessages;
  foreach (const auto& msg, messages) {
    if (approvals.contains(msg->getApproval())) {
      ++approvedMsgCount;
    } else {
      printedMessages.append(QString("[%1] %2").arg(
          msg->getSeverityTr().toUpper(), msg->getMessage()));
    }
  }
  return printedMessages;
}

QString CommandLineInterface::prettyPath(const FilePath& path,
                                         const QString& style) noexcept {
  if (QFileInfo(style).isAbsolute()) {
    return path.toNative();  // absolute path
  } else if (path == FilePath(QDir::currentPath())) {
    return path.getFilename();  // name of current directory
  } else {
    return QDir::toNativeSeparators(
        path.toRelative(FilePath(QDir::currentPath())));  // relative path
  }
}

bool CommandLineInterface::failIfFileFormatUnstable() noexcept {
  if ((!Application::isFileFormatStable()) &&
      (qgetenv("LIBREPCB_DISABLE_UNSTABLE_WARNING") != "1")) {
    printErr(
        tr("This application version is UNSTABLE! Option '%1' is disabled to "
           "avoid breaking projects or libraries. Please use a stable "
           "release instead.")
            .arg("--save"));
    return true;
  } else {
    qInfo() << "Application version is unstable, but warning is disabled with "
               "environment variable LIBREPCB_DISABLE_UNSTABLE_WARNING.";
    return false;
  }
}

void CommandLineInterface::print(const QString& str) noexcept {
  QTextStream s(stdout);
  s << str << endl;
}

void CommandLineInterface::printErr(const QString& str) noexcept {
  QTextStream s(stderr);
  s << str << endl;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace cli
}  // namespace librepcb
