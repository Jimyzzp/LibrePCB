# Enable Qt MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Allow specifying the author of the build to help investigating bug reports.
set(LIBREPCB_BUILD_AUTHOR
    ""
    CACHE STRING "Author of this build."
)

# Allow overriding the share directory path
set(LIBREPCB_SHARE
    "../share/librepcb"
    CACHE STRING "Path to the LibrePCB share directory."
)

# Set this to YES to disable some features that make builds non-reproducible
# (like embedding the git commit hash or the path to the source directory in
# the binary).
set(LIBREPCB_REPRODUCIBLE
    NO
    CACHE BOOL "Disable feature that make builds non-reproducible"
)

if(LIBREPCB_REPRODUCIBLE)
  message(STATUS "Creating a potentially reproducible build")
else()
  # Detect git commit hash
  find_package(Git)
  if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
      COMMAND ${GIT_EXECUTABLE} -C "${CMAKE_SOURCE_DIR}" rev-parse --short
              --verify HEAD
      OUTPUT_VARIABLE SHORT_SHA
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(GIT_COMMIT_SHA ${SHORT_SHA})
  else()
    message(STATUS "Git not found, cannot set version info")
  endif()

  # Detect path to source share directory
  set(LIBREPCB_SHARE_SOURCE "${CMAKE_SOURCE_DIR}/share")

  # Detect path to binary output directory
  set(LIBREPCB_BINARY_DIR "${CMAKE_BINARY_DIR}")
endif()

# Generate build_env.h file
configure_file(build_env.h.in build_env.h @ONLY)

