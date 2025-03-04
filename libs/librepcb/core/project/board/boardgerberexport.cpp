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
#include "boardgerberexport.h"

#include "../../attribute/attributesubstitutor.h"
#include "../../export/excellongenerator.h"
#include "../../export/gerbergenerator.h"
#include "../../geometry/hole.h"
#include "../../library/cmp/componentsignal.h"
#include "../../library/pkg/footprint.h"
#include "../../library/pkg/footprintpad.h"
#include "../../library/pkg/package.h"
#include "../../library/pkg/packagepad.h"
#include "../../utils/transform.h"
#include "../circuit/componentinstance.h"
#include "../circuit/componentsignalinstance.h"
#include "../circuit/netsignal.h"
#include "../project.h"
#include "board.h"
#include "boardfabricationoutputsettings.h"
#include "items/bi_device.h"
#include "items/bi_footprintpad.h"
#include "items/bi_hole.h"
#include "items/bi_netline.h"
#include "items/bi_netpoint.h"
#include "items/bi_netsegment.h"
#include "items/bi_plane.h"
#include "items/bi_polygon.h"
#include "items/bi_stroketext.h"
#include "items/bi_via.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardGerberExport::BoardGerberExport(const Board& board) noexcept
  : mProject(board.getProject()),
    mBoard(board),
    mCreationDateTime(QDateTime::currentDateTime()),
    mProjectName(*mProject.getName()),
    mCurrentInnerCopperLayer(0) {
  // If the project contains multiple boards, add the board name to the
  // Gerber file metadata as well to distinguish between the different boards.
  if (mProject.getBoards().count() > 1) {
    mProjectName += " (" % mBoard.getName() % ")";
  }
}

BoardGerberExport::~BoardGerberExport() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

