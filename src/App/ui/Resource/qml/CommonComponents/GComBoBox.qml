import QtQuick
import QtQuick.Controls
import gdl.sdk

ComboBox {
    id: control
    property int maxPopHeight: 200
    delegate: ItemDelegate {
        width: control.width
        contentItem: Text {
            text: control.textRole
                ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole])
                : modelData
            color: GTheme.dark ? "#ffffff" : "#55575b"
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: control.highlightedIndex === index
        
        background: Rectangle {
            color: parent.highlighted ? (GTheme.dark ? "#404040" : "#f5f5ff") : "transparent"
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
                (control.pressed ? "#808080" : "#ffffff") :
                (control.pressed ? "#666666" : "#55575b")
            context.fill();
        }
    }

    contentItem: Text {
        leftPadding: 10
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: GTheme.dark ? "#ffffff" : "#55575b"
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 40
        color: GTheme.dark ? "#303030" : "#ffffff"
        border.color: control.pressed || control.hovered ? 
            (GTheme.dark ? "#5151f9" : "#5151f9") : 
            (GTheme.dark ? "#404040" : "#d9dbe3")
        border.width: control.visualFocus ? 2 : 1
        radius: 2
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
            color: GTheme.dark ? "#303030" : "#ffffff"
            border.color: GTheme.dark ? "#404040" : "#d9dbe3"
            radius: 2
        }
    }
}
