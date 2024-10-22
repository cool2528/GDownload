import QtQuick
import QtQuick.Controls
Item{
    id:navigator

    SplitView{
        id:navigatorSplitView
        anchors.fill: parent
        Rectangle{
            id:systemNavigator
            color: "#484848"
            implicitWidth: 74
            SplitView.minimumWidth: 74
        }
        Rectangle{
            id:downloadNavigator
            color: "#f2f3f6"
            implicitWidth: 200
            SplitView.minimumWidth: 200
        }
    }

}