FilePath BoardGerberExport::getOutputDirectory(
    const BoardFabricationOutputSettings& settings) const noexcept {
  return getOutputFilePath(settings.getOutputBasePath() + "dummy")
      .getParentDir();  // use dummy suffix
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardGerberExport::exportPcbLayers(
    const BoardFabricationOutputSettings& settings) const {
  mWrittenFiles.clear();

  if (settings.getMergeDrillFiles()) {
    exportDrills(settings);
  } else {
    exportDrillsNpth(settings);
    exportDrillsPth(settings);
  }
  exportLayerBoardOutlines(settings);
  exportLayerTopCopper(settings);
  exportLayerInnerCopper(settings);
  exportLayerBottomCopper(settings);
  exportLayerTopSolderMask(settings);
  exportLayerBottomSolderMask(settings);
  exportLayerTopSilkscreen(settings);
  exportLayerBottomSilkscreen(settings);
  if (settings.getEnableSolderPasteTop()) {
    exportLayerTopSolderPaste(settings);
  }
  if (settings.getEnableSolderPasteBot()) {
    exportLayerBottomSolderPaste(settings);
  }
}

void BoardGerberExport::exportComponentLayer(BoardSide side,
                                             const FilePath& filePath) const {
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  if (side == BoardSide::Top) {
    gen.setFileFunctionComponent(1, GerberGenerator::BoardSide::Top);
  } else {
    gen.setFileFunctionComponent(mBoard.getInnerLayerCount() + 2,
                                 GerberGenerator::BoardSide::Bottom);
  }

  // Export board outline since this is useful for manual review.
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (polygon->getPolygon().getLayer() == Layer::boardOutlines()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(polygon->getPolygon().getLineWidth(),
                           polygon->getPolygon().getLayer());
      gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth,
                          GerberAttribute::ApertureFunction::Profile,
                          tl::nullopt, QString());
    }
  }

  // Export all components on the selected board side.
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    if (device->getMirrored() == (side == BoardSide::Bottom)) {
      const Package::AssemblyType assemblyType =
          device->getLibPackage().getAssemblyType(true);
      GerberGenerator::MountType mountType = GerberGenerator::MountType::Other;
      switch (assemblyType) {
        case Package::AssemblyType::None:
          // Skip devices which don't represent a mountable package.
          continue;
        case Package::AssemblyType::Tht:
        case Package::AssemblyType::Mixed:  // Does this make sense?!
          mountType = GerberGenerator::MountType::Tht;
          break;
        case Package::AssemblyType::Smt:
          mountType = GerberGenerator::MountType::Smt;
          break;
        case Package::AssemblyType::Other:
          mountType = GerberGenerator::MountType::Other;
          break;
        default:
          qWarning() << "Unknown assembly type:"
                     << static_cast<int>(assemblyType);
          break;
      }

      // Export component center and attributes.
      Angle rotation = device->getMirrored() ? -device->getRotation()
                                             : device->getRotation();
      QString designator = *device->getComponentInstance().getName();
      QString value = device->getComponentInstance().getValue(true).trimmed();
      QString manufacturer =
          AttributeSubstitutor::substitute("{{MANUFACTURER}}", device)
              .trimmed();
      QString mpn = AttributeSubstitutor::substitute(
                        "{{MPN or PARTNUMBER or DEVICE}}", device)
                        .trimmed();
      // Note: Always use english locale to make PnP files portable.
      QString footprintName =
          *device->getLibPackage().getNames().getDefaultValue();
      gen.flashComponent(device->getPosition(), rotation, designator, value,
                         mountType, manufacturer, mpn, footprintName);

      // Export component outline. But only closed ones, sunce Gerber specs say
      // that component outlines must be closed.
      QHash<const Layer*, GerberAttribute::ApertureFunction> layerFunction;
      if (side == BoardSide::Top) {
        layerFunction[&Layer::topDocumentation()] =
            GerberAttribute::ApertureFunction::ComponentOutlineBody;
        layerFunction[&Layer::topCourtyard()] =
            GerberAttribute::ApertureFunction::ComponentOutlineCourtyard;
      } else {
        layerFunction[&Layer::botDocumentation()] =
            GerberAttribute::ApertureFunction::ComponentOutlineBody;
        layerFunction[&Layer::botCourtyard()] =
            GerberAttribute::ApertureFunction::ComponentOutlineCourtyard;
      }
      const Transform transform(*device);
      for (const Polygon& polygon :
           device->getLibFootprint().getPolygons().sortedByUuid()) {
        if (!polygon.getPath().isClosed()) {
          continue;
        }
        if (polygon.isFilled()) {
          continue;
        }
        const Layer& layer = transform.map(polygon.getLayer());
        if (!layerFunction.contains(&layer)) {
          continue;
        }
        Path path = transform.map(polygon.getPath());
        gen.drawComponentOutline(path, rotation, designator, value, mountType,
                                 manufacturer, mpn, footprintName,
                                 layerFunction[&layer]);
      }

      // Export component pins.
      foreach (const BI_FootprintPad* pad, device->getPads()) {
        QString pinName, pinSignal;
        if (const PackagePad* pkgPad = pad->getLibPackagePad()) {
          pinName = *pkgPad->getName();
        }
        if (ComponentSignalInstance* cmpSig =
                pad->getComponentSignalInstance()) {
          pinSignal = *cmpSig->getCompSignal().getName();
        }
        bool isPin1 = (pinName == "1");  // Very sophisticated algorithm ;-)
        gen.flashComponentPin(pad->getPosition(), rotation, designator, value,
                              mountType, manufacturer, mpn, footprintName,
                              pinName, pinSignal, isPin1);
      }
    }
  }

  gen.generate();
  gen.saveToFile(filePath);
  mWrittenFiles.append(filePath);
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString BoardGerberExport::getBuiltInAttributeValue(const QString& key) const
    noexcept {
  if ((key == QLatin1String("CU_LAYER")) && (mCurrentInnerCopperLayer > 0)) {
    return QString::number(mCurrentInnerCopperLayer);
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*>
    BoardGerberExport::getAttributeProviderParents() const noexcept {
  return QVector<const AttributeProvider*>{&mBoard};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void BoardGerberExport::exportDrills(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrills());
  std::unique_ptr<ExcellonGenerator> gen =
      BoardGerberExport::createExcellonGenerator(
          settings, ExcellonGenerator::Plating::Mixed);
  drawPthDrills(*gen);
  drawNpthDrills(*gen);
  gen->generate();
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsNpth(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrillsNpth());
  std::unique_ptr<ExcellonGenerator> gen =
      BoardGerberExport::createExcellonGenerator(
          settings, ExcellonGenerator::Plating::No);
  drawNpthDrills(*gen);

  // Note that separate NPTH drill files could lead to issues with some PCB
  // manufacturers, even if it's empty in many cases. However, we generate the
  // NPTH file even if there are no NPTH drills since it could also lead to
  // unexpected behavior if the file is generated only conditionally. See
  // https://github.com/LibrePCB/LibrePCB/issues/998. If the PCB manufacturer
  // doesn't support a separate NPTH file, the user shall enable the
  // "merge PTH and NPTH drills"  option.
  gen->generate();
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportDrillsPth(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixDrillsPth());
  std::unique_ptr<ExcellonGenerator> gen =
      BoardGerberExport::createExcellonGenerator(
          settings, ExcellonGenerator::Plating::Yes);
  drawPthDrills(*gen);
  gen->generate();
  gen->saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBoardOutlines(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixOutlines());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionOutlines(false);
  drawLayer(gen, Layer::boardOutlines());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionCopper(1, GerberGenerator::CopperSide::Top,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::topCopper());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomCopper(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixCopperBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionCopper(mBoard.getInnerLayerCount() + 2,
                            GerberGenerator::CopperSide::Bottom,
                            GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::botCopper());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerInnerCopper(
    const BoardFabricationOutputSettings& settings) const {
  for (int i = 1; i <= mBoard.getInnerLayerCount(); ++i) {
    mCurrentInnerCopperLayer = i;  // used for attribute provider
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixCopperInner());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionCopper(i + 1, GerberGenerator::CopperSide::Inner,
                              GerberGenerator::Polarity::Positive);
    if (const Layer* layer = Layer::innerCopper(i)) {
      drawLayer(gen, *layer);
    } else {
      throw LogicError(__FILE__, __LINE__, "Unknown inner copper layer.");
    }
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
  mCurrentInnerCopperLayer = 0;
}

void BoardGerberExport::exportLayerTopSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderMaskTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Top,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, Layer::topStopMask());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderMask(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderMaskBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionSolderMask(GerberGenerator::BoardSide::Bottom,
                                GerberGenerator::Polarity::Negative);
  drawLayer(gen, Layer::botStopMask());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerTopSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  const QVector<const Layer*>& layers = settings.getSilkscreenLayersTop();
  if (layers.count() > 0) {  // don't export silkscreen if no layers selected
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixSilkscreenTop());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Top,
                              GerberGenerator::Polarity::Positive);
    foreach (const Layer* layer, layers) { drawLayer(gen, *layer); }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::topStopMask());
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportLayerBottomSilkscreen(
    const BoardFabricationOutputSettings& settings) const {
  const QVector<const Layer*>& layers = settings.getSilkscreenLayersBot();
  if (layers.count() > 0) {  // don't export silkscreen if no layers selected
    FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                    settings.getSuffixSilkscreenBot());
    GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                        mProject.getVersion());
    gen.setFileFunctionLegend(GerberGenerator::BoardSide::Bottom,
                              GerberGenerator::Polarity::Positive);
    foreach (const Layer* layer, layers) { drawLayer(gen, *layer); }
    gen.setLayerPolarity(GerberGenerator::Polarity::Negative);
    drawLayer(gen, Layer::botStopMask());
    gen.generate();
    gen.saveToFile(fp);
    mWrittenFiles.append(fp);
  }
}

