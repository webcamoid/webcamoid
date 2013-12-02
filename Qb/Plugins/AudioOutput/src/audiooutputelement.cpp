/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "audiooutputelement.h"

AudioOutputElement::AudioOutputElement(): QbElement()
{
    this->m_audioOutput = NULL;
    this->m_qtOutputInfo = QAudioDeviceInfo::defaultOutputDevice();
    this->m_audioConvert = Qb::create("ACapsConvert");
    this->m_outputSink = Qb::create("MultiSink");

    QObject::connect(this->m_outputSink.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SIGNAL(oStream(const QbPacket &)));

    QObject::connect(this->m_audioConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetAudioSystem();
    this->resetDataMode();
}

QString AudioOutputElement::audioSystem() const
{
    return this->m_audioSystem;
}

QStringList AudioOutputElement::availableAudioSystem() const
{
    QStringList audioSystems;

    audioSystems << "qt";

    if (QFileInfo("/usr/bin/pulseaudio").exists())
        audioSystems << "pulseaudio";

    if (QFileInfo("/proc/asound/version").exists())
        audioSystems << "alsa";

    if (QFileInfo("/dev/dsp").exists())
        audioSystems << "oss";

    return audioSystems;
}

QString AudioOutputElement::dataMode() const
{
    return this->m_dataMode;
}

QbCaps AudioOutputElement::findBestOptions(const QbCaps &caps,
                                           const QAudioDeviceInfo &deviceInfo,
                                           QAudioFormat *bestOption) const
{
    if (caps.mimeType() != "audio/x-raw")
        return QbCaps();

    QMap<AVSampleFormat, QAudioFormat::SampleType> formatToType;
    formatToType[AV_SAMPLE_FMT_NONE] = QAudioFormat::Unknown;
    formatToType[AV_SAMPLE_FMT_U8] = QAudioFormat::UnSignedInt;
    formatToType[AV_SAMPLE_FMT_S16] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_S32] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_FLT] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_DBL] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_U8P] = QAudioFormat::UnSignedInt;
    formatToType[AV_SAMPLE_FMT_S16P] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_S32P] = QAudioFormat::SignedInt;
    formatToType[AV_SAMPLE_FMT_FLTP] = QAudioFormat::Float;
    formatToType[AV_SAMPLE_FMT_DBLP] = QAudioFormat::Float;

    AVSampleFormat sampleFormat = av_get_sample_fmt(caps.property("format")
                                                        .toString()
                                                        .toStdString()
                                                        .c_str());

    QAudioFormat preferredAudioFormat;
    preferredAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    preferredAudioFormat.setChannelCount(caps.property("channels").toInt());
    preferredAudioFormat.setCodec("audio/pcm");
    preferredAudioFormat.setSampleRate(caps.property("rate").toInt());
    preferredAudioFormat.setSampleSize(8 * caps.property("bps").toInt());
    preferredAudioFormat.setSampleType(formatToType[sampleFormat]);

    QAudioFormat bestAudioFormat = deviceInfo.nearestFormat(preferredAudioFormat);
    AVSampleFormat oFormat = AV_SAMPLE_FMT_NONE;

    foreach (AVSampleFormat format, formatToType.keys(bestAudioFormat.sampleType()))
        if (av_get_bytes_per_sample(format) == (bestAudioFormat.sampleSize() >> 3))
            oFormat = format;

    char layout[256];
    int64_t channelLayout = av_get_default_channel_layout(bestAudioFormat.channelCount());

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 bestAudioFormat.channelCount(),
                                 channelLayout);

    QbCaps oCaps(QString("audio/x-raw,"
                         "format=%1,"
                         "bps=%2,"
                         "channels=%3,"
                         "rate=%4,"
                         "layout=%5").arg(av_get_sample_fmt_name(oFormat))
                                     .arg(bestAudioFormat.sampleSize() >> 3)
                                     .arg(bestAudioFormat.channelCount())
                                     .arg(bestAudioFormat.sampleRate())
                                     .arg(layout));

    if (bestOption)
        *bestOption = bestAudioFormat;

    return oCaps;
}