# Export library
add_library(
  librepcb_core STATIC
  algorithm/airwiresbuilder.cpp
  algorithm/airwiresbuilder.h
  application.cpp
  application.h
  attribute/attribute.cpp
  attribute/attribute.h
  attribute/attributekey.h
  attribute/attributeprovider.cpp
  attribute/attributeprovider.h
  attribute/attributesubstitutor.cpp
  attribute/attributesubstitutor.h
  attribute/attributetype.cpp
  attribute/attributetype.h
  attribute/attributeunit.cpp
  attribute/attributeunit.h
  attribute/attrtypecapacitance.cpp
  attribute/attrtypecapacitance.h
  attribute/attrtypecurrent.cpp
  attribute/attrtypecurrent.h
  attribute/attrtypefrequency.cpp
  attribute/attrtypefrequency.h
  attribute/attrtypeinductance.cpp
  attribute/attrtypeinductance.h
  attribute/attrtypepower.cpp
  attribute/attrtypepower.h
  attribute/attrtyperesistance.cpp
  attribute/attrtyperesistance.h
  attribute/attrtypestring.cpp
  attribute/attrtypestring.h
  attribute/attrtypevoltage.cpp
  attribute/attrtypevoltage.h
  debug.cpp
  debug.h
  exceptions.cpp
  exceptions.h
  export/bom.cpp
  export/bom.h
  export/bomcsvwriter.cpp
  export/bomcsvwriter.h
  export/d356netlistgenerator.cpp
  export/d356netlistgenerator.h
  export/excellongenerator.cpp
  export/excellongenerator.h
  export/gerberaperturelist.cpp
  export/gerberaperturelist.h
  export/gerberattribute.cpp
  export/gerberattribute.h
  export/gerberattributewriter.cpp
  export/gerberattributewriter.h
  export/gerbergenerator.cpp
  export/gerbergenerator.h
  export/graphicsexport.cpp
  export/graphicsexport.h
  export/graphicsexportsettings.cpp
  export/graphicsexportsettings.h
  export/graphicspainter.cpp
  export/graphicspainter.h
  export/pickplacecsvwriter.cpp
  export/pickplacecsvwriter.h
  export/pickplacedata.cpp
  export/pickplacedata.h
  fileio/asynccopyoperation.cpp
  fileio/asynccopyoperation.h
  fileio/csvfile.cpp
  fileio/csvfile.h
  fileio/directorylock.cpp
  fileio/directorylock.h
  fileio/filepath.cpp
  fileio/filepath.h
  fileio/filesystem.h
  fileio/fileutils.cpp
  fileio/fileutils.h
  fileio/transactionaldirectory.cpp
  fileio/transactionaldirectory.h
  fileio/transactionalfilesystem.cpp
  fileio/transactionalfilesystem.h
  fileio/versionfile.cpp
  fileio/versionfile.h
  font/strokefont.cpp
  font/strokefont.h
  font/strokefontpool.cpp
  font/strokefontpool.h
  geometry/circle.cpp
  geometry/circle.h
  geometry/hole.cpp
  geometry/hole.h
  geometry/junction.cpp
  geometry/junction.h
  geometry/netlabel.cpp
  geometry/netlabel.h
  geometry/netline.cpp
  geometry/netline.h
  geometry/padgeometry.cpp
  geometry/padgeometry.h
  geometry/padhole.cpp
  geometry/padhole.h
  geometry/path.cpp
  geometry/path.h
  geometry/polygon.cpp
  geometry/polygon.h
  geometry/stroketext.cpp
  geometry/stroketext.h
  geometry/text.cpp
  geometry/text.h
  geometry/trace.cpp
  geometry/trace.h
  geometry/vertex.cpp
  geometry/vertex.h
  geometry/via.cpp
  geometry/via.h
  import/dxfreader.cpp
  import/dxfreader.h
  library/cat/componentcategory.cpp
  library/cat/componentcategory.h
  library/cat/librarycategory.cpp
  library/cat/librarycategory.h
  library/cat/packagecategory.cpp
  library/cat/packagecategory.h
  library/cmp/cmpsigpindisplaytype.cpp
  library/cmp/cmpsigpindisplaytype.h
  library/cmp/component.cpp
  library/cmp/component.h
  library/cmp/componentcheck.cpp
  library/cmp/componentcheck.h
  library/cmp/componentcheckmessages.cpp
  library/cmp/componentcheckmessages.h
  library/cmp/componentpinsignalmap.cpp
  library/cmp/componentpinsignalmap.h
  library/cmp/componentprefix.h
  library/cmp/componentsignal.cpp
  library/cmp/componentsignal.h
  library/cmp/componentsymbolvariant.cpp
  library/cmp/componentsymbolvariant.h
  library/cmp/componentsymbolvariantitem.cpp
  library/cmp/componentsymbolvariantitem.h
  library/cmp/componentsymbolvariantitemsuffix.h
  library/dev/device.cpp
  library/dev/device.h
  library/dev/devicecheck.cpp
  library/dev/devicecheck.h
  library/dev/devicecheckmessages.cpp
  library/dev/devicecheckmessages.h
  library/dev/devicepadsignalmap.cpp
  library/dev/devicepadsignalmap.h
  library/library.cpp
  library/library.h
  library/librarybaseelement.cpp
  library/librarybaseelement.h
  library/librarybaseelementcheck.cpp
  library/librarybaseelementcheck.h
  library/librarybaseelementcheckmessages.cpp
  library/librarybaseelementcheckmessages.h
  library/libraryelement.cpp
  library/libraryelement.h
  library/libraryelementcheck.cpp
  library/libraryelementcheck.h
  library/libraryelementcheckmessages.cpp
  library/libraryelementcheckmessages.h
  library/pkg/footprint.cpp
  library/pkg/footprint.h
  library/pkg/footprintpad.cpp
  library/pkg/footprintpad.h
  library/pkg/footprintpainter.cpp
  library/pkg/footprintpainter.h
  library/pkg/package.cpp
  library/pkg/package.h
  library/pkg/packagecheck.cpp
  library/pkg/packagecheck.h
  library/pkg/packagecheckmessages.cpp
  library/pkg/packagecheckmessages.h
  library/pkg/packagepad.cpp
  library/pkg/packagepad.h
  library/sym/symbol.cpp
  library/sym/symbol.h
  library/sym/symbolcheck.cpp
  library/sym/symbolcheck.h
  library/sym/symbolcheckmessages.cpp
  library/sym/symbolcheckmessages.h
  library/sym/symbolpainter.cpp
  library/sym/symbolpainter.h
  library/sym/symbolpin.cpp
  library/sym/symbolpin.h
  network/filedownload.cpp
  network/filedownload.h
  network/networkaccessmanager.cpp
  network/networkaccessmanager.h
  network/networkrequest.cpp
  network/networkrequest.h
  network/networkrequestbase.cpp
  network/networkrequestbase.h
  network/orderpcbapirequest.cpp
  network/orderpcbapirequest.h
  network/repository.cpp
  network/repository.h
  norms.h
  project/board/board.cpp
  project/board/board.h
  project/board/boardairwiresbuilder.cpp
  project/board/boardairwiresbuilder.h
  project/board/boardd356netlistexport.cpp
  project/board/boardd356netlistexport.h
  project/board/boarddesignrules.cpp
  project/board/boarddesignrules.h
  project/board/boardfabricationoutputsettings.cpp
  project/board/boardfabricationoutputsettings.h
  project/board/boardgerberexport.cpp
  project/board/boardgerberexport.h
  project/board/boardpainter.cpp
  project/board/boardpainter.h
  project/board/boardpickplacegenerator.cpp
  project/board/boardpickplacegenerator.h
  project/board/boardplanefragmentsbuilder.cpp
  project/board/boardplanefragmentsbuilder.h
  project/board/drc/boardclipperpathgenerator.cpp
  project/board/drc/boardclipperpathgenerator.h
  project/board/drc/boarddesignrulecheck.cpp
  project/board/drc/boarddesignrulecheck.h
  project/board/drc/boarddesignrulecheckmessages.cpp
  project/board/drc/boarddesignrulecheckmessages.h
  project/board/drc/boarddesignrulechecksettings.cpp
  project/board/drc/boarddesignrulechecksettings.h
  project/board/items/bi_airwire.cpp
  project/board/items/bi_airwire.h
  project/board/items/bi_base.cpp
  project/board/items/bi_base.h
  project/board/items/bi_device.cpp
  project/board/items/bi_device.h
  project/board/items/bi_footprintpad.cpp
  project/board/items/bi_footprintpad.h
  project/board/items/bi_hole.cpp
  project/board/items/bi_hole.h
  project/board/items/bi_netline.cpp
  project/board/items/bi_netline.h
  project/board/items/bi_netpoint.cpp
  project/board/items/bi_netpoint.h
  project/board/items/bi_netsegment.cpp
  project/board/items/bi_netsegment.h
  project/board/items/bi_plane.cpp
  project/board/items/bi_plane.h
  project/board/items/bi_polygon.cpp
  project/board/items/bi_polygon.h
  project/board/items/bi_stroketext.cpp
  project/board/items/bi_stroketext.h
  project/board/items/bi_via.cpp
  project/board/items/bi_via.h
  project/bomgenerator.cpp
  project/bomgenerator.h
  project/circuit/circuit.cpp
  project/circuit/circuit.h
  project/circuit/componentinstance.cpp
  project/circuit/componentinstance.h
  project/circuit/componentsignalinstance.cpp
  project/circuit/componentsignalinstance.h
  project/circuit/netclass.cpp
  project/circuit/netclass.h
  project/circuit/netsignal.cpp
  project/circuit/netsignal.h
  project/erc/electricalrulecheck.cpp
  project/erc/electricalrulecheck.h
  project/erc/electricalrulecheckmessages.cpp
  project/erc/electricalrulecheckmessages.h
  project/project.cpp
  project/project.h
  project/projectlibrary.cpp
  project/projectlibrary.h
  project/projectloader.cpp
  project/projectloader.h
  project/schematic/items/si_base.cpp
  project/schematic/items/si_base.h
  project/schematic/items/si_netlabel.cpp
  project/schematic/items/si_netlabel.h
  project/schematic/items/si_netline.cpp
  project/schematic/items/si_netline.h
  project/schematic/items/si_netpoint.cpp
  project/schematic/items/si_netpoint.h
  project/schematic/items/si_netsegment.cpp
  project/schematic/items/si_netsegment.h
  project/schematic/items/si_polygon.cpp
  project/schematic/items/si_polygon.h
  project/schematic/items/si_symbol.cpp
  project/schematic/items/si_symbol.h
  project/schematic/items/si_symbolpin.cpp
  project/schematic/items/si_symbolpin.h
  project/schematic/items/si_text.cpp
  project/schematic/items/si_text.h
  project/schematic/schematic.cpp
  project/schematic/schematic.h
  project/schematic/schematicpainter.cpp
  project/schematic/schematicpainter.h
  rulecheck/rulecheckmessage.cpp
  rulecheck/rulecheckmessage.h
  serialization/fileformatmigration.cpp
  serialization/fileformatmigration.h
  serialization/fileformatmigrationunstable.cpp
  serialization/fileformatmigrationunstable.h
  serialization/fileformatmigrationv01.cpp
  serialization/fileformatmigrationv01.h
  serialization/serializablekeyvaluemap.h
  serialization/serializableobjectlist.h
  serialization/sexpression.cpp
  serialization/sexpression.h
  sqlitedatabase.cpp
  sqlitedatabase.h
  systeminfo.cpp
  systeminfo.h
  types/alignment.cpp
  types/alignment.h
  types/angle.cpp
  types/angle.h
  types/boundedunsignedratio.cpp
  types/boundedunsignedratio.h
  types/circuitidentifier.h
  types/elementname.h
  types/layer.cpp
  types/layer.h
  types/length.cpp
  types/length.h
  types/lengthunit.cpp
  types/lengthunit.h
  types/maskconfig.cpp
  types/maskconfig.h
  types/point.cpp
  types/point.h
  types/ratio.cpp
  types/ratio.h
  types/signalrole.cpp
  types/signalrole.h
  types/uuid.cpp
  types/uuid.h
  types/version.cpp
  types/version.h
  utils/clipperhelpers.cpp
  utils/clipperhelpers.h
  utils/mathparser.cpp
  utils/mathparser.h
  utils/qtmetatyperegistration.h
  utils/scopeguard.h
  utils/scopeguardlist.h
  utils/signalslot.h
  utils/tangentpathjoiner.cpp
  utils/tangentpathjoiner.h
  utils/toolbox.cpp
  utils/toolbox.h
  utils/transform.cpp
  utils/transform.h
  workspace/theme.cpp
  workspace/theme.h
  workspace/themecolor.cpp
  workspace/themecolor.h
  workspace/workspace.cpp
  workspace/workspace.h
  workspace/workspacelibrarydb.cpp
  workspace/workspacelibrarydb.h
  workspace/workspacelibrarydbwriter.cpp
  workspace/workspacelibrarydbwriter.h
  workspace/workspacelibraryscanner.cpp
  workspace/workspacelibraryscanner.h
  workspace/workspacesettings.cpp
  workspace/workspacesettings.h
  workspace/workspacesettingsitem.cpp
  workspace/workspacesettingsitem.h
  workspace/workspacesettingsitem_genericvalue.h
  workspace/workspacesettingsitem_genericvaluelist.h
  workspace/workspacesettingsitem_keyboardshortcuts.cpp
  workspace/workspacesettingsitem_keyboardshortcuts.h
  workspace/workspacesettingsitem_themes.cpp
  workspace/workspacesettingsitem_themes.h
)
target_include_directories(
  librepcb_core
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../.."
)
target_link_libraries(
  librepcb_core
  PRIVATE common
          # Third party
          DelaunayTriangulation::DelaunayTriangulation
          Dxflib::Dxflib
          FontoBene::FontoBeneQt5
          MuParser::MuParser
          QuaZip::QuaZip
)
target_link_libraries(
  librepcb_core
  PUBLIC # Third party
         Optional::Optional
         Polyclipping::Polyclipping
         TypeSafe::TypeSafe
         # Qt
         Qt5::Concurrent
         Qt5::Core
         Qt5::Gui
         Qt5::Network
         Qt5::PrintSupport
         Qt5::Sql
         Qt5::Svg
         # Link with -lproc on SunOS / Solaris / Illumos
         # This is required for proc_get_psinfo
         $<$<STREQUAL:$<PLATFORM_ID>,SunOS>:proc>
)

# Alias to namespaced variant
add_library(LibrePCB::Core ALIAS librepcb_core)
