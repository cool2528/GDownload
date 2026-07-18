import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

// Aurora update workflow. Manager calls/signals are unchanged; the dialog adds
// explicit Available, In progress, and Failed presentation states.
GDialogShell {
    id: updateDialog
    objectName: "updateDialog"

    readonly property real outerMargin: GTheme.spaceLG
    readonly property bool narrowLayout: width < 480
    readonly property string releaseNotesCopy: releaseNotes.length > 0
                                               ? releaseNotes
                                               : qsTr("No release notes were provided for this version.")

    width: Math.min(560, parent ? Math.max(0, parent.width - outerMargin * 2) : 560)
    height: Math.min(620, parent ? Math.max(0, parent.height - outerMargin * 2) : 620)

    title: qsTr("New Version Available")
    subtitle: versionNumber
    iconName: "cloud-download"
    iconBgColor: updateState === "failed" ? GTheme.bgDanger : GTheme.bgSuccess
    iconColor: updateState === "failed" ? GTheme.dangerColor : GTheme.successColor

    property string versionNumber: ""
    property string releaseNotes: ""
    property bool updating: false
    property string updateState: "available" // available | progress | failed
    property string statusMessage: qsTr("Review the release notes, then install when you are ready.")
    property string failureMessage: ""
    property Item widePrimaryAction: null
    property Item narrowPrimaryAction: null
    readonly property Item primaryActionButton: narrowLayout
                                                 ? narrowPrimaryAction : widePrimaryAction
    initialFocusItem: primaryActionButton

    readonly property string statusTitle: {
        if (updateState === "progress") {
            return qsTr("Update in progress")
        }
        if (updateState === "failed") {
            return qsTr("Update failed")
        }
        return qsTr("Ready to update")
    }
    readonly property string visibleStatusMessage: {
        if (updateState === "failed") {
            return failureMessage.length > 0
                   ? failureMessage
                   : qsTr("The update could not be completed. Check your connection and try again.")
        }
        if (updateState === "progress") {
            return statusMessage.length > 0 ? statusMessage : qsTr("Preparing the update package...")
        }
        return qsTr("Review the release notes, then install when you are ready.")
    }
    readonly property string statusIconName: updateState === "failed"
                                              ? "error-badge"
                                              : (updateState === "progress"
                                                 ? "cloud-download" : "completed")
    readonly property color statusColor: updateState === "failed"
                                          ? GTheme.dangerColor
                                          : (updateState === "progress"
                                             ? GTheme.primaryColor : GTheme.successColor)
    readonly property color statusBackground: updateState === "failed"
                                               ? GTheme.bgDanger
                                               : (updateState === "progress"
                                                  ? GTheme.bgInfo : GTheme.bgSuccess)
    readonly property color statusBorder: updateState === "failed"
                                           ? GTheme.borderDanger
                                           : (updateState === "progress"
                                              ? GTheme.borderInfo : GTheme.borderSuccess)

    function openDownloadPage() {
        Qt.openUrlExternally("https://github.com/cool2528/GDownload/releases")
    }

    function startUpdate() {
        failureMessage = ""
        if (UpdateManager.StartUpdate()) {
            updateStatusCard.forceActiveFocus()
            updating = true
            updateState = "progress"
            statusMessage = qsTr("Preparing the update package...")
            updateProgressBar.value = 0
        } else {
            updating = false
            updateState = "failed"
            failureMessage = qsTr("The update could not be started. Please try again.")
        }
    }

    function handleUpdateAvailable(info) {
        versionNumber = "v" + info.version
        releaseNotes = info.release_note || ""
        updating = false
        updateState = "available"
        failureMessage = ""
        statusMessage = qsTr("Review the release notes, then install when you are ready.")
        updateProgressBar.value = 0
        updateDialog.open()
    }

    function handleUpdateProgress(progress) {
        const percentage = Number(progress.percentage)
        if (isFinite(percentage)) {
            updateProgressBar.value = Math.max(0, Math.min(100, percentage))
        }

        const progressMessage = progress.message || qsTr("Working on the update...")
        statusMessage = progressMessage

        // 0 check, 1 download, 2 extract, 3 verify, 4 install, 5 complete, 6 failed.
        if (progress.stage === 5) {
            updating = false
            updateState = "available"
            updateDialog.close()
        } else if (progress.stage === 6) {
            updating = false
            updateState = "failed"
            failureMessage = progressMessage
            ToastManager.ShowError(progressMessage)
        } else {
            updateStatusCard.forceActiveFocus()
            updating = true
            updateState = "progress"
            if (progress.stage === 4) {
                ToastManager.ShowInfo(progressMessage)
            }
        }
    }

    function handleUpdateFinished(success) {
        updating = false
        if (!success) {
            updateState = "failed"
            if (failureMessage.length === 0) {
                failureMessage = qsTr("The update could not be completed. Please try again.")
            }
        }
        console.log("update stage", success)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: updateDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
        anchors.rightMargin: updateDialog.narrowLayout ? GTheme.spaceLG : GTheme.space2XL
        anchors.topMargin: GTheme.spaceLG
        anchors.bottomMargin: GTheme.spaceLG
        spacing: GTheme.spaceLG

        Rectangle {
            id: updateStatusCard
            objectName: "updateStatusCard"
            focus: true
            Layout.fillWidth: true
            implicitHeight: updateStatusRow.implicitHeight + GTheme.spaceLG * 2
            radius: GTheme.radiusLarge
            color: updateDialog.statusBackground
            border.width: 1
            border.color: updateDialog.statusBorder
            Accessible.role: Accessible.StatusBar
            Accessible.name: updateDialog.statusTitle
            Accessible.description: updateDialog.visibleStatusMessage

            RowLayout {
                id: updateStatusRow
                anchors.fill: parent
                anchors.margins: GTheme.spaceLG
                spacing: GTheme.spaceMD

                AuroraIcon {
                    name: updateDialog.statusIconName
                    iconSize: GTheme.fontTitle
                    color: updateDialog.statusColor
                    Layout.alignment: Qt.AlignTop
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceXS

                    Text {
                        text: updateDialog.statusTitle
                        font.pixelSize: GTheme.fontBody
                        font.weight: GTheme.weightDemiBold
                        color: updateDialog.statusColor
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        objectName: "updateStatusMessage"
                        text: updateDialog.visibleStatusMessage
                        font.pixelSize: GTheme.fontCaption
                        color: GTheme.textSecondary
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM

            AuroraIcon {
                name: "book"
                iconSize: GTheme.fontSubtitle
                color: GTheme.textSecondary
            }

            Text {
                text: qsTr("Release Notes")
                font.pixelSize: GTheme.fontBody
                font.weight: GTheme.weightMedium
                color: GTheme.textPrimary
                Layout.fillWidth: true
            }
        }

        ScrollText {
            id: releaseNotesText
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: updateDialog.releaseNotesCopy
            textFormat: TextEdit.MarkdownText
            promptPage: true
            Accessible.name: qsTr("Release notes")

            background: Rectangle {
                color: GTheme.fillLighter
                border.width: 1
                border.color: GTheme.borderBase
                radius: GTheme.radiusLarge
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceXS

            GButtonSwitch {
                id: githubAccelerateSwitch
                Layout.fillWidth: true
                Layout.preferredHeight: GTheme.sizeDefault
                text: qsTr("Enable GitHub Accelerated Download")
                checked: SettingsManager.qEnableGithubAccelerate
                Accessible.name: text
                onClicked: SettingsManager.SetEnableGithubAccelerate(checked)
            }

            Text {
                text: qsTr("Use a GitHub mirror (ghproxy) when fetching update packages.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: GTheme.spaceSM
            visible: updateDialog.updateState === "progress"

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: qsTr("Progress")
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightMedium
                    color: GTheme.textSecondary
                    Layout.fillWidth: true
                }

                Text {
                    objectName: "updateProgressValue"
                    text: Math.round(updateProgressBar.value) + "%"
                    font.pixelSize: GTheme.fontCaption
                    font.weight: GTheme.weightDemiBold
                    color: GTheme.primaryColor
                }
            }

            GProgressBar {
                id: updateProgressBar
                objectName: "updateProgressBar"
                Layout.fillWidth: true
                Layout.preferredHeight: 6
                from: 0
                to: 100
                value: 0
                Accessible.name: qsTr("Update progress")
                Accessible.description: Math.round(value) + "%"
            }
        }
    }

    footer: Component {
        Item {
            implicitHeight: updateDialog.narrowLayout
                            ? GTheme.sizeDefault * 2 + GTheme.spaceSM + GTheme.spaceLG * 2
                            : updateDialog.barHeight

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: GTheme.space2XL
                anchors.rightMargin: GTheme.space2XL
                spacing: GTheme.spaceSM
                visible: !updateDialog.narrowLayout

                GButton {
                    iconName: "globe"
                    text: qsTr("Download Page")
                    Accessible.name: text
                    onClicked: updateDialog.openDownloadPage()
                }

                Item {
                    Layout.fillWidth: true
                }

                GButton {
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 80
                    visible: !updateDialog.updating
                    Accessible.name: text
                    onClicked: updateDialog.close()
                }

                GButton {
                    id: wideUpdateButton
                    objectName: "updateNowButton"
                    buttonType: "primary"
                    text: updateDialog.updateState === "failed" ? qsTr("Try Again") : qsTr("Update Now")
                    Layout.preferredWidth: 104
                    visible: !updateDialog.updating
                    Accessible.name: text
                    onClicked: updateDialog.startUpdate()
                    Keys.onReturnPressed: event => {
                        updateDialog.startUpdate()
                        event.accepted = true
                    }
                    Keys.onEnterPressed: event => {
                        updateDialog.startUpdate()
                        event.accepted = true
                    }
                    Component.onCompleted: updateDialog.widePrimaryAction = wideUpdateButton
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: GTheme.spaceLG
                anchors.rightMargin: GTheme.spaceLG
                anchors.topMargin: GTheme.spaceLG
                anchors.bottomMargin: GTheme.spaceLG
                spacing: GTheme.spaceSM
                visible: updateDialog.narrowLayout

                GButton {
                    iconName: "globe"
                    text: qsTr("Open Download Page")
                    Layout.fillWidth: true
                    Accessible.name: text
                    onClicked: updateDialog.openDownloadPage()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: GTheme.spaceSM

                    GButton {
                        text: qsTr("Cancel")
                        Layout.fillWidth: true
                        visible: !updateDialog.updating
                        Accessible.name: text
                        onClicked: updateDialog.close()
                    }

                    GButton {
                        id: narrowUpdateButton
                        objectName: "updateNowButtonNarrow"
                        buttonType: "primary"
                        text: updateDialog.updateState === "failed" ? qsTr("Try Again") : qsTr("Update Now")
                        Layout.fillWidth: true
                        visible: !updateDialog.updating
                        Accessible.name: text
                        onClicked: updateDialog.startUpdate()
                        Keys.onReturnPressed: event => {
                            updateDialog.startUpdate()
                            event.accepted = true
                        }
                        Keys.onEnterPressed: event => {
                            updateDialog.startUpdate()
                            event.accepted = true
                        }
                        Component.onCompleted: updateDialog.narrowPrimaryAction = narrowUpdateButton
                    }
                }
            }
        }
    }

    Connections {
        target: UpdateManager

        function onUpdateAvailable(info) {
            updateDialog.handleUpdateAvailable(info)
        }

        function onUpdateProgress(progress) {
            updateDialog.handleUpdateProgress(progress)
        }

        function onUpdateFinished(success) {
            updateDialog.handleUpdateFinished(success)
        }
    }

}
