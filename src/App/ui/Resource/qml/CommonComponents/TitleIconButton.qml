import QtQuick
import QtQuick.Controls
import gdl.sdk

Button {
    id: control
    // buttonType: min | max | restore | close
    property string buttonType: "min"
    // 自定义基础背景（暗色主题下可用 fillBase 微弱对比）
    property bool subtleBackgroundInDark: true

    implicitWidth: 40
    implicitHeight: 32

    background: Rectangle {
        color: control.hovered
               ? (control.buttonType === "close" ? GTheme.dangerLight(3) : GTheme.fillLight)
               : (GTheme.dark && control.subtleBackgroundInDark ? GTheme.fillBase : "transparent")
    }

    contentItem: Canvas {
        id: icon
        anchors.centerIn: parent
        width: 16
        height: 16
        contextType: "2d"
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const c = (GTheme.dark ? ElementPlusColors.bgWhite(false) : GTheme.textPrimary);
            ctx.strokeStyle = c;
            ctx.fillStyle = c;
            ctx.lineWidth = 2;
            const w = width, h = height;

            if (control.buttonType === "min") {
                ctx.beginPath();
                ctx.moveTo(2, h-4);
                ctx.lineTo(w-2, h-4);
                ctx.stroke();
            } else if (control.buttonType === "max") {
                ctx.strokeRect(2.5, 2.5, w-5, h-5);
            } else if (control.buttonType === "restore") {
                // 后面的方框
                ctx.strokeRect(4.5, 4.5, w-7, h-7);
                // 前面的方框稍微偏移
                ctx.strokeRect(2.5, 2.5, w-7, h-7);
            } else if (control.buttonType === "close") {
                ctx.beginPath();
                ctx.moveTo(3,3); ctx.lineTo(w-3,h-3);
                ctx.moveTo(w-3,3); ctx.lineTo(3,h-3);
                ctx.stroke();
            }
        }

        Connections {
            target: GTheme
            function onDarkChanged() { icon.requestPaint(); }
        }
    }
}