void BoardGerberExport::exportLayerTopSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderPasteTop());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionPaste(GerberGenerator::BoardSide::Top,
                           GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::topSolderPaste());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

void BoardGerberExport::exportLayerBottomSolderPaste(
    const BoardFabricationOutputSettings& settings) const {
  FilePath fp = getOutputFilePath(settings.getOutputBasePath() %
                                  settings.getSuffixSolderPasteBot());
  GerberGenerator gen(mCreationDateTime, mProjectName, mBoard.getUuid(),
                      mProject.getVersion());
  gen.setFileFunctionPaste(GerberGenerator::BoardSide::Bottom,
                           GerberGenerator::Polarity::Positive);
  drawLayer(gen, Layer::botSolderPaste());
  gen.generate();
  gen.saveToFile(fp);
  mWrittenFiles.append(fp);
}

int BoardGerberExport::drawNpthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint holes
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform transform(*device);
    for (const Hole& hole : device->getLibFootprint().getHoles()) {
      gen.drill(transform.map(hole.getPath()), hole.getDiameter(), false,
                ExcellonGenerator::Function::MechanicalDrill);
      ++count;
    }
  }

  // board holes
  foreach (const BI_Hole* hole, mBoard.getHoles()) {
    gen.drill(hole->getHole().getPath(), hole->getHole().getDiameter(), false,
              ExcellonGenerator::Function::MechanicalDrill);
    ++count;
  }

  return count;
}

