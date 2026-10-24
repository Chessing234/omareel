#pragma once

#include <QJsonObject>
#include <QString>

namespace Omareel {

struct RecordingPreferences {
    bool systemAudio = true;
    bool microphone = true;
    QString microphoneDevice = QStringLiteral("default_input");
    bool webcamEnabled = false;
    QString webcamDevice = QStringLiteral("/dev/video2");
    // The camera last picked in the launcher; webcamDevice stands in for it while it is unplugged.
    QString webcamChosenDevice = webcamDevice;
    int webcamHeight = 1080;
    int webcamRotation = 0;
    bool webcamFlipHorizontal = false;
    bool selfViewEnabled = true;
    QString selfViewSize = QStringLiteral("M");
    double selfViewX = 1.0;
    double selfViewY = 1.0;
    QString captureBackend = QStringLiteral("auto");
    bool countdownBeforeRecording = false;
    QJsonObject webcam{
        {QStringLiteral("enabled"), false},
        {QStringLiteral("position"), QStringLiteral("bottom-right")},
        {QStringLiteral("size"), 0.25},
        {QStringLiteral("shape"), QStringLiteral("round")},
        {QStringLiteral("radius"), 16},
        {QStringLiteral("crop"), QStringLiteral("original")},
        {QStringLiteral("flipHorizontal"), false},
        {QStringLiteral("rotation"), 0},
        {QStringLiteral("shadow"), QJsonObject{{QStringLiteral("enabled"), true},
            {QStringLiteral("intensity"), 0.28}, {QStringLiteral("blur"), 23},
            {QStringLiteral("distance"), 20}, {QStringLiteral("angle"), 45}}},
        {QStringLiteral("border"), QJsonObject{{QStringLiteral("enabled"), true},
            {QStringLiteral("width"), 6}, {QStringLiteral("color"), QStringLiteral("#ffffff")},
            {QStringLiteral("alpha"), 0.7}}},
        {QStringLiteral("scaleDuringZoom"), 0.7},
        {QStringLiteral("offset"), QJsonObject{{QStringLiteral("x"), 0.035}, {QStringLiteral("y"), 0.035}}}
    };

    static QString path();
    static RecordingPreferences load();
    bool save(QString *error = nullptr) const;
};

} // namespace Omareel
