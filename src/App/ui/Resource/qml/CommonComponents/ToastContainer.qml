import QtQuick
import gdl.sdk

// Aurora toast host. A clipped viewport and a real Column own stack geometry,
// so long or numerous messages never rely on unconstrained y assignments.
Item {
    id: root

    property int spacing: GTheme.spaceSM
    property int edgeMargin: GTheme.spaceLG
    property int maxToastWidth: 420
    property var activeMessages: []
    property int nextMessageId: 1

    readonly property real stackContentHeight: toastStack.height
    readonly property real viewportHeight: toastViewport.height

    Component {
        id: messageComponent

        GMessage {
            showClose: true

            onMessageClosed: {
                const index = root.activeMessages.indexOf(this)
                if (index !== -1) {
                    root.activeMessages.splice(index, 1)
                    root.activeMessages = root.activeMessages.slice(0)
                }
            }
        }
    }

    Flickable {
        id: toastViewport
        objectName: "toastViewport"

        anchors.top: parent.top
        anchors.topMargin: root.edgeMargin
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.edgeMargin
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.max(0, Math.min(root.maxToastWidth, parent.width - root.edgeMargin * 2))
        contentWidth: width
        contentHeight: toastStack.height
        clip: true
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: toastStack
            objectName: "toastStack"

            width: toastViewport.width
            spacing: root.spacing
        }
    }

    function showToast(message, type, duration) {
        const messageId = nextMessageId++
        const item = messageComponent.createObject(toastStack, {
            messageId: messageId,
            placement: GMessage.Top
        })
        if (item === null) {
            console.error("ToastContainer: failed to create GMessage")
            return
        }

        activeMessages.push(item)
        activeMessages = activeMessages.slice(0)
        item.show(message, type + 1, duration)
    }

    Connections {
        target: ToastManager
        ignoreUnknownSignals: true
        function onMessageRequested(message, type, duration) {
            root.showToast(message, type, duration)
        }
    }
}
