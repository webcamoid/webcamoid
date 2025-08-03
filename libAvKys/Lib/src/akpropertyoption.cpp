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

#include "akpropertyoption.h"

class AkPropertyOptionPrivate
{
    public:
        QString m_name;
        QString m_description;
        QString m_help;
        AkPropertyOption::OptionType m_type {AkPropertyOption::OptionType_Unknown};
        qreal m_min {0.0};
        qreal m_max {0.0};
        qreal m_step {0.0};
        QVariant m_defaultValue;
        AkMenu m_menu;
};

AkPropertyOption::AkPropertyOption(QObject *parent):
    QObject(parent)
{
    this->d = new AkPropertyOptionPrivate();
}

AkPropertyOption::AkPropertyOption(const QString &name,
                                   const QString &description,
                                   const QString &help,
                                   OptionType type,
                                   qreal min,
                                   qreal max,
                                   qreal step,
                                   const QVariant &defaultValue,
                                   const AkMenu &menu):
    QObject()
{
    this->d = new AkPropertyOptionPrivate();
    this->d->m_name = name;
    this->d->m_description = description;
    this->d->m_help = help;
    this->d->m_type = type;
    this->d->m_min = min;
    this->d->m_max = max;
    this->d->m_step = step;
    this->d->m_defaultValue = defaultValue;
    this->d->m_menu = menu;
}

AkPropertyOption::AkPropertyOption(const AkPropertyOption &other):
    QObject()
{
    this->d = new AkPropertyOptionPrivate();
    this->d->m_name = other.d->m_name;
    this->d->m_description = other.d->m_description;
    this->d->m_help = other.d->m_help;
    this->d->m_type = other.d->m_type;
    this->d->m_min = other.d->m_min;
    this->d->m_max = other.d->m_max;
    this->d->m_step = other.d->m_step;
    this->d->m_defaultValue = other.d->m_defaultValue;
    this->d->m_menu = other.d->m_menu;
}

AkPropertyOption::~AkPropertyOption()
{
    delete this->d;
}

AkPropertyOption &AkPropertyOption::operator =(const AkPropertyOption &other)
{
    if (this != &other) {
        this->d->m_name = other.d->m_name;
        this->d->m_description = other.d->m_description;
        this->d->m_help = other.d->m_help;
        this->d->m_type = other.d->m_type;
        this->d->m_min = other.d->m_min;
        this->d->m_max = other.d->m_max;
        this->d->m_step = other.d->m_step;
        this->d->m_defaultValue = other.d->m_defaultValue;
        this->d->m_menu = other.d->m_menu;
    }

    return *this;
}

bool AkPropertyOption::operator ==(const AkPropertyOption &other) const
{
    return this->d->m_name == other.d->m_name
           && this->d->m_description == other.d->m_description
           && this->d->m_help == other.d->m_help
           && this->d->m_type == other.d->m_type
           && this->d->m_min == other.d->m_min
           && this->d->m_max == other.d->m_max
           && this->d->m_step == other.d->m_step
           && this->d->m_defaultValue == other.d->m_defaultValue
           && this->d->m_menu == other.d->m_menu;
}

QObject *AkPropertyOption::create()
{
    return new AkPropertyOption();
}

QObject *AkPropertyOption::create(const QString &name,
                                  const QString &description,
                                  const QString &help,
                                  OptionType type,
                                  qreal min,
                                  qreal max,
                                  qreal step,
                                  const QVariant &defaultValue,
                                  const AkMenu &menu)
{
    return new AkPropertyOption(name,
                                description,
                                help,
                                type,
                                min,
                                max,
                                step,
                                defaultValue,
                                menu);
}

QObject *AkPropertyOption::create(const AkPropertyOption &other)
{
    return new AkPropertyOption(other);
}

QVariant AkPropertyOption::toVariant() const
{
    return QVariant::fromValue(*this);
}

QString AkPropertyOption::name() const
{
    return this->d->m_name;
}

QString AkPropertyOption::description() const
{
    return this->d->m_description;
}

QString AkPropertyOption::help() const
{
    return this->d->m_help;
}

AkPropertyOption::OptionType AkPropertyOption::type() const
{
    return this->d->m_type;
}

qreal AkPropertyOption::min() const
{
    return this->d->m_min;
}

qreal AkPropertyOption::max() const
{
    return this->d->m_max;
}

qreal AkPropertyOption::step() const
{
    return this->d->m_step;
}

QVariant AkPropertyOption::defaultValue() const
{
    return this->d->m_defaultValue;
}

AkMenu AkPropertyOption::menu() const
{
    return this->d->m_menu;
}

void AkPropertyOption::registerTypes()
{
    qRegisterMetaType<AkPropertyOption>("AkPropertyOption");
    qmlRegisterSingletonType<AkPropertyOption>("Ak", 1, 0, "AkPropertyOption",
                                               [] (QQmlEngine *qmlEngine,
                                                   QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPropertyOption();
    });
}

QDebug operator <<(QDebug debug, const AkPropertyOption &propertyOption)
{
    debug.nospace() << "AkPropertyOption("
                    << "name=" << propertyOption.name()
                    << ",description=" << propertyOption.description()
                    << ",help=" << propertyOption.help()
                    << ",type=" << propertyOption.type()
                    << ",min=" << propertyOption.min()
                    << ",max=" << propertyOption.max()
                    << ",step=" << propertyOption.step()
                    << ",defaultValue=" << propertyOption.defaultValue()
                    << ",menu=" << propertyOption.menu()
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkPropertyOption &propertyOption)
{
    QString name;
    istream >> name;
    QString description;
    istream >> description;
    QString help;
    istream >> help;
    AkPropertyOption::OptionType type;
    istream >> type;
    qreal min;
    istream >> min;
    qreal max;
    istream >> max;
    qreal step;
    istream >> step;
    QVariant defaultValue;
    istream >> defaultValue;
    AkMenu menu;
    istream >> menu;

    propertyOption = {name,
                      description,
                      help,
                      type,
                      min,
                      max,
                      step,
                      defaultValue,
                      menu};

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkPropertyOption &propertyOption)
{
    ostream << propertyOption.name();
    ostream << propertyOption.description();
    ostream << propertyOption.help();
    ostream << propertyOption.type();
    ostream << propertyOption.min();
    ostream << propertyOption.max();
    ostream << propertyOption.step();
    ostream << propertyOption.defaultValue();
    ostream << propertyOption.menu();

    return ostream;
}

#include "moc_akpropertyoption.cpp"
