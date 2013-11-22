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

#ifndef AVQUEUE_H
#define AVQUEUE_H

#include <qb.h>

class AVQueue: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int size READ size)
    Q_PROPERTY(int maxSize READ maxSize WRITE setMaxSize RESET resetMaxSize)
    Q_PROPERTY(bool useCache READ useCache WRITE setUseCache RESET resetUseCache)

    public:
        explicit AVQueue(QObject *parent=NULL);
        ~AVQueue();

        Q_INVOKABLE int size(const QString &mimeType="");
        Q_INVOKABLE int maxSize() const;
        Q_INVOKABLE bool useCache() const;
        Q_INVOKABLE QbPacket read(QString mimeType);
        Q_INVOKABLE QbPacket dequeue(QString mimeType);

    private:
        bool m_log;

        int m_maxSize;
        bool m_useCache;

        QQueue<QbPacket> m_audioQueue;
        QQueue<QbPacket> m_videoQueue;

        bool m_fill;

        QMutex m_queueMutex;
        QMutex m_iMutex;
        QMutex m_aoMutex;
        QMutex m_voMutex;

        QWaitCondition m_queueNotFull;
        QWaitCondition m_audioQueueNotEmpty;
        QWaitCondition m_videoQueueNotEmpty;

        QbPacket dequeueAudio();
        QbPacket dequeueVideo();

    public slots:
        void enqueue(const QbPacket &packet);
        void setMaxSize(int maxSize);
        void setUseCache(bool useCache);
        void resetMaxSize();
        void resetUseCache();
};

#endif // AVQUEUE_H
