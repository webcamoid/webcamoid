/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QtGui>
#include <qbelement.h>

extern "C"
{
    #include <libavdevice/avdevice.h>
}

#include "abstractstream.h"

class MultiSrcElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(StreamType)
    Q_PROPERTY(QString location READ location WRITE setLocation RESET resetLocation)
    Q_PROPERTY(bool loop READ loop WRITE setLoop RESET resetLoop)
    Q_PROPERTY(QSize size READ size WRITE setSize RESET resetSize)
    Q_PROPERTY(int streamsCount READ streamsCount)
    Q_PROPERTY(QList<QSize> availableSize READ availableSize)

    public:
        enum StreamType
        {
            StreamTypeUnknown,
            StreamTypeVideo,
            StreamTypeAudio,
            StreamTypeData,
            StreamTypeSubtitle,
            StreamTypeAttachment,
            StreamTypeNb
        };

        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QString location();
        Q_INVOKABLE bool loop();
        Q_INVOKABLE QSize size();
        Q_INVOKABLE int streamsCount();
        Q_INVOKABLE StreamType streamType(int streamIndex);
        Q_INVOKABLE int defaultIndex(StreamType streamType);
        Q_INVOKABLE QList<QSize> availableSize();

    protected:
        bool init();
        void uninit();

    private:
        QString m_location;
        bool m_loop;
        QSize m_size;
        int m_streamsCount;

        int m_audioStream;
        int m_videoStream;
        AVFormatContext *m_inputContext;
        AVPacket m_packet;
        QTimer m_timer;
        QMap<int, QSharedPointer<AbstractStream> > m_streams;

        QSize webcamSize();
        QList<QSize> webcamAvailableSize();

        inline int roundDown(int value, int multiply)
        {
            return value - value % multiply;
        }

    public slots:
        void setLocation(QString location);
        void setLoop(bool loop);
        void setSize(QSize size);
        void resetLocation();
        void resetLoop();
        void resetSize();

        void setState(ElementState state);

    private slots:
        void readPackets();
};

#endif // MULTISRCELEMENT_H
