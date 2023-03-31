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

#ifndef DELAYGRABELEMENT_H
#define DELAYGRABELEMENT_H

#include <akelement.h>

class DelayGrabElementPrivate;

class DelayGrabElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(DelayGrabMode mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int blockSize
               READ blockSize
               WRITE setBlockSize
               RESET resetBlockSize
               NOTIFY blockSizeChanged)
    Q_PROPERTY(int nFrames
               READ nFrames
               WRITE setNFrames
               RESET resetNFrames
               NOTIFY nFramesChanged)

    public:
        enum DelayGrabMode
        {
            // Random delay with square distribution
            DelayGrabModeRandomSquare,
            // Vertical stripes of increasing delay outward from center
            DelayGrabModeVerticalIncrease,
            // Horizontal stripes of increasing delay outward from center
            DelayGrabModeHorizontalIncrease,
            // Rings of increasing delay outward from center
            DelayGrabModeRingsIncrease
        };
        Q_ENUM(DelayGrabMode)

        DelayGrabElement();
        ~DelayGrabElement();

        Q_INVOKABLE DelayGrabMode mode() const;
        Q_INVOKABLE int blockSize() const;
        Q_INVOKABLE int nFrames() const;

    private:
        DelayGrabElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(DelayGrabMode mode);
        void blockSizeChanged(int blockSize);
        void nFramesChanged(int nFrames);
        void frameSizeChanged(const QSize &frameSize);

    public slots:
        void setMode(DelayGrabMode mode);
        void setBlockSize(int blockSize);
        void setNFrames(int nFrames);
        void resetMode();
        void resetBlockSize();
        void resetNFrames();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, DelayGrabElement::DelayGrabMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, DelayGrabElement::DelayGrabMode mode);

Q_DECLARE_METATYPE(DelayGrabElement::DelayGrabMode)

#endif // DELAYGRABELEMENT_H
