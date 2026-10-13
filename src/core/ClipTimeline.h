#pragma once

#include "Project.h"

#include <QPair>

namespace Omareel {

class ClipTimeline
{
public:
    explicit ClipTimeline(QVector<Clip> clips = {}): m_clips(std::move(clips)) {}
    const QVector<Clip> &clips() const { return m_clips; }
    double totalDuration() const;
    // Index of the clip playing at an output time (the last clip past the end), -1 when empty.
    int clipAt(double outputTime, double *clipOutputStart = nullptr) const;
    double sourceTime(double outputTime) const;
    // Recording time at an output time; inside an inserted video, the recording time it
    // interrupts (the end of the recording clip before it, or the start of the one after).
    double recordingSourceTime(double outputTime) const;
    // Recording clips stay in recording order: a clip's in/out may move between the end of the
    // nearest recording clip before it and the start of the nearest after it.
    QPair<double, double> recordingBounds(int index, double sourceDuration) const;
    double outputTime(int clipIndex, double sourceTime) const;
    // Where a recording span plays once cuts are applied, from its first frame still in a clip
    // to its last; {-1, -1} when all of it was cut. Inserted videos never match.
    QPair<double, double> outputRange(double sourceStart, double sourceEnd) const;
    bool split(double outputTime);
    bool trim(int index, double newIn, double newOut);
    bool remove(int index);
    bool deleteRange(double from, double to, const QString &remainderId);

private:
    QVector<Clip> m_clips;
    int m_nextId = 1;
};

} // namespace Omareel
