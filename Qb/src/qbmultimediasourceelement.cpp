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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "qbmultimediasourceelement.h"

class QbMultimediaSourceElementPrivate
{
    public:
        QStringList m_medias;
        QString m_media;
        QList<int> m_streams;
        bool m_loop;
};

QbMultimediaSourceElement::QbMultimediaSourceElement(QObject *parent):
    QbElement(parent)
{
    this->d = new QbMultimediaSourceElementPrivate();
    this->d->m_loop = false;
}

QbMultimediaSourceElement::~QbMultimediaSourceElement()
{
    delete this->d;
}

QStringList QbMultimediaSourceElement::medias() const
{
    return this->d->m_medias;
}

QString QbMultimediaSourceElement::media() const
{
    return this->d->m_media;
}

QList<int> QbMultimediaSourceElement::streams() const
{
    return this->d->m_streams;
}

bool QbMultimediaSourceElement::loop() const
{
    return this->d->m_loop;
}

int QbMultimediaSourceElement::defaultStream(const QString &mimeType) const
{
    Q_UNUSED(mimeType)

    return -1;
}

QString QbMultimediaSourceElement::description(const QString &media) const
{
    Q_UNUSED(media)

    return QString();
}

QbCaps QbMultimediaSourceElement::caps(int stream) const
{
    Q_UNUSED(stream)

    return QbCaps();
}

bool QbMultimediaSourceElement::isCompressed(int stream) const
{
    Q_UNUSED(stream)

    return false;
}

void QbMultimediaSourceElement::setMedia(const QString &media)
{
    if (this->d->m_media == media)
        return;

    this->d->m_media = media;
    emit this->mediaChanged(media);
}

void QbMultimediaSourceElement::setStreams(const QList<int> &streams)
{
    if (this->d->m_streams == streams)
        return;

    this->d->m_streams = streams;
    emit this->streamsChanged(streams);
}

void QbMultimediaSourceElement::setLoop(bool loop)
{
    if (this->d->m_loop == loop)
        return;

    this->d->m_loop = loop;
    emit this->loopChanged(loop);
}

void QbMultimediaSourceElement::resetMedia()
{
    this->setMedia("");
}

void QbMultimediaSourceElement::resetStreams()
{
    this->setStreams(QList<int>());
}

void QbMultimediaSourceElement::resetLoop()
{
    this->setLoop(false);
}
