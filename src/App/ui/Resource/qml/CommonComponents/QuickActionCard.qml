import QtQuick
import QtQuick.Layouts
import gdl.sdk

// Aurora 快捷操作卡：整卡为单一点击目标，支持键盘、焦点、按下和禁用状态。
GCard {
    id: root

    property string title: ""
    property string description: ""
    // iconSource 保留兼容；新调用应优先使用 Aurora 语义 iconName。
    property int iconSource: 0
    property string iconName: ""
    property string accent: "primary" // primary | success | warning | info
    signal clicked()

    activeFocusOnTab: enabled
    compact: false
    interactive: enabled
    outlined: true
    hoverEnabled: enabled
    shadow: false
    clip: false
    disabled: !enabled
    selected: enabled && (actionArea.containsMouse || activeFocus)
    variant: "elevated"
    padding: GTheme.spaceMD
    radius: GTheme.radiusLarge
    implicitWidth: 220
    implicitHeight: GTheme.sizeLarge * 3
    Layout.minimumHeight: implicitHeight
    Accessible.role: Accessible.Button
    Accessible.name: title
    Accessible.description: description

    background: Rectangle {
        radius: root.radius
        color: {
            if (!root.enabled)
                return GTheme.fillLighter
            return actionArea.containsMouse ? GTheme.fillLight : GTheme.surfaceElevated
        }
        border.width: root.selected ? 2 : 1
        border.color: root.selected ? root.accentColor : GTheme.borderLight

        Behavior on color {
            ColorAnimation { duration: GTheme.durationBase }
        }
        Behavior on border.color {
            ColorAnimation { duration: GTheme.durationBase }
        }
    }

    readonly property color accentColor: {
        switch (accent) {
        case "success": return GTheme.successColor
        case "warning": return GTheme.warningColor
        case "info": return GTheme.infoColor
        default: return GTheme.primaryColor
        }
    }

    readonly property color accentBubble: {
        switch (accent) {
        case "success": return GTheme.bgSuccess
        case "warning": return GTheme.bgWarning
        case "info": return GTheme.bgInfo
        default: return GTheme.primaryLight(9)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.padding
        spacing: GTheme.spaceXS

        Rectangle {
            Layout.preferredWidth: GTheme.sizeLarge
            Layout.preferredHeight: GTheme.sizeLarge
            Layout.minimumWidth: GTheme.sizeLarge
            Layout.minimumHeight: GTheme.sizeLarge
            radius: GTheme.radiusMedium
            color: root.enabled ? root.accentBubble : GTheme.fillLighter

            AuroraIcon {
                anchors.centerIn: parent
                visible: root.iconName.length > 0
                name: root.iconName.length > 0 ? root.iconName : "info"
                iconSize: GTheme.fontSubtitle
                color: root.enabled ? root.accentColor : GTheme.textDisabled
            }

            FontIcon {
                anchors.centerIn: parent
                visible: root.iconName.length === 0 && root.iconSource > 0
                iconSource: root.iconSource
                iconSize: GTheme.fontSubtitle
                color: root.enabled ? root.accentColor : GTheme.textDisabled
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root.title
            font.pixelSize: GTheme.fontBody
            font.weight: GTheme.weightDemiBold
            color: root.enabled ? GTheme.textPrimary : GTheme.textDisabled
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.fillHeight: true
            text: root.description
            font.pixelSize: GTheme.fontCaption
            color: root.enabled ? GTheme.textSecondary : GTheme.textDisabled
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.accentColor
        opacity: actionArea.pressed ? 0.08 : 0

        Behavior on opacity {
            NumberAnimation { duration: GTheme.durationFast }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: root.radius + 2
        color: "transparent"
        border.width: 2
        border.color: GTheme.focusRing
        visible: root.activeFocus
        z: 2
    }

    MouseArea {
        id: actionArea
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onPressed: root.forceActiveFocus()
        onClicked: root.clicked()
    }

    Keys.onReturnPressed: event => {
        root.clicked()
        event.accepted = true
    }
    Keys.onEnterPressed: event => {
        root.clicked()
        event.accepted = true
    }
    Keys.onSpacePressed: event => {
        root.clicked()
        event.accepted = true
    }
}
