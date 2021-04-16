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

#ifndef FACEDETECTELEMENT_H
#define FACEDETECTELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class FaceDetectElementPrivate;

class FaceDetectElement: public AkElement
{
    Q_OBJECT
        Q_ENUMS(MarkerType)
        Q_PROPERTY(QString haarFile
                   READ haarFile
                   WRITE setHaarFile
                   RESET resetHaarFile
                   NOTIFY haarFileChanged)
        Q_PROPERTY(QString markerType
                   READ markerType
                   WRITE setMarkerType
                   RESET resetMarkerType
                   NOTIFY markerTypeChanged)
        Q_PROPERTY(QRgb markerColor
                   READ markerColor
                   WRITE setMarkerColor
                   RESET resetMarkerColor
                   NOTIFY markerColorChanged)
        Q_PROPERTY(int markerWidth
                   READ markerWidth
                   WRITE setMarkerWidth
                   RESET resetMarkerWidth
                   NOTIFY markerWidthChanged)
        Q_PROPERTY(QString markerStyle
                   READ markerStyle
                   WRITE setMarkerStyle
                   RESET resetMarkerStyle
                   NOTIFY markerStyleChanged)
        Q_PROPERTY(QString markerImage
                   READ markerImage
                   WRITE setMarkerImage
                   RESET resetMarkerImage
                   NOTIFY markerImageChanged)
        Q_PROPERTY(QString backgroundImage
                   READ backgroundImage
                   WRITE setBackgroundImage
                   RESET resetBackgroundImage
                   NOTIFY backgroundImageChanged)
        Q_PROPERTY(QSize pixelGridSize
                   READ pixelGridSize
                   WRITE setPixelGridSize
                   RESET resetPixelGridSize
                   NOTIFY pixelGridSizeChanged)
        Q_PROPERTY(int blurRadius
                   READ blurRadius
                   WRITE setBlurRadius
                   RESET resetBlurRadius
                   NOTIFY blurRadiusChanged)
        Q_PROPERTY(QSize scanSize
                   READ scanSize
                   WRITE setScanSize
                   RESET resetScanSize
                   NOTIFY scanSizeChanged)
        Q_PROPERTY(qreal scale
                   READ scale
                   WRITE setScale
                   RESET resetScale
                   NOTIFY scaleChanged)
        Q_PROPERTY(qreal rScale
                   READ rScale
                   WRITE setRScale
                   RESET resetRScale
                   NOTIFY rScaleChanged)
        Q_PROPERTY(bool smootheEdges
                   READ smootheEdges
                   WRITE setSmootheEdges
                   RESET resetSmootheEdges
                   NOTIFY smootheEdgesChanged)
        Q_PROPERTY(int hOffset
                   READ hOffset
                   WRITE setHOffset
                   RESET resetHOffset
                   NOTIFY hOffsetChanged)
        Q_PROPERTY(int vOffset
                   READ vOffset
                   WRITE setVOffset
                   RESET resetVOffset
                   NOTIFY vOffsetChanged)
        Q_PROPERTY(int wAdjust
                   READ wAdjust
                   WRITE setWAdjust
                   RESET resetWAdjust
                   NOTIFY wAdjustChanged)
        Q_PROPERTY(int hAdjust
                   READ hAdjust
                   WRITE setHAdjust
                   RESET resetHAdjust
                   NOTIFY hAdjustChanged)
        Q_PROPERTY(int rWAdjust
                   READ rWAdjust
                   WRITE setRWAdjust
                   RESET resetRWAdjust
                   NOTIFY rWAdjustChanged)
        Q_PROPERTY(int rHAdjust
                   READ rHAdjust
                   WRITE setRHAdjust
                   RESET resetRHAdjust
                   NOTIFY rHAdjustChanged)
        Q_PROPERTY(int rHRadius
                   READ rHRadius
                   WRITE setRHRadius
                   RESET resetRHRadius
                   NOTIFY rHRadiusChanged)
        Q_PROPERTY(int rVRadius
                   READ rVRadius
                   WRITE setRVRadius
                   RESET resetRVRadius
                   NOTIFY rVRadiusChanged)

    public:
        enum MarkerType
        {
            MarkerTypeRectangle,
            MarkerTypeEllipse,
            MarkerTypeImage,
            MarkerTypePixelate,
            MarkerTypeBlur,
            MarkerTypeBlurOuter,
            MarkerTypeImageOuter
        };

