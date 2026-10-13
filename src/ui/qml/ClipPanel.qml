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
        PanelLabel { visible: editor.hasCamera; text: "Layout" }
        OmButtonGroup {
            objectName: "clipLayoutGroup"
            visible: editor.hasCamera
            Layout.fillWidth: true
            options: [{value:"default", label:"Screen & camera"}, {value:"camera", label:"Camera"},
                {value:"screen", label:"Screen"}]
            value: root.selected ? root.selected.layout : "default"
            onChanged: value => editor.setClipLayout(editor.selectedClipId, value)
        }
        Label {
            visible: editor.hasCamera && root.selected !== null
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
        EditorButton { Layout.fillWidth: true; text: "Delete clip"; destructive: true; enabled: editor.clips.length > 1; onClicked: editor.removeClip(editor.selectedClipId) }
    }
}
