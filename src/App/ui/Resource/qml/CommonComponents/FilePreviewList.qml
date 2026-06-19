import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk
Rectangle {
    id: root
    color: GTheme.bgWhite
    
    property var previewModel
    // 添加信号
    signal clearRequested()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Header - 固定在顶部
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: root.color
            z: 2
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                
                GCheckBox {
                    checked: true
                    enabled: previewModel !== null
                    onClicked: {
                        if (!previewModel) return  // previewModel 为空时点击会空指针崩溃
                        if (checked) previewModel.selectAll()
                        else previewModel.unselectAll()
                    }
                }
                
                Label {
                    text: qsTr("File Name")
                    Layout.fillWidth: true
                    Layout.minimumWidth: 200
                    color: GTheme.textRegular
                    font.pixelSize: 14
                }
                
                Label {
                    text: qsTr("Extension")
                    Layout.preferredWidth: 100
                    horizontalAlignment: Text.AlignLeft
                    color: GTheme.textRegular
                    font.pixelSize: 14
                }
                
                Label {
                    text: qsTr("Size")
                    Layout.preferredWidth: 100
                    horizontalAlignment: Text.AlignRight
                    color: GTheme.textRegular
                    font.pixelSize: 14
                }

                // 添加删除按钮
                Button {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    flat: true
                    
                    contentItem: Text {
                        text: "×"  // 使用 × 符号作为删除图标
                        color: GTheme.textRegular
                        font.pixelSize: 20
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    
                    background: Rectangle {
                        color: "transparent"
                        radius: 2
                    }
                    
                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }
                    
                    onClicked: {
                        root.clearRequested()  // 发送信号而不是直接调用clear
                    }
                }
            }
        }
        
        // ListView - 中间可滚动区域
        ListView {
            id: fileList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.previewModel
            spacing: 10
            
            ScrollBar.vertical: ScrollBar {
                active: true
                policy: ScrollBar.AsNeeded
            }
            
            delegate: Rectangle {
                width: fileList.width
                height: 40
                color: index % 2 ? (GTheme.dark ? GTheme.fillBase : GTheme.bgWhite) 
                               : (GTheme.dark ? GTheme.fillLight : GTheme.fillLight)
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 5
                    
                    GCheckBox {
                        checked: model.isSelected
                        onClicked: {
                            previewModel.toggleSelection(index)
                            // Controls2 的 CheckBox 点击会自动 toggle checked，破坏上面的绑定，
                            // 之后 selectAll/unselectAll 更新 model.isSelected 时该复选框不再同步，需重建绑定。
                            checked = Qt.binding(function() { return model.isSelected })
                        }
                    }
                    
                    Label {
                        text: model.fileName
                        Layout.fillWidth: true
                        Layout.minimumWidth: 200
                        elide: Text.ElideMiddle
                        color: GTheme.textRegular
                        font.pixelSize: 14
                    }
                    
                    Label {
                        text: model.fileExtension
                        Layout.preferredWidth: 100
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        color: GTheme.textRegular
                        font.pixelSize: 14
                    }
                    
                    Label {
                        text: model.fileSize
                        Layout.preferredWidth: 100
                        horizontalAlignment: Text.AlignRight
                        color: GTheme.textRegular
                        font.pixelSize: 14
                    }
                }
            }
        }
        
        // Footer - 固定在底部
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: root.color
            z: 2
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                
                Item {
                    Layout.fillWidth: true
                }
                
                Label {
                    text: previewModel ? qsTr("Selected: %1 files, Total %2")
                        .arg(previewModel.selectedCount)
                        .arg(previewModel.totalSize) : ""
                    color: GTheme.textRegular
                    font.pixelSize: 14
                }
            }
        }
    }
} 
