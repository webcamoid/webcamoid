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

#include "akmultimediasourceelement.h"

class AkMultimediaSourceElementPrivate
{
    public:
        QStringList m_medias;
        QString m_media;
        QList<int> m_streams;
        bool m_loop;
};

AkMultimediaSourceElement::AkMultimediaSourceElement(QObject *parent):
    AkElement(parent)
{
    this->d = new AkMultimediaSourceElementPrivate();
    this->d->m_loop = false;
}

AkMultimediaSourceElement::~AkMultimediaSourceElement()
{
    delete this->d;
}

QStringList AkMultimediaSourceElement::medias() const
{
    return this->d->m_medias;
}

QString AkMultimediaSourceElement::media() const
{
    return this->d->m_media;
}

QList<int> AkMultimediaSourceElement::streams() const
{
    return this->d->m_streams;
}

bool AkMultimediaSourceElement::loop() const
{
    return this->d->m_loop;
}

int AkMultimediaSourceElement::defaultStream(const QString &mimeType) const
{
    Q_UNUSED(mimeType)

    return -1;
}

QString AkMultimediaSourceElement::description(const QString &media) const
{
    Q_UNUSED(media)

    return QString();
}

AkCaps AkMultimediaSourceElement::caps(int stream) const
{
    Q_UNUSED(stream)

    return AkCaps();
}

void AkMultimediaSourceElement::setMedia(const QString &media)
{
    if (this->d->m_media == media)
        return;

    this->d->m_media = media;
    emit this->mediaChanged(media);
}

void AkMultimediaSourceElement::setStreams(const QList<int> &streams)
{
    if (this->d->m_streams == streams)
        return;

    this->d->m_streams = streams;
    emit this->streamsChanged(streams);
}

void AkMultimediaSourceElement::setLoop(bool loop)
{
    if (this->d->m_loop == loop)
        return;

    this->d->m_loop = loop;
    emit this->loopChanged(loop);
}

void AkMultimediaSourceElement::resetMedia()
{
    this->setMedia("");
}

void AkMultimediaSourceElement::resetStreams()
{
    this->setStreams(QList<int>());
}

void AkMultimediaSourceElement::resetLoop()
{
    this->setLoop(false);
}
