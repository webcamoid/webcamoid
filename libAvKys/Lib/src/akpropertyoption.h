/* Webcamoid, webcam capture application.
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

#ifndef AKPROPERTYOPTION_H
#define AKPROPERTYOPTION_H

#include "akmenuoption.h"

class AkPropertyOption;
class AkPropertyOptionPrivate;
class QDataStream;

using AkPropertyOptions = QList<AkPropertyOption>;

class AKCOMMONS_EXPORT AkPropertyOption: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name
               READ name
               CONSTANT)
    Q_PROPERTY(QString description
               READ description
               CONSTANT)
    Q_PROPERTY(QString help
               READ help
               CONSTANT)
    Q_PROPERTY(OptionType type
               READ type
               CONSTANT)
    Q_PROPERTY(qreal min
               READ min
               CONSTANT)
    Q_PROPERTY(qreal max
               READ max
               CONSTANT)
    Q_PROPERTY(qreal step
               READ step
               CONSTANT)
    Q_PROPERTY(QVariant defaultValue
               READ defaultValue
               CONSTANT)
    Q_PROPERTY(AkMenu menu
               READ menu
               CONSTANT)

    public:
        enum OptionType
        {
            OptionType_Unknown,
            OptionType_Number,
            OptionType_Boolean,
            OptionType_Flags,
            OptionType_String,
            OptionType_Frac,
            OptionType_ImageSize,
            OptionType_Color,
        };
        Q_ENUM(OptionType)

        AkPropertyOption(QObject *parent=nullptr);
        AkPropertyOption(const QString &name,
                         const QString &description,
                         const QString &help,
                         OptionType type,
                         qreal min,
                         qreal max,
                         qreal step,
                         const QVariant &defaultValue,
                         const AkMenu &menu);
        AkPropertyOption(const AkPropertyOption &other);
        virtual ~AkPropertyOption();
        AkPropertyOption &operator =(const AkPropertyOption &other);
        bool operator ==(const AkPropertyOption &other) const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const QString &name,
                                           const QString &description,
                                           const QString &help,
                                           OptionType type,
                                           qreal min,
                                           qreal max,
                                           qreal step,
                                           const QVariant &defaultValue,
                                           const AkMenu &menu);
        Q_INVOKABLE static QObject *create(const AkPropertyOption &other);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString description() const;
        Q_INVOKABLE QString help() const;
        Q_INVOKABLE OptionType type() const;
        Q_INVOKABLE qreal min() const;
        Q_INVOKABLE qreal max() const;
        Q_INVOKABLE qreal step() const;
        Q_INVOKABLE QVariant defaultValue() const;
        Q_INVOKABLE AkMenu menu() const;

    private:
        AkPropertyOptionPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkPropertyOption &propertyOption);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkPropertyOption &propertyOption);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkPropertyOption &propertyOption);

Q_DECLARE_METATYPE(AkPropertyOption)
Q_DECLARE_METATYPE(AkPropertyOption::OptionType)

#endif // AKPROPERTYOPTION_H
