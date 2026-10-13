import QtQuick
import QtQuick.Effects
import Omareel

Item {
    id: composition
    // Preview-only access point. Exporter continues finding the same objectName.
    readonly property alias frameSource: frame.frameSource
    readonly property alias videoOutput: frame.videoOutput
    readonly property alias cameraFrameSource: cameraOverlay.cameraFrameSource
    readonly property alias cameraVideoOutput: cameraOverlay.cameraVideoOutput
    property bool nativePreview: false
    width: comp.outputWidth
    height: comp.outputHeight
    property real ref: height / 1080
    // The cursor keeps the size it had on screen: one logical screen pixel is captureScale
    // source pixels, and the (cropped) source is drawn at videoWidth output pixels. A small
    // region therefore gets a proportionally large cursor instead of a microscopic one.
    property real cursorRef: (comp.captureScale > 0 ? comp.captureScale : 1)
                             * videoWidth / Math.max(1, comp.sourceWidth * comp.project.crop.w)
    property bool noBackground: comp.project.background.type === "none"
    property real pad: noBackground ? 0 : Math.min(width, height) * comp.project.frame.padding
    property real availableWidth: width - 2 * pad
    property real availableHeight: height - 2 * pad
    property real borderWidth: !noBackground && comp.project.frame.border.enabled
                               ? comp.project.frame.border.width * ref : 0
    property real sourceAspect: (comp.sourceWidth * comp.project.crop.w) / Math.max(1, comp.sourceHeight * comp.project.crop.h)
    property real videoWidth: noBackground ? width
        : Math.max(1, Math.min(availableWidth - 2 * borderWidth,
                              (availableHeight - 2 * borderWidth) * sourceAspect))
    property real videoHeight: noBackground ? height : videoWidth / sourceAspect
    readonly property bool cameraShown: comp.cameraAvailable && comp.cameraVisible
    // How far the current clip's layout has grown the camera to full frame.
    readonly property real cameraFull: cameraShown ? comp.layout.cameraFull : 0
    // Once the camera covers the canvas, nothing beneath it needs drawing.
    readonly property bool screenCovered: cameraFull >= 1
    clip: true

    Background {
        anchors.fill: parent
        visible: !composition.screenCovered
        settings: comp.project.background
        softwareRendering: comp.softwareRendering
    }

    Item {
        id: stage
        anchors.fill: parent
        visible: !composition.screenCovered
        layer.enabled: !comp.softwareRendering && comp.project.zoomStyle.motionBlur > 0
            && comp.zoom.velocity > 0.01
        layer.effect: MultiEffect {
            blurEnabled: true
            blurMax: 32
            blur: Math.min(1, comp.zoom.velocity * .32) * comp.project.zoomStyle.motionBlur
        }
        transform: Scale {
            origin.x: frame.x + comp.zoom.cx * frame.width
            origin.y: frame.y + comp.zoom.cy * frame.height
            xScale: comp.zoom.scale
            yScale: comp.zoom.scale
        }
        RoundedFrame {
            id: frame
            x: (composition.width - width) / 2
            y: (composition.height - height) / 2
            width: composition.videoWidth
            height: composition.videoHeight
            settings: comp.project.frame
            crop: comp.project.crop
            softwareRendering: comp.softwareRendering
            ref: composition.ref
            unframed: composition.noBackground
            nativePreview: composition.nativePreview
        }
        CursorOverlay {
            x: frame.x
            y: frame.y
            width: frame.width
            height: frame.height
            cursor: comp.cursor
            ripples: comp.ripples
            settings: comp.project.cursor
            ref: composition.cursorRef
        }
    }
    CameraOverlay {
        id: cameraOverlay
        visible: composition.cameraShown && (composition.cameraFull > 0 || comp.layout.bubble > 0)
        settings: comp.project.camera
        fullProgress: composition.cameraFull
        bubbleOpacity: comp.layout.bubble
        zoomScale: comp.zoom.scale
        outputWidth: composition.width
        outputHeight: composition.height
        sourceWidth: comp.cameraSourceWidth
        sourceHeight: comp.cameraSourceHeight
        softwareRendering: comp.softwareRendering
        nativePreview: composition.nativePreview
    }
    KeystrokeOverlay {
        visible: comp.project.keystrokes.enabled && comp.keystrokePills.length > 0
            && !composition.screenCovered
        opacity: 1 - composition.cameraFull
        pills: comp.keystrokePills
        settings: comp.project.keystrokes
        colors: comp.project.renderTheme
        ref: composition.ref
    }
}
