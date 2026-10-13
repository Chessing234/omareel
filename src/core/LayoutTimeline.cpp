#include "LayoutTimeline.h"

#include "Easing.h"

#include <algorithm>

using namespace Omareel;

static LayoutSample target(const Clip &clip, bool cameraAvailable, bool bubbleEnabled)
{
    if (!cameraAvailable || clip.isMedia()) return {0.0, 0.0};
    if (clip.layout == QLatin1String("camera")) return {1.0, 0.0};
    if (clip.layout == QLatin1String("screen")) return {0.0, 0.0};
    return {0.0, bubbleEnabled ? 1.0 : 0.0};
}

LayoutSample Omareel::sampleLayout(const QVector<Clip> &clips, double outputTime,
                                   bool cameraAvailable, bool bubbleEnabled, double transition)
{
    if (clips.isEmpty()) return target(Clip{}, cameraAvailable, bubbleEnabled);
    double start = 0.0;
    LayoutSample previous = target(clips.first(), cameraAvailable, bubbleEnabled);
    for (int i = 0; i < clips.size(); ++i) {
        const Clip &clip = clips[i];
        const double length = std::max(0.0, (clip.out - clip.in) / std::max(0.000001, clip.speed));
        const LayoutSample next = target(clip, cameraAvailable, bubbleEnabled);
        if (outputTime < start + length || i == clips.size() - 1) {
            const double span = std::min(transition, length);
            // Inserted videos cut in and out; only recording clips morph between layouts.
            if (i == 0 || span <= 0.0 || clip.isMedia() || clips[i - 1].isMedia()) return next;
            const double progress = Easing::easeInOutCubic((outputTime - start) / span);
            return {previous.cameraFull + (next.cameraFull - previous.cameraFull) * progress,
                    previous.bubble + (next.bubble - previous.bubble) * progress};
        }
        previous = next;
        start += length;
    }
    return previous;
}
