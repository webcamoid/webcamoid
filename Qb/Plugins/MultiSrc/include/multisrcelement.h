/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QApplication>
#include <QDesktopWidget>
#include <QtConcurrent>

#include <qbmultimediasourceelement.h>

#include "abstractstream.h"

typedef QSharedPointer<AVFormatContext> FormatContextPtr;
typedef QSharedPointer<AbstractStream> AbstractStreamPtr;

class MultiSrcElement: public QbMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop)
    Q_PROPERTY(QVariantMap streamCaps
               READ streamCaps)
    Q_PROPERTY(QList<int> filterStreams
               READ filterStreams
               WRITE setFilterStreams
               RESET resetFilterStreams)
    Q_PROPERTY(bool audioAlign
               READ audioAlign
               WRITE setAudioAlign
               RESET resetAudioAlign)
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize)

    public:
        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE QVariantMap streamCaps();
        Q_INVOKABLE QList<int> filterStreams() const;
        Q_INVOKABLE bool audioAlign() const;
        Q_INVOKABLE qint64 maxPacketQueueSize() const;
        Q_INVOKABLE int defaultStream(const QString &mimeType);

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        QString m_location;
        bool m_loop;
        QList<int> m_filterStreams;
        bool m_audioAlign;

        Thread *m_decodingThread;
        bool m_run;

        FormatContextPtr m_inputContext;

        qint64 m_maxPacketQueueSize;
        QMutex m_dataMutex;
        QWaitCondition m_packetQueueNotFull;
        QWaitCondition m_packetQueueEmpty;

        QMap<int, AbstractStreamPtr> m_streams;

        QMap<AVMediaType, QString> m_avMediaTypeToMimeType;

        qint64 packetQueueSize();
        static void deleteFormatContext(AVFormatContext *context);
        AbstractStreamPtr createStream(int index, bool noModify=false);

        inline int roundDown(int value, int multiply)
        {
            return value - value % multiply;
        }

    signals:
        void error(const QString &message);
        void queueSizeUpdated(const QMap<int, qint64> &queueSize);

    public slots:
        void setLocation(const QString &location);
        void setLoop(bool loop);
        void setFilterStreams(const QList<int> &filterStreams);
        void setAudioAlign(bool audioAlign);
        void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        void resetLocation();
        void resetLoop();
        void resetFilterStreams();
        void resetAudioAlign();
        void resetMaxPacketQueueSize();

        void setState(QbElement::ElementState state);

    private slots:
        void doLoop();
        void pullData();
        void packetConsumed();
        void unlockQueue();
        bool init();
        bool initContext();
        void uninit();
};

#endif // MULTISRCELEMENT_H
