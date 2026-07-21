import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../CommonComponents"
import gdl.sdk

// eD2k 搜索页：关键词 + 过滤条件 + 结果列表(按源数降序) + Load More
ColumnLayout {
    id: root
    spacing: GTheme.spaceMD

    // 未连接服务器时点击"Go to Servers"：由 Ed2kCenterPage 连接切到服务器 tab
    // (control 是 Ed2kCenterPage.qml 里的 id，跨文档不可见，故改用信号上抛)
    signal goToServers()

    property var resultModel: Ed2kManager.GetSearchResultModel()
    // 记录"最近一次实际发起的搜索"所用的来源(0=Server/1=Kad)，供 Load More 可见性判断，
    // 避免用户在结果展示期间切换下拉框导致 Load More 按钮跟错误的来源联动
    property int lastSearchSource: 0

    // 进入搜索页时刷新一次 Kad 状态，使"Kad 是否就绪"的判断/提示准确
    Component.onCompleted: Ed2kManager.RefreshKadStatus()

    Connections {
        target: Ed2kManager
        function onSearchFailed(error) {
            // Kad 搜索失败多因 Kad 未启用/未就绪(默认关闭,无节点)，
            // 给出明确引导而非透出笼统的引擎错误("connect failed")
            if (root.lastSearchSource === 1) {
                ToastManager.ShowError(
                    qsTr("Kad search is unavailable. Enable Kad in settings, or use Server search instead."))
            } else {
                ToastManager.ShowError(qsTr("Search failed: %1").arg(error))
            }
        }
    }

    // 搜索表单
    // 内容必须经 contentItem 传入：GCard 的隐式高度只统计 contentItem
    // (直接子项进的是 Control.data，卡片会塌缩成 2*padding 并把内容裁掉)
    GCard {
        Layout.fillWidth: true
        outlined: true
        padding: GTheme.spaceMD

        contentItem: RowLayout {
            spacing: GTheme.spaceSM

            GTextField {
                id: keywordInput
                objectName: "ed2kSearchKeyword"
                Layout.fillWidth: true
                placeholderText: qsTr("Enter keywords to search the eD2k network")
                onAccepted: searchButton.clicked()
            }
            GComBoBox {
                id: typeFilter
                objectName: "ed2kSearchType"
                // 顺序与 ed2k::server::FileType 枚举值一致(Any=0..CdImage=7)
                model: [qsTr("Any"), qsTr("Audio"), qsTr("Video"), qsTr("Image"),
                        qsTr("Program"), qsTr("Document"), qsTr("Archive"), qsTr("CD Image")]
            }
            GComBoBox {
                id: sourceFilter
                objectName: "ed2kSearchSource"
                model: [qsTr("Server"), qsTr("Kad")]
            }
            GButton {
                id: searchButton
                objectName: "ed2kSearchButton"
                text: Ed2kManager.searching ? qsTr("Searching...") : qsTr("Search")
                enabled: !Ed2kManager.searching && keywordInput.text.trim().length > 0
                onClicked: {
                    // Kad 源前置就绪检查:Kad 未启用/未生效(通常是启用后未重启)时,
                    // 直接给明确引导而非发起注定失败的搜索、让用户对着"没有结果"猜。
                    if (sourceFilter.currentIndex === 1 && !Ed2kManager.kadRunning) {
                        ToastManager.ShowError(
                            qsTr("Kad is not ready. Enable Kad in settings and restart the app, or use Server search."))
                        return
                    }
                    root.resultModel.clear()
                    root.lastSearchSource = sourceFilter.currentIndex
                    Ed2kManager.StartSearch(keywordInput.text, typeFilter.currentIndex, 0,
                                            sourceFilter.currentIndex)
                }
            }
        }
    }

    // 结果列表（内容经 contentItem 传入，理由同上）
    GCard {
        Layout.fillWidth: true
        Layout.fillHeight: true
        outlined: true
        padding: GTheme.spaceSM

        contentItem: ColumnLayout {
            spacing: GTheme.spaceSM

            // 占位态：未连接 / 空结果 / 搜索中
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.resultModel.count === 0

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: GTheme.spaceSM
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                        text: Ed2kManager.searching ? qsTr("Searching...")
                              : (!Ed2kManager.serverConnected && sourceFilter.currentIndex === 0)
                                ? qsTr("Not connected to any server")
                              : (sourceFilter.currentIndex === 1 && !Ed2kManager.kadRunning)
                                ? qsTr("Kad is not ready. Enable Kad in settings and restart the app.")
                                : qsTr("No results. Try different keywords.")
                    }
                    GButton {
                        Layout.alignment: Qt.AlignHCenter
                        visible: !Ed2kManager.serverConnected && !Ed2kManager.searching
                                 && sourceFilter.currentIndex === 0
                        text: qsTr("Go to Servers")
                        onClicked: root.goToServers()
                    }
                }
            }

            ListView {
                id: resultList
                objectName: "ed2kSearchResultList"
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.resultModel.count > 0
                clip: true
                spacing: GTheme.spaceXS
                model: root.resultModel
                ScrollBar.vertical: ScrollBar {}

                delegate: GCard {
                    width: resultList.width
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
                                text: qsTr("%1 · %2 sources (%3 complete)")
                                      .arg(model.fileSizeText).arg(model.sources).arg(model.completeSources)
                            }
                        }
                        GButton {
                            objectName: "ed2kResultCopy" + index
                            iconName: "link"
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Copy ed2k link")
                            onClicked: {
                                UtilsToolsManager.SetClipboardText(model.rawLink)
                                ToastManager.ShowSuccess(qsTr("Link copied"))
                            }
                        }
                        GButton {
                            objectName: "ed2kResultDownload" + index
                            text: qsTr("Download")
                            onClicked: {
                                if (BrowserManager.AddEd2kTask([model.rawLink], {})) {
                                    ToastManager.ShowSuccess(qsTr("Download started"))
                                } else {
                                    ToastManager.ShowError(qsTr("Failed to add download"))
                                }
                            }
                        }
                    }
                }
            }

            // Load More（仅服务器搜索有翻页）
            GButton {
                objectName: "ed2kLoadMoreButton"
                Layout.alignment: Qt.AlignHCenter
                visible: root.resultModel.count > 0 && root.lastSearchSource === 0
                enabled: !Ed2kManager.searching
                text: Ed2kManager.searching ? qsTr("Loading...") : qsTr("Load More")
                onClicked: Ed2kManager.LoadMore()
            }
        }
    }
}
