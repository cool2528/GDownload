import QtQuick
import QtQuick.Effects
import gdl.sdk
import "../Utils/AuroraIcons.js" as AuroraIcons

Item {
    id: control

    required property string name
    property int iconSize: 20
    property color color: GTheme.textRegular

    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: sourceImage
        anchors.centerIn: parent
        width: control.iconSize
        height: control.iconSize
        source: AuroraIcons.source(control.name)
        sourceSize: Qt.size(control.iconSize, control.iconSize)
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }

    MultiEffect {
        anchors.fill: sourceImage
        source: sourceImage
        colorization: 1.0
        colorizationColor: control.color
    }
}
