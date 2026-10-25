#pragma once

#include "Project.h"

namespace Omareel {

// What a clip's layout shows at an output time. cameraFull is how far the webcam has grown from
// its bubble to full frame; bubble is how visible the bubble is while the screen shows.
struct LayoutSample {
    double cameraFull = 0.0;
    double bubble = 1.0;
};

// Layouts change at clip boundaries. The new clip eases from the previous clip's layout to its own
// over its first `transition` seconds of output (less for shorter clips), so every clip ends on its
// own layout. Output time, not source time, keeps transitions intact across cuts and speed changes.
// Inserted videos show no camera and cut in and out.
LayoutSample sampleLayout(const QVector<Clip> &clips, double outputTime, bool cameraAvailable,
                          bool bubbleEnabled, double transition = 0.5);

} // namespace Omareel
