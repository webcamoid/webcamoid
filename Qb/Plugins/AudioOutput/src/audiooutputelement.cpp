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
    this->m_outputDevice = NULL;
    this->m_outputThread = NULL;
    this->m_run = false;
    this->m_timeDrift = 0;
    this->m_streamId = -1;
    this->m_audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    this->m_convert = Qb::create("ACapsConvert");
    this->resetMaxBufferSize();

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)),
                     Qt::DirectConnection);

    QObject::connect(this,
                     SIGNAL(stateChanged(QbElement::ElementState)),
                     this->m_convert.data(),
                     SLOT(setState(QbElement::ElementState)));
}

AudioOutputElement::~AudioOutputElement()
{
    this->uninit();
}

int AudioOutputElement::bufferSize() const
{
    return this->m_audioOutput? this->m_audioOutput->bufferSize(): 0;
}

QString AudioOutputElement::inputCaps() const
{
    return this->m_inputCaps;
}

int AudioOutputElement::maxBufferSize() const
{
    return this->m_maxBufferSize;
}

double AudioOutputElement::clock() const
{
    return this->hwClock() + this->m_timeDrift;
}

QbCaps AudioOutputElement::findBestOptions(const QbCaps &caps,
                                           const QAudioDeviceInfo &deviceInfo,
                                           QAudioFormat *bestOption) const
{
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

    QAudioFormat bestAudioFormat;

    if (caps.mimeType() == "audio/x-raw") {
        AVSampleFormat sampleFormat = av_get_sample_fmt(caps.property("format")
                                                            .toString()
                                                            .toStdString()
                                                            .c_str());

        QAudioFormat preferredAudioFormat;
        preferredAudioFormat.setByteOrder(QAudioFormat::BigEndian);
        preferredAudioFormat.setChannelCount(caps.property("channels").toInt());
        preferredAudioFormat.setCodec("audio/pcm");
        preferredAudioFormat.setSampleRate(caps.property("rate").toInt());
        preferredAudioFormat.setSampleSize(8 * caps.property("bps").toInt());
        preferredAudioFormat.setSampleType(formatToType[sampleFormat]);

        bestAudioFormat = deviceInfo.nearestFormat(preferredAudioFormat);
    }
    else
        bestAudioFormat = deviceInfo.preferredFormat();

    AVSampleFormat oFormat = AV_SAMPLE_FMT_NONE;

    foreach (AVSampleFormat format, formatToType.keys(bestAudioFormat.sampleType()))
        if (av_get_bytes_per_sample(format) == (bestAudioFormat.sampleSize() >> 3)) {
            oFormat = format;

            break;
        }

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
                         "layout=%5,"
                         "align=%6").arg(av_get_sample_fmt_name(oFormat))
                                    .arg(bestAudioFormat.sampleSize() >> 3)
                                    .arg(bestAudioFormat.channelCount())
                                    .arg(bestAudioFormat.sampleRate())
                                    .arg(layout)
                                    .arg(false));

    if (bestOption)
        *bestOption = bestAudioFormat;

    return oCaps;
}

double AudioOutputElement::hwClock() const
{
    if (!this->m_audioOutput)
        return 0;

    int bytesInBuffer = this->m_audioOutput->bufferSize()
                        - this->m_audioOutput->bytesFree();

    double usInBuffer = 8.0 * bytesInBuffer
                        / (this->m_audioOutput->format().channels()
                           * this->m_audioOutput->format().sampleSize()
                           * this->m_audioOutput->format().frequency());

    double pts = this->m_audioOutput->processedUSecs()
                 / 1.0e6
                 - usInBuffer;

    return pts;
}

bool AudioOutputElement::init()
{
    QAudioDeviceInfo audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    QAudioFormat outputFormat;

    QbCaps bestCaps = this->findBestOptions(this->m_inputCaps,
                                            audioDeviceInfo,
                                            &outputFormat);

    this->m_convert->setProperty("caps", bestCaps.toString());

    this->m_audioOutput = AudioOutputPtr(new QAudioOutput(audioDeviceInfo,
                                                          outputFormat));

    if (this->m_audioOutput) {
        emit this->bufferSizeChanged(this->m_audioOutput->bufferSize());

        this->m_outputDevice = this->m_audioOutput->start();
    }

    this->m_timeDrift = 0;
    this->m_audioBuffer.clear();
    this->m_outputThread = new Thread();

    QObject::connect(this->m_outputThread,
                     SIGNAL(runTh()),
                     this,
                     SLOT(pullFrame()),
                     Qt::DirectConnection);

    this->m_run = true;
    this->m_outputThread->start();

    return this->m_outputDevice? true: false;
}

