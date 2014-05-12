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

#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <QtCore>

class AudioBuffer: public QIODevice
{
    Q_OBJECT

    public:
        explicit AudioBuffer(QObject *parent = NULL);

        Q_INVOKABLE qint64 maxSize() const;
        bool atEnd() const;
        qint64 bytesAvailable() const;
        qint64 bytesToWrite() const;
        bool canReadLine() const;
        void close();
        bool isSequential() const;
        bool open(OpenMode mode);
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
        void cleared();

    public slots:
        void setMaxSize(qint64 maxSize);
        void resetMaxSize();
};

#endif // AUDIOBUFFER_H
