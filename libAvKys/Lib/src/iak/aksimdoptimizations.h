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

#ifndef AKSIMDOPTIMIZATIONS_H
#define AKSIMDOPTIMIZATIONS_H

#include <QObject>

#include "../akcommons.h"

class AkSimdOptimizations;

using AkSimdOptimizationsPtr = QSharedPointer<AkSimdOptimizations>;

class AKCOMMONS_EXPORT AkSimdOptimizations: public QObject
{
    Q_OBJECT

    public:
        explicit AkSimdOptimizations(QObject *parent=nullptr):
            QObject(parent)
        {
        }

        ~AkSimdOptimizations()
        {
        }

        Q_INVOKABLE virtual QFunctionPointer resolve(const char *functionName) const = 0;
};

#endif // AKSIMDOPTIMIZATIONS_H
