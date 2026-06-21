# 视觉用例:Qt Quick Test 框架
# main.cpp 含 QUICK_TEST_MAIN_WITH_SETUP,自动扫描本目录下 tst_*.qml
#
# 资源:复用主程序 qrc,使视觉用例可通过 `qrc:/qml/Browser/DownloadPageView.qml`
# 等路径加载真实 QML 页面。resource_qml.qrc 含全部页面 QML,resource_icons.qrc
# 含 FluentIcons 字体与图标资源(FontIcon.qml 通过字体族名引用,字体仍需 InitFont
# 加载到 QFontDatabase 才能正确渲染图标)。
# 注意:qrc 内文件路径相对 qrc 所在目录(src/App/ui/Resource/),CMake 自动处理。
#
# mainWindow.qml 顶层是 FramelessWindow,依赖 FramelessHelper QML 模块。
# FramelessHelper Core 层在 Win32 安装窗口 proc 钩子,offscreen 平台无原生窗口,
# 加载 mainWindow.qml 会触发 ASSERT(qtWindowProc) 崩溃。故视觉用例不链接
# FramelessHelper,mainWindow.qml 的 Loader 加载失败,截图空白(3674 字节)。
# 这符合 tst_main_views.qml 注释中的设计预期。Task 9 重构主项目为库后,可在真实
# 窗口环境中补充 mainWindow 视觉用例。
qt_add_executable(qml_ui_visual main.cpp
    ${CMAKE_SOURCE_DIR}/src/App/ui/Resource/resource_qml.qrc
    ${CMAKE_SOURCE_DIR}/src/App/ui/Resource/resource_icons.qrc
)
target_link_libraries(qml_ui_visual PRIVATE
    Qt6::QuickTest
    Qt6::Quick
    qml_ui_support
)
target_compile_features(qml_ui_visual PRIVATE cxx_std_20)
# 显式启用 AUTOMOC + AUTORCC:main.cpp 内 TestSetup 含 Q_OBJECT,需 moc 生成
# main.moc 供 #include "main.moc" 使用;resource_qml.qrc / resource_icons.qrc
# 通过 AUTORCC 由 rcc 编译为 qrc_*.cpp 链入二进制,使 qrc:/qml/... 路径可解析
set_target_properties(qml_ui_visual PROPERTIES AUTOMOC ON AUTORCC ON)

# 复制测试所需的 Qt DLL 与 offscreen 平台插件到可执行文件目录
# gdownload 的 windeployqt 不会复制 Qt6Testd/Qt6QuickTestd(主程序不用 QtTest)
get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
get_filename_component(QT_PLATFORMS_DIR "${Qt6_DIR}/../../../plugins/platforms" ABSOLUTE)
set(QT_TEST_DLLS
    "$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
    "$<IF:$<CONFIG:Debug>,Qt6QuickTestd.dll,Qt6QuickTest.dll>"
)
add_custom_command(TARGET qml_ui_visual POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
        "$<TARGET_FILE_DIR:qml_ui_visual>/$<IF:$<CONFIG:Debug>,Qt6Testd.dll,Qt6Test.dll>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_BIN_DIR}/$<IF:$<CONFIG:Debug>,Qt6QuickTestd.dll,Qt6QuickTest.dll>"
        "$<TARGET_FILE_DIR:qml_ui_visual>/$<IF:$<CONFIG:Debug>,Qt6QuickTestd.dll,Qt6QuickTest.dll>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_PLATFORMS_DIR}/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
        "$<TARGET_FILE_DIR:qml_ui_visual>/platforms/$<IF:$<CONFIG:Debug>,qoffscreend.dll,qoffscreen.dll>"
    COMMENT "Copying Qt Test DLLs and offscreen platform plugin for headless testing")

add_test(NAME qml_ui_visual
    COMMAND qml_ui_visual -input ${CMAKE_CURRENT_SOURCE_DIR} -platform offscreen
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(qml_ui_visual PROPERTIES
    LABELS qml_ui
    FIXTURES_REQUIRED qml_ui_artifacts
    # QT_QUICK_CONTROLS_STYLE=Basic:GButton/GCheckBox 等自定义控件依赖可定制样式,
    # Windows 默认样式不支持 background/contentItem 自定义,会刷大量警告且部分控件
    # 渲染异常。Basic 样式纯 QML 实现,offscreen 平台兼容。
    ENVIRONMENT "GDOWNLOAD_TEST=1;QML_UI_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test_artifacts/qml_ui;QT_QUICK_CONTROLS_STYLE=Basic")
