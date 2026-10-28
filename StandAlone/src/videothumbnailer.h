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

#ifndef VIDEOTHUMBNAILER_H
#define VIDEOTHUMBNAILER_H

#include <QObject>
#include <QPointer>

class VideoThumbnailerPrivate;
class ThumbnailResponse;
class QUrl;

/* Generates the thumbnail of one video at a time, so the number of
 * simultaneous decoders is always one no matter how many videos the gallery
 * has. It must live in the main thread.
 */
class VideoThumbnailer: public QObject
{
    Q_OBJECT

    public:
        explicit VideoThumbnailer(QObject *parent=nullptr);
        ~VideoThumbnailer();

        void enqueue(const QPointer<ThumbnailResponse> &response,
                     const QUrl &source,
                     const QString &cachePath);

    private:
        VideoThumbnailerPrivate *d;
};

#endif // VIDEOTHUMBNAILER_H
