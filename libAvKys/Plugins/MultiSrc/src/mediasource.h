/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <akelement.h>

class AkCaps;

class MediaSource: public QObject
{
    Q_OBJECT

    public:
        explicit MediaSource(QObject *parent=nullptr);
        virtual ~MediaSource();

        Q_INVOKABLE virtual QStringList medias() const;
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams() const;
        Q_INVOKABLE virtual QList<int> listTracks(const QString &mimeType);
        Q_INVOKABLE virtual QString streamLanguage(int stream);
        Q_INVOKABLE virtual bool loop() const;

        Q_INVOKABLE virtual int defaultStream(const QString &mimeType);
        Q_INVOKABLE virtual QString description(const QString &media) const;
        Q_INVOKABLE virtual AkCaps caps(int stream);
        Q_INVOKABLE virtual qint64 maxPacketQueueSize() const;
        Q_INVOKABLE virtual bool showLog() const;

    public slots:
        virtual void setMedia(const QString &media);
        virtual void setStreams(const QList<int> &streams);
        virtual void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        virtual void setShowLog(bool showLog);
        virtual void setLoop(bool loop);
        virtual void resetMedia();
        virtual void resetStreams();
        virtual void resetMaxPacketQueueSize();
        virtual void resetShowLog();
        virtual void resetLoop();
        virtual bool setState(AkElement::ElementState state);
};

#endif // MEDIASOURCE_H
