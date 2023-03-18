/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <akaudiocaps.h>

#include "abstractstream.h"

class AudioStreamPrivate;

class AudioStream: public AbstractStream
{
    Q_OBJECT

    public:
        AudioStream(const AVFormatContext *formatContext=nullptr,
                    uint index=0, int streamIndex=-1,
                    const QVariantMap &configs={},
                    const QMap<QString, QVariantMap> &codecOptions={},
                    MediaWriterFFmpeg *mediaWriter=nullptr,
                    QObject *parent=nullptr);
        ~AudioStream();

        Q_INVOKABLE static AkAudioCaps::SampleFormat sampleFormat(AVSampleFormat format);
        Q_INVOKABLE static AkAudioCaps::ChannelLayout channelLayout(uint64_t layout);

    private:
        AudioStreamPrivate *d;

    protected:
        void convertPacket(const AkPacket &packet) override;
        int encodeData(AVFrame *frame) override;
        AVFrame *dequeueFrame() override;

    public slots:
        bool init() override;
        void uninit() override;
};

#endif // AUDIOSTREAM_H
