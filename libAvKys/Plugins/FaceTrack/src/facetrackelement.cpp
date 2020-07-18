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

#include <QDebug>
#include <QVariant>
#include <QMap>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QPainter>
#include <QQmlContext>
#include <QQueue>
#include <akpacket.h>
#include <akvideopacket.h>

#include "facetrackelement.h"
#include "../FaceDetect/src/facedetectelement.h"

class FaceTrackElementPrivate
{
public:
    QString m_haarFile;
    QSize m_scanSize;
    int m_faceBucketSize;
    QVector<QRect> m_faceBuckets;
    int m_expandRate;
    int m_contractRate;
    QRect m_faceMargin;
    QRect m_facePadding;
    QSize m_aspectRatio;
    bool m_overrideAspectRatio;
    bool m_lockedViewport;
    bool m_debugModeEnabled;

    QRect m_lastBounds;
    QSharedPointer<FaceDetectElement> m_faceDetectFilter;

    QList<QColor> m_colors = {
        QColor(255, 179, 0), // Vivid Yellow
        QColor(128, 62, 117), // Strong Purple
        QColor(255, 104, 0), // Vivid Orange
        QColor(166, 189, 215), // Very Light Blue
        QColor(193, 0, 32), // Vivid Red
        QColor(206, 162, 98), // Grayish Yellow
        QColor(129, 112, 102), // Medium Gray

        // The following are not good for people with defective color vision
        QColor(0, 125, 52), // Vivid Green
        QColor(246, 118, 142), // Strong Purplish Pink
        QColor(0, 83, 138), // Strong Blue
        QColor(255, 122, 92), // Strong Yellowish Pink
        QColor(83, 55, 122), // Strong Violet
        QColor(255, 142, 0), // Vivid Orange Yellow
        QColor(179, 40, 81), // Strong Purplish Red
        QColor(244, 200, 0), // Vivid Greenish Yellow
        QColor(127, 24, 13), // Strong Reddish Brown
        QColor(147, 170, 0), // Vivid Yellowish Green
        QColor(89, 51, 21), // Deep Yellowish Brown
        QColor(241, 58, 19), // Vivid Reddish Orange
        QColor(35, 44, 22), // Dark Olive Green
    };
};

FaceTrackElement::FaceTrackElement(): AkElement()
{
    this->d = new FaceTrackElementPrivate;
    this->d->m_faceDetectFilter =
            AkElement::create<FaceDetectElement>("FaceDetect");

    // Set defaults
    this->resetHaarFile();
    this->resetScanSize();
    this->resetFaceBucketSize();
    this->resetFaceBucketCount();
    this->resetExpandRate();
    this->resetContractRate();
    this->resetFacePadding();
    this->resetFaceMargin();
    this->resetAspectRatio();
    this->resetOverrideAspectRatio();
    this->resetLockedViewport();
    this->resetDebugModeEnabled();

}

FaceTrackElement::~FaceTrackElement()
{
    delete this->d;
}

QString FaceTrackElement::haarFile() const
{
    return this->d->m_haarFile;
}

QSize FaceTrackElement::scanSize() const
{
    return this->d->m_scanSize;
}

int FaceTrackElement::faceBucketSize() const
{
    return this->d->m_faceBucketSize;
}

int FaceTrackElement::faceBucketCount() const
{
    return this->d->m_faceBuckets.length();
}

int FaceTrackElement::expandRate() const
{
    return this->d->m_expandRate;
}

int FaceTrackElement::contractRate() const
{
    return this->d->m_contractRate;
}

QRect FaceTrackElement::facePadding() const
{
    return this->d->m_facePadding;
}

QRect FaceTrackElement::faceMargin() const
{
    return this->d->m_faceMargin;
}

QSize FaceTrackElement::aspectRatio() const
{
    return this->d->m_aspectRatio;
}

bool FaceTrackElement::overrideAspectRatio() const
{
    return this->d->m_overrideAspectRatio;
}

bool FaceTrackElement::lockedViewport() const
{
    return this->d->m_lockedViewport;
}

