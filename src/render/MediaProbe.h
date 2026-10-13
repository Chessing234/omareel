#pragma once

#include <QString>

namespace Omareel {

// What an inserted video holds, as ffmpeg will decode it.
struct VideoInfo {
    int width = 0;          // display size: rotation and sample aspect ratio applied
    int height = 0;
    double duration = 0.0;  // of the video stream; of the container when the stream has none
    double fps = 30.0;
    bool audio = false;
    bool isValid() const { return width > 0 && height > 0 && duration > 0.0; }
};

VideoInfo probeVideo(const QString &path, QString *error = nullptr);

} // namespace Omareel
