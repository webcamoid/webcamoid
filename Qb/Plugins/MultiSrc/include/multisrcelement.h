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

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QtGui>
#include <qb.h>

#include "abstractstream.h"

typedef QMap<int, QThread *> IntThreadPtrMap;

class MultiSrcElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString location READ location WRITE setLocation RESET resetLocation)
    Q_PROPERTY(bool loop READ loop WRITE setLoop RESET resetLoop)
    Q_PROPERTY(QVariantMap streamCaps READ streamCaps)

    Q_PROPERTY(QList<int> filterStreams READ filterStreams
                                        WRITE setFilterStreams
                                        RESET resetFilterStreams)

    Q_PROPERTY(bool audioAlign READ audioAlign
                               WRITE setAudioAlign
                               RESET resetAudioAlign)

    Q_PROPERTY(IntThreadPtrMap outputThreads READ outputThreads
                                             WRITE setOutputThreads
                                             RESET resetOutputThreads)

    Q_PROPERTY(QList<int> asPull READ asPull
                                 WRITE setAsPull
                                 RESET resetAsPull)

    public:
        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE QVariantMap streamCaps();
        Q_INVOKABLE QList<int> filterStreams() const;
        Q_INVOKABLE bool audioAlign() const;
        Q_INVOKABLE IntThreadPtrMap outputThreads() const;
        Q_INVOKABLE QList<int> asPull() const;
        Q_INVOKABLE int defaultStream(const QString &mimeType);

    protected:
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        QString m_location;
        bool m_loop;
        QList<int> m_filterStreams;
        bool m_audioAlign;
        IntThreadPtrMap m_outputThreads;
        QList<int> m_asPull;

        QTimer m_decodingTimer;
        ThreadPtr m_decodingThread;

        FormatContextPtr m_inputContext;

        qint64 m_maxPacketQueueSize;
        QMutex m_dataMutex;
        QWaitCondition m_decodingFinished;
        QWaitCondition m_packetQueueNotFull;
        bool m_runInit;
        bool m_runPostClean;
        bool m_runDecoding;
        bool m_userAction;

        QMap<int, AbstractStreamPtr> m_streams;

        QMap<AVMediaType, QString> m_avMediaTypeToMimeType;

        qint64 packetQueueSize();
        static void deleteThread(QThread *thread);
        static void deleteFormatContext(AVFormatContext *context);
        AbstractStreamPtr createStream(int index);

        inline int roundDown(int value, int multiply)
        {
            return value - value % multiply;
        }

    signals:
        void error(QString message);
        void queueSizeUpdated(const QMap<int, qint64> &queueSize);

    public slots:
        void setLocation(const QString &location);
        void setLoop(bool loop);
        void setFilterStreams(const QList<int> &filterStreams);
        void setAudioAlign(bool audioAlign);
        void setOutputThreads(const IntThreadPtrMap &outputThreads);
        void setAsPull(const QList<int> &asPull);
        void resetLocation();
        void resetLoop();
        void resetFilterStreams();
        void resetAudioAlign();
        void resetOutputThreads();
        void resetAsPull();
        void pullFrame(int stream);

        void setState(QbElement::ElementState state);

    private slots:
        void pullData();
        void packetConsumed();
        void unlockQueue();
        bool init();
        bool initContext();
        void uninit();
        void threadExited(uint index);
};

#endif // MULTISRCELEMENT_H