bool FaceTrackElement::debugModeEnabled() const
{
    return this->d->m_debugModeEnabled;
}

QRect FaceTrackElement::calculateNewBounds(const QRect &targetBounds,
                                           const QSize &maxCropSize,
                                           const QSize &srcSize) const
{
    QRect newBounds;

    // Slowly change the bounds
    // Can't use addition/subtraction, need to use ratios,
    // or we'll pass our target and get jittery
    auto l = this->d->m_lastBounds;
    auto t = targetBounds;
    double xRate = (double(this->d->m_expandRate)) / 100;
    double cRate = (double(-this->d->m_contractRate)) / 100;

    // Apply the expand/contract rates to get the new bounds
    newBounds
            .setCoords(l.left() - ((t.left() < l.left() ? xRate : cRate) * abs(t.left() - l.left())),
                       l.top() - ((t.top() < l.top() ? xRate : cRate) * abs(t.top() - l.top())),
                       l.right() + ((t.right() > l.right() ? xRate : cRate) * abs(t.right() - l.right())),
                       l.bottom() + ((t.bottom() > l.bottom() ? xRate : cRate) * abs(t.bottom() - l.bottom())));

    // Make sure the new bounds are the correct aspect ratio
    auto ar = this->aspectRatio();
    int proposedWidth(qMax(newBounds.width(),
                           newBounds.height() * ar.width() / ar.height()));
    int proposedHeight(qMax(newBounds.height(),
                            newBounds.width() * ar.height() / ar.width()));

    if (proposedWidth > maxCropSize.width()) {
        proposedWidth = maxCropSize.width();
        proposedHeight = maxCropSize.width() * ar.height() / ar.width();
    }

    if (proposedHeight > maxCropSize.height()) {
        proposedHeight = maxCropSize.height();
        proposedWidth = maxCropSize.height() * ar.width() / ar.height();
    }

    // Make sure that we pan the image gradually
    QLine centerline(targetBounds.center(), this->d->m_lastBounds.center());

    // Make sure we correctly center and size the new bounds
    int left(int(centerline.center().x() - (proposedWidth / 2)));
    newBounds.setLeft(qMax(0, left));

    int right(proposedWidth + newBounds.left());
    newBounds.setRight(qMin(srcSize.width(), right));

    left = newBounds.left() - (proposedWidth - newBounds.width());
    newBounds.setLeft(left);

    int top(int(centerline.center().y() - (proposedHeight / 2)));
    newBounds.setTop(qMax(0, top));

    int bottom(proposedHeight + newBounds.top());
    newBounds.setBottom(qMin(srcSize.height(), bottom));

    top = newBounds.top() - (proposedHeight - newBounds.height());
    newBounds.setTop(top);

    this->d->m_lastBounds = QRect(newBounds);
    return newBounds;
}

