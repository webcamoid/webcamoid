/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef OUTPUTPARAMS_H
#define OUTPUTPARAMS_H

#include <qb.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/channel_layout.h>
}

class OutputParams: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int inputIndex
               READ inputIndex
               WRITE setInputIndex
               RESET resetInputIndex
               NOTIFY inputIndexChanged)

    public:
        explicit OutputParams(int inputIndex=0, QObject *parent=NULL);
        OutputParams(const OutputParams &other);
        ~OutputParams();

        OutputParams &operator =(const OutputParams &other);

        Q_INVOKABLE int inputIndex() const;
        Q_INVOKABLE int &inputIndex();
        Q_INVOKABLE qint64 nextPts(qint64 pts, qint64 id);

        Q_INVOKABLE void addAudioSamples(const AVFrame *frame, qint64 id);
        Q_INVOKABLE int readAudioSamples(int samples, uint8_t **buffer);
        Q_INVOKABLE qint64 audioPts() const;

    private:
        int m_inputIndex;

        QByteArray m_audioBuffer;
        AVSampleFormat m_audioFormat;
        int m_audioChannels;

        qint64 m_id;
        qint64 m_pts;
        qint64 m_ptsDiff;
        qint64 m_ptsDrift;

        SwrContext *m_resampleContext;
        SwsContext *m_scaleContext;

    signals:
        void inputIndexChanged(int inputIndex);

    public slots:
        void setInputIndex(int inputIndex);
        void resetInputIndex();

        bool convert(const QbPacket &packet, AVFrame *frame);
        bool convert(const QbAudioPacket &packet, AVFrame *frame);
        bool convert(const QbVideoPacket &packet, AVFrame *frame);
};

Q_DECLARE_METATYPE(OutputParams)

#endif // OUTPUTPARAMS_H
