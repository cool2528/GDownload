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
    // 内容为空时 viewContentHeight=0 会除零得 NaN，NaN<=0 为 false 导致滚动条误显且后续计算连锁异常
    property real maxPosition: viewContentHeight > 0
        ? Math.max(0, (viewContentHeight - viewHeight) / viewContentHeight) : 0

    property alias running: anim.running

    x: parent ? parent.width - width : 0
    height: parent ? parent.availableHeight : implicitHeight
    implicitWidth: fixWidth > 0 ? fixWidth : (thin ? 4 : 8)

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

    background: Rectangle {
        implicitWidth: scrollBar.implicitWidth
        radius: width / 2
        color: GTheme.fillBase
        opacity: scrollBar.maxPosition > 0 ? 0.55 : 0
    }

    contentItem: Rectangle {
        id: thumb

        color: scrollBar.activeFocus ? GTheme.focusRing : GTheme.textSecondary
        radius: width / 2

        implicitWidth: fixWidth > 0 ? fixWidth
                                    : (hoverHandler.hovered || scrollBar.pressed ? (thin ? 8 : 10)
                                                                                 : (thin ? 4 : 6))
        implicitHeight: 65

        opacity: maxPosition <= 0 ? 0 :
                scrollBar.policy === ScrollBar.AlwaysOn ? 1 :
                scrollBar.policy === ScrollBar.AlwaysOff ? 0 :
                anim.running || scrollBar.pressed || hoverHandler.hovered ? 0.95 : 0.72

        Behavior on opacity {
            NumberAnimation {
                duration: GTheme.durationFast
                easing.type: GTheme.easingStandard
            }
        }

        Behavior on implicitWidth {
            NumberAnimation {
                duration: GTheme.durationFast
                easing.type: GTheme.easingStandard
            }
        }
    }

    NumberAnimation {
        id: anim

        property real delta: -scrollBar.mouseAngleDelta / 120
        property real scrollRange: scrollBar.viewContentHeight - scrollBar.viewHeight
        property real stepTo: scrollRange > 0 ? delta * scrollBar.step / scrollRange : 0
        property real destPosition: 0.0

        target: scrollBar
        property: "position"
        easing.type: Easing.BezierSpline
        easing.bezierCurve: [ 0.25, 0.05, 0.05, 0.95, 1, 1 ]
        duration: 250
    }

    HoverHandler {
        id: hoverHandler

        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
    }
}
