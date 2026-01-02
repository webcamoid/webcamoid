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
#include <QFileDialog>
#include <QFileInfo>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <QtCore/private/qandroidextras_p.h>
#include <QMimeDatabase>

#define FLAG_GRANT_READ_URI_PERMISSION 0x1
#endif

#include "akmediagallerymodel.h"

struct MediaData
{
    QUrl url;
    AkMediaGalleryModel::Type type {AkMediaGalleryModel::Type_Picture};
    qint64 size {0};
    bool selected {false};
};

class AkMediaGalleryModelPrivate
{
    public:
        QString m_directory;
        int m_selectedCount {0};
        qint64 m_totalSelectedSize {0};
        QList<MediaData> m_medias;

        static bool copyFile(const QUrl &src, const QUrl &dest);

#ifdef Q_OS_ANDROID
        static QJniObject getUriForFile(const QString &filePath);
#endif
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

QString AkMediaGalleryModel::directory() const
{
    return this->d->m_directory;
}

int AkMediaGalleryModel::selectedCount() const
{
    return this->d->m_selectedCount;
}

QStringList AkMediaGalleryModel::selectedUrls() const
{
    QStringList urls;

    for (const auto &media: this->d->m_medias)
        if (media.selected)
            urls << media.url.toString();

    return urls;
}

qint64 AkMediaGalleryModel::totalSelectedSize() const
{
    return this->d->m_totalSelectedSize;
}

QUrl AkMediaGalleryModel::urlAt(int row) const
{
    if (row >= 0 && row < this->d->m_medias.size())
        return this->d->m_medias.at(row).url;

    return {};
}

bool AkMediaGalleryModel::share(const QUrl &src, const QString &message) const
{
    if (!src.isValid()) {
        qWarning() << "The file URI is invalid.";

        return false;
    }

#ifdef Q_OS_ANDROID
    auto fileName = src.toLocalFile();

    // Crear URI con FileProvider
    auto fileUri = AkMediaGalleryModelPrivate::getUriForFile(fileName);

    if (!fileUri.isValid()) {
        qWarning() << "Can't create a valid file URI.";

        return false;
    }

    auto activity =
        QJniObject(qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context());

    if (!activity.isValid()){
        qWarning() << "Can't get the application activity.";

        return false;
    }

    // Create SEND action intent
    QJniObject intent("android/content/Intent", "()V");
    auto action = QJniObject::fromString("android.intent.action.SEND");
    intent.callObjectMethod("setAction",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            action.object<jstring>());

    // Attach the file as an extra stream
    intent.callObjectMethod("putExtra",
                            "(Ljava/lang/String;Landroid/os/Parcelable;)Landroid/content/Intent;",
                            QJniObject::fromString("android.intent.extra.STREAM").object<jstring>(),
                            fileUri.object());

    // Set the MIME type (important for filtering compatible apps)
    QMimeDatabase mimeDb;
    auto mimeType = mimeDb.mimeTypeForFile(fileName);
    auto mimeString = mimeType.name();

    if (mimeString.isEmpty())
        mimeString = "*/*";

    intent.callObjectMethod("setType",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            QJniObject::fromString(mimeString).object<jstring>());

    // Grant read permission
    intent.callObjectMethod("addFlags",
                            "(I)Landroid/content/Intent;",
                            FLAG_GRANT_READ_URI_PERMISSION);

    auto msg = message;

    if (msg.isEmpty())
        msg = tr("Share file");

    // Launch the app selector for sharing
    auto chooserTitle = QJniObject::fromString(msg);
    auto chooser =
            QJniObject::callStaticObjectMethod("android/content/Intent",
                                               "createChooser",
                                               "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
                                               intent.object(),
                                               chooserTitle.object<jstring>());

    if (!chooser.isValid())
        return false;

    return activity.callMethod<jboolean>("startActivity",
                                         "(Landroid/content/Intent;)V",
                                         chooser.object());
#else
    Q_UNUSED(message)

    qWarning() << "Feature not implemented on desktop";

    return false;
#endif
}

bool AkMediaGalleryModel::share(const QStringList &srcPaths,
                                const QString &message) const
{
    if (srcPaths.isEmpty())
        return false;

#ifdef Q_OS_ANDROID
    QJniObject uriList("java/util/ArrayList", "()V");

    if (!uriList.isValid()) {
        qWarning() << "Cannot create ArrayList";

        return false;
    }

    for (const auto &path: srcPaths) {
        if (path.isEmpty())
            continue;

        auto fileUri = AkMediaGalleryModelPrivate::getUriForFile(QUrl(path).toLocalFile());

        if (!fileUri.isValid()) {
            qWarning() << "Invalid URI for file:" << path;

            continue;
        }

        uriList.callMethod<void>("add",
                                 "(Ljava/lang/Object;)Z",
                                 fileUri.object());
    }

    if (uriList.callMethod<jboolean>("isEmpty")) {
        qWarning() << "No valid URIs to share";

        return false;
    }

    QJniObject activity = QNativeInterface::QAndroidApplication::context();

    if (!activity.isValid()) {
        qWarning() << "Cannot get Android activity";

        return false;
    }

    QJniObject intent("android/content/Intent", "()V");

    auto action = QJniObject::fromString("android.intent.action.SEND_MULTIPLE");
    intent.callObjectMethod("setAction",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            action.object<jstring>());

    intent.callObjectMethod("putParcelableArrayListExtra",
                            "(Ljava/lang/String;Ljava/util/ArrayList;)Landroid/content/Intent;",
                            QJniObject::fromString("android.intent.extra.STREAM").object<jstring>(),
                            uriList.object());

    QMimeDatabase mimeDb;
    QString mimeString = "*/*";

    if (!srcPaths.isEmpty()) {
        auto mime = mimeDb.mimeTypeForFile(srcPaths.first());

        if (!mime.name().isEmpty())
            mimeString = mime.name();
    }

    intent.callObjectMethod("setType",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            QJniObject::fromString(mimeString).object<jstring>());

    // Grant read permission a todos los URIs
    intent.callMethod<void>("addFlags",
                            "(I)Landroid/content/Intent;",
                            FLAG_GRANT_READ_URI_PERMISSION);

    auto msg = message;

    if (msg.isEmpty())
        msg = tr("Share files");

    auto chooserTitle = QJniObject::fromString(msg);
    auto chooser =
            QJniObject::callStaticObjectMethod("android/content/Intent",
                                               "createChooser",
                                               "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
                                               intent.object(),
                                               chooserTitle.object<jstring>());

    if (!chooser.isValid())
        return false;

    return activity.callMethod<jboolean>("startActivity",
                                         "(Landroid/content/Intent;)V",
                                         chooser.object());
#else
    Q_UNUSED(message)
    qWarning() << "Feature not implemented on desktop";

    return false;
#endif
}

bool AkMediaGalleryModel::useAs(const QUrl &src, const QString &message) const
{
    if (!src.isValid())
        return false;

#ifdef Q_OS_ANDROID
    auto fileName = src.toLocalFile();

    // Crear URI con FileProvider
    auto fileUri = AkMediaGalleryModelPrivate::getUriForFile(fileName);

    if (!fileUri.isValid()) {
        qWarning() << "Can't create a valid file URI.";

        return false;
    }

    auto activity =
        QJniObject(qApp->nativeInterface<QNativeInterface::QAndroidApplication>()->context());

    if (!activity.isValid()){
        qWarning() << "Can't get the application activity.";

        return false;
    }

    QJniObject intent("android/content/Intent", "()V");

    // Create SEND action intent
    auto action = QJniObject::fromString("android.intent.action.ATTACH_DATA");
    intent.callObjectMethod("setAction",
                            "(Ljava/lang/String;)Landroid/content/Intent;",
                            action.object<jstring>());

    // Set the MIME type (important for filtering compatible apps)
    QMimeDatabase mimeDb;
    auto mimeType = mimeDb.mimeTypeForFile(fileName);
    auto mimeString = mimeType.name();

    if (mimeString.isEmpty())
        mimeString = "*/*";

    // Set the data
    intent.callObjectMethod("setDataAndType", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/content/Intent;",
                            fileUri.object<jobject>(),
                            QJniObject::fromString(mimeString).object<jstring>());

    // Set the mimetype
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                            QJniObject::fromString("mimeType").object<jstring>(),
                            QJniObject::fromString(mimeString).object<jstring>());

