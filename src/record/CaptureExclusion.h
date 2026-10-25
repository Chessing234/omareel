#pragma once
#include <QJsonObject>
#include <QStringList>

namespace Omareel {
class CaptureExclusion
{
public:
    struct Status {
        bool loaded = false;
        QString builtHash;
        QString reason;
        QJsonObject details;
    };
    static Status status();
    static Status ensureLoaded();
    static bool compatibleHash(const QString &built, const QString &running);
    static QString overlayMonitor(bool plugin, const QString &recorded, const QStringList &monitors);
    static bool overlaysOffMonitor(const QString &monitor);
};

// Hyprland draws a software cursor into the frame itself, so screen capture records it, and by
// default it uses one on NVIDIA whenever a monitor has VRR on. Holding hardware cursors for the
// recording keeps the pointer out of screen.mp4; the user's setting comes back on destruction.
class HardwareCursorHold
{
public:
    HardwareCursorHold();
    ~HardwareCursorHold();
    HardwareCursorHold(const HardwareCursorHold &) = delete;
    HardwareCursorHold &operator=(const HardwareCursorHold &) = delete;

private:
    int m_previous = -1;
};
}