int BoardGerberExport::drawPthDrills(ExcellonGenerator& gen) const {
  int count = 0;

  // footprint pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    const Transform deviceTransform(*device);
    foreach (const BI_FootprintPad* pad, device->getPads()) {
      const FootprintPad& libPad = pad->getLibPad();
      const Transform padTransform(libPad.getPosition(), libPad.getRotation());
      for (const PadHole& hole : libPad.getHoles()) {
        gen.drill(deviceTransform.map(padTransform.map(hole.getPath())),
                  hole.getDiameter(), true,
                  ExcellonGenerator::Function::ComponentDrill);  // can throw
        ++count;
      }
    }
  }

  // vias
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    foreach (const BI_Via* via, netsegment->getVias()) {
      gen.drill(via->getPosition(), via->getDrillDiameter(), true,
                ExcellonGenerator::Function::ViaDrill);
      ++count;
    }
  }

  return count;
}

void BoardGerberExport::drawLayer(GerberGenerator& gen,
                                  const Layer& layer) const {
  // draw footprints incl. pads
  foreach (const BI_Device* device, mBoard.getDeviceInstances()) {
    Q_ASSERT(device);
    drawDevice(gen, *device, layer);
  }

  // draw vias and traces (grouped by net)
  foreach (const BI_NetSegment* netsegment, mBoard.getNetSegments()) {
    Q_ASSERT(netsegment);
    QString net = netsegment->getNetSignal()
        ? *netsegment->getNetSignal()->getName()  // Named net.
        : "N/C";  // Anonymous net (reserved name by Gerber specs).
    foreach (const BI_Via* via, netsegment->getVias()) {
      Q_ASSERT(via);
      drawVia(gen, *via, layer, net);
    }
    foreach (const BI_NetLine* netline, netsegment->getNetLines()) {
      Q_ASSERT(netline);
      if (netline->getLayer() == layer) {
        gen.drawLine(netline->getStartPoint().getPosition(),
                     netline->getEndPoint().getPosition(),
                     positiveToUnsigned(netline->getWidth()),
                     GerberAttribute::ApertureFunction::Conductor, net,
                     QString());
      }
    }
  }

  // draw planes
  foreach (const BI_Plane* plane, mBoard.getPlanes()) {
    Q_ASSERT(plane);
    if (plane->getLayer() == layer) {
      foreach (const Path& fragment, plane->getFragments()) {
        gen.drawPathArea(fragment, GerberAttribute::ApertureFunction::Conductor,
                         *plane->getNetSignal().getName(), QString());
      }
    }
  }

  // draw polygons
  GerberGenerator::Function graphicsFunction = tl::nullopt;
  tl::optional<QString> graphicsNet = tl::nullopt;
  if (layer == Layer::boardOutlines()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (layer.isCopper()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  foreach (const BI_Polygon* polygon, mBoard.getPolygons()) {
    Q_ASSERT(polygon);
    if (layer == polygon->getPolygon().getLayer()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(polygon->getPolygon().getLineWidth(), layer);
      gen.drawPathOutline(polygon->getPolygon().getPath(), lineWidth,
                          graphicsFunction, graphicsNet, QString());
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon->getPolygon().isFilled() &&
          polygon->getPolygon().getPath().isClosed()) {
        gen.drawPathArea(polygon->getPolygon().getPath(), graphicsFunction,
                         graphicsNet, QString());
      }
    }
  }

  // draw stroke texts
  GerberGenerator::Function textFunction = tl::nullopt;
  if (layer.isCopper()) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, mBoard.getStrokeTexts()) {
    Q_ASSERT(text);
    if (layer == text->getTextObj().getLayer()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getTextObj().getStrokeWidth(), layer);
      const Transform transform(text->getTextObj());
      foreach (Path path, transform.map(text->getPaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            QString());
      }
    }
  }

  // Draw holes.
  if (layer.isStopMask()) {
    foreach (const BI_Hole* hole, mBoard.getHoles()) {
      if (const tl::optional<Length>& offset = hole->getStopMaskOffset()) {
        const Length diameter =
            (*hole->getHole().getDiameter()) + (*offset) + (*offset);
        const Path path = hole->getHole().getPath()->cleaned();
        if (diameter > 0) {
          if (path.getVertices().count() == 1) {
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(diameter), tl::nullopt, tl::nullopt,
                            QString(), QString(), QString());
          } else {
            gen.drawPathOutline(path, UnsignedLength(diameter), tl::nullopt,
                                tl::nullopt, QString());
          }
        }
      }
    }
  }
}

