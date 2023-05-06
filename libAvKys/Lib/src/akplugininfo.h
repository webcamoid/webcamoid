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

#ifndef AKPLUGININFO_H
#define AKPLUGININFO_H

#include <QObject>

#include "akcommons.h"

class AkPluginInfoPrivate;
class AkPluginManager;

class AKCOMMONS_EXPORT AkPluginInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name
               READ name
               CONSTANT)
    Q_PROPERTY(QString description
               READ description
               CONSTANT)
    Q_PROPERTY(QString id
               READ id
               CONSTANT)
    Q_PROPERTY(QString path
               READ path
               CONSTANT)
    Q_PROPERTY(QStringList implements
               READ implements
               CONSTANT)
    Q_PROPERTY(QStringList depends
               READ depends
               CONSTANT)
    Q_PROPERTY(QString type
               READ type
               CONSTANT)
    Q_PROPERTY(int priority
               READ priority
               CONSTANT)

    public:
        AkPluginInfo(QObject *parent=nullptr);
        AkPluginInfo(const AkPluginInfo &other);
        virtual ~AkPluginInfo();
        AkPluginInfo &operator =(const AkPluginInfo &other);
        bool operator ==(const AkPluginInfo &other) const;
        bool operator !=(const AkPluginInfo &other) const;
        operator bool() const;

        Q_INVOKABLE static QObject *create(const AkPluginInfo &info);
        Q_INVOKABLE QVariant toVariant() const;
        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString description() const;
        Q_INVOKABLE QString id() const;
        Q_INVOKABLE QString path() const;
        Q_INVOKABLE QStringList implements() const;
        Q_INVOKABLE QStringList depends() const;
        Q_INVOKABLE QString type() const;
        Q_INVOKABLE int priority() const;

    private:
        AkPluginInfoPrivate *d;
        AkPluginInfo(const QVariantMap &metaData);

    public Q_SLOTS:
        static void registerTypes();

    friend class AkPluginManager;
    friend QDataStream &operator >>(QDataStream &istream, AkPluginInfo &info);
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkPluginInfo &info);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkPluginInfo &info);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkPluginInfo &infp);

Q_DECLARE_METATYPE(AkPluginInfo)

#endif // AKPLUGININFO_H
