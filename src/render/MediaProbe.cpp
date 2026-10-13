#include "MediaProbe.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <cmath>

using namespace Omareel;

static double ratio(const QString &text)
{
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("[/:]")));
    if (parts.size() != 2) return text.toDouble();
    const double denominator = parts[1].toDouble();
    return denominator != 0.0 ? parts[0].toDouble() / denominator : 0.0;
}

VideoInfo Omareel::probeVideo(const QString &path, QString *error)
{
    const QString name = QFileInfo(path).fileName();
    if (!QFileInfo(path).isFile()) {
        if (error) *error = QStringLiteral("%1 does not exist").arg(path);
        return {};
    }
    QProcess process;
    process.start(QStringLiteral("ffprobe"), {QStringLiteral("-v"), QStringLiteral("error"),
        QStringLiteral("-show_entries"),
        QStringLiteral("stream=codec_type,width,height,r_frame_rate,avg_frame_rate,duration,"
                       "sample_aspect_ratio:stream_side_data=rotation:stream_tags=rotate"
                       ":format=duration"),
        QStringLiteral("-of"), QStringLiteral("json"), path});
    if (!process.waitForFinished(15000) || process.exitCode() != 0) {
        if (process.state() != QProcess::NotRunning) process.kill();
        if (error) *error = QStringLiteral("Could not read %1: %2").arg(name,
            QString::fromUtf8(process.readAllStandardError()).trimmed());
        return {};
    }
    const QJsonObject root = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    VideoInfo info;
    bool foundVideo = false;
    for (const QJsonValue &entry : root.value(QStringLiteral("streams")).toArray()) {
        const QJsonObject stream = entry.toObject();
        const QString type = stream.value(QStringLiteral("codec_type")).toString();
        if (type == QLatin1String("audio")) info.audio = true;
        if (type != QLatin1String("video") || foundVideo) continue;
        foundVideo = true;
        double width = stream.value(QStringLiteral("width")).toInt();
        const double height = stream.value(QStringLiteral("height")).toInt();
        const double sampleAspect = ratio(stream.value(QStringLiteral("sample_aspect_ratio")).toString());
        if (sampleAspect > 0.0) width *= sampleAspect;
        // Phones record upright video as landscape frames with a rotation; ffmpeg turns them.
        int rotation = stream.value(QStringLiteral("tags")).toObject()
                           .value(QStringLiteral("rotate")).toString().toInt();
        for (const QJsonValue &side : stream.value(QStringLiteral("side_data_list")).toArray())
            if (side.toObject().contains(QStringLiteral("rotation")))
                rotation = side.toObject().value(QStringLiteral("rotation")).toInt();
        const bool quarterTurn = std::abs(rotation) % 180 == 90;
        info.width = qRound(quarterTurn ? height : width);
        info.height = qRound(quarterTurn ? width : height);
        const double average = ratio(stream.value(QStringLiteral("avg_frame_rate")).toString());
        const double nominal = ratio(stream.value(QStringLiteral("r_frame_rate")).toString());
        info.fps = average > 0.0 && average <= 240.0 ? average : nominal > 0.0 ? nominal : 30.0;
        info.duration = stream.value(QStringLiteral("duration")).toString().toDouble();
    }
    if (info.duration <= 0.0)
        info.duration = root.value(QStringLiteral("format")).toObject()
                            .value(QStringLiteral("duration")).toString().toDouble();
    if (!info.isValid()) {
        if (error) *error = foundVideo ? QStringLiteral("%1 has no playable video").arg(name)
                                       : QStringLiteral("%1 is not a video").arg(name);
        return {};
    }
    if (error) error->clear();
    return info;
}
