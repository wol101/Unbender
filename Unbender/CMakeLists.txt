cmake_minimum_required(VERSION 3.16)

project(Unbender VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Unbender
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        GraphicsView.h GraphicsView.cpp
        FindCentreLine.h FindCentreLine.cpp
        MarkerItem.h MarkerItem.cpp
        Image.h
        OpenCVTools.h OpenCVTools.cpp





    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Unbender APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Unbender SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Unbender
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Unbender PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Link different libraries depending on build type
target_link_libraries(Unbender PRIVATE
    $<$<CONFIG:Debug>:"c:/dev/opencv/build/x64/vc16/lib/opencv_world4120d">
    $<$<CONFIG:Release>:"c:/dev/opencv/build/x64/vc16/lib/opencv_world4120">
)


target_include_directories(Unbender PRIVATE "c:/dev/opencv/build/include" )


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Unbender)
endif()
set_target_properties(Unbender PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Unbender
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Unbender)
endif()

add_custom_command(TARGET Unbender POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "c:/dev/opencv/build/x64/vc16/bin/opencv_videoio_ffmpeg4120_64.dll" ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND $<$<CONFIG:Debug>:${CMAKE_COMMAND}> -E copy_if_different "c:/dev/opencv/build/x64/vc16/bin/opencv_world4120d.dll" ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND $<$<CONFIG:Release>:${CMAKE_COMMAND}> -E copy_if_different "c:/dev/opencv/build/x64/vc16/bin/opencv_world4120.dll" ${CMAKE_CURRENT_BINARY_DIR}
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_custom_command(TARGET Unbender POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "${CMAKE_CURRENT_BINARY_DIR}/Unbender.exe" # this copies the Qt dlls and a fairly minimal set of plugins
    )
endif()

