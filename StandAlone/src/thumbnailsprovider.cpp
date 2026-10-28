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

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPointer>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUrl>
#include <QtConcurrent>

#include "thumbnailsprovider.h"
#include "thumbnailresponse.h"
#include "videothumbnailer.h"

class ThumbnailsProviderPrivate
{
    public:
        VideoThumbnailer *m_thumbnailer {nullptr};
        QThreadPool m_cachePool;
        QString m_cacheDir;
};

ThumbnailsProvider::ThumbnailsProvider():
    QQuickAsyncImageProvider()
{
    this->d = new ThumbnailsProviderPrivate;
    this->d->m_thumbnailer = new VideoThumbnailer;
    this->d->m_cachePool.setMaxThreadCount(2);

    auto cachePath =
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    if (!cachePath.isEmpty())
        this->d->m_cacheDir = QDir(cachePath).filePath("thumbnails");
}

ThumbnailsProvider::~ThumbnailsProvider()
{
    this->d->m_cachePool.waitForDone();
    delete this->d->m_thumbnailer;
    delete this->d;
}

QQuickImageResponse *ThumbnailsProvider::requestImageResponse(const QString &id,
                                                              const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    auto response = new ThumbnailResponse;
    QPointer<ThumbnailResponse> responsePtr(response);
    auto source =
            QUrl(QString::fromUtf8(QByteArray::fromHex(id.toLatin1())));
    auto thumbnailer = this->d->m_thumbnailer;
    auto cacheDir = this->d->m_cacheDir;

    // Nobody touches the response from other threads directly, the calls are
    // always queued to the thread that owns it.
    auto finish = [responsePtr] (const QImage &image, const QString &error) {
        if (!responsePtr)
            return;

        QMetaObject::invokeMethod(responsePtr.data(),
                                  [responsePtr, image, error] () {
            if (responsePtr)
                responsePtr->finish(image, error);
        }, Qt::QueuedConnection);
    };

    // Check the disk cache out of the main thread, and send only the cache
    // misses to the thumbnailer.
    auto result =
        QtConcurrent::run(&this->d->m_cachePool,
                          [responsePtr, source, cacheDir, thumbnailer, finish] () {
        if (!responsePtr || responsePtr->isCanceled())
            return;

        QString cachePath;

        if (source.isLocalFile()) {
            QFileInfo info(source.toLocalFile());

            if (!info.exists()) {
                finish({}, "File not found");

                return;
            }

            if (!cacheDir.isEmpty()) {
                QCryptographicHash hash(QCryptographicHash::Sha1);
                hash.addData(info.absoluteFilePath().toUtf8());
                hash.addData("|");
                hash.addData(QByteArray::number(info.size()));
                hash.addData("|");
                hash.addData(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
                cachePath =
                        QDir(cacheDir).filePath(QString::fromLatin1(hash.result().toHex())
                                                + ".png");
                QImage cached(cachePath);

                if (!cached.isNull()) {
                    finish(cached, {});

                    return;
                }
            }
        }

        QMetaObject::invokeMethod(thumbnailer,
                                  [thumbnailer, responsePtr, source, cachePath] () {
            thumbnailer->enqueue(responsePtr, source, cachePath);
        }, Qt::QueuedConnection);
    });
    Q_UNUSED(result)

    return response;
}
