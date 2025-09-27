import QtQuick
import QtQuick.Controls
import gdl.sdk
ScrollView {
    id: scrollView
    clip: true
    implicitWidth: 560
    implicitHeight: 300
    property var text
    property bool promptPage: false
    property alias textFormat: textEdit.textFormat
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: CustomVScrollBar {
        id: vScrollBar
        thin:true
        viewHeight: view.height
        viewContentHeight: view.contentHeight
    }
    background: Rectangle {
        color: GTheme.bgWhite
        radius: promptPage ? 6 : 0
    }
    contentItem: Flickable {
        id: view
        implicitWidth: scrollView.width
        boundsBehavior: Flickable.StopAtBounds
        contentHeight: textEdit.height
        MouseArea{
            anchors.fill: textEdit
            cursorShape: textEdit.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            acceptedButtons: Qt.NoButton
        }

        TextEdit {
            id: textEdit
            width: scrollView.width
            height: Math.max(scrollView.height, contentHeight + topPadding)
            wrapMode: TextEdit.Wrap
            readOnly: true
            text: scrollView.text
            textFormat: promptPage ? TextEdit.MarkdownText : TextEdit.PlainText

            selectionColor: GTheme.primaryLight(5)
            color: GTheme.textPrimary
            leftPadding: promptPage ? 15 : 7
            topPadding: promptPage ? 20 : 5
            rightPadding: promptPage ? 31 : 14
            bottomPadding: promptPage ? 15 : 0
            selectByMouse: true
            font.pixelSize: 15

            onLinkActivated: (link)=>{
                                 Qt.openUrlExternally(link)
                             }
        }
    }
}
