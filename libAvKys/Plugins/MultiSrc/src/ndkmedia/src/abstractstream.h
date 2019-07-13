/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef ABSTRACTSTREAM_H
#define ABSTRACTSTREAM_H

#include <QObject>

class AbstractStream;
class AbstractStreamPrivate;
class AkFrac;
class AkCaps;
class AkPacket;
class Clock;
struct AMediaExtractor;
struct AMediaFormat;
struct AMediaCodec;
using AbstractStreamPtr = QSharedPointer<AbstractStream>;

class AbstractStream: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool paused
               READ paused
               WRITE setPaused
               RESET resetPaused
               NOTIFY pausedChanged)

    public:
        AbstractStream(AMediaExtractor *mediaExtractor=nullptr,
                       uint index=0, qint64 id=-1,
                       Clock *globalClock=nullptr,
                       QObject *parent=nullptr);
        virtual ~AbstractStream();

        Q_INVOKABLE bool paused() const;
        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE uint index() const;
        Q_INVOKABLE qint64 id() const;
        Q_INVOKABLE AkFrac timeBase() const;
        Q_INVOKABLE QString mimeType() const;
        Q_INVOKABLE AMediaCodec *codec() const;
        Q_INVOKABLE AMediaFormat *mediaFormat() const;
        Q_INVOKABLE virtual AkCaps caps() const;
        Q_INVOKABLE bool packetEnqueue(bool eos=false);
        Q_INVOKABLE void avPacketEnqueue(const AkPacket &packet);
        Q_INVOKABLE qint64 queueSize();
        Q_INVOKABLE Clock *globalClock();
        Q_INVOKABLE qreal clockDiff() const;
        Q_INVOKABLE qreal &clockDiff();
        Q_INVOKABLE virtual bool decodeData();
        Q_INVOKABLE static QString mimeType(AMediaExtractor *mediaExtractor,
                                            uint index);

    protected:
        bool m_paused;
        bool m_isValid;
        qreal m_clockDiff;
        int m_maxData;

        virtual void processPacket(const AkPacket &packet);

    private:
        AbstractStreamPrivate *d;

    signals:
        void pausedChanged(bool paused);
        void oStream(const AkPacket &packet);
        void eof();

    public slots:
        void setPaused(bool paused);
        void resetPaused();
        virtual bool init();
        virtual void uninit();

        friend class AbstractStreamPrivate;
};

#endif // ABSTRACTSTREAM_H
