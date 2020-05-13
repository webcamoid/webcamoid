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

#include <limits>
#include <QApplication>
#include <QIcon>
#include <QSettings>

#include "iconsprovider.h"

class IconsProviderPrivate
{
    public:
        QList<QSize> m_availableSizes;
        QString m_iconsPath {":/Webcamoid/share/themes/WebcamoidTheme/icons"};
        QString m_themeName {"hicolor"};

        QSize nearestSize(const QSize &requestedSize) const;
        QSize nearestSize(const QList<QSize> &availableSizes,
                          const QSize &requestedSize) const;
};

inline bool operator <(const QSize &a, const QSize &b)
{
    return a.width() * a.width() + a.height() * a.height()
         < b.width() * b.width() + b.height() * b.height();
}

IconsProvider::IconsProvider():
    QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    this->d = new IconsProviderPrivate;
    QSettings theme(this->d->m_iconsPath
                    + "/"
                    + this->d->m_themeName
                    + "/index.theme",
                    QSettings::IniFormat);
    theme.beginGroup("Icon Theme");

    for (auto &size: theme.value("Directories").toStringList()) {
        auto dims = size.split('x');

        if (dims.size() < 2)
            continue;

        auto width = dims.value(0).toInt();
        auto height = dims.value(1).toInt();

        if (width < 1 || height < 1)
            continue;

        this->d->m_availableSizes << QSize(width, height);
    }

    theme.endGroup();

    this->themeSetup();
}

IconsProvider::~IconsProvider()
{
    delete this->d;
}

QImage IconsProvider::requestImage(const QString &id,
                                   QSize *size,
                                   const QSize &requestedSize)
{
    auto iconSize = this->d->nearestSize(requestedSize);
    *size = iconSize;

    if (iconSize.isEmpty())
        return QImage();

    QImage icon(QString("%1/%2/%3x%4/%5.png")
                .arg(this->d->m_iconsPath)
                .arg(this->d->m_themeName)
                .arg(iconSize.width())
                .arg(iconSize.height())
                .arg(id));

    return icon;
}

QPixmap IconsProvider::requestPixmap(const QString &id,
                                     QSize *size,
                                     const QSize &requestedSize)
{
    auto iconSize = this->d->nearestSize(requestedSize);
    *size = iconSize;

    if (iconSize.isEmpty())
        return QPixmap();

    QPixmap icon(QString("%1/%2/%3x%4/%5.png")
                 .arg(this->d->m_iconsPath)
                 .arg(this->d->m_themeName)
                 .arg(iconSize.width())
                 .arg(iconSize.height())
                 .arg(id));

    return icon;
}

void IconsProvider::themeSetup()
{
    auto paths = QIcon::fallbackSearchPaths();

    if (!paths.contains(this->d->m_iconsPath))
        QIcon::setFallbackSearchPaths(paths +
                                      QStringList {this->d->m_iconsPath});

#ifdef Q_OS_OSX
    QIcon fallbackIcon(QString("%1/%2.icns")
                       .arg(this->d->m_iconsPath,
                            COMMONS_TARGET));
#elif defined(Q_OS_WIN32)
    QIcon fallbackIcon(QString("%1/%2/256x256/%3.ico")
                       .arg(this->d->m_iconsPath,
                            this->d->m_themeName,
                            COMMONS_TARGET));
#else
    QIcon fallbackIcon(QString("%1/%2/scalable/%3.svg")
                       .arg(this->d->m_iconsPath,
                            this->d->m_themeName,
                            COMMONS_TARGET));
#endif

    QApplication::setWindowIcon(QIcon::fromTheme("webcamoid", fallbackIcon));
}

QSize IconsProviderPrivate::nearestSize(const QSize &requestedSize) const
{
    return this->nearestSize(this->m_availableSizes, requestedSize);
}

QSize IconsProviderPrivate::nearestSize(const QList<QSize> &availableSizes,
                                        const QSize &requestedSize) const
{
    QSize nearestSize;
    QSize nearestGreaterSize;
    int r = std::numeric_limits<int>::max();
    int s = std::numeric_limits<int>::max();
    int requestedArea = requestedSize.width() * requestedSize.height();

    for (auto &size: availableSizes) {
        int area = size.width() * size.height();
        int diffWidth = size.width() - requestedSize.width();
        int diffHeight = size.height() - requestedSize.height();
        int k = diffWidth * diffWidth + diffHeight * diffHeight;

        if (k < r) {
            nearestSize = size;
            r = k;
        }

        if (area >= requestedArea && k < s) {
            nearestGreaterSize = size;
            s = k;
        }
    }

    if (nearestGreaterSize.isEmpty())
        nearestGreaterSize = nearestSize;

    return nearestGreaterSize;
}
