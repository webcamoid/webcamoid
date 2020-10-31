/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef SCALEELEMENT_H
#define SCALEELEMENT_H

#include <akelement.h>

class ScaleElementPrivate;

class ScaleElement: public AkElement
{
    Q_OBJECT
    Q_DISABLE_COPY(ScaleElement)
    Q_ENUMS(ScalingMode)
    Q_ENUMS(AspectRatioMode)
    Q_PROPERTY(int width
               READ width
               WRITE setWidth
               RESET resetWidth
               NOTIFY widthChanged)
    Q_PROPERTY(int height
               READ height
               WRITE setHeight
               RESET resetHeight
               NOTIFY heightChanged)
    Q_PROPERTY(ScalingMode scaling
               READ scaling
               WRITE setScaling
               RESET resetScaling
               NOTIFY scalingChanged)
    Q_PROPERTY(AspectRatioMode aspectRatio
               READ aspectRatio
               WRITE setAspectRatio
               RESET resetAspectRatio
               NOTIFY aspectRatioChanged)

    public:
        enum ScalingMode {
            Fast,
            Linear
        };
        enum AspectRatioMode {
            Ignore,
            Keep,
            Expanding
        };

        ScaleElement();
        ~ScaleElement();

        Q_INVOKABLE int width() const;
        Q_INVOKABLE int height() const;
        Q_INVOKABLE ScalingMode scaling() const;
        Q_INVOKABLE AspectRatioMode aspectRatio() const;

    private:
        ScaleElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;
        AkPacket iVideoStream(const AkVideoPacket &packet);

    signals:
        void widthChanged(int width);
        void heightChanged(int height);
        void scalingChanged(ScalingMode scaling);
        void aspectRatioChanged(AspectRatioMode aspectRatio);

    public slots:
        void setWidth(int width);
        void setHeight(int height);
        void setScaling(ScalingMode scaling);
        void setAspectRatio(AspectRatioMode aspectRatio);
        void resetWidth();
        void resetHeight();
        void resetScaling();
        void resetAspectRatio();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, ScaleElement::ScalingMode &scaling);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, ScaleElement::ScalingMode scaling);
Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, ScaleElement::AspectRatioMode &aspectRatioMode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, ScaleElement::AspectRatioMode aspectRatioMode);

Q_DECLARE_METATYPE(ScaleElement::ScalingMode)
Q_DECLARE_METATYPE(ScaleElement::AspectRatioMode)

#endif // SCALEELEMENT_H
