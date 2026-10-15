#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <memory>

class QAudioSource;
class QIODevice;

namespace Omareel {

// Listens to a microphone while the launcher is open so it can show that sound is arriving
// before a recording starts. A wedged audio server or a muted microphone otherwise only shows
// up as a silent recording.
class MicrophoneMeter : public QObject
{
    Q_OBJECT
public:
    explicit MicrophoneMeter(QObject *parent = nullptr);
    ~MicrophoneMeter() override;

    // A PulseAudio source name, as the recorder takes it, or default_input. Empty stops listening.
    // Opens the device again when it has changed or the previous one stopped.
    void setDevice(const QString &device);
    // 0 at -60 dBFS or below, 1 at full scale.
    double level() const { return m_level; }
    static double peakLevel(const QAudioFormat &format, const QByteArray &samples);

signals:
    void levelChanged();

private:
    void stop();
    void read();
    std::unique_ptr<QAudioSource> m_source;
    QIODevice *m_input = nullptr;
    QByteArray m_inputId;
    QTimer m_timer;
    double m_level = 0.0;
};

} // namespace Omareel