void FaceTrackElement::collectFaces(const QVector<QRect> &vecFaces)
{
    /*
     * Track faces grouped by the inverse of the scan frequency
     * so that we can mitigate the effect of flashing face detection.
     * This will cause each bucket to contain `m_faceBucketSize`
     * number of seconds of face scans grouped together
     */

    int now = QDateTime::currentSecsSinceEpoch() / this->faceBucketSize();
    int nextTimeSlot = (now + 1) % this->d->m_faceBuckets.length();
    int timeSlot = now % this->d->m_faceBuckets.length();

    auto size = this->scanSize();
    auto pad = this->facePadding();
    auto margin = this->faceMargin();

    for (const QRect &face: vecFaces) {
        // Add a padding around an incoming face
        QRect head(face);
        // Forehead
        head.setTop(qMax(
                        0,
                        head.top() - int(face.height() * pad.top() / 100))
                    );
        // Neck
        head.setHeight(qMin(
                           size.height(),
                           head.height() + int(face.height() * pad.bottom() / 100))
                       );
        // Left shoulder
        head.setLeft(qMax(
                         0,
                         head.left() - int(face.width() * pad.left() / 100))
                     );
        // Right shoulder
        head.setWidth(qMin(
                          size.width(),
                          head.width() + int(face.width() * pad.right() / 100))
                      );

        if (this->d->m_faceBuckets[timeSlot].isNull()) {
            this->d->m_faceBuckets[timeSlot] = head;
        } else {
            /*
             * Include a "margin" option that will only include the new head
             * if it is outside the old bucket + margin. This will further
             * help reduce jittery or very small movements of the crop/pan.
             */
            QRect headMargin;
            auto b = this->d->m_faceBuckets[timeSlot];
            headMargin
                    .setCoords(b.left() - int(head.left() * margin.left() / 100),
                               b.top() - int(head.top() * margin.top() / 100),
                               b.right() + int(head.right() * margin.right() / 100),
                               b.bottom() + int(head.bottom() * margin.bottom() / 100));
            // If `head` is completely inside `headMargin`,
            // then don't merge it with the bucket
            if (!(headMargin.contains(head.topLeft())
                  && headMargin.contains(head.bottomRight()))) {
                this->d->m_faceBuckets[timeSlot] =
                        this->d->m_faceBuckets[timeSlot].united(head);
            }
        }

        // Degrade the last occupied slot by intersecting it with the current one
        if (!this->d->m_faceBuckets[nextTimeSlot].isNull()) {
            this->d->m_faceBuckets[nextTimeSlot] =
                    this->d->m_faceBuckets[nextTimeSlot].intersected(head);
        }
    }
}

QString FaceTrackElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FaceTrack/share/qml/main.qml");
}

void FaceTrackElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FaceTrack",
                                const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket FaceTrackElement::iVideoStream(const AkVideoPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty()
            || scanSize.isEmpty()) {
        akSend(packet)
    }

    auto src = packet.toImage();

    if (src.isNull())
        return AkPacket();
    else
        if (!this->overrideAspectRatio())
            this->setAspectRatio(src.size());

    QImage oFrame = src.convertToFormat(QImage::Format_ARGB32);
    qreal scale = 1;

    QImage scanFrame(src.scaled(scanSize, Qt::KeepAspectRatio));

    if (scanFrame.width() == scanSize.width())
        scale = qreal(src.width()) / scanSize.width();
    else
        scale = qreal(src.height()) / scanSize.height();

    QRect bounds;

    if (this->d->m_lastBounds.isNull())
        this->d->m_lastBounds = QRect(src.rect());

    if (this->lockedViewport()) {
        bounds = this->d->m_lastBounds;
    } else {
        QVector<QRect> detectedFaces;

        QMetaObject::invokeMethod(this->d->m_faceDetectFilter.data(),
                                  "detectFaces",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(QVector<QRect>, detectedFaces),
                                  Q_ARG(AkVideoPacket, packet));

        if (detectedFaces.length() > 0)
            this->collectFaces(detectedFaces);

        QPen pen;
        QPainter painter;
        int currentColorIndex = 0;
        auto debugging = this->debugModeEnabled();

        if (debugging) {
            pen.setStyle(Qt::SolidLine);
            pen.setWidth(2);

            painter.begin(&oFrame);
        }

        for (int i = 0; i < this->d->m_faceBuckets.length(); i++) {
            QRect face(this->d->m_faceBuckets[i]);

            if (!face.isNull()) {
                QRect scaledFace;
                scaledFace.setCoords(scale * face.left(),
                                     scale * face.top(),
                                     scale * face.right(),
                                     scale * face.bottom());
                bounds = bounds.united(scaledFace);

                if (debugging) {
                    pen.setColor(this->d->m_colors[currentColorIndex]);
                    painter.setPen(pen);
                    painter.drawRect(scaledFace);
                }
            }

            if (debugging) {
                currentColorIndex += 1;

                if (currentColorIndex >= this->d->m_colors.length()) {
                    currentColorIndex = 0;
                    pen.setWidth(pen.width() + 2);
                }
            }
        }

        // Don't allow bounds outside the frame, even if the face does
        bounds.setCoords(qMax(0, bounds.left()),
                         qMax(0, bounds.top()),
                         qMin(src.width(), bounds.right()),
                         qMin(src.height(), bounds.bottom()));

        if (debugging) {
            // Draw a boarder so we can see what is going on
            pen.setColor(Qt::white);
            pen.setWidth(pen.width() + 2);

            painter.setPen(pen);
            painter.drawRect(bounds);
            painter.end();
        }
    }

    // Calcuate the maximum size allowed given the source and desired aspect ratio
    auto ar = this->aspectRatio();
    QSize maxCropSize(qMin(src.width(), src.height() * ar.width() / ar.height()),
                      qMin(src.height(), src.width() * ar.height() / ar.width()));

    if (bounds.height() == 0 || bounds.width() == 0)
        bounds.setCoords(int(src.width() / 2) - maxCropSize.width(),
                         int(src.height() / 2) - maxCropSize.height(),
                         int(src.width() / 2) + maxCropSize.width(),
                         int(src.height() / 2) + maxCropSize.height());

    QImage croppedFrame;

    if (this->lockedViewport()) {
        croppedFrame = oFrame.copy(bounds);
    } else {
        QRect newBounds(this->calculateNewBounds(bounds, maxCropSize, src.size()));
        croppedFrame = oFrame.copy(newBounds);
    }

    auto oPacket = AkVideoPacket::fromImage(croppedFrame, packet);
    akSend(oPacket)
}

