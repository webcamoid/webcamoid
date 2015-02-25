/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <QMutex>
#include <QWaitCondition>

#include <qb.h>

class AudioBuffer: public QIODevice
{
    Q_OBJECT

    public:
        explicit AudioBuffer(QObject *parent = NULL);

        bool atEnd() const;
        qint64 bytesAvailable() const;
        qint64 bytesToWrite() const;
        bool canReadLine() const;
        bool isSequential() const;
        qint64 pos() const;
        bool seek(qint64 pos);
        qint64 size() const;

    private:
        qint64 m_maxSize;
        QByteArray m_audioBuffer;

        QMutex m_mutex;
        QWaitCondition m_bufferNotFull;

    protected:
        qint64 readData(char *data, qint64 maxSize);
        qint64 writeData(const char *data, qint64 maxSize);

    signals:
        void bytesConsumed();
        void dataReady(const QByteArray &data);
};

#endif // AUDIOBUFFER_H
