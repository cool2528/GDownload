import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

GCard {
    id: advancedCard
    required property int standardSpacing
    property string optionMode: "http"
    property alias userAgentField: userAgent
    property alias userAgentText: userAgent.text
    property alias authorizationField: authorization
    property alias authorizationText: authorization.text
    property alias referrerField: referrer
    property alias referrerText: referrer.text
    property alias cookieField: cookie
    property alias cookieText: cookie.text
    property alias headerArea: customRequestHeaderList
    property alias trackerUrlsField: trackerUrls
    property alias trackerUrlsText: trackerUrls.text
    property alias dhtSwitch: enableDhtSwitch
    property alias dhtEnabled: enableDhtSwitch.checked
    property alias peerLimitSpinner: peerLimit
    property alias peerLimitValue: peerLimit.value
    property alias encryptionField: encryptionSelector
    property alias encryptionIndex: encryptionSelector.currentIndex
    property alias seedRatioField: seedRatio
    property alias seedRatioText: seedRatio.text
    property alias seedTimeSpinner: seedTime
    property alias seedTimeValue: seedTime.value
    outlined: true
    padding: standardSpacing
    property alias view: viewLayout
    readonly property bool compactLayout: width < 520
    readonly property bool torrentMode: optionMode === "torrent"
    readonly property int torrentFieldHeight: GTheme.sizeLarge

    function collectRequestHeaders() {
        return customRequestHeaderList.getRequestHeaderList()
    }

    function collectTrackerUrls() {
        const trackers = []
        const seen = ({})
        const entries = trackerUrls.text.split(/[\r\n,]+/)
        for (let i = 0; i < entries.length; ++i) {
            const tracker = String(entries[i]).trim()
            if (tracker.length === 0 || seen[tracker])
                continue
            seen[tracker] = true
            trackers.push(tracker)
        }
        return trackers
    }

    function torrentValidationMessage() {
        const trackers = collectTrackerUrls()
        for (let i = 0; i < trackers.length; ++i) {
            if (!/^(https?|udp):\/\//i.test(trackers[i]))
                return qsTr("Tracker URLs must start with http://, https://, or udp://.")
        }
        if (!seedRatio.acceptableInput || seedRatio.text.trim().length === 0)
            return qsTr("Stop ratio must be a number between 0 and 100.")
        return ""
    }

    function collectTorrentOptions() {
        const options = ({})
        const trackers = collectTrackerUrls()
        if (trackers.length > 0)
            options["bt-tracker"] = trackers.join(",")

        options["enable-dht"] = enableDhtSwitch.checked ? "true" : "false"
        options["bt-max-peers"] = String(peerLimit.value)

        // aria2 separates encrypted-handshake requirements from payload encryption.
        // Prefer = opportunistic, Require = encrypted handshake, Force = encrypted payload.
        if (encryptionSelector.currentIndex === 2) {
            options["bt-require-crypto"] = "true"
            options["bt-force-encryption"] = "true"
        } else if (encryptionSelector.currentIndex === 1) {
            options["bt-require-crypto"] = "true"
            options["bt-force-encryption"] = "false"
        } else {
            options["bt-require-crypto"] = "false"
            options["bt-force-encryption"] = "false"
        }

        options["seed-ratio"] = String(Number(seedRatio.text))
        options["seed-time"] = String(seedTime.value)
        return options
    }

    ColumnLayout {
        id:viewLayout
        anchors.fill: parent
        anchors.margins: advancedCard.padding
        spacing: 0

        Text {
            text: advancedCard.torrentMode ? qsTr("Torrent Options") : qsTr("Advanced Options")
            font.pixelSize: 14
            font.weight: Font.Medium
            color: GTheme.textPrimary
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            visible: advancedCard.torrentMode
            text: qsTr("Tune peer discovery, encryption, seeding, and tracker behavior for this task.")
            font.pixelSize: GTheme.fontCaption
            color: GTheme.textSecondary
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            visible: !advancedCard.torrentMode
            columns: advancedCard.compactLayout ? 1 : 2
            columnSpacing: standardSpacing
            rowSpacing: 12

            Text {
                text: qsTr("User-Agent:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: advancedCard.compactLayout
                                  ? Qt.AlignVCenter | Qt.AlignLeft
                                  : Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: advancedCard.compactLayout ? -1 : 100
            }
            GTextField {
                id: userAgent
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                placeholderText: qsTr("User-Agent")
                Accessible.name: qsTr("User-Agent")
                Component.onCompleted: {
                    userAgent.text = SettingsManager.qUserAgent && SettingsManager.qUserAgent.length > 0
                                     ? SettingsManager.qUserAgent
                                     : SettingsManager.GetDefaultBrowserUserAgent()
                }
            }

            Text {
                text: qsTr("Authorization:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: advancedCard.compactLayout
                                  ? Qt.AlignVCenter | Qt.AlignLeft
                                  : Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: advancedCard.compactLayout ? -1 : 100
            }
            GTextField {
                id: authorization
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                placeholderText: qsTr("Authorization")
                Accessible.name: qsTr("Authorization header")
            }

            Text {
                text: qsTr("Referer:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: advancedCard.compactLayout
                                  ? Qt.AlignVCenter | Qt.AlignLeft
                                  : Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: advancedCard.compactLayout ? -1 : 100
            }
            GTextField {
                id: referrer
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                placeholderText: qsTr("Referer")
                Accessible.name: qsTr("Referrer URL")
            }

            Text {
                text: qsTr("Cookie:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: advancedCard.compactLayout
                                  ? Qt.AlignVCenter | Qt.AlignLeft
                                  : Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: advancedCard.compactLayout ? -1 : 100
            }
            GTextField {
                id: cookie
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: GTheme.sizeLarge
                placeholderText: qsTr("Cookie")
                Accessible.name: qsTr("Cookie header")
            }

            Text {
                text: qsTr("Custom Headers:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: advancedCard.compactLayout
                                  ? Qt.AlignTop | Qt.AlignLeft
                                  : Qt.AlignTop | Qt.AlignRight
                Layout.preferredWidth: advancedCard.compactLayout ? -1 : 100
            }

            TextArea {
                id: customRequestHeaderList
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: 80
                font.pixelSize: 12
                placeholderText: qsTr("Custom request headers (one per line: KEY:VALUE)")
                color: GTheme.textPrimary
                placeholderTextColor: GTheme.textPlaceholder
                selectByMouse: true
                wrapMode: TextArea.Wrap
                activeFocusOnTab: enabled && visible
                focusPolicy: Qt.StrongFocus
                Accessible.name: qsTr("Custom request headers")

                background: Rectangle {
                    color: GTheme.fillLighter
                    border.width: 1
                    border.color: customRequestHeaderList.activeFocus ? GTheme.primaryColor : GTheme.borderLight
                    radius: 6

                    Behavior on border.color {
                        ColorAnimation { duration: 150 }
                    }
                }

                function getRequestHeaderList() {
                    let headers = []
                    if (customRequestHeaderList.text.trim().length === 0) {
                        return headers
                    }
                    let lines = customRequestHeaderList.text.split('\n')
                    for (let line of lines) {
                        line = line.trim()
                        if (line.length === 0) continue
                        let colonIndex = line.indexOf(':')
                        if (colonIndex === -1) continue
                        let key = line.substring(0, colonIndex).trim()
                        let value = line.substring(colonIndex + 1).trim()
                        if (key.length === 0 || value.length === 0) continue
                        headers.push(key + ": " + value)
                    }
                    return headers
                }
            }
        }

        ColumnLayout {
            id: torrentLayout
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            visible: advancedCard.torrentMode
            spacing: GTheme.spaceMD

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: GTheme.spaceXS

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: qsTr("Tracker URLs")
                    font.pixelSize: GTheme.fontBody
                    color: GTheme.textSecondary
                }

                TextArea {
                    id: trackerUrls
                    objectName: "taskTorrentTrackerUrls"
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: 88
                    font.pixelSize: GTheme.fontBody
                    placeholderText: qsTr("Tracker URLs (one per line)")
                    color: GTheme.textPrimary
                    placeholderTextColor: GTheme.textPlaceholder
                    selectByMouse: true
                    wrapMode: TextArea.WrapAnywhere
                    activeFocusOnTab: enabled && visible
                    focusPolicy: Qt.StrongFocus
                    Accessible.name: qsTr("Tracker URLs")
                    Accessible.description: qsTr("Enter one HTTP, HTTPS, or UDP tracker URL per line")

                    background: Rectangle {
                        color: GTheme.surfaceBase
                        border.width: 1
                        border.color: trackerUrls.activeFocus ? GTheme.primaryColor : GTheme.borderBase
                        radius: GTheme.radiusBase

                        Behavior on border.color {
                            ColorAnimation { duration: GTheme.durationBase }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                columns: advancedCard.compactLayout ? 1 : 2
                columnSpacing: standardSpacing
                rowSpacing: GTheme.spaceMD

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Peer discovery")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }

                    GButtonSwitch {
                        id: enableDhtSwitch
                        objectName: "taskTorrentDhtSwitch"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: advancedCard.torrentFieldHeight
                        text: qsTr("Enable DHT peer discovery")
                        checked: SettingsManager.qEnableDht
                        Accessible.name: qsTr("Enable DHT peer discovery")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Peer limit")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }

                    GSpinBox {
                        id: peerLimit
                        objectName: "taskTorrentPeerLimit"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: advancedCard.torrentFieldHeight
                        from: 0
                        to: 999
                        value: SettingsManager.qBtMaxPeers
                        Accessible.name: qsTr("Maximum peers for this torrent")
                        Accessible.description: qsTr("Use zero for no peer limit")
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                columns: advancedCard.compactLayout ? 1 : 3
                columnSpacing: standardSpacing
                rowSpacing: GTheme.spaceMD

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Encryption")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }

                    GComBoBox {
                        id: encryptionSelector
                        objectName: "taskTorrentEncryption"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: advancedCard.torrentFieldHeight
                        model: [
                            qsTr("Prefer encrypted"),
                            qsTr("Require encrypted"),
                            qsTr("Force full encryption")
                        ]
                        currentIndex: SettingsManager.qBtRequireCrypto ? 1 : 0
                        Accessible.name: qsTr("BitTorrent encryption mode")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Stop ratio")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }

                    GTextField {
                        id: seedRatio
                        objectName: "taskTorrentSeedRatio"
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        Layout.preferredHeight: advancedCard.torrentFieldHeight
                        text: qsTr("1.0")
                        validator: DoubleValidator {
                            bottom: 0.0
                            top: 100.0
                            decimals: 2
                            notation: DoubleValidator.StandardNotation
                        }
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        status: acceptableInput ? "normal" : "danger"
                        Accessible.name: qsTr("Stop seeding ratio")
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: GTheme.spaceXS

                    Text {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        text: qsTr("Seed time")
                        font.pixelSize: GTheme.fontBody
                        color: GTheme.textSecondary
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: GTheme.spaceSM

                        GSpinBox {
                            id: seedTime
                            objectName: "taskTorrentSeedTime"
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredHeight: advancedCard.torrentFieldHeight
                            from: 0
                            to: 10080
                            value: 30
                            Accessible.name: qsTr("Torrent seed time in minutes")
                        }

                        Text {
                            text: qsTr("min")
                            font.pixelSize: GTheme.fontCaption
                            color: GTheme.textSecondary
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                text: qsTr("These values apply only to this torrent task and do not change Preferences.")
                font.pixelSize: GTheme.fontCaption
                color: GTheme.textSecondary
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }
    }
}
