cmake_minimum_required(VERSION 2.8.12)
option(QT5BUILD "build for qt5" ON)
option(DEBUG "verbose debugging messages in output" OFF)

if (DEBUG)
    add_definitions(-DDEBUG=1)
endif (DEBUG)

if (NOT LIBDIR)
    set(LIBDIR lib)
endif (NOT LIBDIR)

if (QT5BUILD)
    set(PROJECT styleproject_qt5)
else (QT5BUILD)
    set(PROJECT styleproject)
endif (QT5BUILD)

project(${PROJECT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (QT5BUILD)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Gui REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5X11Extras)
    find_package(Qt5OpenGL)
    find_package(Qt5DBus)
    include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5X11Extras_INCLUDE_DIRS})
    if (Qt5DBus_FOUND)
        include_directories(${Qt5DBus_INCLUDE_DIRS})
        add_definitions(${Qt5DBus_DEFINITIONS})
        set(QTDBUS_FOUND ON)
    endif (Qt5DBus_FOUND)
    add_definitions(${Qt5Widgets_DEFINITIONS})
    find_package(ECM)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
    
    # uninstall target, stolen from http://www.cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
else (QT5BUILD)
    find_package(Qt4 REQUIRED)
    find_package(KDE4)
    include(${QT_USE_FILE})
    add_definitions(${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS})
    set(QTDBUS_FOUND ${QT_QTDBUS_FOUND})
    if (KDE4_FOUND)
	add_definitions(${KDE4_DEFINITIONS})
	include_directories(${KDE4_INCLUDES})
	include(KDE4Defaults)
	include(MacroLibrary)
    endif (KDE4_FOUND)
endif (QT5BUILD)

add_subdirectory(config)

#source
# file(GLOB SRCS *.cpp)
#file(GLOB HDRS *.h)
set(SRCS buttons.cpp contentssize.cpp events.cpp init.cpp inputs.cpp items.cpp overlay.cpp panels.cpp pixelmetric.cpp polish.cpp sliders.cpp stylehint.cpp dsp.cpp subrects.cpp tabs.cpp)
set(HDRS overlay.h dsp.h namespace.h)
set(MOC_HDRS overlay.h dsp.h)

if (QTDBUS_FOUND)
    message(STATUS "Found qtdbus, building bespin macmenu support")
    add_definitions(-DHASDBUS=1)
    if (NOT QT5BUILD)
        include_directories(${QT_QTDBUS_INCLUDE_DIR})
    endif (NOT QT5BUILD)
endif (QTDBUS_FOUND)

set_property(SOURCE moc_styleproject.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/styleproject.json)

if (QT5BUILD)
    qt5_wrap_cpp(MOC_SRCS ${MOC_HDRS})
else (QT5BUILD)
    qt4_wrap_cpp(MOC_SRCS ${MOC_HDRS})
endif (QT5BUILD)

add_library(${PROJECT} SHARED ${SRCS} ${MOC_SRCS} ${HDRS})

#xcb and x11....
#if we are qt5 then we want xcb... 
#but we can also work w/ x11, so we
#first check for xcb and if not, then
#for x11
if (QT5BUILD)
    find_package(XCB COMPONENTS XCB)
    if (XCB_FOUND)
        add_definitions(-DHASXCB=1)
        target_link_libraries(${PROJECT} XCB::XCB)
        include_directories(${XCB_INCLUDES})
    else (XCB_FOUND)
        find_package(X11)
        if (X11_FOUND)
            add_definitions(-DHASX11=1)
            include_directories(${X11_INCLUDES})
            target_link_libraries(${PROJECT} ${X11_X11_LIB} ${X11_LIBRARIES})
        endif (X11_FOUND)
    endif (XCB_FOUND)
    set(KF5_MIN_VERSION "5.9.0")
    find_package(KF5 ${KF5_MIN_VERSION} COMPONENTS
        WindowSystem
    )
    if (KF5_FOUND)
        include(KDEInstallDirs)
        include(KDECMakeSettings)
        include(KDECompilerSettings)
        target_link_libraries(${PROJECT} KF5::WindowSystem)
        add_definitions(-DHASKF5)
    endif (KF5_FOUND)
else (QT5BUILD)
    find_package(X11)
    if (X11_FOUND)
        add_definitions(-DHASX11=1)
        include_directories(${X11_INCLUDES})
        target_link_libraries(${PROJECT} ${X11_X11_LIB} ${X11_LIBRARIES})
    endif (X11_FOUND)
endif (QT5BUILD)
# set_target_properties(${PROJECT} PROPERTIES AUTOMOC ON)

add_subdirectory(stylelib)

if (QT5BUILD)
    target_link_libraries(${PROJECT} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::DBus Qt5::X11Extras dspsettings_qt5 dspstylelib_qt5)
    get_target_property(QMAKE Qt5::qmake IMPORTED_LOCATION)
    execute_process(COMMAND ${QMAKE} -query QT_INSTALL_PLUGINS OUTPUT_VARIABLE PLUGINPATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    install(TARGETS ${PROJECT} LIBRARY DESTINATION ${PLUGINPATH}/styles)
    find_package(KDecoration2)
    if (KDecoration2_FOUND)
        add_subdirectory(kwin)
    endif (KDecoration2_FOUND)
else (QT5BUILD)
    target_link_libraries(${PROJECT} ${QT_LIBRARIES} ${QT_QTDBUS_LIBRARY} dspsettings dspstylelib)
    install(TARGETS ${PROJECT} LIBRARY DESTINATION ${QT_PLUGINS_DIR}/styles)
    if (KDE4_FOUND)
        install(FILES styleproject.themerc DESTINATION ${DATA_INSTALL_DIR}/kstyle/themes)
        add_subdirectory(kwin)
    endif (KDE4_FOUND)
endif (QT5BUILD)


