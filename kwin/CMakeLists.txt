message(STATUS "preparing kwin deco")

if (QT5BUILD)
    #NOTE: most of this copied from the default breeze deco and partly rewritten for dsp
    find_package(KF5 REQUIRED COMPONENTS CoreAddons ConfigWidgets WindowSystem GuiAddons)

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings)
    include(GenerateExportHeader)

    execute_process(COMMAND kf5-config --install module OUTPUT_VARIABLE MODULEPATH OUTPUT_STRIP_TRAILING_WHITESPACE)

    find_package(dbusmenu-qt5)

#     find_path(EFFECTS_H kwineffects.h PATH_SUFFIXES kf5)
#     
#     if (EFFECTS_H)
#         include_directories(${EFFECTS_H})
#     else (EFFECTS_H)
#         message(STATUS "didnt find kwineffects.h, not building effects")
#     endif (EFFECTS_H)
#     
#     find_library(KWIN_EFFECTS NAMES kwineffects PATH_SUFFIXES kf5)
#     find_library(KWIN_GLUTILS NAMES kwinglutils PATH_SUFFIXES kf5)
#     find_library(OPENGL NAMES GL)
# 
#     if (NOT KWIN_EFFECTS)
#         message(STATUS "didnt find kwineffects lib, not building effects")
#     endif (NOT KWIN_EFFECTS)
# 
#     if (NOT KWIN_GLUTILS)
#         message(STATUS "didnt find kwin glutils lib, not building effects")
#     endif (NOT KWIN_GLUTILS)
# 
#     if (NOT OPENGL)
#         message(STATUS "didnt find opengl, not building effects")
#     endif (NOT OPENGL)
# 
#     if (EFFECTS_H AND KWIN_GLUTILS AND KWIN_EFFECTS AND OPENGL)
#         add_subdirectory(effects)
#     endif (EFFECTS_H AND KWIN_GLUTILS AND KWIN_EFFECTS AND OPENGL)

    set(SRCS
        kwinclient2.cpp
        decobutton.cpp
        menubar.cpp
        xcbatoms.cpp)
    set(HDRS
        kwinclient2.h
        decobutton.h
        decoadaptor.h
        menubar.h
        xcbatoms.h)

    ### build library
    add_library(dspdecoration MODULE ${SRCS} ${HDRS})

    target_link_libraries(dspdecoration
        PUBLIC
            Qt5::Core
            Qt5::Gui
            dspsettings_qt5
            dspstylelib_qt5
        PRIVATE
            KDecoration2::KDecoration
            KF5::ConfigCore
            KF5::CoreAddons
            KF5::ConfigWidgets
            KF5::GuiAddons
            KF5::WindowSystem)
    if (QTDBUS_FOUND)
        target_link_libraries(dspdecoration PUBLIC Qt5::DBus)
    endif (QTDBUS_FOUND)

    if (dbusmenu-qt5_FOUND)
        target_link_libraries(dspdecoration PUBLIC dbusmenu-qt5)
        add_definitions(-DHASDBUSMENU=1)
    endif (dbusmenu-qt5_FOUND)

    if (XCB_FOUND)
        target_link_libraries(dspdecoration PUBLIC Qt5::X11Extras XCB::XCB)
    else (XCB_FOUND)
        if (X11_FOUND)
            target_link_libraries(dspdecoration PUBLIC Qt5::X11Extras ${X11_X11_LIB} ${X11_LIBRARIES})
        endif (X11_FOUND)
    endif (XCB_FOUND)
    
    install(TARGETS dspdecoration DESTINATION ${MODULEPATH}/org.kde.kdecoration2)
else (QT5BUILD)
    find_file(KDECORATION_H kdecoration.h ${KDE4_INCLUDES})
    if (KDECORATION_H)
        project(kwin3_styleproject)
        set(KWIN_SRCS factory.cpp kwinclient.cpp sizegrip.cpp)
	message(STATUS "kwin deco header found: " ${KDECORATION_H})
	qt4_wrap_cpp(MOC_ADAPTOR decoadaptor.h)
        kde4_add_plugin(kwin3_styleproject ${KWIN_SRCS} ${MOC_ADAPTOR})
        target_link_libraries(kwin3_styleproject styleproject ${QT_LIBRARIES} ${KDE4_KDEUI_LIBS} kdecorations)
        if (X11_FOUND)
            target_link_libraries(kwin3_styleproject ${X11_LIBRARIES})
         endif (X11_FOUND)
        install(TARGETS kwin3_styleproject DESTINATION ${PLUGIN_INSTALL_DIR})
        install(FILES styleproject.desktop DESTINATION ${DATA_INSTALL_DIR}/kwin)
    else (KDECORATION_H)
	message(WARNING "kwin deco header not found, not building deco even if you have kde4")
    endif (KDECORATION_H)
endif (QT5BUILD)
