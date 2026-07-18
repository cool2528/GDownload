import QtQuick
import gdl.sdk

Item {
    id: control

    property string variant: "color" // color | monochrome | inverse
    property int markSize: 24

    implicitWidth: markSize
    implicitHeight: markSize

    readonly property url source: {
        if (variant === "inverse")
            return "qrc:/images/logo/aurora-brand-inverse.svg"
        if (variant === "monochrome")
            return "qrc:/images/logo/aurora-brand-monochrome.svg"
        return GTheme.dark
                ? "qrc:/images/logo/aurora-brand-dark.svg"
                : "qrc:/images/logo/aurora-brand-light.svg"
    }

    Image {
        anchors.centerIn: parent
        width: control.markSize
        height: control.markSize
        source: control.source
        sourceSize: Qt.size(control.markSize, control.markSize)
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
    }
}
