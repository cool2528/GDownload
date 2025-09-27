import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import gdl.sdk

ComboBox {
    id: control
    // 规格化：尺寸 large/default/small
    property string size: "default"
    readonly property int implicitH: (size === "large" ? 40 : (size === "small" ? 24 : 32))
    readonly property int radiusPx: 4
    property int maxPopHeight: 200
    delegate: ItemDelegate {
        width: control.width
        contentItem: Text {
            text: control.textRole
                ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole])
                : modelData
            color: (control.currentIndex === index)
                   ? GTheme.primaryColor
                   : (GTheme.dark ? GTheme.textPrimary : GTheme.textRegular)
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: control.highlightedIndex === index
        
        background: Rectangle {
            color: parent.highlighted
                   ? (GTheme.dark ? GTheme.fillLight : GTheme.primaryLight(9))
                   : (control.currentIndex === index ? (GTheme.dark ? GTheme.fillBase : GTheme.fillLight) : "transparent")
        }
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: control
            function onPressedChanged() { canvas.requestPaint(); }
        }

        Connections {
            target: GTheme
            function onDarkChanged() { canvas.requestPaint(); }
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = GTheme.dark ?
                (control.pressed ? GTheme.textSecondary : GTheme.textPrimary) :
                (control.pressed ? GTheme.textSecondary : GTheme.textRegular)
            context.fill();
        }
    }

    contentItem: Text {
        leftPadding: 10
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: control.enabled ? (GTheme.dark ? GTheme.textPrimary : GTheme.textRegular) : GTheme.textDisabled
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: control.implicitH
        color: control.enabled ? (GTheme.dark ? GTheme.fillBase : GTheme.bgWhite) : GTheme.fillLighter
        border.color: (control.pressed || control.hovered || control.visualFocus) ? GTheme.primaryColor : GTheme.borderBase
        border.width: control.visualFocus ? 2 : 1
        radius: control.radiusPx
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: maxPopHeight < contentItem.implicitHeight ? maxPopHeight : contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: GTheme.dark ? GTheme.fillBase : GTheme.bgWhite
            border.color: GTheme.borderBase
            radius: control.radiusPx
            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: Qt.rgba(0,0,0,0.12)
                shadowBlur: 8
                shadowVerticalOffset: 4
                // 与圆角匹配（MultiEffect 不直接识别 radius，借助源半透明背景过渡）
            }
        }
    }
}