    // Grant read permission
    intent.callObjectMethod("addFlags", "(I)Landroid/content/Intent;",
                            FLAG_GRANT_READ_URI_PERMISSION);

    auto msg = message;

    if (msg.isEmpty())
        msg = tr("Use as...");

    // Launch the app selector for sharing
    auto chooserTitle = QJniObject::fromString(msg);
    auto chooser =
            QJniObject::callStaticObjectMethod("android/content/Intent",
                                               "createChooser",
                                               "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
                                               intent.object(),
                                               chooserTitle.object<jstring>());

    if (!chooser.isValid())
        return false;

    return activity.callMethod<jboolean>("startActivity",
                                         "(Landroid/content/Intent;)V",
                                         chooser.object());
#else
    qWarning() << "Feature not implemented on desktop";

    return false;
#endif
}

bool AkMediaGalleryModel::copyTo(const QUrl &src, const QString &message) const
{
    if (!src.isValid() || !src.isLocalFile() && src.scheme() != "content")
        return false;

    auto fileName = QFileInfo(src.path()).fileName();

    if (fileName.isEmpty())
        fileName = "file";

    auto msg = message;

    if (msg.isEmpty())
        msg = tr("Copy to...");

    auto destPath =
            QFileDialog::getSaveFileName(nullptr,
                                         msg,
                                         fileName,
                                         tr("All Files (*)"),
                                         nullptr,
                                         QFileDialog::Options());

    if (destPath.isEmpty())
        return false;

    return AkMediaGalleryModelPrivate::copyFile(src,
                                                QUrl::fromUserInput(destPath));
}

