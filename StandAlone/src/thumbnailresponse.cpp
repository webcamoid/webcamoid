/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QAtomicInt>
#include <QQuickTextureFactory>

#include "thumbnailresponse.h"

class ThumbnailResponsePrivate
{
    public:
        QImage m_image;
        QString m_error;
        QAtomicInt m_canceled {0};
};

ThumbnailResponse::ThumbnailResponse():
    QQuickImageResponse()
{
    this->d = new ThumbnailResponsePrivate;
}

ThumbnailResponse::~ThumbnailResponse()
{
    delete this->d;
}

QQuickTextureFactory *ThumbnailResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(this->d->m_image);
}

QString ThumbnailResponse::errorString() const
{
    return this->d->m_error;
}

void ThumbnailResponse::cancel()
{
    this->d->m_canceled.storeRelaxed(1);
}

bool ThumbnailResponse::isCanceled() const
{
    return this->d->m_canceled.loadRelaxed() != 0;
}

void ThumbnailResponse::finish(const QImage &image, const QString &error)
{
    this->d->m_image = image;
    this->d->m_error = error;
    emit this->finished();
}

#include "moc_thumbnailresponse.cpp"
