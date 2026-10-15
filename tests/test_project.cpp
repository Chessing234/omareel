#include "core/Project.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>

using namespace Omareel;

class ProjectTest : public QObject
{
    Q_OBJECT
private slots:
    void defaultsAndRoundTrip()
    {
        const Project original = Project::defaults(QStringLiteral("Demo"), 12.5);
        const auto json = original.toJson();
        QCOMPARE(json.value("version").toInt(), 1);
        QCOMPARE(json.value("defaultsVersion").toInt(), Project::CurrentDefaultsVersion);
        QVERIFY(json.value("aspect").isNull());
        QCOMPARE(json.value("clips").toArray().first().toObject().value("out").toDouble(), 12.5);
        QCOMPARE(original.frame.padding, 0.10);
        QCOMPARE(original.frame.shadow.intensity, 0.25);
        QCOMPARE(original.frame.shadow.blur, 40.0);
        QCOMPARE(original.frame.shadow.distance, 10.0);
        QCOMPARE(original.frame.border.enabled, false);
        QCOMPARE(original.frame.border.width, 7.0);
        QCOMPARE(original.frame.border.color, QColor(QStringLiteral("#000000")));
        QCOMPARE(original.frame.border.alpha, 1.0);
        QCOMPARE(original.zoomStyle.spring.mass, 2.25);
        QCOMPARE(original.cursor.spring.stiffness, 470.0);
        QCOMPARE(original.cursor.clickShrink, 0.8);
        QCOMPARE(original.cursor.style, QStringLiteral("light-arrow"));
        QCOMPARE(original.cursor.clickSound, QStringLiteral("none"));
        QCOMPARE(original.zoomStyle.motionBlur, 0.0);
        QCOMPARE(original.keystrokes.enabled, false);
        QCOMPARE(original.keystrokes.position, QStringLiteral("bottom-center"));
        QCOMPARE(original.keystrokes.size, 1.0);
        QCOMPARE(original.keystrokes.showOnlyShortcuts, true);
        QCOMPARE(original.keystrokes.holdMs, 900);
        QCOMPARE(original.exportSettings.height, 1080);
        QCOMPARE(original.exportSettings.quality, QStringLiteral("social"));
        QCOMPARE(original.exportSettings.gif.height, 480);
        QCOMPARE(original.camera.enabled, false);
        QCOMPARE(original.camera.position, QStringLiteral("bottom-right"));
        QCOMPARE(original.camera.size, 0.25);
        QCOMPARE(original.camera.shape, QStringLiteral("round"));
        QCOMPARE(original.camera.radius, 16.0);
        QCOMPARE(original.camera.flipHorizontal, false);
        QCOMPARE(original.camera.rotation, 0);
        QCOMPARE(original.camera.shadow.enabled, false);
        QCOMPARE(original.camera.border.enabled, false);
        QCOMPARE(original.camera.scaleDuringZoom, 0.7);
        QCOMPARE(original.camera.offset, QPointF(0.02, 0.02));
        QVERIFY(allowedAspects().contains(QStringLiteral("3:4")));
        QCOMPARE(allowedClipSpeeds().last(), 24.0);
        QTemporaryDir directory;
        const QString path = directory.filePath(QStringLiteral("project.json"));
        QString error;
        QVERIFY2(original.save(path, &error), qPrintable(error));
        const auto loaded = Project::load(path, &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(loaded.toJson(), original.toJson());
        const QJsonObject writtenFrame = loaded.toJson().value(QStringLiteral("frame")).toObject();
        QVERIFY(writtenFrame.contains(QStringLiteral("border")));
        QVERIFY(!writtenFrame.contains(QStringLiteral("inset")));
    }

    void clipLayoutsAndFullFrameCamera()
    {
        Project project = Project::defaults(QStringLiteral("Layouts"), 6.0);
        QCOMPARE(project.clips.first().layout, QStringLiteral("default"));
        QCOMPARE(project.camera.fullFrame, QStringLiteral("fill"));
        // A clip that keeps the default layout is written exactly as before layouts existed.
        const QJsonObject plain = project.toJson().value("clips").toArray().first().toObject();
        QVERIFY(!plain.contains(QStringLiteral("layout")));
        QCOMPARE(plain.keys(), (QStringList{"id", "in", "out", "speed"}));

        project.clips = {Clip{"intro", 0, 2, 1.0, "camera"}, Clip{"main", 2, 5, 1.0},
                         Clip{"quiet", 5, 6, 1.0, "screen"}};
        project.camera.fullFrame = QStringLiteral("square");
        const Project loaded = Project::fromJson(project.toJson());
        QCOMPARE(loaded.clips[0].layout, QStringLiteral("camera"));
        QCOMPARE(loaded.clips[1].layout, QStringLiteral("default"));
        QCOMPARE(loaded.clips[2].layout, QStringLiteral("screen"));
        QCOMPARE(loaded.camera.fullFrame, QStringLiteral("square"));
        QCOMPARE(loaded.toJson(), project.toJson());

        QJsonObject json = project.toJson();
        QJsonArray clips = json.value("clips").toArray();
        QJsonObject odd = clips.first().toObject();
        odd.insert(QStringLiteral("layout"), QStringLiteral("sideways"));
        clips[0] = odd;
        json.insert(QStringLiteral("clips"), clips);
        QJsonObject camera = json.value("camera").toObject();
        camera.insert(QStringLiteral("fullFrame"), QStringLiteral("stretch"));
        json.insert(QStringLiteral("camera"), camera);
        const Project fallback = Project::fromJson(json);
        QCOMPARE(fallback.clips.first().layout, QStringLiteral("default"));
        QCOMPARE(fallback.camera.fullFrame, QStringLiteral("fill"));
    }

    void insertedVideosStayInsideTheBundle()
    {
        QCOMPARE(sanitizedMediaPath(QStringLiteral("media/demo.mp4")), QStringLiteral("media/demo.mp4"));
        QCOMPARE(sanitizedMediaPath(QStringLiteral("media/./demo.mp4")), QStringLiteral("media/demo.mp4"));
        QCOMPARE(sanitizedMediaPath(QStringLiteral("media/talk..final.mp4")),
                 QStringLiteral("media/talk..final.mp4"));
        QCOMPARE(sanitizedMediaPath(QStringLiteral("/home/demo.mp4")), QString());
        QCOMPARE(sanitizedMediaPath(QStringLiteral("media/../screen.mp4")), QString());
        QCOMPARE(sanitizedMediaPath(QStringLiteral("media/../../etc/passwd")), QString());
        QCOMPARE(sanitizedMediaPath(QStringLiteral("demo.mp4")), QString());
        QCOMPARE(sanitizedMediaPath(QString()), QString());

        Project project = Project::defaults(QStringLiteral("Inserts"), 4.0);
        Clip insert{QStringLiteral("m"), 0.5, 2.0, 1.0};
        insert.media = QStringLiteral("media/demo.mp4");
        insert.fit = QStringLiteral("framed");
        project.clips << insert;
        const QJsonObject written = project.toJson().value("clips").toArray().last().toObject();
        QCOMPARE(written.value("media").toString(), QStringLiteral("media/demo.mp4"));
        QCOMPARE(written.value("fit").toString(), QStringLiteral("framed"));
        QVERIFY(!project.toJson().value("clips").toArray().first().toObject().contains("media"));
        const Project loaded = Project::fromJson(project.toJson());
        QCOMPARE(loaded.clips.size(), 2);
        QVERIFY(!loaded.clips[0].isMedia());
        QVERIFY(loaded.clips[1].isMedia());
        QCOMPARE(loaded.clips[1].fit, QStringLiteral("framed"));
        QCOMPARE(loaded.toJson(), project.toJson());

        // An escaping path drops the clip instead of turning it into recording footage; an
        // unknown fit falls back to fit.
        QJsonObject json = project.toJson();
        QJsonArray clips = json.value("clips").toArray();
        QJsonObject escaping = clips[1].toObject();
        escaping.insert(QStringLiteral("media"), QStringLiteral("../outside.mp4"));
        QJsonObject oddFit = clips[1].toObject();
        oddFit.insert(QStringLiteral("fit"), QStringLiteral("stretch"));
        clips[1] = escaping;
        clips << oddFit;
        json.insert(QStringLiteral("clips"), clips);
        const Project guarded = Project::fromJson(json);
        QCOMPARE(guarded.clips.size(), 2);
        QVERIFY(guarded.clips[1].isMedia());
        QCOMPARE(guarded.clips[1].fit, QStringLiteral("fit"));
    }

    void migratesLegacyInsetToBorder()
    {
        const Project migrated = Project::fromJson(QJsonObject{
            {QStringLiteral("frame"), QJsonObject{
                {QStringLiteral("inset"), QJsonObject{
                    {QStringLiteral("enabled"), true}, {QStringLiteral("width"), 5.0},
                    {QStringLiteral("color"), QStringLiteral("#123456")},
                    {QStringLiteral("alpha"), 0.6}}}}},
            {QStringLiteral("camera"), QJsonObject{
                {QStringLiteral("inset"), QJsonObject{
                    {QStringLiteral("enabled"), true}, {QStringLiteral("width"), 3.0},
                    {QStringLiteral("color"), QStringLiteral("#abcdef")},
                    {QStringLiteral("alpha"), 0.8}}}}}
        });
        QVERIFY(migrated.frame.border.enabled);
        QCOMPARE(migrated.frame.border.width, 5.0);
        QCOMPARE(migrated.frame.border.color, QColor(QStringLiteral("#123456")));
        QCOMPARE(migrated.frame.border.alpha, 0.6);
        QVERIFY(migrated.camera.border.enabled);
        QCOMPARE(migrated.camera.border.width, 3.0);
        const QJsonObject json = migrated.toJson();
        QVERIFY(json.value(QStringLiteral("frame")).toObject().contains(QStringLiteral("border")));
        QVERIFY(!json.value(QStringLiteral("frame")).toObject().contains(QStringLiteral("inset")));
        QVERIFY(json.value(QStringLiteral("camera")).toObject().contains(QStringLiteral("border")));
        QVERIFY(!json.value(QStringLiteral("camera")).toObject().contains(QStringLiteral("inset")));
    }

    void migratesOnlyUntouchedOldFrameDefaults()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto writeAndLoad = [&directory](const QString &name, const QJsonObject &frame) {
            const QString path = directory.filePath(name + QStringLiteral(".json"));
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly)) return Project{};
            file.write(QJsonDocument(QJsonObject{{QStringLiteral("frame"), frame}})
                           .toJson(QJsonDocument::Indented));
            file.close();
            QString error;
            const Project loaded = Project::load(path, &error);
            if (!error.isEmpty()) return Project{};
            return loaded;
        };

        const Project untouched = writeAndLoad(QStringLiteral("untouched"), QJsonObject{
            {QStringLiteral("shadow"), QJsonObject{
                {QStringLiteral("enabled"), true}, {QStringLiteral("intensity"), 0.75},
                {QStringLiteral("blur"), 20.0}, {QStringLiteral("distance"), 25.0}}},
            {QStringLiteral("inset"), QJsonObject{
                {QStringLiteral("enabled"), false}, {QStringLiteral("width"), 0.0},
                {QStringLiteral("color"), QStringLiteral("#000000")},
                {QStringLiteral("alpha"), 0.5}}}});
        QCOMPARE(untouched.defaultsVersion, Project::CurrentDefaultsVersion);
        QCOMPARE(untouched.frame.shadow.intensity, 0.25);
        QCOMPARE(untouched.frame.shadow.blur, 40.0);
        QCOMPARE(untouched.frame.shadow.distance, 10.0);
        QCOMPARE(untouched.frame.border.width, 7.0);
        QCOMPARE(untouched.frame.border.alpha, 1.0);

        const Project customised = writeAndLoad(QStringLiteral("customised"), QJsonObject{
            {QStringLiteral("shadow"), QJsonObject{
                {QStringLiteral("enabled"), true}, {QStringLiteral("intensity"), 0.61},
                {QStringLiteral("blur"), 20.0}, {QStringLiteral("distance"), 25.0}}},
            {QStringLiteral("inset"), QJsonObject{
                {QStringLiteral("enabled"), true}, {QStringLiteral("width"), 3.0},
                {QStringLiteral("color"), QStringLiteral("#123456")},
                {QStringLiteral("alpha"), 0.5}}}});
        QCOMPARE(customised.defaultsVersion, Project::CurrentDefaultsVersion);
        QCOMPARE(customised.frame.shadow.intensity, 0.61);
        QCOMPARE(customised.frame.shadow.blur, 20.0);
        QCOMPARE(customised.frame.shadow.distance, 25.0);
        QVERIFY(customised.frame.border.enabled);
        QCOMPARE(customised.frame.border.width, 3.0);
        QCOMPARE(customised.frame.border.color, QColor(QStringLiteral("#123456")));
        QCOMPARE(customised.frame.border.alpha, 0.5);

        const Project toggled = writeAndLoad(QStringLiteral("toggled"), QJsonObject{
            {QStringLiteral("shadow"), QJsonObject{
                {QStringLiteral("enabled"), false}, {QStringLiteral("intensity"), 0.75},
                {QStringLiteral("blur"), 20.0}, {QStringLiteral("distance"), 25.0},
                {QStringLiteral("angle"), 90.0}}},
            {QStringLiteral("inset"), QJsonObject{
                {QStringLiteral("enabled"), true}, {QStringLiteral("width"), 0.0},
                {QStringLiteral("color"), QStringLiteral("#000000")},
                {QStringLiteral("alpha"), 0.5}}}});
        QCOMPARE(toggled.defaultsVersion, Project::CurrentDefaultsVersion);
        QVERIFY(!toggled.frame.shadow.enabled);
        QCOMPARE(toggled.frame.shadow.intensity, 0.75);
        QCOMPARE(toggled.frame.shadow.blur, 20.0);
        QCOMPARE(toggled.frame.shadow.distance, 25.0);
        QVERIFY(toggled.frame.border.enabled);
        QCOMPARE(toggled.frame.border.width, 0.0);
        QCOMPARE(toggled.frame.border.alpha, 0.5);
    }

    void cameraCropFoldsIntoShape_data()
    {
        QTest::addColumn<int>("defaultsVersion");
        QTest::addColumn<QString>("shape");
        QTest::addColumn<QString>("crop");
        QTest::addColumn<QString>("expectedShape");
        QTest::addColumn<double>("expectedRadius");

        QTest::newRow("round") << 2 << "round" << "original" << "round" << 12.0;
        QTest::newRow("rounded square") << 2 << "rounded" << "square" << "square" << 12.0;
        QTest::newRow("rounded original") << 2 << "rounded" << "original" << "rectangle" << 12.0;
        QTest::newRow("sharp square") << 2 << "square" << "square" << "square" << 0.0;
        QTest::newRow("sharp original") << 2 << "square" << "original" << "rectangle" << 0.0;
        QTest::newRow("no defaults version") << 0 << "rounded" << "original" << "rectangle" << 12.0;
        QTest::newRow("current square keeps its corners") << 3 << "square" << "" << "square" << 12.0;
        QTest::newRow("current rectangle") << 3 << "rectangle" << "" << "rectangle" << 12.0;
    }

    void cameraCropFoldsIntoShape()
    {
        QFETCH(int, defaultsVersion);
        QFETCH(QString, shape);
        QFETCH(QString, crop);
        QFETCH(QString, expectedShape);
        QFETCH(double, expectedRadius);
        QJsonObject camera{{QStringLiteral("shape"), shape}, {QStringLiteral("radius"), 12.0}};
        if (!crop.isEmpty()) camera.insert(QStringLiteral("crop"), crop);
        QJsonObject root{{QStringLiteral("camera"), camera}};
        if (defaultsVersion > 0) root.insert(QStringLiteral("defaultsVersion"), defaultsVersion);
        const Project loaded = Project::fromJson(root);
        QCOMPARE(loaded.camera.shape, expectedShape);
        QCOMPARE(loaded.camera.radius, expectedRadius);
        const QJsonObject saved = loaded.toJson().value(QStringLiteral("camera")).toObject();
        QVERIFY(!saved.contains(QStringLiteral("crop")));
        QCOMPARE(Project::fromJson(loaded.toJson()).camera.shape, expectedShape);
    }
};

QTEST_APPLESS_MAIN(ProjectTest)
#include "test_project.moc"
