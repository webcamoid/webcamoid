/* Webcamoid, webcam capture application.
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

#ifndef AKCPUFEATURES_H
#define AKCPUFEATURES_H

#include "aksimd.h"

class AkCpuFeatures;
class AkCpuFeaturesPrivate;

using AkCpuFeaturesPtr = QSharedPointer<AkCpuFeatures>;

class AKCOMMONS_EXPORT AkCpuFeatures: public QObject
{
    Q_OBJECT

    public:
        AkCpuFeatures(QObject *parent=nullptr);
        AkCpuFeatures(const AkCpuFeatures &other);
        ~AkCpuFeatures();

        Q_INVOKABLE static quint64 frequency();
        Q_INVOKABLE static size_t paralellizableBytesThreshold(int operationsPerByte,
                                                               AkSimd::SimdInstructionSet instructionSet=AkSimd::SimdInstructionSet_none,
                                                               int threads=2,
                                                               quint64 overheadNs=50000);

    private:
        AkCpuFeaturesPrivate *d;
};

Q_DECLARE_METATYPE(AkCpuFeatures)

#endif // AKCPUFEATURES_H
