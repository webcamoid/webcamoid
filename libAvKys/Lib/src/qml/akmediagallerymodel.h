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

#ifndef AKMEDIAGALLERYMODEL_H
#define AKMEDIAGALLERYMODEL_H

#include <QAbstractListModel>

class AkMediaGalleryModelPrivate;
class QUrl;

class AkMediaGalleryModel: public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(Location location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
    Q_PROPERTY(int selectedCount
               READ selectedCount
               NOTIFY selectedCountChanged)
    Q_PROPERTY(qint64 totalSelectedSize
               READ totalSelectedSize
               NOTIFY selectionChanged)

    public:
        enum Location
        {
            Location_Pictures,
            Location_Movies
        };
        Q_ENUM(Location)

        enum Roles
        {
            UrlRole = Qt::UserRole + 1,
            SizeRole,
            SelectedRole
        };
        Q_ENUM(Roles)

        explicit AkMediaGalleryModel(QObject *parent=nullptr);
        ~AkMediaGalleryModel();

        Q_INVOKABLE Location location() const;
        Q_INVOKABLE int selectedCount() const;
        Q_INVOKABLE qint64 totalSelectedSize() const;
        Q_INVOKABLE QUrl urlAt(int row) const;

        // QAbstractItemModel
        Q_INVOKABLE int rowCount(const QModelIndex &parent={}) const override;
        Q_INVOKABLE QVariant data(const QModelIndex &index,
                                  int role=Qt::DisplayRole) const override;
        Q_INVOKABLE bool setData(const QModelIndex &index,
                                 const QVariant &value,
                                 int role=Qt::EditRole) override;
        Q_INVOKABLE Qt::ItemFlags flags(const QModelIndex &index) const override;
        Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

    private:
        AkMediaGalleryModelPrivate *d;

    signals:
        void locationChanged(Location location);
        void selectedCountChanged(int selectedCount);
        void selectionChanged(qint64 totalSelectedSize);

    public slots:
        void setLocation(Location location);
        void resetLocation();
        void reload();
        void toggleSelected(int row);
        void clearSelection();
        void selectAll();
        void deleteSelected();
        void deleteSelectedAt(int row);
        static void registerTypes();
};

Q_DECLARE_METATYPE(AkMediaGalleryModel::Location)

#endif // AKMEDIAGALLERYMODEL_H
