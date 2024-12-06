import QtQuick
import QtQuick.Controls.Basic
import gdl.sdk
import QtQuick.Layouts
import QtQuick.Controls

Popup {
    id:taskPage
    width: 640
    height: flick.contentHeight
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2
    modal: true
    padding: 0
    closePolicy:Popup.CloseOnEscape //| Popup.CloseOnPressOutside | Popup.CloseOnReleaseOutside
    focus: true
    background: Rectangle{
        color: GTheme.dark ? "#2e2e2e" : "#ffffff"
    }
    contentItem: ScrollView {
        id:scrollView
        clip: true
        width: parent.width
        height: parent.height
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentItem: Flickable{
            id:flick
            implicitWidth: flick.width
            contentHeight: columnLayout.height
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout{
                id:columnLayout
                width: parent.width
                ListView{
                    id:tabTitle
                    orientation: ListView.Horizontal
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 20
                    Layout.topMargin: 10
                    spacing: 20
                    Layout.maximumHeight: 40
                    Layout.preferredHeight: 40
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
                                color: index === tabTitle.currentIndex || parent.hovered ? "#5151f9" : GTheme.dark ? "#ffffff" :"#2a2b2d"
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
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    height: 2
                    z:tabTitle.z - 1
                    color:  GTheme.dark ? "#555555" : "#e0e3ea"
                }
                // tab Page
                StackLayout{
                    id:taskPageLayout
                    currentIndex: tabTitle.currentIndex
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    height: 120
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
                                color: GTheme.dark  ? "#303030" : "#ffffff"
                                border.color: GTheme.dark ? input.enabled ? "#5151f9" : "#545454" : input.enabled ? "#5151f9" : "#b8bcc5"
                            }
                        }
                    }
                    Rectangle{
                        id:seedPage
                        GDropArea{
                            anchors.fill: parent
                            id:dropTorent
                        }

                        color: GTheme.dark ? "#2e2e2e" : "#ffffff"
                    }
                    function geturls(){
                        if(currentIndex == 0){
                            return input.text
                        }else{
                           return dropTorent.path
                        }

                    }
                }

                //General Configuration
                ColumnLayout{
                    id:generalConfig
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    GridLayout{
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 30
                        rowSpacing: 10
                        // rename
                        Label{
                            id:rename
                            Layout.leftMargin: 10
                            text: qsTr("Rename:")
                            font.pixelSize: 14
                            color:GTheme.dark ? "#d9d9d9" : "#68696d"
                        }
                        RowLayout{
                            id:renameLayout
                            GTextField{
                                id:renameEdit
                                placeholderText: qsTr("Optional")
                                Layout.fillWidth: true
                                Layout.maximumHeight: 30
                                Layout.preferredHeight: 30
                            }
                            Label{
                                id:splits
                                text: qsTr("Splits:")
                                font.pixelSize: 14
                                color:GTheme.dark ? "#d9d9d9" : "#68696d"
                            }
                            GSpinBox{
                                id:spinbox
                                from: 1
                                to:64
                                value: 64
                                focus: true
                            }
                        }
                        Label{
                            id:save
                            Layout.leftMargin: 10
                            text: qsTr("Save to:")
                            font.pixelSize: 14
                            color:GTheme.dark ? "#d9d9d9" : "#68696d"
                        }
                        FolderSelector{
                            id:savePath
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                        }
                    }
                }


                //Additional Configuration
                ColumnLayout {
                    id: additionalConfig
                    visible: advanced.checked
                    Layout.fillWidth: true
                    Layout.margins: 10
                    Layout.rightMargin: 20
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10

                        Label {
                            text: qsTr("User-Agent:")
                            font.pixelSize: 14
                            color: GTheme.dark ? "#d9d9d9" : "#68696d"
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: qsTr("User-Agent")
                        }

                        Label {
                            text: qsTr("Authorization:")
                            font.pixelSize: 14
                            color: GTheme.dark ? "#d9d9d9" : "#68696d"
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: qsTr("Authorization")
                        }

                        Label {
                            text: qsTr("Referer:")
                            font.pixelSize: 14
                            color: GTheme.dark ? "#d9d9d9" : "#68696d"
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: qsTr("Referer")
                        }

                        Label {
                            text: qsTr("Cookie:")
                            font.pixelSize: 14
                            color: GTheme.dark ? "#d9d9d9" : "#68696d"
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        }
                        GTextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            placeholderText: qsTr("Cookie")
                        }
                    }
                }

                // submit layout
                RowLayout{
                    id:submitLayout
                    spacing: 10
                    Layout.bottomMargin: 20
                    GCheckBox{
                        id:advanced
                        Layout.maximumWidth: 150
                        Layout.alignment: Qt.AlignLeft
                        Layout.leftMargin: 10
                        Layout.preferredHeight: 15
                        text: qsTr("Advanced Options")
                    }
                    Item {
                        id: placeholder
                        Layout.fillWidth: true
                    }
                    GButton{
                        id:cancel
                        type: 0
                        Layout.alignment: Qt.AlignRight
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: 70
                        text: qsTr("Cancel")
                        onClicked:taskPage.close()
                    }

                    GButton{
                        id:submit
                        type: 1
                        Layout.alignment: Qt.AlignRight
                        Layout.rightMargin: 10
                        Layout.preferredHeight: 26
                        Layout.preferredWidth: 70
                        text: qsTr("Submit")
                        onClicked: {
                            BrowserManager.AddHttpTask(taskPageLayout.geturls(),{})
                        }
                    }

                }
            }
        }
    }
}
