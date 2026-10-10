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
    double sourceTime(double outputTime) const;
    double outputTime(int clipIndex, double sourceTime) const;
    // Where a source span plays once cuts are applied, from its first frame still in a clip to
    // its last; {-1, -1} when all of it was cut.
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
