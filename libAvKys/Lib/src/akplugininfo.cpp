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
#include <QRegExp>
#include <QStringList>

#include "akplugininfo.h"

class AkPluginInfoPrivate
{
    public:
        QString m_name;
        QString m_description;
        QString m_id;
        QString m_path;
        QStringList m_implements;
        QStringList m_depends;
        QString m_type;
        int m_priority {-1000};
};

AkPluginInfo::AkPluginInfo(QObject *parent):
    QObject(parent)
{
    this->d = new AkPluginInfoPrivate();
}

AkPluginInfo::AkPluginInfo(const QVariantMap &metaData):
    QObject()
{
    this->d = new AkPluginInfoPrivate();
    this->d->m_name = metaData.value("name").toString();
    this->d->m_description = metaData.value("description").toString();
    this->d->m_id = metaData.value("id").toString();
    this->d->m_path = metaData.value("path").toString();
    this->d->m_implements = metaData.value("implements").toStringList();
    this->d->m_depends = metaData.value("depends").toStringList();
    this->d->m_type = metaData.value("type").toString();
    this->d->m_priority = metaData.value("priority", -1000).toInt();
}

AkPluginInfo::AkPluginInfo(const AkPluginInfo &other):
    QObject()
{
    this->d = new AkPluginInfoPrivate();
    this->d->m_name = other.d->m_name;
    this->d->m_description = other.d->m_description;
    this->d->m_id = other.d->m_id;
    this->d->m_path = other.d->m_path;
    this->d->m_implements = other.d->m_implements;
    this->d->m_depends = other.d->m_depends;
    this->d->m_type = other.d->m_type;
    this->d->m_priority = other.d->m_priority;
}

AkPluginInfo::~AkPluginInfo()
{
    delete this->d;
}

AkPluginInfo &AkPluginInfo::operator =(const AkPluginInfo &other)
{
    if (this != &other) {
        this->d->m_name = other.d->m_name;
        this->d->m_description = other.d->m_description;
        this->d->m_id = other.d->m_id;
        this->d->m_path = other.d->m_path;
        this->d->m_implements = other.d->m_implements;
        this->d->m_depends = other.d->m_depends;
        this->d->m_type = other.d->m_type;
        this->d->m_priority = other.d->m_priority;
    }

    return *this;
}

bool AkPluginInfo::operator ==(const AkPluginInfo &other) const
{
    if (this->d->m_name != other.d->m_name)
        return false;

    if (this->d->m_description != other.d->m_description)
        return false;

    if (this->d->m_id != other.d->m_id)
        return false;

    if (this->d->m_path != other.d->m_path)
        return false;

    if (this->d->m_implements != other.d->m_implements)
        return false;

    if (this->d->m_depends != other.d->m_depends)
        return false;

    if (this->d->m_type != other.d->m_type)
        return false;

    if (this->d->m_priority != other.d->m_priority)
        return false;

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

QString AkPluginInfo::name() const
{
    return this->d->m_name;
}

QString AkPluginInfo::description() const
{
    return this->d->m_description;
}

QString AkPluginInfo::id() const
{
    return this->d->m_id;
}

QString AkPluginInfo::path() const
{
    return this->d->m_path;
}

QStringList AkPluginInfo::implements() const
{
    return this->d->m_implements;
}

QStringList AkPluginInfo::depends() const
{
    return this->d->m_depends;
}

QString AkPluginInfo::type() const
{
    return this->d->m_type;
}

int AkPluginInfo::priority() const
{
    return this->d->m_priority;
}

void AkPluginInfo::registerTypes()
{
    qRegisterMetaType<AkPluginInfo>("AkPluginInfo");
    qRegisterMetaTypeStreamOperators<AkPluginInfo>("AkPluginInfo");
    QMetaType::registerDebugStreamOperator<AkPluginInfo>();
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
                    << "    name: " << info.name() << Qt::endl
                    << "    description: " << info.description() << Qt::endl
                    << "    id: " << info.id() << Qt::endl
                    << "    path: " << info.path() << Qt::endl
                    << "    implements: [" << info.implements().join(", ") << "]" << Qt::endl
                    << "    depends: [" << info.depends().join(", ") << "]" << Qt::endl
                    << "    type: " << info.type() << Qt::endl
                    << "    priority: " << info.priority() << Qt::endl
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkPluginInfo &info)
{
    QVariantMap metaData;
    istream >> metaData;
    info = {metaData};

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkPluginInfo &info)
{
    QVariantMap metaData {
        {"num"        , info.name()       },
        {"description", info.description()},
        {"id"         , info.id()         },
        {"path"       , info.path()       },
        {"implements" , info.implements() },
        {"depends"    , info.depends()    },
        {"type"       , info.type()       },
        {"priority"   , info.priority()   }
    };

    ostream << metaData;

    return ostream;
}

#include "moc_akplugininfo.cpp"