void AudioOutputElement::setAudioSystem(const QString &audioSystem)
{
    if (this->m_audioSystem != "qt"
        && audioSystem == "qt")
    {
        QbElement::ElementState state = this->m_outputSink->state();
        this->m_outputSink->setState(QbElement::ElementStateNull);
        this->m_audioConvert->setState(state);
    }
    else if (this->m_audioSystem == "qt"
             && audioSystem != "qt")
    {
        QbElement::ElementState state = this->m_audioConvert->state();
        this->m_audioConvert->setState(QbElement::ElementStateNull);
        this->m_outputSink->setState(state);
    }

    this->m_audioSystem = audioSystem;
}

void AudioOutputElement::setDataMode(const QString &dataMode)
{
    this->m_dataMode = dataMode;
}

void AudioOutputElement::resetAudioSystem()
{
    QStringList audioSystems = availableAudioSystem();

    this->setAudioSystem(audioSystems.isEmpty()? "": audioSystems[0]);
}

void AudioOutputElement::resetDataMode()
{
    this->setDataMode("push");
}

void AudioOutputElement::processFrame(const QbPacket &packet)
{

}

void AudioOutputElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "audio/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    static QbCaps curCaps;
    static QString curAudioSystem;

    QbCaps caps1(packet.caps());
    QbCaps caps2(curCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());

    if (caps1 != caps2
        || this->audioSystem() != curAudioSystem)
    {
        this->m_audioConvert->setState(QbElement::ElementStateNull);
        this->m_outputSink->setState(QbElement::ElementStateNull);

        int inputIndex = packet.index();
        QString location;
        QString options;

        if (this->audioSystem() == "qt")
        {
            QAudioFormat outputFormat;
            QbCaps caps = this->findBestOptions(packet.caps(), this->m_qtOutputInfo, &outputFormat);
            this->m_audioConvert->setProperty("caps", caps.toString());

            if (this->m_audioOutput)
                delete this->m_audioOutput;

            this->m_audioOutput = new QAudioOutput(this->m_qtOutputInfo,
                                                   outputFormat,
                                                   this);

            if (this->dataMode() == "push")
                this->m_audioOutput->start(&this->m_outputBuffer);
            else
                this->m_outputBufferPtr = this->m_audioOutput->start();
        }
        else if (this->audioSystem() == "pulseaudio")
        {
            location = "pulse";
            options = QString("-i %1 -o -f alsa").arg(inputIndex);
        }
        else if (this->audioSystem() == "alsa")
        {
            location = "hw:0";
            options = QString("-i %1 -o -f alsa").arg(inputIndex);
        }
        else if (this->audioSystem() == "oss")
        {
            location = "/dev/dsp";
            options = QString("-i %1 -o -f oss").arg(inputIndex);
        }

        if (this->audioSystem() != "qt")
        {
            this->m_outputSink->setProperty("location", location);
            this->m_outputSink->setProperty("options", options);

            QVariantMap streamCaps;
            streamCaps[QString("%1").arg(inputIndex)] = packet.caps().toString();

            this->m_outputSink->setProperty("streamCaps", streamCaps);
        }

        curCaps = packet.caps();
        curAudioSystem = this->audioSystem();
    }

    if (this->audioSystem() == "qt")
    {
        if (this->m_audioConvert->state() != QbElement::ElementStatePlaying)
            this->m_audioConvert->setState(QbElement::ElementStatePlaying);

        this->m_audioConvert->iStream(packet);
    }
    else
    {
        if (this->m_outputSink->state() != QbElement::ElementStatePlaying)
            this->m_outputSink->setState(QbElement::ElementStatePlaying);

        this->m_outputSink->iStream(packet);
    }
}

void AudioOutputElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (state == QbElement::ElementStateNull)
        if (this->m_audioOutput)
            delete this->m_audioOutput;

    if (this->audioSystem() == "qt")
        this->m_audioConvert->setState(state);
    else
        this->m_outputSink->setState(state);
}
