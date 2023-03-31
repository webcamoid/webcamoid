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

#include "akmultimediasourceelement.h"

class AkMultimediaSourceElementPrivate
{
    public:
        QStringList m_medias;
        QString m_media;
        QList<int> m_streams;
        bool m_loop {false};
};

AkMultimediaSourceElement::AkMultimediaSourceElement(QObject *parent):
    AkElement(parent)
{
    this->d = new AkMultimediaSourceElementPrivate();
}

AkMultimediaSourceElement::~AkMultimediaSourceElement()
{
    delete this->d;
}

QStringList AkMultimediaSourceElement::medias()
{
    return this->d->m_medias;
}

QString AkMultimediaSourceElement::media() const
{
    return this->d->m_media;
}

QList<int> AkMultimediaSourceElement::streams()
{
    return this->d->m_streams;
}

bool AkMultimediaSourceElement::loop() const
{
    return this->d->m_loop;
}

int AkMultimediaSourceElement::defaultStream(AkCaps::CapsType type)
{
    Q_UNUSED(type)

    return -1;
}

QString AkMultimediaSourceElement::description(const QString &media)
{
    Q_UNUSED(media)

    return {};
}

AkCaps AkMultimediaSourceElement::caps(int stream)
{
    Q_UNUSED(stream)

    return {};
}

void AkMultimediaSourceElement::setMedia(const QString &media)
{
    this->d->m_media = media;
}

void AkMultimediaSourceElement::setStreams(const QList<int> &streams)
{
    this->d->m_streams = streams;
}

void AkMultimediaSourceElement::setLoop(bool loop)
{
    this->d->m_loop = loop;
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

#include "moc_akmultimediasourceelement.cpp"
