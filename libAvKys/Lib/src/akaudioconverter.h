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

#ifndef AKAUDIOCONVERTER_H
#define AKAUDIOCONVERTER_H

#include "akaudiocaps.h"

class AkAudioConverterPrivate;
class AkAudioPacket;

class AKCOMMONS_EXPORT AkAudioConverter: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(AkAudioConverter::ResampleMethod resampleMethod
               READ resampleMethod
               WRITE setResampleMethod
               RESET resetResampleMethod
               NOTIFY resampleMethodChanged)

    public:
        enum ResampleMethod
        {
            ResampleMethod_Fast,
            ResampleMethod_Linear,
            ResampleMethod_Quadratic
        };
        Q_ENUM(ResampleMethod)

        AkAudioConverter(const AkAudioCaps &outputCaps={},
                         QObject *parent=nullptr);
        AkAudioConverter(const AkAudioConverter &other);
        ~AkAudioConverter();
        AkAudioConverter &operator =(const AkAudioConverter &other);

        Q_INVOKABLE static QObject *create();

        Q_INVOKABLE AkAudioCaps outputCaps() const;
        Q_INVOKABLE AkAudioConverter::ResampleMethod resampleMethod() const;
        Q_INVOKABLE static bool canConvertFormat(AkAudioCaps::SampleFormat input,
                                                 AkAudioCaps::SampleFormat output);
        Q_INVOKABLE AkAudioPacket convert(const AkAudioPacket &packet);
        Q_INVOKABLE AkAudioPacket scale(const AkAudioPacket &packet,
                                        int samples) const;

    private:
        AkAudioConverterPrivate *d;

    Q_SIGNALS:
        void outputCapsChanged(const AkAudioCaps &outputCaps);
        void resampleMethodChanged(AkAudioConverter::ResampleMethod resampleMethod);

    public Q_SLOTS:
        void setOutputCaps(const AkAudioCaps &outputCaps);
        void setResampleMethod(AkAudioConverter::ResampleMethod resampleMethod);
        void resetOutputCaps();
        void resetResampleMethod();
        void reset();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkAudioConverter::ResampleMethod method);

Q_DECLARE_METATYPE(AkAudioConverter)
Q_DECLARE_METATYPE(AkAudioConverter::ResampleMethod)

#endif // AKAUDIOCONVERTER_H