void BoardGerberExport::drawVia(GerberGenerator& gen, const BI_Via& via,
                                const Layer& layer,
                                const QString& netName) const {
  bool drawCopper = via.isOnLayer(layer);
  bool drawStopMask = layer.isStopMask() && via.getStopMaskOffset();
  if (drawCopper || drawStopMask) {
    PositiveLength outerDiameter = via.getSize();
    UnsignedLength radius(0);
    if (drawStopMask) {
      radius = UnsignedLength(std::max(*via.getStopMaskOffset(), Length(0)));
      outerDiameter += UnsignedLength(radius * 2);
    }

    // Via attributes (only on copper layers).
    GerberGenerator::Function function = tl::nullopt;
    tl::optional<QString> net = tl::nullopt;
    if (drawCopper) {
      function = GerberAttribute::ApertureFunction::ViaPad;
      net = netName;
    }

    gen.flashCircle(via.getPosition(), outerDiameter, function, net, QString(),
                    QString(), QString());
  }
}

void BoardGerberExport::drawDevice(GerberGenerator& gen,
                                   const BI_Device& device,
                                   const Layer& layer) const {
  GerberGenerator::Function graphicsFunction = tl::nullopt;
  tl::optional<QString> graphicsNet = tl::nullopt;
  if (layer == Layer::boardOutlines()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Profile;
  } else if (layer.isCopper()) {
    graphicsFunction = GerberAttribute::ApertureFunction::Conductor;
    graphicsNet = "";  // Not connected to any net.
  }
  QString component = *device.getComponentInstance().getName();

  // draw pads
  foreach (const BI_FootprintPad* pad, device.getPads()) {
    drawFootprintPad(gen, *pad, layer);
  }

  // draw polygons
  const Transform transform(device);
  for (const Polygon& polygon :
       device.getLibFootprint().getPolygons().sortedByUuid()) {
    const Layer& polygonLayer = transform.map(polygon.getLayer());
    if (polygonLayer == layer) {
      Path path = transform.map(polygon.getPath());
      gen.drawPathOutline(
          path, calcWidthOfLayer(polygon.getLineWidth(), polygonLayer),
          graphicsFunction, graphicsNet, component);
      // Only fill closed paths (for consistency with the appearance in the
      // board editor, and because Gerber expects area outlines as closed).
      if (polygon.isFilled() && path.isClosed()) {
        gen.drawPathArea(path, graphicsFunction, graphicsNet, component);
      }
    }
  }

  // draw circles
  for (const Circle& circle :
       device.getLibFootprint().getCircles().sortedByUuid()) {
    const Layer& circleLayer = transform.map(circle.getLayer());
    if (circleLayer == layer) {
      Point absolutePos = transform.map(circle.getCenter());
      if (circle.isFilled()) {
        PositiveLength outerDia = circle.getDiameter() + circle.getLineWidth();
        gen.drawPathArea(Path::circle(outerDia).translated(absolutePos),
                         graphicsFunction, graphicsNet, component);
      } else {
        UnsignedLength lineWidth =
            calcWidthOfLayer(circle.getLineWidth(), circleLayer);
        gen.drawPathOutline(
            Path::circle(circle.getDiameter()).translated(absolutePos),
            lineWidth, graphicsFunction, graphicsNet, component);
      }
    }
  }

  // draw stroke texts (from footprint instance, *NOT* from library footprint!)
  GerberGenerator::Function textFunction = tl::nullopt;
  if (layer.isCopper()) {
    textFunction = GerberAttribute::ApertureFunction::NonConductor;
  }
  foreach (const BI_StrokeText* text, device.getStrokeTexts()) {
    if (layer == text->getTextObj().getLayer()) {
      UnsignedLength lineWidth =
          calcWidthOfLayer(text->getTextObj().getStrokeWidth(), layer);
      Transform transform(text->getTextObj());
      foreach (Path path, transform.map(text->getPaths())) {
        gen.drawPathOutline(path, lineWidth, textFunction, graphicsNet,
                            component);
      }
    }
  }

  // Draw holes.
  if (layer.isStopMask()) {
    for (const Hole& hole :
         device.getLibFootprint().getHoles().sortedByUuid()) {
      if (tl::optional<Length> offset =
              device.getHoleStopMasks().value(hole.getUuid())) {
        const Length diameter = (*hole.getDiameter()) + (*offset) + (*offset);
        if (diameter > 0) {
          const Path path = transform.map(hole.getPath()->cleaned());
          if (path.getVertices().count() == 1) {
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(diameter), tl::nullopt, tl::nullopt,
                            QString(), QString(), QString());
          } else {
            gen.drawPathOutline(path, UnsignedLength(diameter), tl::nullopt,
                                tl::nullopt, QString());
          }
        }
      }
    }
  }
}