        FaceDetectElement();
        ~FaceDetectElement();

        Q_INVOKABLE QString haarFile() const;
        Q_INVOKABLE QString markerType() const;
        Q_INVOKABLE QRgb markerColor() const;
        Q_INVOKABLE int markerWidth() const;
        Q_INVOKABLE QString markerStyle() const;
        Q_INVOKABLE QString markerImage() const;
        Q_INVOKABLE QString backgroundImage() const;
        Q_INVOKABLE QSize pixelGridSize() const;
        Q_INVOKABLE int blurRadius() const;
        Q_INVOKABLE QSize scanSize() const;
        Q_INVOKABLE QVector<QRect> detectFaces(const AkVideoPacket &packet);
        Q_INVOKABLE qreal scale() const;
        Q_INVOKABLE qreal rScale() const;
        Q_INVOKABLE bool smootheEdges() const;
        Q_INVOKABLE int hOffset() const;
        Q_INVOKABLE int vOffset() const;
        Q_INVOKABLE int wAdjust() const;
        Q_INVOKABLE int hAdjust() const;
        Q_INVOKABLE int rWAdjust() const;
        Q_INVOKABLE int rHAdjust() const;
        Q_INVOKABLE int rHRadius() const;
        Q_INVOKABLE int rVRadius() const;


    private:
        FaceDetectElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;
        AkPacket iVideoStream(const AkVideoPacket &packet);

    signals:
        void haarFileChanged(const QString &haarFile);
        void markerTypeChanged(const QString &markerType);
        void markerColorChanged(QRgb markerColor);
        void markerWidthChanged(int markerWidth);
        void markerStyleChanged(const QString &markerStyle);
        void markerImageChanged(const QString &markerImage);
        void backgroundImageChanged(const QString &backgroundImage);
        void pixelGridSizeChanged(const QSize &pixelGridSize);
        void blurRadiusChanged(int blurRadius);
        void scanSizeChanged(const QSize &scanSize);
        void scaleChanged(const qreal scale);
        void rScaleChanged(const qreal rScale);
        void smootheEdgesChanged(const bool smootheEdges);
        void hOffsetChanged(const int hOffset);
        void vOffsetChanged(const int vOffset);
        void wAdjustChanged(const int wAdjust);
        void hAdjustChanged(const int hAdjust);
        void rWAdjustChanged(const int rWAdjust);
        void rHAdjustChanged(const int rHAdjust);
        void rHRadiusChanged(const int rHRadius);
        void rVRadiusChanged(const int rVRadius);

    public slots:
        void setHaarFile(const QString &haarFile);
        void setMarkerType(const QString &markerType);
        void setMarkerColor(QRgb markerColor);
        void setMarkerWidth(int markerWidth);
        void setMarkerStyle(const QString &markerStyle);
        void setMarkerImage(const QString &markerImage);
        void setBackgroundImage(const QString &backgroundImage);
        void setPixelGridSize(const QSize &pixelGridSize);
        void setBlurRadius(int blurRadius);
        void setScanSize(const QSize &scanSize);
        void setScale(const qreal scale);
        void setRScale(const qreal rScale);
        void setSmootheEdges(const bool smootheEdges);
        void setHOffset(const int hOffset);
        void setVOffset(const int vOffset);
        void setWAdjust(const int wAdjust);
        void setHAdjust(const int hAdjust);
        void setRWAdjust(const int rWAdjust);
        void setRHAdjust(const int rHAdjust);
        void setRHRadius(const int rHRadius);
        void setRVRadius(const int rVRadius);
        void resetHaarFile();
        void resetMarkerType();
        void resetMarkerColor();
        void resetMarkerWidth();
        void resetMarkerStyle();
        void resetMarkerImage();
        void resetBackgroundImage();
        void resetPixelGridSize();
        void resetBlurRadius();
        void resetScanSize();
        void resetScale();
        void resetRScale();
        void resetSmootheEdges();
        void resetHOffset();
        void resetVOffset();
        void resetWAdjust();
        void resetHAdjust();
        void resetRWAdjust();
        void resetRHAdjust();
        void resetRHRadius();
        void resetRVRadius();
};

#endif // FACEDETECTELEMENT_H
