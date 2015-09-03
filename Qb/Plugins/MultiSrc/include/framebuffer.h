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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QSharedPointer>

extern "C"
{
    #include <libavcodec/avcodec.h>
}

typedef QSharedPointer<AVFrame> AVFramePtr;

class FrameBuffer: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxSize
               READ maxSize
               WRITE setMaxSize
               RESET resetMaxSize
               NOTIFY maxSizeChanged)
    Q_PROPERTY(int size
               READ size
               NOTIFY sizeChanged)

    public:
        explicit FrameBuffer(QObject *parent=NULL);

        Q_INVOKABLE int maxSize() const;
        Q_INVOKABLE int &maxSize();
        Q_INVOKABLE int size();
        Q_INVOKABLE AVFramePtr dequeue();

    private:
        int m_maxSize;
        QQueue<AVFramePtr> m_buffer;
        QMutex m_mutex;
        QWaitCondition m_bufferNotFull;
        QWaitCondition m_bufferNotEmpty;

    signals:
        void maxSizeChanged(int maxSize);
        void sizeChanged(int size);

    public slots:
        void setMaxSize(int maxSize);
        void resetMaxSize();
        void enqueue(AVFrame *frame);
        void clear();
};

#endif // FRAMEBUFFER_H
