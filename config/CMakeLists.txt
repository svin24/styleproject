message(STATUS "preparing config")

#shared settings lib...
set(PNAME dspsettings)

if (QT5BUILD)
    set(PNAME dspsettings_qt5)
endif (QT5BUILD)

project(${PNAME})

set(sharedSRCS settings.cpp)
set(sharedHDRS settings.h)

add_library(${PNAME} SHARED ${sharedSRCS} ${sharedHDRS})

if (QT5BUILD)
    target_link_libraries(${PNAME} Qt5::Core)
else (QT5BUILD)
    target_link_libraries(${PNAME} ${QT_LIBRARIES})
endif (QT5BUILD)

install(TARGETS ${PNAME} LIBRARY DESTINATION ${LIBDIR})
install(FILES ${sharedHDRS} DESTINATION include/dsp)

if (QT5BUILD)
    set(PRO_NAME styleproject_config_qt5)
else (QT5BUILD)
    set(PRO_NAME styleproject_config)
endif (QT5BUILD)
    
# file(GLOB configSRCS *.cpp)
# file(GLOB configHDRS *.h)
set(configSRCS main.cpp styleconfig.cpp)
set(configHDRS styleconfig.h)

if (QT5BUILD)
#     qt5_add_resources(${configSRCS})
    qt5_wrap_ui(UI_HDRS config.ui)
    qt5_wrap_cpp(MOC_SRCS ${configHDRS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
else (QT5BUILD)
#     qt4_add_resources(${configSRCS})
    qt4_wrap_ui(UI_HDRS config.ui)
    qt4_wrap_cpp(MOC_SRCS ${configHDRS})
endif (QT5BUILD)

# standalone application to launch from terminal w/o going to systemsettings
project(${PRO_NAME})
add_executable(${PRO_NAME} ${UI_HDRS} ${configSRCS} ${MOC_SRCS} ${configHDRS})

if (QT5BUILD)
    target_link_libraries(${PRO_NAME} Qt5::Core Qt5::Gui Qt5::Widgets ${PNAME})
else (QT5BUILD)
    target_link_libraries(${PRO_NAME} ${QT_LIBRARIES} ${PNAME})
endif (QT5BUILD)
install(TARGETS ${PRO_NAME} DESTINATION bin)

# the systemsettings module in kde
if (NOT QT5BUILD AND KDE4_FOUND)
    project(kstyle_styleproject_config)
    kde4_add_plugin(kstyle_styleproject_config ${configSRCS} ${UI_HDRS})
    target_link_libraries(kstyle_styleproject_config ${QT_LIBRARIES} ${PNAME})
    install(TARGETS kstyle_styleproject_config DESTINATION ${PLUGIN_INSTALL_DIR})
endif (NOT QT5BUILD AND KDE4_FOUND)