bool AkMediaGalleryModel::moveTo(const QUrl &src, const QString &message) const
{
    auto msg = message;

    if (msg.isEmpty())
        msg = tr("Move to...");

    if (!copyTo(src, msg))
        return false;

    // Remove the source file
    return QFile::remove(src.toLocalFile());
}

int AkMediaGalleryModel::rowCount(const QModelIndex &) const
{
    return this->d->m_medias.size();
}

QVariant AkMediaGalleryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= this->d->m_medias.size())
        return {};

    const auto &item = this->d->m_medias.at(index.row());

    switch (role) {
    case UrlRole:
        return item.url;

    case SizeRole:
        return item.size;

    case SelectedRole:
        return item.selected;

    case TypeRole:
        return item.type;

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

    auto &item = this->d->m_medias[index.row()];
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
        {SelectedRole, "selected"},
        {TypeRole    , "type"    }
    };
}

void AkMediaGalleryModel::setDirectory(const QString &directory)
{
    if (this->d->m_directory == directory)
        return;

    this->d->m_directory = directory;
    emit this->directoryChanged(directory);
    this->reload();
}

void AkMediaGalleryModel::resetDirectory()
{
    this->setDirectory("");
}

void AkMediaGalleryModel::reload()
{
    const QStringList pictureFilters = {
        "*.jpg", "*.jpeg", "*.jpe", "*.png", "*.gif",
        "*.bmp", "*.webp", "*.tiff", "*.tif", "*.svg",
        "*.heic", "*.heif"
    };

    const QStringList videoFilters = {
        "*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv",
        "*.flv", "*.webm", "*.3gp", "*.m4v", "*.mpg",
        "*.mpeg", "*.ogv", "*.ts", "*.vob"
    };

    auto allFilters = pictureFilters + videoFilters;

    this->beginResetModel();
    this->d->m_medias.clear();
    this->d->m_selectedCount = 0;
    this->d->m_totalSelectedSize = 0;

    QDir dir(this->d->m_directory);

    if (!dir.exists()) {
        this->endResetModel();

        return;
    }

    dir.setNameFilters(allFilters);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time | QDir::Reversed);

    for (const auto &info: dir.entryInfoList()) {
        if (!info.isFile())
            continue;

        MediaData item;
        item.url = QUrl::fromLocalFile(info.absoluteFilePath());
        item.size = info.size();
        item.selected = false;
        auto suffix = info.suffix().toLower();
        item.type = videoFilters.contains("*." + suffix)?
                        Type_Movie:
                        Type_Picture;

        this->d->m_medias.append(item);
    }

    this->endResetModel();
}

void AkMediaGalleryModel::toggleSelected(int row)
{
    if (row < 0 || row >= this->d->m_medias.size())
        return;

    auto idx = index(row);
    bool newVal = !data(idx, SelectedRole).toBool();
    this->setData(idx, newVal, SelectedRole);
}