void BoardGerberExport::drawFootprintPad(GerberGenerator& gen,
                                         const BI_FootprintPad& pad,
                                         const Layer& layer) const {
  foreach (const PadGeometry& geometry, pad.getGeometries().value(&layer)) {
    // Pad attributes (most of them only on copper layers).
    GerberGenerator::Function function = tl::nullopt;
    tl::optional<QString> net = tl::nullopt;
    QString component = *pad.getDevice().getComponentInstance().getName();
    QString pin, signal;
    if (layer.isCopper()) {
      if (pad.getLibPad().isTht()) {
        function = GerberAttribute::ApertureFunction::ComponentPad;
      } else {
        function = GerberAttribute::ApertureFunction::SmdPadCopperDefined;
      }
      net = pad.getCompSigInstNetSignal()
          ? *pad.getCompSigInstNetSignal()->getName()  // Named net.
          : "N/C";  // Anonymous net (reserved name by Gerber specs).
      if (const PackagePad* pkgPad = pad.getLibPackagePad()) {
        pin = *pkgPad->getName();
      }
      if (ComponentSignalInstance* cmpSig = pad.getComponentSignalInstance()) {
        signal = *cmpSig->getCompSignal().getName();
      }
    }

    // Helper to flash a custom outline by flattening all arcs.
    const Transform padTransform(pad.getLibPad().getPosition(),
                                 pad.getLibPad().getRotation());
    const Transform devTransform(pad.getDevice());
    auto flashPadOutline = [&]() {
      foreach (Path outline, geometry.toOutlines()) {
        outline.flattenArcs(PositiveLength(5000));
        outline = devTransform.map(padTransform.map(outline))
                      .translated(-pad.getPosition());
        gen.flashOutline(pad.getPosition(), StraightAreaPath(outline),
                         Angle::deg0(), function, net, component, pin,
                         signal);  // can throw
      }
    };

    // Flash shape.
    const Length width = geometry.getWidth();
    const Length height = geometry.getHeight();
    switch (geometry.getShape()) {
      case PadGeometry::Shape::RoundedRect: {
        if ((width > 0) && (height > 0)) {
          gen.flashRect(pad.getPosition(), PositiveLength(width),
                        PositiveLength(height), geometry.getCornerRadius(),
                        pad.getRotation(), function, net, component, pin,
                        signal);
        }
        break;
      }
      case PadGeometry::Shape::RoundedOctagon: {
        if ((width > 0) && (height > 0)) {
          gen.flashOctagon(pad.getPosition(), PositiveLength(width),
                           PositiveLength(height), geometry.getCornerRadius(),
                           pad.getRotation(), function, net, component, pin,
                           signal);
        }
        break;
      }
      case PadGeometry::Shape::Stroke: {
        if ((width > 0) && (!geometry.getPath().getVertices().isEmpty())) {
          const Path path =
              devTransform.map(padTransform.map(geometry.getPath()));
          if (path.getVertices().count() == 1) {
            // For maximum compatibility, convert the stroke to a circle.
            gen.flashCircle(path.getVertices().first().getPos(),
                            PositiveLength(width), function, net, component,
                            pin, signal);
          } else if ((path.getVertices().count() == 2) &&
                     (path.getVertices().first().getAngle() == 0)) {
            // For maximum compatibility, convert the stroke to an obround.
            const Point p0 = path.getVertices().at(0).getPos();
            const Point p1 = path.getVertices().at(1).getPos();
            const Point delta = p1 - p0;
            const Point center = (p0 + p1) / 2;
            const PositiveLength height(width);
            const PositiveLength width = height + (p1 - p0).getLength();
            const Angle rotation = Angle::fromRad(
                qAtan2(delta.getY().toMm(), delta.getX().toMm()));
            gen.flashObround(center, width, height, rotation, function, net,
                             component, pin, signal);
          } else {
            // As a last resort, convert the outlines to straight path segments
            // and flash them with outline apertures.
            flashPadOutline();  // can throw
          }
        }
        break;
      }
      case PadGeometry::Shape::Custom: {
        flashPadOutline();  // can throw
        break;
      }
      default: { throw LogicError(__FILE__, __LINE__, "Unknown pad shape!"); }
    }
  }
}

