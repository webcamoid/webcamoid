/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef AKMULTIMEDIASOURCEELEMENT_H
#define AKMULTIMEDIASOURCEELEMENT_H

#include "akelement.h"

class AkMultimediaSourceElement;
class AkMultimediaSourceElementPrivate;

typedef QSharedPointer<AkMultimediaSourceElement> AkMultimediaSourceElementPtr;

class AKCOMMONS_EXPORT AkMultimediaSourceElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop
               NOTIFY loopChanged)

    public:
        AkMultimediaSourceElement(QObject *parent=NULL);
        ~AkMultimediaSourceElement();

        Q_INVOKABLE virtual QStringList medias() const;
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams() const;
        Q_INVOKABLE virtual bool loop() const;

        Q_INVOKABLE virtual int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE virtual QString description(const QString &media) const;
        Q_INVOKABLE virtual AkCaps caps(int stream) const;

    private:
        AkMultimediaSourceElementPrivate *d;

    Q_SIGNALS:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);

    public Q_SLOTS:
        virtual void setMedia(const QString &media);
        virtual void setStreams(const QList<int> &streams);
        virtual void setLoop(bool loop);
        virtual void resetMedia();
        virtual void resetStreams();
        virtual void resetLoop();
};

#endif // AKMULTIMEDIASOURCEELEMENT_H
