#include "core/ClipTimeline.h"

#include <QtTest>

using namespace Omareel;

class ClipTimelineTest : public QObject
{
    Q_OBJECT
private slots:
    void frameSourceMappingWithSpeedChanges()
    {
        ClipTimeline timeline({Clip{"slow", 1.0, 3.0, 0.5},
                               Clip{"fast", 10.0, 14.0, 2.0}});
        const double fps = 60.0;
        QCOMPARE(timeline.sourceTime(0 / fps), 1.0);
        QCOMPARE(timeline.sourceTime(120 / fps), 2.0);
        QCOMPARE(timeline.sourceTime(240 / fps), 10.0);
        QCOMPARE(timeline.sourceTime(270 / fps), 11.0);
        QCOMPARE(timeline.sourceTime(359 / fps), 13.966666666666667);
    }

    void mappingAndOperations()
    {
        ClipTimeline timeline({Clip{"a", 2.0, 6.0, 1.0}, Clip{"b", 10.0, 14.0, 2.0}});
        QCOMPARE(timeline.totalDuration(), 6.0);
        QCOMPARE(timeline.sourceTime(1.5), 3.5);
        QCOMPARE(timeline.sourceTime(5.0), 12.0);
        QCOMPARE(timeline.outputTime(1, 12.0), 5.0);
        QVERIFY(timeline.split(1.0));
        QCOMPARE(timeline.clips().size(), 3);
        QCOMPARE(timeline.clips()[0].out, 3.0);
        QCOMPARE(timeline.clips()[1].in, 3.0);
        QVERIFY(timeline.trim(1, 3.5, 5.5));
        QCOMPARE(timeline.totalDuration(), 5.0);
        QVERIFY(timeline.remove(0));
        QCOMPARE(timeline.sourceTime(0.0), 3.5);
    }

    void insertedVideosSitBetweenRecordingClips()
    {
        Clip insert{"m", 2.0, 4.5, 1.0};
        insert.media = QStringLiteral("media/demo.mp4");
        // Recording 0-3, a 2.5 s insert from 2 s into the video, recording 5-7 at 2x.
        ClipTimeline timeline({Clip{"a", 0.0, 3.0, 1.0}, insert, Clip{"b", 5.0, 7.0, 2.0}});
        QCOMPARE(timeline.totalDuration(), 6.5);
        double start = -1.0;
        QCOMPARE(timeline.clipAt(1.0, &start), 0);
        QCOMPARE(start, 0.0);
        QCOMPARE(timeline.clipAt(3.0, &start), 1);
        QCOMPARE(start, 3.0);
        QCOMPARE(timeline.clipAt(5.6, &start), 2);
        QCOMPARE(start, 5.5);
        QCOMPARE(timeline.clipAt(99.0), 2);
        QCOMPARE(ClipTimeline().clipAt(0.0), -1);
        // Clip-local time inside the insert, and the recording time it interrupts.
        QCOMPARE(timeline.sourceTime(4.0), 3.0);
        QCOMPARE(timeline.recordingSourceTime(4.0), 3.0);
        QCOMPARE(timeline.recordingSourceTime(1.5), 1.5);
        QCOMPARE(timeline.recordingSourceTime(6.0), 6.0);
        ClipTimeline leading({insert, Clip{"b", 5.0, 7.0, 1.0}});
        QCOMPARE(leading.recordingSourceTime(1.0), 5.0);
        // Recording neighbours bound a recording clip, across the insert.
        QCOMPARE(timeline.recordingBounds(0, 9.0), (QPair<double, double>{0.0, 5.0}));
        QCOMPARE(timeline.recordingBounds(2, 9.0), (QPair<double, double>{3.0, 9.0}));
        // Recording spans never map into the insert, whose in/out overlap them numerically.
        QCOMPARE(timeline.outputRange(2.5, 4.5), (QPair<double, double>{2.5, 3.0}));
        QCOMPARE(timeline.outputRange(3.5, 4.5), (QPair<double, double>{-1.0, -1.0}));
        QCOMPARE(timeline.outputRange(4.0, 6.0), (QPair<double, double>{5.5, 6.0}));
    }

