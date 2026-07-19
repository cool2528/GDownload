import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// Aurora installation flow for the browser extension. Browser brands remain
// image assets; all product/action symbols use the semantic Aurora icon set.
GCard {
    id: installCard
    objectName: "extensionInstallationCard"

    Layout.fillWidth: true
    implicitHeight: contentLayout.implicitHeight + padding * 2
    outlined: true
    hoverEnabled: false
    interactive: false
    variant: "elevated"
    padding: GTheme.spaceLG
    radius: GTheme.radiusLarge

    readonly property bool compactLayout: width < 520
    readonly property int browserColumns: width >= 620 ? 3 : 1
    readonly property int linkColumns: width >= 620 ? 2 : 1

    function openExternal(url, message) {
        Qt.openUrlExternally(url)
        ToastManager.ShowInfo(message, 2000)
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: installCard.padding
        spacing: GTheme.spaceXL

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceXS

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Installation Guide")
                font.pixelSize: GTheme.fontSubtitle
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("Download, install, and connect the extension in three short steps.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }

        // 配对状态横幅：Native host 收到扩展握手即视为连通（设计文档完成判据）
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: statusRow.implicitHeight + GTheme.spaceMD * 2
            radius: GTheme.radiusMedium
            color: GTheme.fillLighter
            border.width: 1
            border.color: InstallGuideManager.extensionPaired ? GTheme.successColor : GTheme.borderLight

            RowLayout {
                id: statusRow
                anchors.fill: parent
                anchors.leftMargin: GTheme.spaceMD
                anchors.rightMargin: GTheme.spaceMD
                anchors.verticalCenter: parent.verticalCenter
                spacing: GTheme.spaceSM

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: InstallGuideManager.extensionPaired ? GTheme.successColor : GTheme.textPlaceholder
                }
                Text {
                    Layout.fillWidth: true
                    text: InstallGuideManager.extensionPaired
                          ? qsTr("Extension connected. Setup complete.")
                          : qsTr("Not connected yet. Finish the steps below, then reopen your browser.")
                    color: InstallGuideManager.extensionPaired ? GTheme.successColor : GTheme.textSecondary
                    font.pixelSize: GTheme.fontBody
                    wrapMode: Text.WordWrap
                }
            }

            // 定期刷新配对状态，握手到达后自动转为已连通
            Timer {
                interval: 3000
                running: true
                repeat: true
                onTriggered: InstallGuideManager.refresh()
            }
        }

        StepSection {
            stepNumber: 1
            stepTitle: qsTr("Download Extension")
            stepDescription: qsTr("Choose the browser package you want to install.")

            stepContent: ColumnLayout {
                spacing: GTheme.spaceMD

                GridLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    columns: installCard.browserColumns
                    columnSpacing: GTheme.spaceMD
                    rowSpacing: GTheme.spaceMD

                    BrowserButton {
                        objectName: "extensionChromeButton"
                        browserName: qsTr("Chrome")
                        iconSource: "qrc:/images/browser-extension/chrome.svg"
                        onClicked: installCard.openExternal(
                                       "https://github.com/cool2528/gd-browser-extension/releases",
                                       qsTr("Opening the Chrome download page..."))
                    }

                    BrowserButton {
                        objectName: "extensionFirefoxButton"
                        browserName: qsTr("Firefox")
                        iconSource: "qrc:/images/browser-extension/firefox.svg"
                        onClicked: installCard.openExternal(
                                       "https://github.com/cool2528/gd-browser-extension/releases",
                                       qsTr("Opening the Firefox download page..."))
                    }

                    BrowserButton {
                        objectName: "extensionEdgeButton"
                        browserName: qsTr("Edge")
                        iconSource: "qrc:/images/browser-extension/edge.svg"
                        onClicked: installCard.openExternal(
                                       "https://github.com/cool2528/gd-browser-extension/releases",
                                       qsTr("Opening the Edge download page..."))
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    columns: installCard.linkColumns
                    columnSpacing: GTheme.spaceMD
                    rowSpacing: GTheme.spaceSM

                    ResourceLink {
                        objectName: "extensionRepositoryLink"
                        iconName: "repository"
                        title: qsTr("GitHub Repository")
                        description: qsTr("Releases and source code")
                        onClicked: installCard.openExternal(
                                       "https://github.com/cool2528/gd-browser-extension",
                                       qsTr("Opening GitHub..."))
                    }

                    ResourceLink {
                        objectName: "extensionWebsiteLink"
                        iconName: "globe"
                        title: qsTr("Official Website")
                        description: qsTr("Download from gdownload.uk")
                        onClicked: installCard.openExternal(
                                       "https://gdownload.uk/",
                                       qsTr("Opening the official website..."))
                    }
                }
            }
        }

        StepSection {
            stepNumber: 2
            stepTitle: qsTr("Install Extension")
            stepDescription: qsTr("Load the downloaded package in your browser.")

            stepContent: ColumnLayout {
                spacing: GTheme.spaceMD

                InstallInstructionItem {
                    browserName: qsTr("Chrome and Edge")
                    instructions: [
                        qsTr("Open chrome://extensions/ or edge://extensions/."),
                        qsTr("Enable Developer mode."),
                        qsTr("Select Load unpacked."),
                        qsTr("Choose the dist folder from the extracted release.")
                    ]
                }

                InstallInstructionItem {
                    browserName: qsTr("Firefox")
                    instructions: [
                        qsTr("Open about:debugging#/runtime/this-firefox."),
                        qsTr("Select Load Temporary Add-on."),
                        qsTr("Choose manifest.json from the dist folder."),
                        qsTr("Keep the debugging page available while testing the temporary add-on.")
                    ]
                }

                AlertTip {
                    Layout.fillWidth: true
                    severity: "info"
                    iconName: "info"
                    title: qsTr("Web-store installation is planned")
                    description: qsTr("For now, install the release package manually using the steps above.")
                }
            }
        }

        StepSection {
            stepNumber: 3
            stepTitle: qsTr("Configure Connection")
            stepDescription: qsTr("Pair the extension with the local GDownload service.")

            stepContent: ColumnLayout {
                spacing: GTheme.spaceSM

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("The extension sends captured links to GDownload through the local aria2c JSON-RPC endpoint.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textRegular
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }

                CheckRow { text: qsTr("Default connection values are already available in GDownload.") }
                CheckRow { text: qsTr("The configuration helper below provides the exact endpoint and secret.") }

                GButton {
                    objectName: "extensionViewConfigurationButton"
                    Layout.fillWidth: installCard.compactLayout
                    Layout.maximumWidth: installCard.compactLayout ? 100000 : 220
                    Layout.topMargin: GTheme.spaceSM
                    text: qsTr("View Configuration")
                    iconName: "settings"
                    activeFocusOnTab: true
                    Accessible.name: text
                    onClicked: ToastManager.ShowInfo(qsTr("See Configuration Helper below."), 2000)
                }
            }
        }
    }

    component StepSection: ColumnLayout {
        property int stepNumber: 1
        property string stepTitle: ""
        property string stepDescription: ""
        property alias stepContent: contentContainer.data

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: GTheme.spaceMD

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: GTheme.sizeDefault
                Layout.preferredHeight: GTheme.sizeDefault
                Layout.minimumWidth: GTheme.sizeDefault
                Layout.minimumHeight: GTheme.sizeDefault
                Layout.alignment: Qt.AlignTop
                radius: GTheme.radiusCircle
                color: GTheme.primaryColor

                Text {
                    anchors.centerIn: parent
                    text: stepNumber
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textInverse
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: stepTitle
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: stepDescription
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }

        Item {
            id: contentContainer
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.leftMargin: installCard.compactLayout ? 0 : GTheme.sizeDefault + GTheme.spaceMD
            implicitHeight: childrenRect.height

            onChildrenChanged: bindChildWidths()
            Component.onCompleted: bindChildWidths()

            function bindChildWidths() {
                for (let index = 0; index < children.length; ++index) {
                    children[index].width = Qt.binding(function() {
                        return contentContainer.width
                    })
                }
            }
        }

        Divider {
            Layout.fillWidth: true
            Layout.topMargin: GTheme.spaceSM
            color: GTheme.borderLighter
        }
    }

    component BrowserButton: Rectangle {
        property string browserName: ""
        property url iconSource: ""
        signal clicked()

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: GTheme.sizeLarge * 2 + GTheme.spaceSM
        radius: GTheme.radiusMedium
        color: hoverHandler.hovered ? GTheme.fillLight : GTheme.surfaceBase
        border.width: 1
        border.color: hoverHandler.hovered ? GTheme.primaryColor : GTheme.borderLight
        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: qsTr("Download for %1").arg(browserName)

        HoverHandler {
            id: hoverHandler
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: GTheme.spaceXS

            Image {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.alignment: Qt.AlignHCenter
                source: iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: browserName
                font.pixelSize: GTheme.fontCaption
                font.weight: GTheme.weightDemiBold
                color: GTheme.textPrimary
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Download")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.primaryColor
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: parent.forceActiveFocus()
            onClicked: parent.clicked()
        }

        Keys.onReturnPressed: event => {
            clicked()
            event.accepted = true
        }
        Keys.onSpacePressed: event => {
            clicked()
            event.accepted = true
        }
    }

    component ResourceLink: Rectangle {
        property string iconName: "link"
        property string title: ""
        property string description: ""
        signal clicked()

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: Math.max(GTheme.sizeLarge + GTheme.spaceMD * 2,
                                         linkLayout.implicitHeight + GTheme.spaceMD * 2)
        radius: GTheme.radiusMedium
        color: linkHover.hovered ? GTheme.fillLight : GTheme.fillLighter
        border.width: 1
        border.color: GTheme.borderLighter
        activeFocusOnTab: true
        Accessible.role: Accessible.Link
        Accessible.name: title
        Accessible.description: description

        RowLayout {
            id: linkLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            AuroraIcon {
                Layout.alignment: Qt.AlignTop
                name: iconName
                iconSize: GTheme.fontSubtitle
                color: GTheme.primaryColor
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: title
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: description
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }

            AuroraIcon {
                name: "chevron-right"
                iconSize: GTheme.fontBody
                color: GTheme.textSecondary
            }
        }

        HoverHandler {
            id: linkHover
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: parent.forceActiveFocus()
            onClicked: parent.clicked()
        }
        Keys.onReturnPressed: event => {
            clicked()
            event.accepted = true
        }
        Keys.onSpacePressed: event => {
            clicked()
            event.accepted = true
        }
    }

    component InstallInstructionItem: Rectangle {
        property string browserName: ""
        property var instructions: []

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: instructionLayout.implicitHeight + GTheme.spaceMD * 2
        radius: GTheme.radiusMedium
        color: GTheme.fillLighter
        border.width: 1
        border.color: GTheme.borderLighter

        ColumnLayout {
            id: instructionLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM

                AuroraIcon {
                    name: "globe"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: browserName
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }

            Repeater {
                model: instructions

                RowLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceSM

                    Rectangle {
                        Layout.preferredWidth: GTheme.spaceXS
                        Layout.preferredHeight: GTheme.spaceXS
                        Layout.alignment: Qt.AlignTop
                        Layout.topMargin: GTheme.spaceSM
                        radius: GTheme.radiusCircle
                        color: GTheme.primaryColor
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: modelData
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textRegular
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }
            }
        }
    }

    component CheckRow: RowLayout {
        property alias text: checkText.text

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: GTheme.spaceSM

        AuroraIcon {
            Layout.alignment: Qt.AlignTop
            name: "completed"
            iconSize: GTheme.fontBody
            color: GTheme.successColor
        }

        Text {
            id: checkText
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            font.pixelSize: GTheme.fontCaption
            color: GTheme.textRegular
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
    }
}
