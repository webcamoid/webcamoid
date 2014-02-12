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
    Q_PROPERTY(qint64 size READ size)
    Q_PROPERTY(qint64 maxSize READ maxSize WRITE setMaxSize RESET resetMaxSize)

    public:
        explicit AVQueue(QObject *parent=NULL);
        ~AVQueue();

        Q_INVOKABLE qint64 size(const QString &mimeType="", bool lock=true);
        Q_INVOKABLE qint64 maxSize() const;
        Q_INVOKABLE QbPacket read(QString mimeType);
        Q_INVOKABLE QbPacket dequeue(QString mimeType);

    private:
        qint64 m_maxSize;

        QQueue<QbPacket> m_avQueue;
        QMap<QString, qint64> m_queueSize;

        QMutex m_mutex;
        QWaitCondition m_queueNotFull;

    public slots:
        void clear();
        void enqueue(const QbPacket &packet);
        void setMaxSize(qint64 maxSize);
        void resetMaxSize();
};

#endif // AVQUEUE_H
