import QtQuick
import QtQuick.Effects
import QtMultimedia
import Omareel

Item {
    id: root
    required property var settings
    required property real zoomScale
    required property int outputWidth
    required property int outputHeight
    required property int sourceWidth
    required property int sourceHeight
    property bool softwareRendering: false
    property bool nativePreview: false
    // 0 shows the bubble, 1 the full-frame camera; values between morph one into the other.
    property real fullProgress: 0
    // How visible the bubble is while the camera is not full frame.
    property real bubbleOpacity: 1
    property real ref: outputHeight / 1080
    readonly property string fullMode: settings.fullFrame === "square" ? "square" : "fill"
    readonly property real sourceBorderWidth: settings.border.enabled ? settings.border.width * ref : 0
    readonly property real borderWidth: sourceBorderWidth * (1 - fullProgress)
    readonly property bool squareCrop: settings.shape !== "rectangle"
    readonly property bool quarterTurn: settings.rotation === 90 || settings.rotation === 270
    readonly property real sourceAspect: sourceWidth / Math.max(1, sourceHeight)
    readonly property real aspect: squareCrop ? 1 : (quarterTurn ? 1 / sourceAspect : sourceAspect)
    readonly property real zoomProgress: Math.max(0, Math.min(1, (zoomScale - 1.05) / 0.1))
    readonly property real zoomMultiplier: 1 - (1 - settings.scaleDuringZoom) * zoomProgress

    readonly property real bubbleHeight: outputHeight * settings.size * zoomMultiplier
    readonly property real bubbleWidth: bubbleHeight * aspect
    // The gap to the corner is a share of the shorter side, so it is the same at the side and the
    // bottom, and it ends at the outside of the border.
    readonly property real marginX: Math.min(outputWidth, outputHeight) * settings.offset.x + sourceBorderWidth
    readonly property real marginY: Math.min(outputWidth, outputHeight) * settings.offset.y + sourceBorderWidth
    readonly property real bubbleX: settings.position.indexOf("left") >= 0
        ? marginX : outputWidth - marginX - bubbleWidth
    readonly property real bubbleY: settings.position.indexOf("top") >= 0
        ? marginY : outputHeight - marginY - bubbleHeight
    readonly property real bubbleRadius: settings.shape === "round" ? Math.min(bubbleWidth, bubbleHeight) / 2
        : settings.radius * ref
    // Fill covers the whole canvas; square keeps the largest centred square over a blurred copy.
    readonly property real fullSide: Math.min(outputWidth, outputHeight)
    readonly property real fullWidth: fullMode === "square" ? fullSide : outputWidth
    readonly property real fullHeight: fullMode === "square" ? fullSide : outputHeight
    // The corner radius shrinks while the camera grows, so a round bubble passes through rounded
    // rectangles instead of stretching into a stadium.
    readonly property real cornerRadius: bubbleRadius * (1 - fullProgress)
    readonly property real outerRadius: cornerRadius > 0 ? cornerRadius + borderWidth : 0
    readonly property real shadowAngle: settings.shadow.angle * Math.PI / 180
    readonly property real shadowX: Math.cos(shadowAngle) * settings.shadow.distance * ref
    readonly property real shadowY: Math.sin(shadowAngle) * settings.shadow.distance * ref

    x: bubbleX + ((outputWidth - fullWidth) / 2 - bubbleX) * fullProgress
    y: bubbleY + ((outputHeight - fullHeight) / 2 - bubbleY) * fullProgress
    width: bubbleWidth + (fullWidth - bubbleWidth) * fullProgress
    height: bubbleHeight + (fullHeight - bubbleHeight) * fullProgress
    // No layout shows both the full-frame camera and the bubble, so their sum stays at 1 while one
    // turns into the other, and falls to 0 only where the camera leaves the frame.
    readonly property real presence: Math.min(1, fullProgress + bubbleOpacity)
    opacity: presence
    scale: .85 + .15 * presence

    // The part of the camera frame shown in an item of this size: centred, filling it.
    function coverCrop(itemWidth, itemHeight) {
        if (sourceWidth <= 0 || sourceHeight <= 0 || itemWidth <= 0 || itemHeight <= 0)
            return Qt.rect(0, 0, 1, 1)
        const itemAspect = itemWidth / itemHeight
        if (sourceAspect > itemAspect) {
            const visible = itemAspect / sourceAspect
            return Qt.rect((1 - visible) / 2, 0, visible, 1)
        }
        const visible = sourceAspect / itemAspect
        return Qt.rect(0, (1 - visible) / 2, 1, visible)
    }

    BlurredBackdrop {
        // Spans the canvas, not the camera, so it is placed back at the canvas origin.
        x: -root.x
        y: -root.y
        width: root.outputWidth
        height: root.outputHeight
        z: -1
        visible: root.fullMode === "square" && root.fullProgress > 0
        // Fades faster than the camera shrinks, so a morph does not wash the screen out.
        opacity: root.fullProgress * root.fullProgress
        source: cameraContent
        sourceAspect: root.width / Math.max(1, root.height)
        softwareRendering: root.softwareRendering
    }
    Rectangle {
        id: shadowShape
        x: -root.borderWidth
        y: -root.borderWidth
        width: parent.width + 2 * root.borderWidth
        height: parent.height + 2 * root.borderWidth
        radius: root.outerRadius
        color: Qt.rgba(0, 0, 0, 1)
        layer.enabled: true
        antialiasing: true
    }
    MultiEffect {
        anchors.fill: shadowShape
        source: shadowShape
        visible: root.settings.shadow.enabled && !root.softwareRendering
        shadowEnabled: true
        shadowColor: Qt.rgba(0, 0, 0, 1)
        shadowOpacity: root.settings.shadow.intensity * (1 - root.fullProgress)
        shadowBlur: Math.min(1, root.settings.shadow.blur / 64)
        shadowHorizontalOffset: root.shadowX
        shadowVerticalOffset: root.shadowY
        autoPaddingEnabled: true
    }
    Rectangle {
        x: -root.borderWidth + root.shadowX
        y: -root.borderWidth + root.shadowY
        width: parent.width + 2 * root.borderWidth
        height: parent.height + 2 * root.borderWidth
        radius: shadowShape.radius
        color: "black"
        opacity: root.softwareRendering && root.settings.shadow.enabled
            ? root.settings.shadow.intensity * .45 * (1 - root.fullProgress) : 0
    }
    Item {
        z: 2
        id: cameraClip
        anchors.fill: parent
        clip: true
        layer.enabled: !root.softwareRendering && root.outerRadius > 0
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: cameraMask
        }
        Rectangle {
            id: cameraMask
            anchors.fill: parent
            radius: shadowShape.radius
            color: "white"
            visible: false
            layer.enabled: true
        }
        // Untransformed holder of the turned camera, also the source of the blurred backdrop.
        Item {
            id: cameraContent
            anchors.fill: parent
            Item {
                id: cameraRotated
                objectName: "cameraRotated"
                anchors.centerIn: parent
                width: root.quarterTurn ? parent.height : parent.width
                height: root.quarterTurn ? parent.width : parent.height
                rotation: root.settings.rotation
                transform: Scale {
                    objectName: "cameraFlipTransform"
                    origin.x: cameraRotated.width / 2
                    origin.y: cameraRotated.height / 2
                    xScale: root.settings.flipHorizontal ? -1 : 1
                }
                VideoOutput {
                    id: nativeCameraVideo
                    visible: root.nativePreview
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectCrop
                }
                FrameSource {
                    id: cameraVideo
                    objectName: "cameraFrameSource"
                    visible: !root.nativePreview
                    anchors.fill: parent
                    cropRect: root.coverCrop(width, height)
                    cornerRadiusRatio: !root.softwareRendering ? 0
                        : root.cornerRadius / Math.max(1, Math.min(width, height))
                }
            }
        }
    }
    Rectangle {
        z: 1
        x: -root.borderWidth
        y: -root.borderWidth
        width: parent.width + 2 * root.borderWidth
        height: parent.height + 2 * root.borderWidth
        radius: shadowShape.radius
        color: root.settings.border.enabled
            ? Qt.rgba(Qt.color(root.settings.border.color).r,
                      Qt.color(root.settings.border.color).g,
                      Qt.color(root.settings.border.color).b,
                      root.settings.border.alpha * (1 - root.fullProgress)) : "transparent"
        antialiasing: true
    }
    readonly property alias cameraFrameSource: cameraVideo
    readonly property alias cameraVideoOutput: nativeCameraVideo
}
