#include "core/LayoutTimeline.h"

#include <QtTest>

using namespace Omareel;

static Clip clip(const char *id, double in, double out, const char *layout, double speed = 1.0)
{
    Clip result{QString::fromLatin1(id), in, out, speed};
    result.layout = QString::fromLatin1(layout);
    return result;
}

class LayoutTimelineTest : public QObject
{
    Q_OBJECT
private slots:
    void eachLayoutHasItsTarget()
    {
        const QVector<Clip> clips{clip("a", 0, 2, "camera")};
        QCOMPARE(sampleLayout(clips, 1.0, true, true).cameraFull, 1.0);
        QCOMPARE(sampleLayout(clips, 1.0, true, true).bubble, 0.0);
        const QVector<Clip> screen{clip("a", 0, 2, "screen")};
        QCOMPARE(sampleLayout(screen, 1.0, true, true).cameraFull, 0.0);
        QCOMPARE(sampleLayout(screen, 1.0, true, true).bubble, 0.0);
        const QVector<Clip> standard{clip("a", 0, 2, "default")};
        QCOMPARE(sampleLayout(standard, 1.0, true, true).bubble, 1.0);
        QCOMPARE(sampleLayout(standard, 1.0, true, false).bubble, 0.0);
    }

    void withoutCameraEveryLayoutShowsTheScreen()
    {
        const QVector<Clip> clips{clip("a", 0, 2, "camera")};
        QCOMPARE(sampleLayout(clips, 1.0, false, true).cameraFull, 0.0);
        QCOMPARE(sampleLayout(clips, 1.0, false, true).bubble, 0.0);
    }

    void firstClipStartsOnItsLayout()
    {
        const QVector<Clip> clips{clip("intro", 0, 2, "camera"), clip("main", 2, 6, "default")};
        QCOMPARE(sampleLayout(clips, 0.0, true, true).cameraFull, 1.0);
    }

    void nextClipEasesFromThePreviousLayout()
    {
        const QVector<Clip> clips{clip("intro", 0, 2, "camera"), clip("main", 2, 6, "default")};
        QCOMPARE(sampleLayout(clips, 1.999, true, true).cameraFull, 1.0);
        QCOMPARE(sampleLayout(clips, 2.0, true, true).cameraFull, 1.0);
        const LayoutSample middle = sampleLayout(clips, 2.25, true, true);
        QVERIFY(qAbs(middle.cameraFull - 0.5) < 1e-9);
        QVERIFY(qAbs(middle.bubble - 0.5) < 1e-9);
        QCOMPARE(sampleLayout(clips, 2.5, true, true).cameraFull, 0.0);
        QCOMPARE(sampleLayout(clips, 2.5, true, true).bubble, 1.0);
        const double early = sampleLayout(clips, 2.05, true, true).cameraFull;
        const double late = sampleLayout(clips, 2.45, true, true).cameraFull;
        QVERIFY(early > 0.9 && early < 1.0);
        QVERIFY(late > 0.0 && late < 0.1);
    }

    void transitionUsesOutputTimeAtAnySpeed()
    {
        // The second clip plays 4 s of source at 2x: 2 s of output, transition still 0.5 s.
        const QVector<Clip> clips{clip("intro", 0, 1, "default"), clip("outro", 1, 5, "camera", 2.0)};
        QVERIFY(qAbs(sampleLayout(clips, 1.25, true, true).cameraFull - 0.5) < 1e-9);
        QCOMPARE(sampleLayout(clips, 1.5, true, true).cameraFull, 1.0);
    }

    void shortClipsStillEndOnTheirLayout()
    {
        const QVector<Clip> clips{clip("a", 0, 1, "camera"), clip("b", 1, 1.2, "screen"),
                                  clip("c", 1.2, 3, "camera")};
        QCOMPARE(sampleLayout(clips, 1.1, true, true).cameraFull, 0.5);
        QVERIFY(sampleLayout(clips, 1.1999, true, true).cameraFull < 1e-6);
        QCOMPARE(sampleLayout(clips, 1.2, true, true).cameraFull, 0.0);
        QVERIFY(qAbs(sampleLayout(clips, 1.45, true, true).cameraFull - 0.5) < 1e-9);
    }

    void bubbleFadesWhenTheScreenLosesIt()
    {
        const QVector<Clip> clips{clip("a", 0, 1, "default"), clip("b", 1, 3, "screen")};
        QVERIFY(qAbs(sampleLayout(clips, 1.25, true, true).bubble - 0.5) < 1e-9);
        QCOMPARE(sampleLayout(clips, 1.25, true, true).cameraFull, 0.0);
        QCOMPARE(sampleLayout(clips, 2.0, true, true).bubble, 0.0);
    }

    void insertedVideosCutInAndOut()
    {
        Clip insert = clip("m", 0, 1, "default");
        insert.media = QStringLiteral("media/demo.mp4");
        const QVector<Clip> clips{clip("a", 0, 1, "camera"), insert, clip("b", 1, 3, "default")};
        // No camera during the insert, and the clip after it starts on its own layout.
        QCOMPARE(sampleLayout(clips, 1.1, true, true).cameraFull, 0.0);
        QCOMPARE(sampleLayout(clips, 1.1, true, true).bubble, 0.0);
        QCOMPARE(sampleLayout(clips, 2.0, true, true).bubble, 1.0);
        QCOMPARE(sampleLayout(clips, 2.0, true, true).cameraFull, 0.0);
    }

    void pastTheEndHoldsTheLastLayout()
    {
        const QVector<Clip> clips{clip("a", 0, 1, "default"), clip("b", 1, 3, "camera")};
        QCOMPARE(sampleLayout(clips, 10.0, true, true).cameraFull, 1.0);
        QCOMPARE(sampleLayout({}, 1.0, true, true).bubble, 1.0);
    }
};

QTEST_MAIN(LayoutTimelineTest)
#include "test_layouttimeline.moc"
