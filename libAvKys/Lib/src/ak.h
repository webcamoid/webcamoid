/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef AK_H
#define AK_H

#include <QObject>

#include "akcommons.h"

class QQmlEngine;
class AkPluginManager;

class AKCOMMONS_EXPORT Ak: public QObject
{
    Q_OBJECT

    public:
        Ak();
        Ak(const Ak &other);
        ~Ak() = default;

        Q_INVOKABLE static void registerTypes();
        Q_INVOKABLE static qint64 id();
        Q_INVOKABLE static QString platform();
        Q_INVOKABLE static void setQmlEngine(QQmlEngine *engine);
};

Q_DECLARE_METATYPE(Ak)

#endif // AK_H
