import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import gdl.sdk

GCard {
    id: advancedCard
    required property int standardSpacing
    property alias userAgentField: userAgent
    property alias userAgentText: userAgent.text
    property alias authorizationField: authorization
    property alias authorizationText: authorization.text
    property alias referrerField: referrer
    property alias referrerText: referrer.text
    property alias cookieField: cookie
    property alias cookieText: cookie.text
    property alias headerArea: customRequestHeaderList
    outlined: true
    padding: standardSpacing
    property alias view: viewLayout

    function collectRequestHeaders() {
        return customRequestHeaderList.getRequestHeaderList()
    }

    ColumnLayout {
        id:viewLayout
        anchors.fill: parent
        spacing: 0

        Text {
            text: qsTr("Advanced Options")
            font.pixelSize: 14
            font.weight: Font.Medium
            color: GTheme.textPrimary
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: standardSpacing
            rowSpacing: 12

            Text {
                text: qsTr("User-Agent:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: 100
            }
            GTextField {
                id: userAgent
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                placeholderText: qsTr("User-Agent")
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
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: 100
            }
            GTextField {
                id: authorization
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                placeholderText: qsTr("Authorization")
            }

            Text {
                text: qsTr("Referer:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: 100
            }
            GTextField {
                id: referrer
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                placeholderText: qsTr("Referer")
            }

            Text {
                text: qsTr("Cookie:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                Layout.preferredWidth: 100
            }
            GTextField {
                id: cookie
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                placeholderText: qsTr("Cookie")
            }

            Text {
                text: qsTr("Custom Headers:")
                font.pixelSize: 13
                color: GTheme.textSecondary
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                Layout.preferredWidth: 100
            }

            TextArea {
                id: customRequestHeaderList
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                font.pixelSize: 12
                placeholderText: qsTr("Custom request headers (one per line: KEY:VALUE)")
                color: GTheme.textPrimary
                placeholderTextColor: GTheme.textPlaceholder
                selectByMouse: true
                wrapMode: TextArea.Wrap

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
    }
}
