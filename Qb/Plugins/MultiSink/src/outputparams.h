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
    #include <libavutil/pixdesc.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
}

#include "customdeleters.h"

typedef QSharedPointer<AVCodecContext> CodecContextPtr;

class OutputParams: public QObject
{
    Q_OBJECT
    Q_PROPERTY(CodecContextPtr codecContext
               READ codecContext
               WRITE setCodecContext
               RESET resetCodecContext
               NOTIFY codecContextChanged)
    Q_PROPERTY(int outputIndex
               READ outputIndex
               WRITE setOutputIndex
               RESET resetOutputIndex
               NOTIFY outputIndexChanged)

    public:
        explicit OutputParams(QObject *parent=NULL);
        OutputParams(const CodecContextPtr &codecContext,
                     int outputIndex);
        OutputParams(const OutputParams &other);
        ~OutputParams();

        OutputParams &operator =(const OutputParams &other);

        Q_INVOKABLE CodecContextPtr codecContext() const;
        Q_INVOKABLE CodecContextPtr &codecContext();
        Q_INVOKABLE int outputIndex() const;
        Q_INVOKABLE int &outputIndex();
        Q_INVOKABLE qint64 nextPts(qint64 pts, qint64 id);

    private:
        CodecContextPtr m_codecContext;
        int m_outputIndex;

        qint64 m_id;
        qint64 m_pts;
        qint64 m_ptsDiff;
        qint64 m_ptsDrift;

        SwrContext *m_resampleContext;
        SwsContext *m_scaleContext;

    signals:
        void codecContextChanged(const CodecContextPtr &codecContext);
        void outputIndexChanged(int outputIndex);
        void ptsChanged(qint64 pts);

    public slots:
        void setCodecContext(const CodecContextPtr &codecContext);
        void setOutputIndex(int outputIndex);
        void resetCodecContext();
        void resetOutputIndex();

        bool convert(const QbPacket &packet, AVFrame *frame);

    private slots:
        bool convertAudio(const QbPacket &packet, AVFrame *frame);
        bool convertVideo(const QbPacket &packet, AVFrame *frame);
};

Q_DECLARE_METATYPE(OutputParams)

#endif // OUTPUTPARAMS_H