void FaceTrackElement::setHaarFile(const QString &haarFile)
{
    if (this->haarFile() == haarFile)
        return;

    this->d->m_faceDetectFilter->setProperty("haarFile", haarFile);
    this->d->m_haarFile = this->d->m_faceDetectFilter->property("haarFile").value<QString>();
    emit this->haarFileChanged(this->haarFile());
}

void FaceTrackElement::setScanSize(const QSize &scanSize)
{
    if (this->scanSize() == scanSize)
        return;

    this->d->m_faceDetectFilter->setProperty("scanSize", scanSize);
    this->d->m_scanSize = this->d->m_faceDetectFilter->property("scanSize").value<QSize>();
    emit this->scanSizeChanged(this->scanSize());
}

void FaceTrackElement::setFaceBucketSize(int seconds)
{
    if (this->faceBucketSize() == seconds)
        return;

    this->d->m_faceBucketSize = seconds;
    emit this->faceBucketSizeChanged(this->faceBucketSize());
}

void FaceTrackElement::setFaceBucketCount(int count)
{
    if (this->d->m_faceBuckets.length() == count)
        return;

    this->d->m_faceBuckets.resize(abs(count));
    emit this->faceBucketCountChanged(abs(count));
}

void FaceTrackElement::setExpandRate(int rate)
{
    if (this->expandRate() == abs(rate))
        return;

    this->d->m_expandRate = abs(rate);
    emit this->expandRateChanged(this->expandRate());
}

void FaceTrackElement::setContractRate(int rate)
{
    if (this->contractRate() == abs(rate))
        return;

    this->d->m_contractRate = abs(rate);
    emit this->contractRateChanged(this->contractRate());
}

void FaceTrackElement::setFacePadding(const QRect facePadding)
{
    auto p = this->facePadding();
    QRect newFacePadding;
    newFacePadding
            .setCoords(facePadding.left() == -1 ? p.left() : facePadding.left(),
                       facePadding.top() == -1 ? p.top() : facePadding.top(),
                       facePadding.right() == -1 ? p.right() : facePadding.right(),
                       facePadding.bottom() == -1 ? p.bottom() : facePadding.bottom());

    if (p == newFacePadding)
        return;

    this->d->m_facePadding = newFacePadding;
    emit this->facePaddingChanged(this->facePadding());
}

void FaceTrackElement::setFaceMargin(QRect faceMargin)
{

    auto p = this->faceMargin();
    faceMargin
            .setCoords(faceMargin.left() == -1 ? p.left() : faceMargin.left(),
                       faceMargin.top() == -1 ? p.top() : faceMargin.top(),
                       faceMargin.right() == -1 ? p.right() : faceMargin.right(),
                       faceMargin.bottom() == -1 ? p.bottom() : faceMargin.bottom());

    if (p == faceMargin)
        return;

    this->d->m_faceMargin = faceMargin;
    emit this->faceMarginChanged(this->faceMargin());
}

