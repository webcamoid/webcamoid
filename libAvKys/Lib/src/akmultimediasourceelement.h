/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include "akcaps.h"
#include "akelement.h"

class AkMultimediaSourceElement;
class AkMultimediaSourceElementPrivate;
class AkCaps;

using AkMultimediaSourceElementPtr = QSharedPointer<AkMultimediaSourceElement>;

class AKCOMMONS_EXPORT AkMultimediaSourceElement: public AkElement
{
    Q_OBJECT

    public:
        AkMultimediaSourceElement(QObject *parent=nullptr);
        ~AkMultimediaSourceElement();

        Q_INVOKABLE virtual QStringList medias();
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams();
        Q_INVOKABLE virtual bool loop() const;
        Q_INVOKABLE virtual int defaultStream(AkCaps::CapsType type);
        Q_INVOKABLE virtual QString description(const QString &media);
        Q_INVOKABLE virtual AkCaps caps(int stream);

    private:
        AkMultimediaSourceElementPrivate *d;

    public Q_SLOTS:
        virtual void setMedia(const QString &media);
        virtual void setStreams(const QList<int> &streams);
        virtual void setLoop(bool loop);
        virtual void resetMedia();
        virtual void resetStreams();
        virtual void resetLoop();
};

#endif // AKMULTIMEDIASOURCEELEMENT_H
