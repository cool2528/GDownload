import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform
import "../CommonComponents"
import "../Utils/utils.js" as Utils
import gdl.sdk

// eD2k 分享页：目录 chips + 上传统计条 + 分享文件列表
ColumnLayout {
    id: root
    spacing: GTheme.spaceMD

    property var sharedModel: Ed2kManager.GetSharedFileModel()
    property var sharedDirs: Ed2kManager.GetSharedDirs()

    function refreshDirs() { root.sharedDirs = Ed2kManager.GetSharedDirs() }

    // 字节数格式化：本地实现，语义对齐 C++ 侧 QLocale().formattedDataSize()（IEC 单位、二进制进制、保留两位小数）
    // 之所以在 QML 侧另写一份而非复用 C++ 格式化文本，是因为 uploadSpeedBps/totalUploaded
    // 是裸 double 属性（无配套的 xxxText 角色/属性），Utils.js 里也没有现成的字节格式化函数
    function formatBytes(bytes) {
        var value = Number(bytes)
        if (!isFinite(value) || value < 0) value = 0
        var units = ["B", "KiB", "MiB", "GiB", "TiB", "PiB"]
        var unitIndex = 0
        while (value >= 1024 && unitIndex < units.length - 1) {
            value /= 1024
            unitIndex++
        }
        return unitIndex === 0 ? (Math.round(value) + " " + units[unitIndex])
                                : (value.toFixed(2) + " " + units[unitIndex])
    }

    Connections {
        target: Ed2kManager
        function onShareOpFailed(error) {
            ToastManager.ShowError(qsTr("Share operation failed: %1").arg(error))
        }
    }

    // 目录 chips 行
    // 内容必须经 contentItem 传入：GCard 的隐式高度只统计 contentItem
    // (直接子项进的是 Control.data，卡片会塌缩成 2*padding 并把内容裁掉)
    GCard {
        Layout.fillWidth: true
        outlined: true
        padding: GTheme.spaceSM

        contentItem: RowLayout {
            spacing: GTheme.spaceSM

            Flow {
                Layout.fillWidth: true
                spacing: GTheme.spaceXS
                Repeater {
                    model: root.sharedDirs
                    delegate: Rectangle {
                        objectName: "ed2kShareDirChip" + index
                        radius: GTheme.radiusRound
                        color: GTheme.fillLight
                        implicitHeight: 28
                        implicitWidth: chipRow.implicitWidth + GTheme.spaceMD * 2
                        RowLayout {
                            id: chipRow
                            anchors.centerIn: parent
                            spacing: GTheme.spaceXS
                            Text {
                                text: modelData
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textRegular
                                elide: Text.ElideMiddle
                            }
                            GButton {
                                objectName: "ed2kShareDirRemove" + index
                                iconName: "delete"
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Stop sharing this folder")
                                onClicked: { Ed2kManager.RemoveSharedDir(modelData); root.refreshDirs() }
                            }
                        }
                    }
                }
            }
            GButton {
                objectName: "ed2kShareAddDir"
                iconName: "add"
                text: qsTr("Add Folder")
                onClicked: shareFolderDialog.open()
            }
            GButton {
                objectName: "ed2kShareRescan"
                iconName: "refresh"
                text: qsTr("Rescan")
                enabled: root.sharedDirs.length > 0
                onClicked: Ed2kManager.RescanShares()
            }
        }
    }

    // 上传统计条（内容经 contentItem 传入，理由同上）
    GCard {
        Layout.fillWidth: true
        outlined: true
        padding: GTheme.spaceSM
        contentItem: RowLayout {
            spacing: GTheme.spaceSM
            Text {
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                // queued 恒 0 的引擎版本下该段自然显示 0, 不额外隐藏(引擎 v2.5.0 队列真实存在)
                text: qsTr("Upload: %1/s · Queued %2 · Active %3 · Total %4")
                      .arg(root.formatBytes(Ed2kManager.uploadSpeedBps))
                      .arg(Ed2kManager.uploadQueued)
                      .arg(Ed2kManager.uploadActive)
                      .arg(root.formatBytes(Ed2kManager.totalUploaded))
            }
            Item { Layout.fillWidth: true }
        }
    }

    // 分享文件列表（内容经 contentItem 传入，理由同上）
    GCard {
        Layout.fillWidth: true
        Layout.fillHeight: true
        outlined: true
        padding: GTheme.spaceSM

        contentItem: ColumnLayout {
            spacing: GTheme.spaceSM

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.sharedModel.count === 0
                Text {
                    anchors.centerIn: parent
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textSecondary
                    text: root.sharedDirs.length === 0
                          ? qsTr("No shared folders. Add a folder to start sharing.")
                          : qsTr("Scanning or no files found in shared folders.")
                }
            }

            ListView {
                objectName: "ed2kSharedFileList"
                id: sharedList
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.sharedModel.count > 0
                clip: true
                spacing: GTheme.spaceXS
                model: root.sharedModel
                ScrollBar.vertical: ScrollBar {}

                delegate: GCard {
                    width: sharedList.width
                    outlined: true
                    padding: GTheme.spaceSM
                    contentItem: RowLayout {
                        spacing: GTheme.spaceMD
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: model.fileName
                                elide: Text.ElideMiddle
                                font.pixelSize: GTheme.fontBody
                                color: GTheme.textPrimary
                            }
                            Text {
                                font.pixelSize: GTheme.fontCaption
                                color: GTheme.textSecondary
                                // uploaded 为会话内近似值(引擎按对端汇总)
                                text: qsTr("%1 · Uploaded ~%2 · %3 requests")
                                      .arg(model.fileSizeText).arg(model.uploadedText).arg(model.requests)
                            }
                        }
                        GButton {
                            objectName: "ed2kSharedCopy" + index
                            iconName: "link"
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Copy ed2k link")
                            onClicked: {
                                UtilsToolsManager.SetClipboardText(model.rawLink)
                                ToastManager.ShowSuccess(qsTr("Link copied"))
                            }
                        }
                        GButton {
                            objectName: "ed2kSharedOpenDir" + index
                            iconName: "folder"
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Open containing folder")
                            onClicked: UtilsToolsManager.OpenContainingFolder(model.filePath)
                        }
                    }
                }
            }
        }
    }

    // 目录选择(照 FolderSelector 内部的 FolderDialog 用法)
    FolderDialog {
        id: shareFolderDialog
        onAccepted: {
            Ed2kManager.AddSharedDir(Utils.urlToLocalPath(folder))
            root.refreshDirs()
        }
    }
}
