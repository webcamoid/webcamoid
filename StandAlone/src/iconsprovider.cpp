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

#include <limits>
#include <algorithm>
#include <QIcon>
#include <QtMath>

#include "iconsprovider.h"

bool operator <(const QSize &a, const QSize &b)
{
    return a.width() * a.width() + a.height() * a.height()
         < b.width() * b.width() + b.height() * b.height();
}

IconsProvider::IconsProvider():
    QQuickImageProvider(QQuickImageProvider::Pixmap)
{

}

QPixmap IconsProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (!QIcon::hasThemeIcon(id)) {
        *size = QSize();

        return QPixmap();
    }

    QIcon icon = QIcon::fromTheme(id);
    QList<QSize> availableSizes = icon.availableSizes();
    QSize nearestSize;

    if (requestedSize.isEmpty()) {
        nearestSize = *std::max_element(availableSizes.begin(), availableSizes.end());
    } else {
        int q = std::numeric_limits<int>::max();

        for  (QSize &size: availableSizes) {
            int diffWidth = size.width() - requestedSize.width();
            int diffHeight = size.height() - requestedSize.height();
            int k = diffWidth * diffWidth + diffHeight * diffHeight;

            if (k < q) {
                nearestSize = size;
                q = k;
            }
        }
    }

    QPixmap pixmap = icon.pixmap(nearestSize);
    *size = pixmap.size();

    return pixmap;
}
