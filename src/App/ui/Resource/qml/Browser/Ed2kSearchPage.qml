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
    // 记录"最近一次实际发起的搜索"实际使用的来源(0=Server/1=Kad,Auto 已解析)，
    // 供 Load More 可见性判断，避免用户在结果展示期间切换下拉框导致按钮跟错误的来源联动
    property int lastSearchSource: 0
    // 本次点击搜索将实际使用的来源(0=Server/1=Kad)：下拉框选"自动"(index 0)时按
    // "已连接服务器优先，否则 Kad 就绪走 Kad，两者皆不可用回落服务器(失败提示引导)"解析；
    // 手动选定(index 1/2)时原样映射。开箱即用的关键：全新安装服务器未连接时 Kad 兜底可搜
    readonly property int effectiveSource: sourceFilter.currentIndex === 0
                                           ? (Ed2kManager.serverConnected ? 0
                                              : (Ed2kManager.kadRunning ? 1 : 0))
                                           : sourceFilter.currentIndex - 1
    // 是否已发起过搜索：零结果占位在"从未搜索"时显示来源引导，搜索过则显示"无结果"
    property bool hasSearched: false

    // 进入搜索页时刷新一次 Kad 状态，使"Kad 是否就绪"的判断/提示准确
    Component.onCompleted: Ed2kManager.RefreshKadStatus()

    Connections {
        target: Ed2kManager
        function onSearchFailed(error) {
            // Kad 搜索失败多因 Kad 尚未完成引导(节点未就绪)，
            // 给出明确引导而非透出笼统的引擎错误("connect failed")
            if (root.lastSearchSource === 1) {
                ToastManager.ShowError(
                    qsTr("Kad search is unavailable. Kad may still be starting; try again shortly or use Server search."))
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
                // "自动"为默认项:按 effectiveSource 的规则在点击时解析实际来源
                model: [qsTr("Auto"), qsTr("Server"), qsTr("Kad")]
            }
            GButton {
                id: searchButton
                objectName: "ed2kSearchButton"
                text: Ed2kManager.searching ? qsTr("Searching...") : qsTr("Search")
                enabled: !Ed2kManager.searching && keywordInput.text.trim().length > 0
                onClicked: {
                    // Kad 源前置就绪检查(仅用户手动强制 Kad 时可能命中;"自动"只在
                    // Kad 就绪时才会解析到 Kad):未就绪时直接给明确引导，
                    // 而非发起注定失败的搜索、让用户对着"没有结果"猜。
                    if (root.effectiveSource === 1 && !Ed2kManager.kadRunning) {
                        ToastManager.ShowError(
                            qsTr("Kad is not ready. Enable Kad in settings and restart the app, or use Server search."))
                        return
                    }
                    root.resultModel.clear()
                    root.hasSearched = true
                    root.lastSearchSource = root.effectiveSource
                    Ed2kManager.StartSearch(keywordInput.text, typeFilter.currentIndex, 0,
                                            root.effectiveSource)
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
                        // 占位提示跟随解析后的实际来源:自动模式下 Kad 兜底时提示 Kad 搜索可用,
                        // 避免"未连接服务器"吓退本可直接搜索的用户;已搜索过则统一显示"无结果"
                        text: Ed2kManager.searching ? qsTr("Searching...")
                              : root.hasSearched ? qsTr("No results. Try different keywords.")
                              : (root.effectiveSource === 1 && sourceFilter.currentIndex === 0)
                                ? qsTr("No server connected. Searches will use the Kad network.")
                              : (root.effectiveSource === 0 && !Ed2kManager.serverConnected)
                                ? qsTr("Not connected to any server")
                              : (root.effectiveSource === 1 && !Ed2kManager.kadRunning)
                                ? qsTr("Kad is not ready. Enable Kad in settings and restart the app.")
                                : qsTr("No results. Try different keywords.")
                    }
                    GButton {
                        Layout.alignment: Qt.AlignHCenter
                        visible: !Ed2kManager.serverConnected && !Ed2kManager.searching
                                 && root.effectiveSource === 0
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
