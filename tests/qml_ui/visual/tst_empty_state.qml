import QtQuick
import QtTest
import gdl.sdk
import "qrc:/qml/CommonComponents"
import "qrc:/tests/qml_ui/support"

TestCase {
    id: testCase
    name: "tst_empty_state"
    when: windowShown
    width: 520
    height: 360

    PageHarness {
        id: harness
        anchors.fill: parent
    }

    Component {
        id: emptyStateComponent

        Rectangle {
            width: 520
            height: 360
            color: GTheme.bgPage

            EmptyState {
                id: emptyState
                objectName: "auroraEmptyState"
                anchors.centerIn: parent
                width: Math.min(parent.width - GTheme.space2XL * 2, 420)
                title: qsTr("Nothing here yet")
                description: qsTr("Add a download or change the current filter to see results.")
                iconName: "download"
                actionText: qsTr("Add download")
                onActionTriggered: objectName = "auroraEmptyStateTriggered"
            }
        }
    }

    function captureState(tag, theme) {
        harness.themeMode = theme
        harness.load("")
        wait(80)
        var item = emptyStateComponent.createObject(harness)
        verify(item !== null)
        item.x = Math.round((harness.width - item.width) / 2)
        item.y = Math.round((harness.height - item.height) / 2)
        wait(120)
        var state = findChild(item, "auroraEmptyState")
        verify(state !== null)
        verify(state.width <= item.width)
        verify(state.height <= item.height)
        verify(Screenshot.capture(item, tag, theme))
        item.destroy()
    }

    function test_empty_state_light() { captureState("empty_state_light", "light") }
    function test_empty_state_dark() { captureState("empty_state_dark", "dark") }
}
