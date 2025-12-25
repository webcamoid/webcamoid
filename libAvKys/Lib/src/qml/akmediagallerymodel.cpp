/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

#include "akmediagallerymodel.h"

struct MediaData
{
    QUrl url;
    qint64 size {0};
    bool selected {false};
};

class AkMediaGalleryModelPrivate
{
    public:
        AkMediaGalleryModel::Location m_location {AkMediaGalleryModel::Location_Pictures};
        int m_selectedCount {0};
        qint64 m_totalSelectedSize {0};
        QList<MediaData> m_images;
};

AkMediaGalleryModel::AkMediaGalleryModel(QObject *parent):
    QAbstractListModel(parent)
{
    this->d = new AkMediaGalleryModelPrivate;
    this->reload();
}

AkMediaGalleryModel::~AkMediaGalleryModel()
{
    delete this->d;
}

AkMediaGalleryModel::Location AkMediaGalleryModel::location() const
{
    return this->d->m_location;
}

int AkMediaGalleryModel::selectedCount() const
{
    return this->d->m_selectedCount;
}

qint64 AkMediaGalleryModel::totalSelectedSize() const
{
    return this->d->m_totalSelectedSize;
}

QUrl AkMediaGalleryModel::urlAt(int row) const
{
    if (row >= 0 && row < this->d->m_images.size())
        return this->d->m_images.at(row).url;

    return {};
}

int AkMediaGalleryModel::rowCount(const QModelIndex &) const
{
    return this->d->m_images.size();
}

QVariant AkMediaGalleryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= this->d->m_images.size())
        return {};

    const auto &item = this->d->m_images.at(index.row());

    switch (role) {
    case UrlRole:
        return item.url;

    case SizeRole:
        return item.size;

    case SelectedRole:
        return item.selected;

    default:
        break;
    }

    return {};
}

bool AkMediaGalleryModel::setData(const QModelIndex &index,
                                  const QVariant &value,
                                  int role)
{
    if (!index.isValid() || role != SelectedRole)
        return false;

    auto &item = this->d->m_images[index.row()];
    bool newSelected = value.toBool();

    if (item.selected == newSelected)
        return true;

    item.selected = newSelected;

    if (newSelected) {
        ++this->d->m_selectedCount;
        this->d->m_totalSelectedSize += item.size;
    } else {
        --this->d->m_selectedCount;
        this->d->m_totalSelectedSize -= item.size;
    }

    emit this->dataChanged(index, index, {SelectedRole});
    emit this->selectedCountChanged(this->d->m_selectedCount);
    emit this->selectionChanged(this->d->m_totalSelectedSize);

    return true;
}

Qt::ItemFlags AkMediaGalleryModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> AkMediaGalleryModel::roleNames() const
{
    return {
        {UrlRole     , "url"     },
        {SizeRole    , "fileSize"},
        {SelectedRole, "selected"}
    };
}

void AkMediaGalleryModel::setLocation(Location location)
{
    if (this->d->m_location == location)
        return;

    this->d->m_location = location;
    emit this->locationChanged(location);
    this->reload();
}

void AkMediaGalleryModel::resetLocation()
{
    this->setLocation(Location_Pictures);
}

void AkMediaGalleryModel::reload()
{
    QString path;
    QStringList filters;

    switch (this->d->m_location) {
    case Location_Pictures:
        path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        filters = {
            "*.jpg", "*.jpeg", "*.png", "*.gif",
            "*.bmp", "*.webp", "*.tiff", "*.svg"
        };

        break;
    case Location_Movies:
        path = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        filters = {
            "*.mp4", "*.mkv", "*.avi", "*.mov",
            "*.wmv", "*.flv", "*.webm", "*.3gp",
            "*.m4v", "*.mpg", "*.mpeg", "*.ogv"
        };

        break;

    default:
        return;
    }

    this->beginResetModel();
    this->d->m_images.clear();
    this->d->m_selectedCount = 0;
    this->d->m_totalSelectedSize = 0;

    QDir dir(path);

    if (dir.exists()) {
        dir.setNameFilters(filters);
        dir.setFilter(QDir::Files);
        dir.setSorting(QDir::Time | QDir::Reversed);

        for (const auto &info : dir.entryInfoList())
            if (info.isFile()) {
                MediaData item;
                item.url = QUrl::fromLocalFile(info.absoluteFilePath());
                item.size = info.size();
                this->d->m_images.append(item);
            }
    }

    this->endResetModel();
}

void AkMediaGalleryModel::toggleSelected(int row)
{
    if (row < 0 || row >= this->d->m_images.size())
        return;

    auto idx = index(row);
    bool newVal = !data(idx, SelectedRole).toBool();
    this->setData(idx, newVal, SelectedRole);
}

void AkMediaGalleryModel::clearSelection()
{
    bool changed = false;

    for (int i = 0; i < this->d->m_images.size(); ++i)
        if (this->d->m_images[i].selected) {
            this->d->m_images[i].selected = false;
            --this->d->m_selectedCount;
            this->d->m_totalSelectedSize -= this->d->m_images[i].size;
            changed = true;
        }

    if (changed) {
        emit this->dataChanged(index(0), index(rowCount() - 1), {SelectedRole});
        emit this->selectedCountChanged(this->d->m_selectedCount);
        emit this->selectionChanged(this->d->m_totalSelectedSize);
    }
}

void AkMediaGalleryModel::selectAll()
{
    bool changed = false;

    for (int i = 0; i < this->d->m_images.size(); ++i)
        if (!this->d->m_images[i].selected) {
            this->d->m_images[i].selected = true;
            ++this->d->m_selectedCount;
            this->d->m_totalSelectedSize += this->d->m_images[i].size;
            changed = true;
        }

    if (changed) {
        emit this->dataChanged(index(0), index(rowCount() - 1), {SelectedRole});
        emit this->selectedCountChanged(this->d->m_selectedCount);
        emit this->selectionChanged(this->d->m_totalSelectedSize);
    }
}

void AkMediaGalleryModel::deleteSelected()
{
    QList<int> rowsToDelete;

    for (int i = 0; i < this->d->m_images.size(); ++i)
        if (this->d->m_images.at(i).selected)
            rowsToDelete.append(i);

    if (rowsToDelete.isEmpty())
        return;

    // Delete from highest to lowest index to avoid invalidating indexes
    std::sort(rowsToDelete.rbegin(), rowsToDelete.rend());

    for (auto row: rowsToDelete) {
        QString localPath = this->d->m_images.at(row).url.toLocalFile();
        QFile::remove(localPath);

        this->beginRemoveRows(QModelIndex(), row, row);
        this->d->m_images.removeAt(row);
        this->endRemoveRows();
    }

    this->d->m_selectedCount = 0;
    this->d->m_totalSelectedSize = 0;
    emit this->selectedCountChanged(this->d->m_selectedCount);
    emit this->selectionChanged(this->d->m_totalSelectedSize);
}

void AkMediaGalleryModel::deleteSelectedAt(int row)
{
    if (row < 0 || row >= this->d->m_images.size())
        return;

    auto localPath = this->d->m_images.at(row).url.toLocalFile();
    QFile::remove(localPath);

    this->beginRemoveRows(QModelIndex(), row, row);
    this->d->m_images.removeAt(row);
    this->endRemoveRows();
}

void AkMediaGalleryModel::registerTypes()
{
    qmlRegisterType<AkMediaGalleryModel>("Ak", 1, 0, "AkMediaGalleryModel");
    qRegisterMetaType<Location>("AkMediaGalleryModelLocation");
}

#include "moc_akmediagallerymodel.cpp"
