import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
import QtQuick.Layouts
import QtQuick.Controls
Popup {
    id:taskPage
    width: 640
    height: 320
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    padding: 0
    closePolicy:Popup.CloseOnEscape //| Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    focus: true
    background: Rectangle{
        color: GTheme.dark ? "#2e2e2e" : "#ffffff"
    }
    contentItem: Item {
        ListView{
            id:tabTitle
            orientation: ListView.Horizontal
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            width: parent.width
            spacing: 20
            height: 40
            clip:true
            focus: true
            interactive: false
            model: [qsTr("URL"),qsTr("Torent")]
            displaced: Transition { NumberAnimation { properties: "x"; duration: 300; easing: Easing.OutExpo } }
            delegate: Item {
                id: delegateItem
                width: textMetrics.width
                height: tabTitle.height
                Button{
                    anchors.fill: parent
                    contentItem: Text {
                        id: name
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData
                        color: index === tabTitle.currentIndex || parent.hovered ? "#5151f9" : "#2a2b2d"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                    }
                    background: MouseArea{
                        cursorShape: Qt.PointingHandCursor;
                        acceptedButtons: Qt.NoButton
                    }
                    onClicked: {
                        tabTitle.currentIndex = index
                    }
                }
                TextMetrics{
                    id:textMetrics
                    font.pixelSize: name.font.pixelSize
                    text: modelData
                }
            }
            highlight: Rectangle{
                height: 2
                y:tabTitle.currentItem.y + tabTitle.currentItem.height - 2
                x:tabTitle.currentItem.x
                z:999
                width: tabTitle.currentItem.width
                color:"#5151f9"
                Behavior on x {
                    NumberAnimation {
                        easing.type: Easing.OutExpo
                        duration: 300
                    }
                }
                Behavior on width {
                    NumberAnimation {
                        easing.type: Easing.OutExpo
                        duration: 300
                    }
                }
            }
            highlightFollowsCurrentItem: false

        }
        Rectangle{
            id:splitLine
            anchors.top: tabTitle.bottom
            anchors.topMargin: -2
            anchors.left: tabTitle.left
            anchors.right: tabTitle.right
            width: parent.width
            height: 2
            z:tabTitle.z - 1
            color: "#e0e3ea"
        }
        // tab Page
        StackLayout{
            id:taskPageLayout
            currentIndex: tabTitle.currentIndex
            anchors.top: splitLine.bottom
            anchors.topMargin: 10
            anchors.left: tabTitle.left
            anchors.right: tabTitle.right
            height: 180
            Rectangle{
                id:linkingPage
                color: GTheme.dark ? "#2e2e2e" : "#ffffff"
                TextArea{
                    id:input
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    font.pixelSize: 12
                    placeholderText: qsTr("One task url per line (supports magnet)")
                    color: GTheme.dark ? "#9a9a9a" : "#bababa"
                    background: Rectangle{
                        implicitHeight: taskPageLayout.height
                        implicitWidth: taskPageLayout.width
                        border.color: GTheme.dark ? input.enabled ? "#5151f9" : "#545454" : input.enabled ? "#5151f9" : "#b8bcc5"
                    }
                }
            }
            Rectangle{
                id:seedPage
                color: GTheme.dark ? "#2e2e2e" : "#ffffff"
            }
        }
        // General Configuration
        Rectangle{
            id:generalConfig
            color: GTheme.dark ? "#2e2e2e" : "#ffffff"
            anchors{
                top:taskPageLayout.bottom
                topMargin: 10
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            Label{
                id:rename
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.top: parent.top
                anchors.topMargin: 10
                text: qsTr("Rename:")
                font.pixelSize: 14
                color:GTheme.dark ? "#d9d9d9" : "#68696d"
            }
            TextField{
                id:renameEdit
                anchors{
                    top: parent.top
                    left: rename.right
                    leftMargin: 20
                }
                placeholderText: qsTr("Optional")
                width: 257
                height: 30
            }
            Label{
                id:splits
                anchors.left: renameEdit.right
                anchors.leftMargin: 10
                anchors.top: rename.top
                text: qsTr("Splits:")
                font.pixelSize: 14
                color:GTheme.dark ? "#d9d9d9" : "#68696d"
            }
            SpinBox{
                id:slicesNum
                height: 30
                anchors{
                    left: splits.right
                    leftMargin: 20
                    top: renameEdit.top
                }

                value: 64
                from: 1
                to:64
            }
        }
    }
}
