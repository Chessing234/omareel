import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Omareel.Ui

ColumnLayout {
    id: root
    spacing: theme.space.panelGap
    property var selected: {
        for (let i = 0; i < editor.clips.length; ++i)
            if (editor.clips[i].id === editor.selectedClipId) return editor.clips[i]
        return null
    }
    readonly property int selectedIndex: {
        for (let i = 0; i < editor.clips.length; ++i)
            if (editor.clips[i].id === editor.selectedClipId) return i
        return -1
    }
    readonly property bool insert: root.selected !== null && root.selected.kind === "media"
    readonly property int recordingClips: {
        let count = 0
        for (let i = 0; i < editor.clips.length; ++i)
            if (editor.clips[i].kind !== "media") ++count
        return count
    }
    Label {
        visible: root.selected === null
        Layout.fillWidth: true
        text: "Select a clip on the timeline"
        color: theme.textMuted
        font.pixelSize: theme.font.bodySmall
        wrapMode: Text.WordWrap
    }
    ColumnLayout {
        visible: root.selected !== null
        Layout.fillWidth: true
        spacing: 10
        PanelLabel { visible: editor.hasCamera && !root.insert; text: "Layout" }
        OmButtonGroup {
            objectName: "clipLayoutGroup"
            visible: editor.hasCamera && !root.insert
            Layout.fillWidth: true
            options: [{value:"default", label:"Screen & camera"}, {value:"camera", label:"Camera"},
                {value:"screen", label:"Screen"}]
            value: root.selected ? root.selected.layout : "default"
            onChanged: value => editor.setClipLayout(editor.selectedClipId, value)
        }
        Label {
            visible: editor.hasCamera && root.selected !== null && !root.insert
            Layout.fillWidth: true
            text: !root.selected ? ""
                : root.selected.layout === "camera" ? "The webcam fills the frame. Choose Fill or Square in Camera."
                : root.selected.layout === "screen" ? "The screen alone, without the camera bubble."
                : editor.project.camera.enabled ? "The screen with the camera bubble."
                : "The screen alone: the camera bubble is off in Camera."
            color: theme.textMuted
            font.pixelSize: theme.font.bodySmall
            wrapMode: Text.WordWrap
        }

        PanelLabel { visible: root.insert; text: "Inserted video" }
        Label {
            visible: root.insert
            Layout.fillWidth: true
            text: !root.insert ? ""
                : root.selected.missing ? root.selected.mediaName + " is missing from the recording."
                : root.selected.mediaName
            color: root.insert && root.selected.missing ? theme.record : theme.foreground
            font.pixelSize: theme.font.body
            elide: Text.ElideMiddle
        }
        PanelLabel { visible: root.insert; text: "Fit" }
        OmButtonGroup {
            objectName: "clipFitGroup"
            visible: root.insert
            Layout.fillWidth: true
            options: [{value:"fit", label:"Fit"}, {value:"fill", label:"Fill"},
                {value:"framed", label:"Framed"}]
            value: root.insert ? root.selected.fit : "fit"
            onChanged: value => editor.setClipFit(editor.selectedClipId, value)
        }
        Label {
            visible: root.insert
            Layout.fillWidth: true
            text: !root.insert ? ""
                : root.selected.fit === "fill" ? "Covers the whole frame, cropping what does not fit."
                : root.selected.fit === "framed" ? "Sits in the frame over the background, like the screen."
                : "Shows the whole video over a blurred copy of itself."
            color: theme.textMuted
            font.pixelSize: theme.font.bodySmall
            wrapMode: Text.WordWrap
        }
        RowLayout {
            visible: root.insert
            Layout.fillWidth: true
            spacing: theme.space.md
            EditorButton {
                Layout.fillWidth: true
                text: "Move earlier"
                enabled: root.selectedIndex > 0
                onClicked: editor.reorderClip(editor.selectedClipId, -1)
            }
            EditorButton {
                Layout.fillWidth: true
                text: "Move later"
                enabled: root.selectedIndex >= 0 && root.selectedIndex < editor.clips.length - 1
                onClicked: editor.reorderClip(editor.selectedClipId, 1)
            }
        }

        PanelLabel { text: "Speed" }
        EditorComboBox {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            model: [0.5, 0.75, 1, 1.2, 1.4, 1.6, 1.8, 2, 3, 4, 8, 16, 24]
            currentIndex: root.selected ? model.indexOf(root.selected.speed) : -1
            textRole: ""
            displayText: currentIndex >= 0 ? model[currentIndex] + "×" : ""
            onActivated: editor.setClipSpeed(editor.selectedClipId, model[currentIndex])
        }
        EditorButton { Layout.fillWidth: true; text: "Remove trims"; onClicked: editor.resetClipTrims(editor.selectedClipId) }
        EditorButton {
            Layout.fillWidth: true
            text: "Delete clip"
            destructive: true
            // The recording always keeps one clip.
            enabled: editor.clips.length > 1 && (root.insert || root.recordingClips > 1)
            onClicked: editor.removeClip(editor.selectedClipId)
        }
    }
    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: theme.separator }
    EditorButton {
        objectName: "insertClipButton"
        Layout.fillWidth: true
        text: editor.importing ? "Adding clip…" : "Insert video at playhead…"
        enabled: !editor.importing
        onClicked: insertDialog.open()
    }
    Label {
        Layout.fillWidth: true
        text: "Add a demo or a retake between clips. It is copied into the recording, splitting the clip at the playhead."
        color: theme.textMuted
        font.pixelSize: theme.font.bodySmall
        wrapMode: Text.WordWrap
    }
    InsertClipDialog { id: insertDialog }
}