void AkMediaGalleryModel::clearSelection()
{
    bool changed = false;

    for (int i = 0; i < this->d->m_medias.size(); ++i)
        if (this->d->m_medias[i].selected) {
            this->d->m_medias[i].selected = false;
            --this->d->m_selectedCount;
            this->d->m_totalSelectedSize -= this->d->m_medias[i].size;
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

    for (int i = 0; i < this->d->m_medias.size(); ++i)
        if (!this->d->m_medias[i].selected) {
            this->d->m_medias[i].selected = true;
            ++this->d->m_selectedCount;
            this->d->m_totalSelectedSize += this->d->m_medias[i].size;
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

    for (int i = 0; i < this->d->m_medias.size(); ++i)
        if (this->d->m_medias.at(i).selected)
            rowsToDelete.append(i);

    if (rowsToDelete.isEmpty())
        return;

    // Delete from highest to lowest index to avoid invalidating indexes
    std::sort(rowsToDelete.rbegin(), rowsToDelete.rend());

    for (auto row: rowsToDelete) {
        QString localPath = this->d->m_medias.at(row).url.toLocalFile();
        QFile::remove(localPath);

        this->beginRemoveRows(QModelIndex(), row, row);
        this->d->m_medias.removeAt(row);
        this->endRemoveRows();
    }

    this->d->m_selectedCount = 0;
    this->d->m_totalSelectedSize = 0;
    emit this->selectedCountChanged(this->d->m_selectedCount);
    emit this->selectionChanged(this->d->m_totalSelectedSize);
}

void AkMediaGalleryModel::deleteSelectedAt(int row)
{
    if (row < 0 || row >= this->d->m_medias.size())
        return;

    auto localPath = this->d->m_medias.at(row).url.toLocalFile();
    QFile::remove(localPath);

    this->beginRemoveRows(QModelIndex(), row, row);
    this->d->m_medias.removeAt(row);
    this->endRemoveRows();
}

void AkMediaGalleryModel::registerTypes()
{
    qmlRegisterType<AkMediaGalleryModel>("Ak", 1, 0, "AkMediaGalleryModel");
    qRegisterMetaType<Type>("AkMediaGalleryModelType");
    qRegisterMetaType<Roles>("AkMediaGalleryModelRoles");
}

bool AkMediaGalleryModelPrivate::copyFile(const QUrl &src, const QUrl &dest)
{
    QFile srcFile(src.toLocalFile());

    if (!srcFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open source file:" << srcFile.errorString();

        return false;
    }

    QFile destFile(dest.toString());

    if (!destFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Cannot create destination file:" << destFile.errorString();

        return false;
    }

    char buffer[4096];
    qint64 bytesRead;

    while ((bytesRead = srcFile.read(buffer, sizeof(buffer))) > 0)
        if (destFile.write(buffer, bytesRead) != bytesRead) {
            qWarning() << "Write error:" << destFile.errorString();

            return false;
        }

    if (srcFile.error() != QFileDevice::NoError) {
        qWarning() << "Read error:" << srcFile.errorString();

        return false;
    }

    return true;
}

#ifdef Q_OS_ANDROID
QJniObject AkMediaGalleryModelPrivate::getUriForFile(const QString &filePath)
{
    if (!QFileInfo(filePath).exists()) {
        qWarning() << "File not found:" << filePath;

        return {};
    }

    auto context = QJniObject(QNativeInterface::QAndroidApplication::context());
    auto jniPath = QJniObject::fromString(filePath);

    // Create Java File object
    QJniObject file("java/io/File",
                    "(Ljava/lang/String;)V",
                    jniPath.object<jstring>());

    // Get the app's package name (for the authority)
    auto packageName = context.callObjectMethod("getPackageName",
                                                "()Ljava/lang/String;");
    auto authority = packageName.toString() + ".qtprovider";
    auto jniAuthority = QJniObject::fromString(authority);

    // Obtain the secure URI of the FileProvider
    return QJniObject::callStaticObjectMethod("androidx/core/content/FileProvider",
                                              "getUriForFile",
                                              "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
                                              context.object(),
                                              jniAuthority.object<jstring>(),
                                              file.object());
}
#endif

#include "moc_akmediagallerymodel.cpp"
