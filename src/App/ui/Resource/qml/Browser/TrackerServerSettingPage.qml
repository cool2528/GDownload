import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk
Rectangle {
    id:trackServerPage
    Layout.margins: 10
    Layout.fillWidth: true
    Layout.preferredHeight: 550
    color: "transparent"
    border.color: "#409EFF"
    border.width: 1
    RowLayout{
        id:comboxLayout
        spacing: 30
        Label{
            text: qsTr("Tracker Servers:")
            color: GTheme.dark ?  "#FFFFFF" : "#3b3b3b"
            font.pixelSize: 14
            Layout.preferredWidth: 100
            Layout.leftMargin: 10
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.minimumWidth: 450
            Layout.preferredHeight: 280
            Layout.margins: 10
            clip: true
            GridLayout {
                id: checkboxGroup
                columns: 3
                property var selectedItems: JSON.parse(SettingsManager.qTrackerSourceNames)
                Repeater {
                    model: ["ngosang-best-link","ngosang-best-mirror","ngosang-best-cdn","ngosang-all-link","ngosang-all-mirror","ngosang-all-cdn",
                        "ngosang-all_udp-link","ngosang-all_udp-mirror","ngosang-all_udp-cdn",
                        "ngosang-all_http-link","ngosang-all_http-mirror","ngosang-all_http-cdn",
                        "ngosang-all_https-link","ngosang-all_https-mirror","ngosang-all_https-cdn",
                        "XIU2-best-link","XIU2-best-cdn","XIU2-all-link","XIU2-all-cdn","XIU2-http-link","XIU2-http-cdn","XIU2-nohttp-link","XIU2-nohttp-cdn"]
                    GCheckBox {
                        text: modelData
                        checked: checkboxGroup.selectedItems.indexOf(modelData) !== -1
                        onClicked:{
                            let pos = checkboxGroup.selectedItems.indexOf(modelData)
                            if (checked) {
                                if(pos === -1){
                                    checkboxGroup.selectedItems.push(modelData)
                                }
                            } else {
                                if(pos !== -1){
                                    removeAllInPlace(checkboxGroup.selectedItems,modelData)
                                }
                            }
                            let new_names = JSON.stringify(checkboxGroup.selectedItems)
                            SettingsManager.SetTrackerSourceNames(new_names)
                        }
                        function removeAllInPlace(arr, value) {
                            for (var i = arr.length - 1; i >= 0; --i)
                                if (arr[i] === value) arr.splice(i, 1);
                        }
                    }
                }
            }
        }
    }

    RowLayout{
        anchors.top: serverResultScrollView.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        GButton{
            id:syncBtn
            type: 1
            Layout.alignment: Qt.AlignLeft
            Layout.leftMargin: 20
            Layout.preferredWidth: 120
            Layout.preferredHeight: 30
            text: qsTr("Sync")
            onClicked: {
                console.debug("sync")
                BrowserManager.SyncTrackersServerlist()
            }
        }
        Item{
            Layout.fillWidth: true
        }
        GButtonSwitch{
            id:autoUpdateSwitch
            checked: SettingsManager.qEnableTrackerSourceAutoUpdate
            Layout.alignment: Qt.AlignRight
            Layout.rightMargin: 20
            Layout.preferredWidth: 230
            Layout.preferredHeight: 30
            text: qsTr("Enable daily auto-update")
            font.pixelSize: 14
        }
    }

    ScrollView {
        id: serverResultScrollView
        anchors.top: comboxLayout.bottom
        anchors.topMargin: 5
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        height: 150 // ScrollView 保持固定高度，作为 TextArea 的视口

        TextArea{
            id:serverResult
            text: UtilsToolsManager.serverList
            color: GTheme.dark ? "#ffffff" : "#303133"
            placeholderTextColor: GTheme.dark ? "#9a9a9a" : "#bababa"
            background: Rectangle{
                implicitHeight: serverResult.implicitHeight 
                implicitWidth: serverResultScrollView.availableWidth 
                color: GTheme.dark  ? "#303030" : "#ffffff"
                border.color: GTheme.dark ? serverResult.enabled ? "#5151f9" : "#545454" : serverResult.enabled ? "#5151f9" : "#b8bcc5"
            }
        }
    }

}