void FaceTrackElement::setAspectRatio(const QSize &aspectRatio)
{
    auto ar = this->aspectRatio();
    double t = ((double(ar.width())) / ar.height());
    double u = ((double(aspectRatio.width())) / aspectRatio.height());

    if (t == u)
        return;

    auto d = FaceTrackElement::gcd(uint(aspectRatio.width()),
                                   uint(aspectRatio.height()));
    QSize scaledAspectRatio(aspectRatio.width() / d, aspectRatio.height() / d);

    this->d->m_aspectRatio = scaledAspectRatio;
    emit this->aspectRatioChanged(this->aspectRatio());
}

void FaceTrackElement::setOverrideAspectRatio(bool overrideAspectRatio)
{
    if (this->overrideAspectRatio() == overrideAspectRatio)
        return;

    this->d->m_overrideAspectRatio = overrideAspectRatio;
    emit this->overrideAspectRatioChanged(overrideAspectRatio);
}

void FaceTrackElement::setLockedViewport(bool lockViewport)
{
    if (this->lockedViewport() == lockViewport)
        return;

    this->d->m_lockedViewport = lockViewport;
    emit this->lockedViewportChanged(lockViewport);
}

void FaceTrackElement::setDebugModeEnabled(bool enabled)
{
    if (this->debugModeEnabled() == enabled)
        return;

    this->d->m_debugModeEnabled = enabled;
    emit this->debugModeEnabledChanged(enabled);
}

void FaceTrackElement::resetHaarFile()
{
    this->setHaarFile(":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml");
}

void FaceTrackElement::resetScanSize()
{
    this->setScanSize(QSize(160, 120));
}

void FaceTrackElement::resetFaceBucketSize()
{
    this->setFaceBucketSize(1);
}

void FaceTrackElement::resetFaceBucketCount()
{
    this->setFaceBucketCount(5);
}

void FaceTrackElement::resetExpandRate()
{
    this->setExpandRate(30);
}

void FaceTrackElement::resetContractRate()
{
    this->setContractRate(5);
}

void FaceTrackElement::resetFacePadding()
{
    QRect facePadding;
    facePadding.setCoords(20, 50, 20, 130);
    this->setFacePadding(facePadding);
}

void FaceTrackElement::resetFaceMargin()
{
    QRect faceMargin;
    faceMargin.setCoords(30, 30, 30, 30);
    this->setFaceMargin(faceMargin);
}

void FaceTrackElement::resetAspectRatio()
{
    this->setAspectRatio(QSize(16, 9));
}

void FaceTrackElement::resetOverrideAspectRatio()
{
    this->setOverrideAspectRatio(false);
}

void FaceTrackElement::resetLockedViewport()
{
    this->setLockedViewport(false);
}

void FaceTrackElement::resetDebugModeEnabled()
{
    this->setDebugModeEnabled(false);
}

/** https://en.wikipedia.org/wiki/Binary_GCD_algorithm
 * This can be removed and replaced with the C++ std::gcd
 * from <numeric> when we upgrade to C++ 17.
 *
 * @brief FaceTrackElement::gcd
 * @param u
 * @param v
 * @return
 */
unsigned int FaceTrackElement::gcd(unsigned int u, unsigned int v)
{
    // simple cases (termination)
    if (u == v)
        return u;

    if (u == 0)
        return v;

    if (v == 0)
        return u;

    // look for factors of 2
    if (~u & 1) { // u is even
        if (v & 1) // v is odd
            return FaceTrackElement::gcd(u >> 1, v);
        else // both u and v are even
            return FaceTrackElement::gcd(u >> 1, v >> 1) << 1;
    }

    if (~v & 1) // u is odd, v is even
        return FaceTrackElement::gcd(u, v >> 1);

    // reduce larger argument
    if (u > v)
        return FaceTrackElement::gcd((u - v) >> 1, v);

    return FaceTrackElement::gcd((v - u) >> 1, u);
}

#include "moc_facetrackelement.cpp"