std::unique_ptr<ExcellonGenerator> BoardGerberExport::createExcellonGenerator(
    const BoardFabricationOutputSettings& settings,
    ExcellonGenerator::Plating plating) const {
  std::unique_ptr<ExcellonGenerator> gen(new ExcellonGenerator(
      mCreationDateTime, mProjectName, mBoard.getUuid(), mProject.getVersion(),
      plating, 1, mBoard.getInnerLayerCount() + 2));
  gen->setUseG85Slots(settings.getUseG85SlotCommand());
  return gen;
}

FilePath BoardGerberExport::getOutputFilePath(QString path) const noexcept {
  path = AttributeSubstitutor::substitute(path, this, [&](const QString& str) {
    return FilePath::cleanFileName(
        str, FilePath::ReplaceSpaces | FilePath::KeepCase);
  });

  if (QDir::isAbsolutePath(path)) {
    return FilePath(path);
  } else {
    return mBoard.getProject().getPath().getPathTo(path);
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

UnsignedLength BoardGerberExport::calcWidthOfLayer(
    const UnsignedLength& width, const Layer& layer) noexcept {
  if ((layer == Layer::boardOutlines()) && (width < UnsignedLength(1000))) {
    return UnsignedLength(1000);  // outlines should have a minimum width of 1um
  } else {
    return width;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
