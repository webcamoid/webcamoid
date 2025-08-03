/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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
#include <QQmlEngine>

#include "akmenuoption.h"

class AkMenuOptionPrivate
{
    public:
        QString m_name;
        QString m_description;
        QString m_help;
        QVariant m_value;
};

AkMenuOption::AkMenuOption(QObject *parent):
    QObject(parent)
{
    this->d = new AkMenuOptionPrivate();
}

AkMenuOption::AkMenuOption(const QString &name,
                           const QString &description,
                           const QString &help,
                           const QVariant &value):
    QObject()
{
    this->d = new AkMenuOptionPrivate();
    this->d->m_name = name;
    this->d->m_description = description;
    this->d->m_help = help;
    this->d->m_value = value;
}

AkMenuOption::AkMenuOption(const AkMenuOption &other)
{
    this->d = new AkMenuOptionPrivate();
    this->d->m_name = other.d->m_name;
    this->d->m_description = other.d->m_description;
    this->d->m_help = other.d->m_help;
    this->d->m_value = other.d->m_value;
}

AkMenuOption::~AkMenuOption()
{
    delete this->d;
}

AkMenuOption &AkMenuOption::operator =(const AkMenuOption &other)
{
    if (this != &other) {
        this->d->m_name = other.d->m_name;
        this->d->m_description = other.d->m_description;
        this->d->m_help = other.d->m_help;
        this->d->m_value = other.d->m_value;
    }

    return *this;
}

bool AkMenuOption::operator ==(const AkMenuOption &other) const
{
    return this->d->m_name == other.d->m_name
           && this->d->m_description == other.d->m_description
           && this->d->m_help == other.d->m_help
           && this->d->m_value == other.d->m_value;
}

QObject *AkMenuOption::create()
{
    return new AkMenuOption();
}

QObject *AkMenuOption::create(const QString &name,
                              const QString &description,
                              const QString &help,
                              const QVariant &value)
{
    return new AkMenuOption(name, description, help, value);
}

QObject *AkMenuOption::create(const AkMenuOption &other)
{
    return new AkMenuOption(other);
}

QVariant AkMenuOption::toVariant() const
{
    return QVariant::fromValue(*this);
}

QString AkMenuOption::name() const
{
    return this->d->m_name;
}

QString AkMenuOption::description() const
{
    return this->d->m_description;
}

QString AkMenuOption::help() const
{
    return this->d->m_help;
}

QVariant AkMenuOption::value() const
{
    return this->d->m_value;
}

void AkMenuOption::registerTypes()
{
    qRegisterMetaType<AkMenuOption>("AkMenuOption");
    qmlRegisterSingletonType<AkMenuOption>("Ak", 1, 0, "AkMenuOption",
                                           [] (QQmlEngine *qmlEngine,
                                               QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkMenuOption();
    });
}

QDebug operator <<(QDebug debug, const AkMenuOption &menuOption)
{
    debug.nospace() << "AkMenuOption("
                    << "name=" << menuOption.name()
                    << ",description=" << menuOption.description()
                    << ",help=" << menuOption.help()
                    << ",value=" << menuOption.value()
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkMenuOption &menuOption)
{
    QString name;
    istream >> name;
    QString description;
    istream >> description;
    QString help;
    istream >> help;
    QVariant value;
    istream >> value;

    menuOption = {name, description, help, value};

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkMenuOption &menuOption)
{
    ostream << menuOption.name();
    ostream << menuOption.description();
    ostream << menuOption.help();
    ostream << menuOption.value();

    return ostream;
}

#include "moc_akmenuoption.cpp"
