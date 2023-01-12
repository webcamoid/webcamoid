/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Chris Barth
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

#ifndef FACETRACKELEMENT_H
#define FACETRACKELEMENT_H

#include <akelement.h>

class FaceTrackElementPrivate;
class AkFrac;

class FaceTrackElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString haarFile
               READ haarFile
               WRITE setHaarFile
               RESET resetHaarFile
               NOTIFY haarFileChanged)
    Q_PROPERTY(QSize scanSize
               READ scanSize
               WRITE setScanSize
               RESET resetScanSize
               NOTIFY scanSizeChanged)
    Q_PROPERTY(int faceBucketSize
               READ faceBucketSize
               WRITE setFaceBucketSize
               RESET resetFaceBucketSize
               NOTIFY faceBucketSizeChanged)
    Q_PROPERTY(int faceBucketCount
               READ faceBucketCount
               WRITE setFaceBucketCount
               RESET resetFaceBucketCount
               NOTIFY faceBucketCountChanged)
    Q_PROPERTY(int expandRate
               READ expandRate
               WRITE setExpandRate
               RESET resetExpandRate
               NOTIFY expandRateChanged)
    Q_PROPERTY(int contractRate
               READ contractRate
               WRITE setContractRate
               RESET resetContractRate
               NOTIFY contractRateChanged)
    Q_PROPERTY(QRect facePadding
               READ facePadding
               WRITE setFacePadding
               RESET resetFacePadding
               NOTIFY facePaddingChanged)
    Q_PROPERTY(QRect faceMargin
               READ faceMargin
               WRITE setFaceMargin
               RESET resetFaceMargin
               NOTIFY faceMarginChanged)
    Q_PROPERTY(AkFrac aspectRatio
               READ aspectRatio
               WRITE setAspectRatio
               RESET resetAspectRatio
               NOTIFY aspectRatioChanged)
    Q_PROPERTY(bool overrideAspectRatio
               READ overrideAspectRatio
               WRITE setOverrideAspectRatio
               RESET resetOverrideAspectRatio
               NOTIFY overrideAspectRatioChanged)
    Q_PROPERTY(bool lockedViewport
               READ lockedViewport
               WRITE setLockedViewport
               RESET resetLockedViewport
               NOTIFY lockedViewportChanged)
    Q_PROPERTY(bool debugModeEnabled
               READ debugModeEnabled
               WRITE setDebugModeEnabled
               RESET resetDebugModeEnabled
               NOTIFY debugModeEnabledChanged)

    public:
        FaceTrackElement();
        ~FaceTrackElement();

        Q_INVOKABLE QString haarFile() const;
        Q_INVOKABLE QSize scanSize() const;
        Q_INVOKABLE int faceBucketSize() const;
        Q_INVOKABLE int faceBucketCount() const;
        Q_INVOKABLE int expandRate() const;
        Q_INVOKABLE int contractRate() const;
        Q_INVOKABLE QRect facePadding() const;
        Q_INVOKABLE QRect faceMargin() const;
        Q_INVOKABLE AkFrac aspectRatio() const;
        Q_INVOKABLE bool overrideAspectRatio() const;
        Q_INVOKABLE bool lockedViewport() const;
        Q_INVOKABLE bool debugModeEnabled() const;

    private:
        FaceTrackElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void haarFileChanged(const QString &haarFile);
        void scanSizeChanged(const QSize &scanSize);
        void faceBucketSizeChanged(int seconds);
        void faceBucketCountChanged(int count);
        void expandRateChanged(int rate);
        void contractRateChanged(int rate);
        void facePaddingChanged(const QRect &facePadding);
        void faceMarginChanged(const QRect &faceMargin);
        void aspectRatioChanged(const AkFrac &aspectRatio);
        void overrideAspectRatioChanged(bool overrideAspectRatio);
        void lockedViewportChanged(bool lockViewport);
        void debugModeEnabledChanged(bool enabled);

    public slots:
        void setHaarFile(const QString &haarFile);
        void setScanSize(const QSize &scanSize);
        void setFaceBucketSize(int seconds);
        void setFaceBucketCount(int count);
        void setExpandRate(int rate);
        void setContractRate(int rate);
        void setFacePadding(const QRect &facePadding);
        void setFaceMargin(const QRect &faceMargin);
        void setAspectRatio(const AkFrac &aspectRatio);
        void setOverrideAspectRatio(bool overrideAspectRatio);
        void setLockedViewport(bool lockViewport);
        void setDebugModeEnabled(bool enabled);
        void resetHaarFile();
        void resetScanSize();
        void resetFaceBucketSize();
        void resetFaceBucketCount();
        void resetExpandRate();
        void resetContractRate();
        void resetFacePadding();
        void resetFaceMargin();
        void resetAspectRatio();
        void resetOverrideAspectRatio();
        void resetLockedViewport();
        void resetDebugModeEnabled();
};

#endif // FACETRACKELEMENT_H