    void sourceSpanCoversWhatSurvivesTheCuts()
    {
        // Source 1.438–1.903 and everything after 2.245 were cut.
        ClipTimeline timeline({Clip{"a", 0.0, 1.438, 1.0}, Clip{"b", 1.903, 2.245, 1.0}});
        const auto across = timeline.outputRange(0.376, 2.583);
        QCOMPARE(across.first, 0.376);
        QVERIFY(qAbs(across.second - (1.438 + 2.245 - 1.903)) < 1e-9);
        const auto endsInCut = timeline.outputRange(0.781, 1.781);
        QCOMPARE(endsInCut.first, 0.781);
        QCOMPARE(endsInCut.second, 1.438);
        const auto startsInCut = timeline.outputRange(1.6, 2.0);
        QCOMPARE(startsInCut.first, 1.438);
        QVERIFY(qAbs(startsInCut.second - (1.438 + 2.0 - 1.903)) < 1e-9);
        const auto allCut = timeline.outputRange(2.3, 2.583);
        QCOMPARE(allCut.first, -1.0);
        QCOMPARE(allCut.second, -1.0);

        ClipTimeline faster({Clip{"x", 1.0, 3.0, 2.0}});
        const auto halved = faster.outputRange(0.0, 2.0);
        QCOMPARE(halved.first, 0.0);
        QCOMPARE(halved.second, 0.5);
    }

    void deletesMiddleAndMapsTheNewCut()
    {
        ClipTimeline timeline({Clip{"first", 0.0, 3.0, 1.0}});
        QVERIFY(timeline.deleteRange(1.0, 2.0, QStringLiteral("remainder")));
        QCOMPARE(timeline.clips().size(), 2);
        QCOMPARE(timeline.clips()[0].id, QStringLiteral("first"));
        QCOMPARE(timeline.clips()[0].out, 1.0);
        QCOMPARE(timeline.clips()[1].id, QStringLiteral("remainder"));
        QCOMPARE(timeline.clips()[1].in, 2.0);
        QCOMPARE(timeline.totalDuration(), 2.0);
        QCOMPARE(timeline.sourceTime(0.99), 0.99);
        QCOMPARE(timeline.sourceTime(1.0), 2.0);
    }

    void deletesAcrossClipsWithDifferentSpeeds()
    {
        ClipTimeline timeline({Clip{"normal", 0.0, 2.0, 1.0},
                               Clip{"fast", 4.0, 8.0, 2.0}});
        QVERIFY(timeline.deleteRange(1.5, 2.5, QStringLiteral("unused")));
        QCOMPARE(timeline.clips().size(), 2);
        QCOMPARE(timeline.clips()[0].out, 1.5);
        QCOMPARE(timeline.clips()[1].in, 5.0);
        QCOMPARE(timeline.totalDuration(), 3.0);
        QCOMPARE(timeline.sourceTime(1.5), 5.0);
        QCOMPARE(timeline.sourceTime(2.0), 6.0);
    }

    void rejectsUnrepresentableOrCompleteCutsWithoutChangingClips()
    {
        ClipTimeline timeline({Clip{"only", 0.0, 2.0, 1.0}});
        QVERIFY(!timeline.deleteRange(0.0, 2.0, QStringLiteral("right")));
        QVERIFY(!timeline.deleteRange(0.05, 1.0, QStringLiteral("right")));
        QVERIFY(!timeline.deleteRange(1.0, 1.0, QStringLiteral("right")));
        QVERIFY(!timeline.deleteRange(0.5, 1.5, QStringLiteral("only")));
        QCOMPARE(timeline.clips().size(), 1);
        QCOMPARE(timeline.totalDuration(), 2.0);
    }
};

QTEST_APPLESS_MAIN(ClipTimelineTest)
#include "test_cliptimeline.moc"
