import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../CommonComponents"
import gdl.sdk

// Aurora FAQ and support surface. Questions keep their full answer text and
// support actions collapse to one column on narrow settings pages.
GCard {
    id: faqCard
    objectName: "extensionFaqCard"

    Layout.fillWidth: true
    implicitHeight: contentLayout.implicitHeight + padding * 2
    outlined: true
    hoverEnabled: false
    interactive: false
    variant: "elevated"
    padding: GTheme.spaceLG
    radius: GTheme.radiusLarge

    readonly property bool compactLayout: width < 520
    readonly property int supportColumns: width >= 700 ? 3 : (width >= 480 ? 2 : 1)

    function openExternal(url, message) {
        Qt.openUrlExternally(url)
        ToastManager.ShowInfo(message, 2000)
    }

    ColumnLayout {
        id: contentLayout
        anchors.fill: parent
        anchors.margins: faqCard.padding
        spacing: GTheme.spaceXL

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.minimumWidth: GTheme.sizeLarge
                Layout.minimumHeight: GTheme.sizeLarge
                Layout.alignment: Qt.AlignTop
                radius: GTheme.radiusMedium
                color: GTheme.bgInfo

                AuroraIcon {
                    anchors.centerIn: parent
                    name: "help"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.infoColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Frequently Asked Questions")
                    font.pixelSize: GTheme.fontSubtitle
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Connection, privacy, compatibility, and capture behavior.")
                    font.pixelSize: GTheme.fontCaption
                    color: GTheme.textSecondary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceSM

            FAQItem {
                objectName: "extensionFaqWhy"
                question: qsTr("Why do I need the browser extension?")
                answer: qsTr("The extension captures download links directly from web pages and sends them to GDownload with one action, so you do not need to copy each URL manually.")
            }

            FAQItem {
                objectName: "extensionFaqPrivacy"
                question: qsTr("Is my data safe?")
                answer: qsTr("Yes. The extension connects locally to aria2c through WebSocket. The connection values remain on your computer and are not sent to an external service.")
            }

            FAQItem {
                objectName: "extensionFaqConnection"
                question: qsTr("Connection failed. What should I do?")
                answer: qsTr("Keep GDownload running. Confirm that aria2c is enabled, verify that the WebSocket URL and RPC Secret match the configuration helper, and then restart GDownload and the browser before testing again.")
            }

            FAQItem {
                objectName: "extensionFaqBrowsers"
                question: qsTr("Which browsers are supported?")
                answer: qsTr("The extension supports Chrome 110+, Firefox 115+, and Edge 110+ through the standard Web Extensions API and Manifest V3 where available.")
            }

            FAQItem {
                objectName: "extensionFaqFiltering"
                question: qsTr("Can I customize which links are captured?")
                answer: qsTr("Yes. Configure minimum file size, allowed file types, URL blacklist patterns, and a domain whitelist in the extension options.")
            }

            FAQItem {
                objectName: "extensionFaqProtectedSites"
                question: qsTr("Does it work with password-protected sites?")
                answer: qsTr("The extension can optionally send cookies and authorization headers. These options are disabled by default; enable them only for trusted sites that require an authenticated request.")
            }
        }

        Divider {
            Layout.fillWidth: true
            color: GTheme.borderLight
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: GTheme.spaceMD

            RowLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM

                AuroraIcon {
                    name: "book"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Need more help?")
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                columns: faqCard.supportColumns
                columnSpacing: GTheme.spaceMD
                rowSpacing: GTheme.spaceMD

                HelpLinkItem {
                    objectName: "extensionIssuesButton"
                    iconName: "repository"
                    label: qsTr("GitHub Issues")
                    description: qsTr("Report a bug")
                    onClicked: faqCard.openExternal(
                                   "https://github.com/cool2528/gd-browser-extension/issues",
                                   qsTr("Opening GitHub Issues..."))
                }

                HelpLinkItem {
                    objectName: "extensionDocsButton"
                    iconName: "book"
                    label: qsTr("Documentation")
                    description: qsTr("Read the user guide")
                    onClicked: faqCard.openExternal(
                                   "https://github.com/cool2528/gd-browser-extension/blob/main/README.md",
                                   qsTr("Opening documentation..."))
                }

                HelpLinkItem {
                    objectName: "extensionWebsiteButton"
                    iconName: "globe"
                    label: qsTr("Official Website")
                    description: qsTr("Visit gdownload.uk")
                    onClicked: faqCard.openExternal(
                                   "https://gdownload.uk/",
                                   qsTr("Opening the website..."))
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredHeight: communityLayout.implicitHeight + GTheme.spaceLG * 2
            radius: GTheme.radiusLarge
            color: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)
            border.width: 1
            border.color: GTheme.dark ? GTheme.borderBase : GTheme.primaryLight(7)

            GridLayout {
                id: communityLayout
                anchors.fill: parent
                anchors.margins: GTheme.spaceLG
                columns: faqCard.compactLayout ? 1 : 2
                columnSpacing: GTheme.spaceLG
                rowSpacing: GTheme.spaceSM

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: GTheme.spaceSM

                        AuroraIcon {
                            name: "people"
                            iconSize: GTheme.fontSubtitle
                            color: GTheme.primaryColor
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            text: qsTr("Join the GDownload community")
                            font.pixelSize: GTheme.fontBody
                            font.weight: GTheme.weightDemiBold
                            color: GTheme.textPrimary
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Share feedback, compare capture rules, and follow new extension releases on GitHub.")
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textRegular
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                    }
                }

                GButton {
                    objectName: "extensionStarButton"
                    Layout.fillWidth: faqCard.compactLayout
                    Layout.maximumWidth: faqCard.compactLayout ? 100000 : 180
                    Layout.alignment: faqCard.compactLayout ? Qt.AlignLeft : Qt.AlignVCenter | Qt.AlignRight
                    text: qsTr("Star on GitHub")
                    iconName: "repository"
                    type: 1
                    activeFocusOnTab: true
                    Accessible.name: text
                    onClicked: {
                        Qt.openUrlExternally("https://github.com/cool2528/GDownload")
                        ToastManager.ShowSuccess(qsTr("Thank you for supporting GDownload."), 2000)
                    }
                }
            }
        }
    }

    component FAQItem: Rectangle {
        id: faqItem

        property string question: ""
        property string answer: ""
        property bool expanded: false

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: questionRow.implicitHeight + GTheme.spaceMD * 2 +
                                (faqItem.expanded
                                 ? GTheme.spaceSM + answerText.implicitHeight
                                 : 0)
        radius: GTheme.radiusMedium
        color: hoverHandler.hovered ? GTheme.fillLight : GTheme.fillLighter
        border.width: activeFocus ? 2 : 1
        border.color: activeFocus ? GTheme.focusRing : GTheme.borderLighter
        activeFocusOnTab: true
        Accessible.role: Accessible.Button
        Accessible.name: faqItem.question
        Accessible.description: faqItem.expanded ? faqItem.answer : qsTr("Collapsed")

        Behavior on color {
            ColorAnimation { duration: GTheme.durationFast }
        }

        ColumnLayout {
            id: faqLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            RowLayout {
                id: questionRow
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceSM

                AuroraIcon {
                    Layout.alignment: Qt.AlignTop
                    name: faqItem.expanded ? "chevron-down" : "chevron-right"
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: faqItem.question
                    font.pixelSize: GTheme.fontBody
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.textPrimary
                    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                }
            }

            Text {
                id: answerText
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.leftMargin: GTheme.fontSubtitle + GTheme.spaceSM
                visible: faqItem.expanded
                text: faqItem.answer
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textRegular
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                lineHeight: 1.35
            }
        }

        HoverHandler {
            id: hoverHandler
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onPressed: faqItem.forceActiveFocus()
            onClicked: faqItem.expanded = !faqItem.expanded
        }
        Keys.onReturnPressed: event => {
            faqItem.expanded = !faqItem.expanded
            event.accepted = true
        }
        Keys.onSpacePressed: event => {
            faqItem.expanded = !faqItem.expanded
            event.accepted = true
        }
    }

    component HelpLinkItem: Rectangle {
        property string iconName: "help"
        property string label: ""
        property string description: ""
        signal clicked()

        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredHeight: Math.max(GTheme.sizeLarge + GTheme.sizeDefault,
                                         helpLayout.implicitHeight + GTheme.spaceMD * 2)
        radius: GTheme.radiusMedium
        color: helpHover.hovered ? GTheme.fillLight : GTheme.surfaceBase
        border.width: activeFocus ? 2 : 1
        border.color: activeFocus ? GTheme.focusRing : GTheme.borderLight
        activeFocusOnTab: true
        Accessible.role: Accessible.Link
        Accessible.name: label
        Accessible.description: description

        RowLayout {
            id: helpLayout
            anchors.fill: parent
            anchors.margins: GTheme.spaceMD
            spacing: GTheme.spaceSM

            Rectangle {
                Layout.preferredWidth: GTheme.sizeLarge
                Layout.preferredHeight: GTheme.sizeLarge
                Layout.minimumWidth: GTheme.sizeLarge
                Layout.minimumHeight: GTheme.sizeLarge
                radius: GTheme.radiusMedium
                color: GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9)

                AuroraIcon {
                    anchors.centerIn: parent
                    name: iconName
                    iconSize: GTheme.fontSubtitle
                    color: GTheme.primaryColor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: label
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
        }

        HoverHandler {
            id: helpHover
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
}