void AudioOutputElement::uninit()
{
    this->m_mutex.lock();
    qDebug() << "AudioOutputElement::uninit lock";
    this->m_run = false;

    if (this->m_audioOutput) {
        this->m_audioOutput->stop();
        this->m_audioOutput.clear();
        this->m_outputDevice = NULL;
    }

    this->m_audioBuffer.clear();

    this->m_bufferEmpty.wakeAll();
    this->m_bufferNotFull.wakeAll();
    this->m_bufferNotEmpty.wakeAll();
    qDebug() << "AudioOutputElement::uninit unlock";
    this->m_mutex.unlock();

    if (this->m_outputThread) {
//        this->m_outputThread->quit();
        this->m_outputThread->wait();
        delete this->m_outputThread;
        this->m_outputThread = NULL;
    }
}

void AudioOutputElement::stateChange(QbElement::ElementState from,
                                     QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

void AudioOutputElement::setInputCaps(const QString &inputCaps)
{
    this->m_inputCaps = inputCaps;
}

void AudioOutputElement::setMaxBufferSize(int maxBufferSize)
{
    this->m_maxBufferSize = maxBufferSize;
}

void AudioOutputElement::resetInputCaps()
{
    this->m_inputCaps = "";
}

void AudioOutputElement::resetMaxBufferSize()
{
    this->setMaxBufferSize(1024 * 1024);
}

void AudioOutputElement::processFrame(const QbPacket &packet)
{
    int bufferSize = packet.caps().property("bps").toInt()
                     * packet.caps().property("channels").toInt()
                     * packet.caps().property("samples").toInt();

    this->m_audioBuffer.append((const char *) packet.buffer().data(),
                               bufferSize);
}

void AudioOutputElement::pullFrame()
{
    while (this->m_run) {
        this->m_mutex.lock();
        qDebug() << "AudioOutputElement::pullFrame lock";

        if (this->m_audioBuffer.isEmpty())
            this->m_bufferNotEmpty.wait(&this->m_mutex);

        if (!this->m_audioOutput) {
            qDebug() << "AudioOutputElement::pullFrame unlock";
            this->m_mutex.unlock();

            continue;
        }

        int writeBytes = qMin(qMin(this->m_audioOutput->bytesFree(),
                                   this->m_audioOutput->periodSize()),
                              this->m_audioBuffer.size());

        if (writeBytes < 1) {
            qDebug() << "AudioOutputElement::pullFrame unlock";
            this->m_mutex.unlock();

            continue;
        }

        emit this->elapsedTime(this->clock());

        this->m_outputDevice->write(this->m_audioBuffer.mid(0, writeBytes));
        this->m_audioBuffer.remove(0, writeBytes);

        if (this->m_audioBuffer.isEmpty())
            this->m_bufferEmpty.wakeAll();

        if (this->m_audioBuffer.size() < this->m_maxBufferSize)
            this->m_bufferNotFull.wakeAll();

        qDebug() << "AudioOutputElement::pullFrame unlock";
        this->m_mutex.unlock();
    }
}

void AudioOutputElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "audio/x-raw"
        || !this->m_audioOutput
        || !this->m_run)
        return;

    this->m_mutex.lock();
    qDebug() << "AudioOutputElement::iStream lock";

    if (packet.id() != this->m_streamId) {
        if (!this->m_audioBuffer.isEmpty())
            this->m_bufferEmpty.wait(&this->m_mutex);

        this->m_streamId = packet.id();

        this->m_timeDrift = packet.pts()
                            * packet.timeBase().value()
                            - this->hwClock();
    }

    if (this->m_audioBuffer.size() >= this->m_maxBufferSize)
        this->m_bufferNotFull.wait(&this->m_mutex);

    this->m_convert->iStream(packet);
    this->m_bufferNotEmpty.wakeAll();
    qDebug() << "AudioOutputElement::iStream unlock";
    this->m_mutex.unlock();
}
