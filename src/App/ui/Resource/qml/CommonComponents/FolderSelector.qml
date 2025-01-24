import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import gdl.sdk
import "../Utils/utils.js" as Utils

Item {
    id: folderSelector
    property string path: ""
    property bool readOnly: true
    property var historyPaths: ["C:/Users/Administrator/Downloads/Compressed","C:/Users/Administrator/Downloads/"]
    signal actived
    signal textChanged(string text)
    signal historySelected(string path)
    
    // 内部属性用于颜色管理
    readonly property var colors: {
        const darkTheme = {
            background: "#2E2E2E",
            backgroundFocus: Qt.lighter("#2E2E2E"),
            border: "#545454",
            borderFocus: "#5151f9",
            text: "#ffffff",
            buttonBg: "#2d2d2d",
            buttonIcon: "#c4c4c4",
            buttonIconHover: "#5151f9",
            popupBg: "#323232",
            historyItemHover: "#3c3c3c",
            historyIcon: "#808080"
        }
        
        const lightTheme = {
            background: "#ffffff", 
            backgroundFocus: Qt.lighter("#ffffff"),
            border: "#a9a9a9",
            borderFocus: "#5151f9",
            text: "#303030",
            buttonBg: "#f3f6f9",
            buttonIcon: "#8d9096",
            buttonIconHover: "#5151f9",
            popupBg: "#ffffff",
            historyItemHover: "#f5f5f5",
            historyIcon: "#a0a0a0"
        }
        
        return GTheme.dark ? darkTheme : lightTheme
    }

    // 历史记录按钮
    Button {
        id: historyBtn
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        width: 30
        height: 30
        
        background: Rectangle {
            color: colors.buttonBg
            border.color: historyBtn.hovered ? colors.borderFocus : colors.border
            border.width: 1
            radius: 2
            
            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        contentItem: Canvas {
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = historyBtn.hovered ? colors.buttonIconHover : colors.buttonIcon
                ctx.lineWidth = 2
                
                // 绘制时钟图标
                const centerX = width / 2
                const centerY = height / 2
                const radius = Math.min(width, height) / 3
                
                // 绘制圆形
                ctx.beginPath()
                ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI)
                ctx.stroke()
                
                // 绘制指针
                ctx.beginPath()
                ctx.moveTo(centerX, centerY)
                ctx.lineTo(centerX, centerY - radius * 0.6) // 分针
                ctx.moveTo(centerX, centerY)
                ctx.lineTo(centerX + radius * 0.4, centerY) // 时针
                ctx.stroke()
            }
            
            Connections {
                target: historyBtn
                function onHoveredChanged() { parent.requestPaint() }
            }
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        onClicked: historyPopup.opened ? historyPopup.close() : historyPopup.open()
    }

    TextField {
        id: textField
        anchors {
            left: historyBtn.right
            right: selectorBtn.left
            leftMargin: 5
            rightMargin: 5
            top: parent.top
            bottom: parent.bottom
        }
        readOnly: folderSelector.readOnly
        padding: 0
        leftPadding: 10
        
        color: colors.text
        text: path
        selectByMouse: true
        selectionColor: "#3078BB"
        font.pixelSize: 14
        
        background: Rectangle {
            color: parent.activeFocus ? colors.backgroundFocus : colors.background
            border.color: parent.activeFocus ? colors.borderFocus : colors.border
            border.width: parent.activeFocus ? 1.5 : 1
            radius: 2
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        onEditingFinished: {
            if (text !== path) {
                let tmp = text
                text = Qt.binding(function() { return path })
                folderSelector.textChanged(tmp)
            }
        }

        onActiveFocusChanged: {
            if (activeFocus) {
                selectAll()
                folderSelector.actived()
            }
        }
    }

    Button {
        id: selectorBtn
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        width: 30
        height: 30
        
        background: Rectangle {
            color: colors.buttonBg
            border.color: selectorBtn.hovered ? colors.borderFocus : colors.border
            border.width: 1
            radius: 2
            
            Behavior on border.color {
                ColorAnimation { duration: 150 }
            }
        }

        contentItem: Canvas {
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = selectorBtn.hovered ? colors.buttonIconHover : colors.buttonIcon
                ctx.lineWidth = 2
                
                // 绘制文件夹图标 - 调整大小和位置
                const margin = 4  // 减小边距以增大图标
                const w = width - 2 * margin
                const h = height - 2 * margin
                
                // 文件夹主体
                ctx.beginPath()
                ctx.moveTo(margin, margin + h * 0.3)
                ctx.lineTo(margin + w * 0.3, margin + h * 0.3)
                ctx.lineTo(margin + w * 0.4, margin)
                ctx.lineTo(margin + w, margin)
                ctx.lineTo(margin + w, margin + h)
                ctx.lineTo(margin, margin + h)
                ctx.closePath()
                ctx.stroke()
            }
            
            Connections {
                target: selectorBtn
                function onHoveredChanged() { parent.requestPaint() }
            }
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        onClicked: folderDialog.open()
    }

    Popup {
        id: historyPopup
        y: parent.height
        width: parent.width
        padding: 1
        
        background: Rectangle {
            color: colors.popupBg
            border.color: colors.border
            border.width: 1
            radius: 2
        }

        contentItem: ListView {
            id: historyList
            implicitHeight: Math.min(contentHeight, 200)
            model: folderSelector.historyPaths
            clip: true
            
            delegate: ItemDelegate {
                width: parent.width
                height: 36
                
                background: Rectangle {
                    color: parent.hovered ? colors.historyItemHover : "transparent"
                }
                
                contentItem: RowLayout {
                    spacing: 8
                    
                    Text {
                        text: "⏱"
                        color: colors.historyIcon
                        font.pixelSize: 14
                    }
                    
                    Text {
                        Layout.fillWidth: true
                        text: modelData
                        color: colors.text
                        elide: Text.ElideMiddle
                        font.pixelSize: 14
                    }
                    
                    IconButton {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        visible: parent.parent.hovered
                        iconSource: 0xE711
                        iconSize: 12
                        iconColor: colors.historyIcon
                        
                        onClicked: {
                            let index = folderSelector.historyPaths.indexOf(modelData)
                            if (index !== -1) {
                                let newPaths = folderSelector.historyPaths.slice()
                                newPaths.splice(index, 1)
                                folderSelector.historyPaths = newPaths
                            }
                        }
                    }
                }
                
                onClicked: {
                    // 直接修改path属性，这会触发文本框的更新
                    folderSelector.path = modelData
                    historyPopup.close()
                    folderSelector.actived()
                }
            }
            
            ScrollBar.vertical: ScrollBar {}
        }
    }

    FolderDialog {
        id: folderDialog
        folder: Qt.resolvedUrl(path)
        
        onAccepted: {
            if (folder !== "") {
                let newPath = Utils.urlToLocalPath(folder)
                folderSelector.path = newPath
                
                if (!folderSelector.historyPaths.includes(newPath)) {
                    let newPaths = folderSelector.historyPaths.slice()
                    newPaths.unshift(newPath)
                    if (newPaths.length > 10) {
                        newPaths.pop()
                    }
                    folderSelector.historyPaths = newPaths
                }
            }
            folder = Qt.binding(function() { return Qt.resolvedUrl(folderSelector.path) })
            folderSelector.actived()
        }
    }
}
