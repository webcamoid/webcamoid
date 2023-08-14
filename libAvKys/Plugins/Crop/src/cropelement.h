/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <qrgb.h>
#include <akelement.h>

class CropElementPrivate;

class CropElement: public AkElement
{
    Q_OBJECT
    Q_DISABLE_COPY(CropElement)
    Q_PROPERTY(bool editMode
               READ editMode
               WRITE setEditMode
               RESET resetEditMode
               NOTIFY editModeChanged)
    Q_PROPERTY(bool relative
               READ relative
               WRITE setRelative
               RESET resetRelative
               NOTIFY relativeChanged)
    Q_PROPERTY(bool keepResolution
               READ keepResolution
               WRITE setKeepResolution
               RESET resetKeepResolution
               NOTIFY keepResolutionChanged)
    Q_PROPERTY(qreal left
               READ left
               WRITE setLeft
               RESET resetLeft
               NOTIFY leftChanged)
    Q_PROPERTY(qreal right
               READ right
               WRITE setRight
               RESET resetRight
               NOTIFY rightChanged)
    Q_PROPERTY(qreal top
               READ top
               WRITE setTop
               RESET resetTop
               NOTIFY topChanged)
    Q_PROPERTY(qreal bottom
               READ bottom
               WRITE setBottom
               RESET resetBottom
               NOTIFY bottomChanged)
    Q_PROPERTY(QRgb fillColor
               READ fillColor
               WRITE setFillColor
               RESET resetFillColor
               NOTIFY fillColorChanged)
    Q_PROPERTY(int frameWidth
               READ frameWidth
               NOTIFY frameWidthChanged)
    Q_PROPERTY(int frameHeight
               READ frameHeight
               NOTIFY frameHeightChanged)

    public:
        CropElement();
        ~CropElement();

        Q_INVOKABLE bool editMode() const;
        Q_INVOKABLE bool relative() const;
        Q_INVOKABLE bool keepResolution() const;
        Q_INVOKABLE qreal left() const;
        Q_INVOKABLE qreal right() const;
        Q_INVOKABLE qreal top() const;
        Q_INVOKABLE qreal bottom() const;
        Q_INVOKABLE QRgb fillColor() const;
        Q_INVOKABLE int frameWidth() const;
        Q_INVOKABLE int frameHeight() const;

    private:
        CropElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void editModeChanged(bool editMode);
        void relativeChanged(bool relative);
        void keepResolutionChanged(bool keepResolution);
        void leftChanged(qreal left);
        void rightChanged(qreal right);
        void topChanged(qreal top);
        void bottomChanged(qreal bottom);
        void fillColorChanged(QRgb fillColor);
        void frameWidthChanged(int frameWidth);
        void frameHeightChanged(int frameHeight);

    public slots:
        void setEditMode(bool editMode);
        void setRelative(bool relative);
        void setKeepResolution(bool keepResolution);
        void setLeft(qreal left);
        void setRight(qreal right);
        void setTop(qreal top);
        void setBottom(qreal bottom);
        void setFillColor(QRgb fillColor);
        void resetEditMode();
        void resetRelative();
        void resetKeepResolution();
        void resetLeft();
        void resetRight();
        void resetTop();
        void resetBottom();
        void resetFillColor();
        void reset();
};

#endif // CROPELEMENT_H
