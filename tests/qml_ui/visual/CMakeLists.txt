# 视觉用例:Qt Quick Test 框架
# main.cpp 含 QUICK_TEST_MAIN_WITH_SETUP,自动扫描本目录下 tst_*.qml
qt_add_executable(qml_ui_visual main.cpp)
target_link_libraries(qml_ui_visual PRIVATE
    Qt6::QuickTest
    Qt6::Quick
    qml_ui_support
)
target_compile_features(qml_ui_visual PRIVATE cxx_std_20)
# 显式启用 AUTOMOC:main.cpp 内 TestSetup 含 Q_OBJECT,
# 需 moc 生成 main.moc 供 #include "main.moc" 使用
set_target_properties(qml_ui_visual PROPERTIES AUTOMOC ON)

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
    ENVIRONMENT "GDOWNLOAD_TEST=1;QML_UI_ARTIFACT_DIR=${CMAKE_BINARY_DIR}/test_artifacts/qml_ui")
