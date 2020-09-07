## Build visocyte

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# Set your files and resources here
set(Srcs
  Viewer.cpp
  Visocyte.cpp
  Reader.cpp
  ImarisReader.cpp
  MotocyteReader.cpp
  SpatiocyteReader.cpp
  Util.cpp
  )

set(Hdrs
  Viewer.hpp
  Reader.hpp
  ImarisReader.hpp
  MotocyteReader.hpp
  SpatiocyteReader.hpp
  Util.hpp
  )

set(UIs
  Viewer.ui)

set(QRCs
  Icons/icons.qrc)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

source_group("Resources" FILES
  ${UIs}
  ${QRCs})

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_executable(visocyte MACOSX_BUNDLE
  ${Srcs} ${Hdrs} ${UIs} ${QRCs})

target_link_libraries(visocyte
  PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    VTK::CommonCore
    VTK::FiltersSources
    VTK::InfovisCore
    VTK::IOInfovis
    VTK::IOImage
    VTK::RenderingFreeType
    VTK::RenderingFreeTypeFontConfig
    VTK::ViewsQt
    #VTK::FiltersModeling
    VTK::RenderingAnnotation
    VTK::GUISupportQt)

target_include_directories(visocyte PUBLIC
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}"
      "$<TARGET_PROPERTY:VTK::CommonCore,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::FiltersSources,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::InfovisCore,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::RenderingFreeType,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::ViewsQt,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::IOInfovis,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::IOImage,INTERFACE_INCLUDE_DIRECTORIES>"
      #"$<TARGET_PROPERTY:VTK::FiltersModeling,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::RenderingAnnotation,INTERFACE_INCLUDE_DIRECTORIES>"
      "$<TARGET_PROPERTY:VTK::GUISupportQt,INTERFACE_INCLUDE_DIRECTORIES>")

