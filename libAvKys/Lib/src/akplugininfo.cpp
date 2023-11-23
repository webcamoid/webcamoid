/* Webcamoid, webcam capture application.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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

#include <QDataStream>
#include <QDebug>
#include <QJsonObject>
#include <QQmlEngine>
#include <QStringList>

#include "akplugininfo.h"

class AkPluginInfoPrivate
{
    public:
        QString m_id;
        QString m_description;
        QString m_path;
        QStringList m_depends;
        AkElementType m_type {AkElementType_Unknown};
        AkElementCategory m_category {AkElementCategory_Unknown};
        int m_priority {-1000};
        IAkElementProvider *m_provider {nullptr};
};

AkPluginInfo::AkPluginInfo(QObject *parent)
{
    this->d = new AkPluginInfoPrivate();
}

AkPluginInfo::AkPluginInfo(const QString &id,
                           const QString &description,
                           const QStringList &depends,
                           AkElementType type,
                           AkElementCategory category,
                           int priority,
                           IAkElementProvider *provider,
                           QObject *parent):
    QObject(parent)
{
    this->d = new AkPluginInfoPrivate();
    this->d->m_id = id;
    this->d->m_description = description;
    this->d->m_depends = depends;
    this->d->m_type = type;
    this->d->m_category = category;
    this->d->m_priority = priority;
    this->d->m_provider = provider;
}

AkPluginInfo::AkPluginInfo(const QString &id,
                           const QString &description,
                           const QString &path,
                           const QStringList &depends,
                           AkElementType type,
                           AkElementCategory category,
                           int priority,
                           IAkElementProvider *provider,
                           QObject *parent):
      QObject(parent)
{
    this->d = new AkPluginInfoPrivate();
    this->d->m_id = id;
    this->d->m_description = description;
    this->d->m_path = path;
    this->d->m_depends = depends;
    this->d->m_type = type;
    this->d->m_category = category;
    this->d->m_priority = priority;
    this->d->m_provider = provider;
}

AkPluginInfo::AkPluginInfo(const AkPluginInfo &other):
    QObject()
{
    this->d = new AkPluginInfoPrivate();
    this->d->m_id = other.d->m_id;
    this->d->m_description = other.d->m_description;
    this->d->m_path = other.d->m_path;
    this->d->m_depends = other.d->m_depends;
    this->d->m_type = other.d->m_type;
    this->d->m_category = other.d->m_category;
    this->d->m_priority = other.d->m_priority;
    this->d->m_provider = other.d->m_provider;
}

AkPluginInfo::~AkPluginInfo()
{
    delete this->d;
}

AkPluginInfo &AkPluginInfo::operator =(const AkPluginInfo &other)
{
    if (this != &other) {
        this->d->m_id = other.d->m_id;
        this->d->m_description = other.d->m_description;
        this->d->m_path = other.d->m_path;
        this->d->m_depends = other.d->m_depends;
        this->d->m_type = other.d->m_type;
        this->d->m_category = other.d->m_category;
        this->d->m_priority = other.d->m_priority;
        this->d->m_provider = other.d->m_provider;
    }

    return *this;
}

bool AkPluginInfo::operator ==(const AkPluginInfo &other) const
{
    if (this->d->m_id != other.d->m_id
        || this->d->m_description != other.d->m_description
        || this->d->m_path != other.d->m_path
        || this->d->m_depends != other.d->m_depends
        || this->d->m_type != other.d->m_type
        || this->d->m_category != other.d->m_category
        || this->d->m_priority != other.d->m_priority
        || this->d->m_provider != other.d->m_provider) {
        return false;
    }

    return true;
}

bool AkPluginInfo::operator !=(const AkPluginInfo &other) const
{
    return !(*this == other);
}

AkPluginInfo::operator bool() const
{
    return !this->d->m_id.isEmpty();
}

QObject *AkPluginInfo::create(const AkPluginInfo &info)
{
    return new AkPluginInfo(info);
}

QVariant AkPluginInfo::toVariant() const
{
    return QVariant::fromValue(*this);
}

QString AkPluginInfo::id() const
{
    return this->d->m_id;
}

QString AkPluginInfo::description() const
{
    return this->d->m_description;
}

QString AkPluginInfo::path() const
{
    return this->d->m_path;
}

QStringList AkPluginInfo::depends() const
{
    return this->d->m_depends;
}

AkElementType AkPluginInfo::type() const
{
    return this->d->m_type;
}

AkElementCategory AkPluginInfo::category() const
{
    return this->d->m_category;
}

int AkPluginInfo::priority() const
{
    return this->d->m_priority;
}

IAkElementProvider *AkPluginInfo::provider() const
{
    return this->d->m_provider;
}

void AkPluginInfo::registerTypes()
{
    qRegisterMetaType<AkPluginInfo>("AkPluginInfo");
    qmlRegisterSingletonType<AkPluginInfo>("Ak", 1, 0, "AkPluginInfo",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPluginInfo();
    });
}

QDebug operator <<(QDebug debug, const AkPluginInfo &info)
{
    debug.nospace() << "QDataStream(" << Qt::endl
                    << "    id: " << info.id() << Qt::endl
                    << "    description: " << info.description() << Qt::endl
                    << "    path: " << info.path() << Qt::endl
                    << "    depends: [" << info.depends().join(", ") << "]" << Qt::endl
                    << "    type: " << info.type() << Qt::endl
                    << "    category: " << info.category() << Qt::endl
                    << "    priority: " << info.priority() << Qt::endl
                    << "    provider: " << info.provider() << Qt::endl
                    << ")";

    return debug.space();
}

#include "moc_akplugininfo.cpp"
