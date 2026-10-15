#include "MicrophoneMeter.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>
#include <algorithm>
#include <cmath>

using namespace Omareel;

MicrophoneMeter::MicrophoneMeter(QObject *parent): QObject(parent)
{
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &MicrophoneMeter::read);
}

MicrophoneMeter::~MicrophoneMeter()
{
    stop();
}

void MicrophoneMeter::setDevice(const QString &device)
{
    QAudioDevice input;
    if (!device.isEmpty()) {
        input = QMediaDevices::defaultAudioInput();
        const QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
        for (const QAudioDevice &candidate : inputs)
            if (QString::fromUtf8(candidate.id()) == device) input = candidate;
    }
    if (m_source && m_source->state() != QAudio::StoppedState && input.id() == m_inputId) return;
    stop();
    m_inputId = input.id();
    if (input.isNull()) return;
    m_source = std::make_unique<QAudioSource>(input, input.preferredFormat());
    m_input = m_source->start();
    if (m_input) m_timer.start();
}

double MicrophoneMeter::peakLevel(const QAudioFormat &format, const QByteArray &samples)
{
    const int bytes = format.bytesPerSample();
    if (bytes <= 0) return 0.0;
    float peak = 0.0f;
    for (qsizetype offset = 0; offset + bytes <= samples.size(); offset += bytes)
        peak = std::max(peak, std::abs(format.normalizedSampleValue(samples.constData() + offset)));
    if (peak <= 0.0f) return 0.0;
    return std::clamp((20.0 * std::log10(double(peak)) + 60.0) / 60.0, 0.0, 1.0);
}

void MicrophoneMeter::stop()
{
    m_timer.stop();
    m_input = nullptr;
    if (m_source) m_source->stop();
    m_source.reset();
    m_inputId.clear();
    if (m_level == 0.0) return;
    m_level = 0.0;
    emit levelChanged();
}

void MicrophoneMeter::read()
{
    if (!m_input) return;
    // Rises with each peak and falls back over a few hundred milliseconds, like a VU meter.
    double level = std::max(peakLevel(m_source->format(), m_input->readAll()), m_level * 0.75);
    if (level < 0.01) level = 0.0;
    if (std::abs(level - m_level) < 0.005) return;
    m_level = level;
    emit levelChanged();
}
