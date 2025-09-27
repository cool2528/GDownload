import QtQuick
import gdl.sdk

Rectangle {
    id: divider
    width: parent ? parent.width : 100
    height: 1
    color: GTheme.dark ? GTheme.borderBase : GTheme.borderLight
    opacity: GTheme.dark ? 0.3 : 0.6
}

