/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "audioinputelement.h"

AudioInputElement::AudioInputElement(): QbElement()
{
    this->m_inputDevice = NULL;
    this->m_streamId = -1;
    this->m_pts = 0;
    this->m_audioDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
    this->resetBufferSize();

    QObject::connect(&this->m_audioBuffer,
                     SIGNAL(dataReady(const QByteArray &)),
                     this,
                     SLOT(processFrame(const QByteArray &)));
}

AudioInputElement::~AudioInputElement()
{
    this->uninit();
}

int AudioInputElement::bufferSize() const
{
    return this->m_bufferSize;
}

QString AudioInputElement::streamCaps() const
{
    QAudioDeviceInfo audioDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
    QAudioFormat inputFormat = audioDeviceInfo.preferredFormat();

    return this->findBestOptions(inputFormat).toString();
}

QbCaps AudioInputElement::findBestOptions(const QAudioFormat &audioFormat) const
{
    QString format = QbUtils::defaultSampleFormat(audioFormat.sampleType(),
                                                  audioFormat.sampleSize(),
                                                  false);
    QString layout = QbUtils::defaultChannelLayout(audioFormat.channelCount());

    QbCaps caps(QString("audio/x-raw,"
                        "format=%1,"
                        "bps=%2,"
                        "channels=%3,"
                        "rate=%4,"
                        "layout=%5,"
                        "align=%6").arg(format)
                                   .arg(audioFormat.sampleSize() >> 3)
                                   .arg(audioFormat.channelCount())
                                   .arg(audioFormat.sampleRate())
                                   .arg(layout)
                                   .arg(false));

    return caps;
}

void AudioInputElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void AudioInputElement::setBufferSize(int bufferSize)
{
    this->m_bufferSize = bufferSize;
}

void AudioInputElement::resetBufferSize()
{
    this->setBufferSize(4096);
}

bool AudioInputElement::init()
{
    QAudioDeviceInfo audioDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
    QAudioFormat inputFormat = audioDeviceInfo.preferredFormat();

    this->m_caps = this->findBestOptions(inputFormat);

    this->m_audioInput = AudioInputPtr(new QAudioInput(audioDeviceInfo,
                                                        inputFormat));

    if (this->m_audioInput) {
        int bps = this->m_caps.property("bps").toInt();
        int channels = this->m_caps.property("channels").toInt();
        int frameRate = this->m_caps.property("rate").toInt();
        qint64 bufferSize = bps * channels * this->m_bufferSize;

        this->m_streamId = Qb::id();
        this->m_pts = 0;
        this->m_timeBase = QbFrac(1, frameRate);

        this->m_audioInput->setBufferSize(bufferSize);
        this->m_audioBuffer.open(QIODevice::ReadWrite);
        this->m_audioInput->start(&this->m_audioBuffer);
    }

    return this->m_inputDevice? true: false;
}

void AudioInputElement::uninit()
{
    if (this->m_audioInput) {
        this->m_audioInput->stop();
        this->m_audioInput.clear();
        this->m_inputDevice = NULL;
    }

    this->m_audioBuffer.close();
}

void AudioInputElement::processFrame(const QByteArray &data)
{
    QbBufferPtr oBuffer(new char[data.size()]);
    memcpy(oBuffer.data(), data.constData(), data.size());

    QbCaps caps = this->m_caps;
    int bps = this->m_caps.property("bps").toInt();
    int channels = this->m_caps.property("channels").toInt();
    int samples = data.size() / bps / channels;
    caps.setProperty("samples", samples);

    QbPacket oPacket(caps,
                     oBuffer,
                     data.size());

    oPacket.setPts(this->m_pts);
    oPacket.setTimeBase(this->m_timeBase);
    oPacket.setIndex(0);
    oPacket.setId(this->m_streamId);
    this->m_pts += samples;

    emit this->oStream(oPacket);
}
