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

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QtCore>
#include <qbpacket.h>

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libavutil/imgutils.h>
}

class AbstractStream;

typedef QSharedPointer<AVFormatContext> FormatContextPtr;
typedef QSharedPointer<AbstractStream> AbstractStreamPtr;

class AbstractStream: public QObject
{
    Q_OBJECT

    public:
        explicit AbstractStream(QObject *parent=NULL);
        AbstractStream(const FormatContextPtr &formatContext, uint index);
        virtual ~AbstractStream();

        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE uint index() const;
        Q_INVOKABLE QbFrac timeBase() const;
        Q_INVOKABLE AVMediaType mediaType() const;
        Q_INVOKABLE FormatContextPtr formatContext() const;
        Q_INVOKABLE AVStream *stream() const;
        Q_INVOKABLE AVCodecContext *codecContext() const;
        Q_INVOKABLE AVCodec *codec() const;
        Q_INVOKABLE AVDictionary *codecOptions() const;
        Q_INVOKABLE virtual QbCaps caps() const;
        Q_INVOKABLE virtual QList<QbPacket> readPackets(AVPacket *packet);

        static AVMediaType type(const FormatContextPtr &formatContext, uint index);

    protected:
        bool m_isValid;

        virtual void cleanUp();

    private:
        uint m_index;
        QbFrac m_timeBase;
        AVMediaType m_mediaType;
        FormatContextPtr m_formatContext;
        AVStream *m_stream;
        AVCodecContext *m_codecContext;
        AVCodec *m_codec;
        AVDictionary *m_codecOptions;
};

#endif // ABSTRACTSTREAM_H
