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

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QApplication>
#include <QDesktopWidget>

#include <qbmultimediasourceelement.h>

#include "abstractstream.h"

typedef QSharedPointer<AVFormatContext> FormatContextPtr;
typedef QSharedPointer<AbstractStream> AbstractStreamPtr;

class MultiSrcElement: public QbMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize
               NOTIFY maxPacketQueueSizeChanged)
    Q_PROPERTY(bool showLog
               READ showLog
               WRITE setShowLog
               RESET resetShowLog
               NOTIFY showLogChanged)

    public:
        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QStringList medias() const;
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;

        using QbMultimediaSourceElement::defaultStream;
        using QbMultimediaSourceElement::caps;

        Q_INVOKABLE int defaultStream(const QString &mimeType);
        Q_INVOKABLE QString description(const QString &media) const;
        Q_INVOKABLE QbCaps caps(int stream);
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE bool showLog() const;

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        QString m_media;
        QList<int> m_streams;
        bool m_run;

        FormatContextPtr m_inputContext;
        qint64 m_maxPacketQueueSize;
        bool m_showLog;
        QThreadPool m_threadPool;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_packetQueueEmpty;
        QMap<int, AbstractStreamPtr> m_streamsMap;
        Clock m_globalClock;

        qint64 packetQueueSize();
        static void deleteFormatContext(AVFormatContext *context);
        AbstractStreamPtr createStream(int index, bool noModify=false);
        static void readPackets(MultiSrcElement *element);
        static void unlockQueue(MultiSrcElement *element);

        inline int roundDown(int value, int multiply)
        {
            return value - value % multiply;
        }

    signals:
        void error(const QString &message);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueue);
        void showLogChanged(bool showLog);

    public slots:
        void setMedia(const QString &media);
        void setStreams(const QList<int> &streams);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void setShowLog(bool showLog);
        void resetMedia();
        void resetStreams();
        void resetMaxPacketQueueSize();
        void resetShowLog();

    private slots:
        void doLoop();
        void packetConsumed();
        bool init();
        bool initContext();
        void uninit();
        void log();
};

#endif // MULTISRCELEMENT_H
