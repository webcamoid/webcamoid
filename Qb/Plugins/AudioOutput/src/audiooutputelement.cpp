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
    this->m_outputDevice = NULL;
    this->m_audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    this->m_audioConvert = Qb::create("ACapsConvert");

    this->resetOutputThread();

    this->m_soundInitTimer = runThread(QCoreApplication::instance()->thread(),
                                       SLOT(init()));

    this->m_soundUninitTimer = runThread(QCoreApplication::instance()->thread(),
                                         SLOT(uninit()));

    this->m_soundInitTimer->setSingleShot(true);
    this->m_soundUninitTimer->setSingleShot(true);

    QObject::connect(this->m_audioConvert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    QObject::connect(this,
                     SIGNAL(stateChanged(QbElement::ElementState)),
                     this->m_audioConvert.data(),
                     SLOT(setState(QbElement::ElementState)));
}

AudioOutputElement::~AudioOutputElement()
{
    if (this->m_audioOutput)
        delete this->m_audioOutput;
}

int AudioOutputElement::bufferSize() const
{
    return this->m_audioOutput? this->m_audioOutput->bufferSize(): 0;
}

QString AudioOutputElement::outputThread() const
{
    return this->m_outputTh;
}

TimerPtr AudioOutputElement::runThread(QThread *thread, const char *method)
{
    TimerPtr timer = TimerPtr(new QTimer());

    QObject::connect(timer.data(), SIGNAL(timeout()), this, method, Qt::DirectConnection);
    timer->moveToThread(thread);

    return timer;
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

    if (caps.mimeType() == "audio/x-raw")
    {
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
        if (av_get_bytes_per_sample(format) == (bestAudioFormat.sampleSize() >> 3))
        {
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

bool AudioOutputElement::init(QbCaps caps)
{
    if (!this->m_mutex.tryLock())
        return false;

    this->uninit(false);
    bool result = false;

    caps = caps? caps: this->m_inCaps;

    if (caps)
    {
        QAudioDeviceInfo audioDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
        QAudioFormat outputFormat;

        QbCaps bestCaps = this->findBestOptions(caps,
                                            audioDeviceInfo,
                                            &outputFormat);

        this->m_audioConvert->setProperty("caps", bestCaps.toString());

        this->m_audioOutput = new QAudioOutput(audioDeviceInfo,
                                               outputFormat);

        if (this->m_audioOutput)
        {
            emit this->bufferSizeChanged(this->m_audioOutput->bufferSize());

            this->m_outputDevice = this->m_audioOutput->start();

            if (this->m_outputDevice)
            {
                this->m_curCaps = caps;
                result = true;
            }
        }
    }

    this->m_mutex.unlock();

    return result;
}

void AudioOutputElement::uninit(bool lock)
{
    if (lock)
        if (!this->m_mutex.tryLock())
            return;

    if (this->m_audioOutput)
    {
        qDebug() << QThread::currentThread() << QCoreApplication::instance()->thread();
        this->m_audioOutput->stop();
        delete this->m_audioOutput;
        this->m_audioOutput = NULL;
        this->m_outputDevice = NULL;
        this->m_audioBuffer.clear();
    }

    if (lock)
        this->m_mutex.unlock();
}

void AudioOutputElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        QMetaObject::invokeMethod(this->m_soundInitTimer.data(), "start");
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        QMetaObject::invokeMethod(this->m_soundUninitTimer.data(), "start");
    else if (from == QbElement::ElementStatePlaying
             && to == QbElement::ElementStatePaused
             && this->m_pullTimer)
        QMetaObject::invokeMethod(this->m_pullTimer.data(), "stop");
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStatePlaying
             && this->m_pullTimer)
        QMetaObject::invokeMethod(this->m_pullTimer.data(), "start");
}

void AudioOutputElement::setOutputThread(const QString &outputThread)
{
    QbElement::ElementState state = this->state();
    this->setState(QbElement::ElementStateNull);

    this->m_outputTh = outputThread;
    this->m_outputThread = Qb::requestThread(this->m_outputTh);

    if (this->m_outputTh == "MAIN")
        this->m_pullTimer = this->runThread(QCoreApplication::instance()->thread(),
                                            SLOT(pullFrame()));
    else if (!this->m_outputThread)
        this->m_pullTimer = this->runThread(this->thread(),
                                            SLOT(pullFrame()));
    else
        this->m_pullTimer = this->runThread(this->m_outputThread.data(),
                                            SLOT(pullFrame()));

    this->setState(state);
}

void AudioOutputElement::resetOutputThread()
{
    this->setOutputThread("");
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
    this->m_packetQueueMutex.lock();

    while (!this->m_packetQueue.isEmpty())
    {
        QbPacket packet = this->m_packetQueue.at(0);
        this->feedAudio();

        this->m_inCaps = packet.caps();
        this->m_inCaps.setProperty("samples", QVariant());

        if (this->m_inCaps != this->m_curCaps
            || !this->m_audioOutput)
        {
            QMetaObject::invokeMethod(this->m_soundInitTimer.data(), "start");

            break;
        }

        this->m_audioConvert->iStream(packet);
        this->m_packetQueue.removeAt(0);
    }

    this->m_packetQueueMutex.unlock();

    if (!this->m_mutex.tryLock())
        return;

    if (this->m_audioOutput)
    {
        if (this->m_audioBuffer.size() < this->m_audioOutput->bufferSize())
            emit this->requestFrame(this->m_audioOutput->bufferSize());
    }
    else
        emit this->requestFrame(0);

    this->m_mutex.unlock();

    this->feedAudio();
}

void AudioOutputElement::feedAudio()
{
    if (!this->m_mutex.tryLock())
        return;

    if (this->m_audioOutput)
    {
        int audioBufferSize = this->m_audioBuffer.size();

        if (audioBufferSize < this->m_audioOutput->bufferSize())
            emit this->requestFrame(this->m_audioOutput->bufferSize());

        int writeBytes = qMin(qMin(this->m_audioOutput->bytesFree(),
                                   this->m_audioOutput->periodSize()),
                              audioBufferSize);

        if (writeBytes > 0)
        {
            int bytesInBuffer = this->m_audioOutput->bufferSize()
                                - this->m_audioOutput->bytesFree();

            double usInBuffer = 8.0 * bytesInBuffer
                                / (this->m_audioOutput->format().channels()
                                   * this->m_audioOutput->format().sampleSize()
                                   * this->m_audioOutput->format().frequency());

            double pts = this->m_audioOutput->processedUSecs()
                         / 1.0e6
                         - usInBuffer;

            emit this->elapsedTime(pts);

            this->m_outputDevice->write(this->m_audioBuffer.mid(0, writeBytes));
            this->m_audioBuffer.remove(0, writeBytes);
        }
    }

    this->m_mutex.unlock();
}

void AudioOutputElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "audio/x-raw")
        return;

    this->m_packetQueueMutex.lock();
    this->m_packetQueue.enqueue(packet);
    this->m_packetQueueMutex.unlock();
}
