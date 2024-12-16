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

#ifndef FILLAUDIOGAPSELEMENT_H
#define FILLAUDIOGAPSELEMENT_H

#include <iak/akelement.h>

class FillAudioGapsElementPrivate;
class AkAudioCaps;
class AkFrac;

class FillAudioGapsElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(AkAudioCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(int outputSamples
               READ outputSamples
               WRITE setOutputSamples
               RESET resetOutputSamples
               NOTIFY outputSamplesChanged)
    Q_PROPERTY(bool fillGaps
               READ fillGaps
               WRITE setFillGaps
               RESET resetFillGaps
               NOTIFY fillGapsChanged)

    public:
        FillAudioGapsElement();
        ~FillAudioGapsElement();

        Q_INVOKABLE AkAudioCaps outputCaps() const;
        Q_INVOKABLE int outputSamples() const;
        Q_INVOKABLE bool fillGaps() const;

    private:
        FillAudioGapsElementPrivate *d;

    protected:
        AkPacket iAudioStream(const AkAudioPacket &packet) override;

    signals:
        void outputCapsChanged(const AkAudioCaps &outputCaps);
        void outputSamplesChanged(int outputSamples);
        void fillGapsChanged(bool fillGaps);

    public slots:
        void setOutputCaps(const AkAudioCaps &outputCaps);
        void setOutputSamples(int outputSamples);
        void setFillGaps(bool fillGaps);
        void resetOutputCaps();
        void resetOutputSamples();
        void resetFillGaps();
        bool setState(AkElement::ElementState state) override;
};

#endif // FILLAUDIOGAPSELEMENT_H
