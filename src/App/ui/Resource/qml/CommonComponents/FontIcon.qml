import QtQuick

// 优化的字体图标组件
Item {
    id: control
    property int iconSource
    property int iconSize: 16
    property alias color: icon.color
    property alias horizontalAlignment: icon.horizontalAlignment
    property alias verticalAlignment: icon.verticalAlignment

    // 设置默认尺寸
    implicitWidth: iconSize
    implicitHeight: iconSize
    Text {
        id: icon
        text: String.fromCharCode(iconSource)
        font.pixelSize: iconSize
        font.family: "Segoe Fluent Icons"
        anchors.centerIn: parent
    }

}
