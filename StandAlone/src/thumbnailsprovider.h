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

#ifndef THUMBNAILSPROVIDER_H
#define THUMBNAILSPROVIDER_H

#include <QQuickImageProvider>

class ThumbnailsProviderPrivate;

/* Image provider for the video thumbnails of the media gallery.
 *
 * Usage: image://thumbnails/<hex>
 *
 * Where <hex> is the media URL as an UTF-8 string encoded in hexadecimal.
 * The thumbnails are cached in disk, so each video is decoded only once.
 */
class ThumbnailsProvider: public QQuickAsyncImageProvider
{
    public:
        ThumbnailsProvider();
        ~ThumbnailsProvider();

        QQuickImageResponse *requestImageResponse(const QString &id,
                                                  const QSize &requestedSize) override;

    private:
        ThumbnailsProviderPrivate *d;
};

#endif // THUMBNAILSPROVIDER_H
