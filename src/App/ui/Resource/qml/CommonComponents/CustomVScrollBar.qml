import QtQuick
import QtQuick.Controls
import gdl.sdk

ScrollBar {
    id: scrollBar

    property real mouseAngleDelta: 0.0
    property real viewHeight: 1.0
    property real viewContentHeight: 1.0

    property real step: 120
    property bool thin: false
    property real fixWidth: 0

    property real minPosition: 0.0
    property real maxPosition: (viewContentHeight - viewHeight) / viewContentHeight

    property alias running: anim.running

    x: parent.width - width
    height: parent.availableHeight

    policy: ScrollBar.AsNeeded
    interactive: true
    wheelEnabled: false

    function startScroll() {
        if (!validDestPosition()) return
        if (anim.running) anim.stop()

        anim.destPosition = calcDestPosition()
        anim.from = scrollBar.position
        anim.to = anim.destPosition
        anim.start()
    }

    function stopScroll() {
        anim.stop()
    }

    function scrollTo(y: real) {
        if (anim.running) anim.complete()

        anim.destPosition = calcDestPositionWithY(y)
        anim.from = scrollBar.position
        anim.to = anim.destPosition
        anim.start()
    }

    function willScrollDistance() {
        if (!validDestPosition()) return 0.0
        let dest = calcDestPosition()
        if (dest === position) return 0.0
        return (dest - position) * viewContentHeight
    }

    function validDestPosition() {
        return !(anim.destPosition == scrollBar.maxPosition && anim.stepTo > 0 ||
                anim.destPosition == scrollBar.minPosition && anim.stepTo < 0)
    }

    function calcDestPosition() {
        return Math.max(0, Math.min(scrollBar.maxPosition, anim.destPosition + anim.stepTo))
    }

    function calcDestPositionWithY(y: real) {
        let pos = y / viewContentHeight
        return Math.max(0, Math.min(scrollBar.maxPosition, pos))
    }

    onPositionChanged: {
        if (anim.running) return
        anim.destPosition = scrollBar.position
    }

    contentItem: Rectangle {
        id: contentItem

        color: GTheme.dark ? GTheme.borderLight : GTheme.borderBase
        radius: 5

        implicitWidth: fixWidth > 0 ? fixWidth : 15
        implicitHeight: 65

        opacity: maxPosition <= 0 ? 0 :
                scrollBar.policy === ScrollBar.AlwaysOn ? 1 :
                scrollBar.policy === ScrollBar.AlwaysOff ? 0 :
                anim.running ? 1 :
                hoverHandler.hovered ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutExpo
            }
        }
    }

    NumberAnimation {
        id: anim

        property real delta: -scrollBar.mouseAngleDelta / 120
        property real stepTo: delta * scrollBar.step / (scrollBar.viewContentHeight - scrollBar.viewHeight)
        property real destPosition: 0.0

        target: scrollBar
        property: "position"
        easing.type: Easing.BezierSpline
        easing.bezierCurve: [ 0.25, 0.05, 0.05, 0.95, 1, 1 ]
        duration: 250
    }

    HoverHandler {
        id: hoverHandler

        onHoveredChanged: {
            if (!hovered) contentItem.implicitWidth = 5
        }

        onPointChanged: {
            if(fixWidth > 0) {
               contentItem.implicitWidth = fixWidth 
               return
            }
            if (scrollBar.thin) {
                contentItem.implicitWidth = Math.min(10, 5 + point.position.x / 4)
            } else {
                contentItem.implicitWidth = Math.min(15, 5 + point.position.x / 1.5)
            }
        }
    }
}
