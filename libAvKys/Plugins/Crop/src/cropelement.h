/* Webcamoid, webcam capture application. Crop Plug-in.
 * Copyright (C) 2022  Tj <hacker@iam.tj>
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

#ifndef CROPELEMENT_H
#define CROPELEMENT_H

// QRectF (not QRect) needed to interface with QML which uses real (float/double) values
#include <QRectF>
#include <akelement.h>

class CropElementPrivate;

class CropElement: public AkElement
{
    Q_OBJECT
    Q_DISABLE_COPY(CropElement)
    Q_PROPERTY(QRectF box
               READ box
               WRITE setBox
               RESET resetBox
               NOTIFY boxChanged)
    Q_PROPERTY(QRectF minimum
               READ minimum
               WRITE setMinimum
               RESET resetMinimum
               NOTIFY minimumChanged)
    // represents size of original frame; not changable externally
    Q_PROPERTY(QRectF maximum
               READ maximum
               NOTIFY maximumChanged)
    Q_PROPERTY(double aspectRatio
               READ aspectRatio
               NOTIFY aspectRatioChanged)

    public:

        CropElement();
        ~CropElement();

        Q_INVOKABLE QRectF box() const;
        Q_INVOKABLE QRectF maximum() const;
        Q_INVOKABLE QRectF minimum() const;
        Q_INVOKABLE double aspectRatio() const;

    private:
        CropElementPrivate *d;

    protected:
        enum Changed {
            eLeft, eTop, eWidth, eHeight
        };
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const;
        bool aspectRatioGood(QRect rect);
        int recalcHeight(const int width);
        int recalcWidth(const int height);
        AkPacket iVideoStream(const AkVideoPacket &packet);

    signals:
        void boxChanged(QRectF box);
        void minimumChanged(QRectF minimum);
        void maximumChanged(QRectF maximum);
        void aspectRatioChanged(double aspectRatio);

    public slots:
        void initLimits();
        void setBox(QRectF box);
        void setMinimum(QRectF minimum);
        void resetBox();
        void resetMinimum();

};

#endif // CROPELEMENT_H
