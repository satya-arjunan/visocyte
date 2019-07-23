## Build VTK
set(BUILD_TESTING OFF)
set(vtk_cmake_dir "${CMAKE_CURRENT_SOURCE_DIR}/vtk/CMake")
list(APPEND CMAKE_MODULE_PATH
  "${vtk_cmake_dir}")
include(vtkCompilerChecks)
include(vtkCompilerPlatformFlags)
include(vtkCompilerExtraFlags)
include(vtkInitializeBuildType)
include(vtkSupportMacros)
include(vtkDirectories)

get_property(generator_is_multi_config GLOBAL
  PROPERTY GENERATOR_IS_MULTI_CONFIG)
set_property(GLOBAL
  PROPERTY
    "_vtk_module_autoinit_include" "\"vtkAutoInit.h\"")

## if a VTK include header file is missing:
## 1. search for the header file under src/vtk
## 2. after finding out the directory of the file, include the module of the 
##    directory below
## 3. look in build/src/lib/cmake/vtk/VTK-targets.cmake for module names
## 4. to test if the module name is valid, grep the module name in src/vtk
## 5. cmake ..
## 6. make -j16
set(vtk_requested_modules)
list(APPEND vtk_requested_modules
  VTK::CommonCore
  VTK::CommonDataModel
  VTK::CommonColor
  VTK::WrappingTools
  VTK::GUISupportQt
  VTK::ViewsQt
  VTK::IOImage
  VTK::ImagingColor
  VTK::ImagingGeneral
  VTK::RenderingCore
  VTK::RenderingOpenGL2
  VTK::RenderingGL2PSOpenGL2
  VTK::RenderingContextOpenGL2
  VTK::RenderingGL2PSOpenGL2
  VTK::RenderingFreeType
  VTK::RenderingFreeTypeFontConfig
  VTK::RenderingQt
  VTK::ChartsCore
  VTK::ViewsContext2D
  VTK::FiltersGeneral
  VTK::FiltersProgrammable
  VTK::FiltersSources
  VTK::InteractionImage
  VTK::InteractionStyle
  VTK::InteractionWidgets
  VTK::RenderingCore
  VTK::IOInfovis
  VTK::IOImage
  VTK::InfovisCore)

set(vtk_rejected_modules)
set(vtk_enable_tests "OFF")

include(vtkCMakeBackports)
include(vtkModule)
include(vtkEncodeString)
include(vtkObjectFactory)
include(vtkExternalData)
include(vtkVersion)

set(VTK_WRAP_PYTHON OFF)
set(VTK_WRAP_JAVA OFF)

vtk_module_find_modules(vtk_module_files "${CMAKE_CURRENT_SOURCE_DIR}/vtk")
vtk_module_find_kits(vtk_kit_files "${CMAKE_CURRENT_SOURCE_DIR}/vtk")

vtk_module_scan(
  MODULE_FILES        ${vtk_module_files}
  KIT_FILES           ${vtk_kit_files}
  REQUEST_MODULES     ${vtk_requested_modules}
                      ${visocyte_required_modules}
  REJECT_MODULES      ${vtk_rejected_modules}
  PROVIDES_MODULES    vtk_modules
  PROVIDES_KITS       vtk_kits
  REQUIRES_MODULES    vtk_required_modules
  UNRECOGNIZED_MODULES vtk_unrecognized_modules
  WANT_BY_DEFAULT     OFF
  ENABLE_TESTS        OFF
  HIDE_MODULES_FROM_CACHE ON)

if (vtk_required_modules OR vtk_unrecognized_modules)
  message(FATAL_ERROR
    "The following modules were requested or required, but not found: "
    "${vtk_required_modules};${vtk_unrecognized_modules}.")
endif ()

set(vtk_cmake_destination "${CMAKE_INSTALL_LIBDIR}/cmake/vtk")

vtk_module_build(
  MODULES             ${vtk_modules}
  KITS                ${vtk_kits}
  PACKAGE             VTK
  INSTALL_EXPORT      VTK
  HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/visualizer_vtk"
  CMAKE_DESTINATION   "${vtk_cmake_destination}")

set(vtk_cmake_build_dir "${CMAKE_CURRENT_BINARY_DIR}/${vtk_cmake_destination}")
include("${CMAKE_CURRENT_SOURCE_DIR}/vtk/CMake/vtkInstallCMakePackage.cmake")

add_subdirectory(visocyte)
