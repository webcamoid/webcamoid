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

#ifndef AKMENUOPTION_H
#define AKMENUOPTION_H

#include <QObject>

#include "akcommons.h"

class AkMenuOption;
class AkMenuOptionPrivate;
class QDataStream;

using AkMenu = QList<AkMenuOption>;

class AKCOMMONS_EXPORT AkMenuOption: public QObject
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
    Q_PROPERTY(QVariant value
               READ value
               CONSTANT)

    public:
        AkMenuOption(QObject *parent=nullptr);
        AkMenuOption(const QString &name,
                     const QString &description,
                     const QString &help,
                     const QVariant &value);
        AkMenuOption(const AkMenuOption &other);
        virtual ~AkMenuOption();
        AkMenuOption &operator =(const AkMenuOption &other);
        bool operator ==(const AkMenuOption &other) const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const QString &name,
                                           const QString &description,
                                           const QString &help,
                                           const QVariant &value);
        Q_INVOKABLE static QObject *create(const AkMenuOption &other);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString description() const;
        Q_INVOKABLE QString help() const;
        Q_INVOKABLE QVariant value() const;

    private:
        AkMenuOptionPrivate *d;

    public Q_SLOTS:
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkMenuOption &menuOption);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkMenuOption &menuOption);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkMenuOption &menuOption);

Q_DECLARE_METATYPE(AkMenuOption)

#endif // AKMENUOPTION_H
