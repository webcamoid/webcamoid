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

#include <QDateTime>
#include <QMap>
#include <QMutex>
#include <QPainter>
#include <QQmlContext>
#include <QVariant>
#include <akfrac.h>
#include <akpacket.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "facetrackelement.h"

class FaceTrackElementPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        QString m_haarFile {":/FaceDetect/share/haarcascades/haarcascade_frontalface_alt.xml"};
        QSize m_scanSize {160, 120};
        int m_faceBucketSize {1};
        QVector<QRect> m_faceBuckets;
        int m_expandRate {30};
        int m_contractRate {5};

        /* Margins/Paddings are defined as a rectangle where:
         *
         * x1 = Left Margin/Padding
         * y1 = Top Margin/Padding
         * x2 = Right Margin/Padding
         * y2 = Bottom Margin/Padding
         *
         * and
         *
         * width  = x2 - x1
         * height = y2 - y1
         */
        QRect m_faceMargin {30, 30, 1, 1};   // 30, 30, 30, 30
        QRect m_facePadding {20, 50, 1, 81}; // 20, 50, 20, 130

        AkFrac m_aspectRatio {16, 9};
        bool m_overrideAspectRatio {false};
        bool m_lockedViewport {false};
        bool m_debugModeEnabled {false};
        QRect m_lastBounds;
        AkElementPtr m_faceDetectFilter {akPluginManager->create<AkElement>("VideoFilter/FaceDetect")};
        QMutex m_mutex;

        QRect calculateNewBounds(const QRect &targetBounds,
                                 const QSize &maxCropSize,
                                 const QSize &srcSize);
        void collectFaces(const QVector<QRect> &vecFaces);
};

FaceTrackElement::FaceTrackElement(): AkElement()
{
    this->d = new FaceTrackElementPrivate;
    this->d->m_faceBuckets.resize(5);
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

AkFrac FaceTrackElement::aspectRatio() const
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
}

AkPacket FaceTrackElement::iVideoStream(const AkVideoPacket &packet)
{
    QSize scanSize(this->d->m_scanSize);

    if (this->d->m_haarFile.isEmpty() || scanSize.isEmpty()) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QImage iFrame(src.caps().width(),
                  src.caps().height(),
                  QImage::Format_ARGB32);
    auto lineSize = qMin<size_t>(src.lineSize(0), iFrame.bytesPerLine());

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = src.constLine(0, y);
        auto dstLine = iFrame.scanLine(y);
        memcpy(dstLine, srcLine, lineSize);
    }

    if (!this->overrideAspectRatio())
        this->setAspectRatio(AkFrac(iFrame.width(), iFrame.height()));

    qreal scale;

    if (scanSize.height() * iFrame.width() >= scanSize.width() * iFrame.height())
        scale = qreal(iFrame.width()) / scanSize.width();
    else
        scale = qreal(iFrame.height()) / scanSize.height();

    QRect bounds;

    if (this->d->m_lastBounds.isNull())
        this->d->m_lastBounds = iFrame.rect();

    if (this->d->m_lockedViewport) {
        bounds = this->d->m_lastBounds;
    } else {
        QVector<QRect> detectedFaces;
        QMetaObject::invokeMethod(this->d->m_faceDetectFilter.data(),
                                  "detectFaces",
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(QVector<QRect>, detectedFaces),
                                  Q_ARG(AkVideoPacket, packet));

        if (detectedFaces.length() > 0)
            this->d->collectFaces(detectedFaces);

        QPen pen;
        QPainter painter;
        int penWidth = 1;

        if (this->d->m_debugModeEnabled) {
            pen.setStyle(Qt::SolidLine);
            painter.begin(&iFrame);
        }

        for (int i = 0; i < this->d->m_faceBuckets.size(); i++) {
            auto &face = this->d->m_faceBuckets[i];

            if (!face.isNull()) {
                QRect scaledFace;
                scaledFace.setCoords(scale * face.left(),
                                     scale * face.top(),
                                     scale * face.right(),
                                     scale * face.bottom());
                bounds = bounds.united(scaledFace);

                if (this->d->m_debugModeEnabled) {
                    auto color =
                            QColor::fromHsv(360 * i
                                            / this->d->m_faceBuckets.size(),
                                            255,
                                            255);
                    pen.setColor(color);
                    painter.setPen(pen);
                    pen.setWidth(penWidth);
                    painter.drawRect(scaledFace);
                    penWidth += 1;
                }
            }
        }

        // Don't allow bounds outside the frame, even if the face does
        bounds.setCoords(qMax(0, bounds.left()),
                         qMax(0, bounds.top()),
                         qMin(iFrame.width(), bounds.right()),
                         qMin(iFrame.height(), bounds.bottom()));

        if (this->d->m_debugModeEnabled) {
            // Draw a boarder so we can see what is going on
            pen.setColor(Qt::white);
            pen.setWidth(penWidth);
            painter.setPen(pen);
            painter.drawRect(bounds);
            painter.end();
        }
    }

    // Calcuate the maximum size allowed given the source and desired aspect ratio
    auto aspectRatio = this->d->m_aspectRatio;
    QSize maxCropSize(qMin<int>(iFrame.width(),
                                iFrame.height() * aspectRatio.value()),
                      qMin<int>(iFrame.height(),
                                iFrame.width() / aspectRatio.value()));

    if (bounds.height() == 0 || bounds.width() == 0)
        bounds.setCoords(int(iFrame.width() / 2) - maxCropSize.width(),
                         int(iFrame.height() / 2) - maxCropSize.height(),
                         int(iFrame.width() / 2) + maxCropSize.width(),
                         int(iFrame.height() / 2) + maxCropSize.height());

    QImage oFrame;

    if (this->lockedViewport()) {
        oFrame = iFrame.copy(bounds);
    } else {
        oFrame = iFrame.copy(this->d->calculateNewBounds(bounds,
                                                         maxCropSize,
                                                         iFrame.size()));
    }

    auto ocaps = src.caps();
    ocaps.setWidth(oFrame.width());
    ocaps.setHeight(oFrame.height());
    AkVideoPacket dst(ocaps);
    dst.copyMetadata(src);
    lineSize = qMin<size_t>(oFrame.bytesPerLine(), dst.lineSize(0));

    for (int y = 0; y < dst.caps().height(); y++) {
        auto srcLine = oFrame.constScanLine(y);
        auto dstLine = dst.line(0, y);
        memcpy(dstLine, srcLine, lineSize);
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
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

void FaceTrackElement::setFacePadding(const QRect &facePadding)
{
    if (this->d->m_facePadding == facePadding)
        return;

    this->d->m_facePadding = facePadding;
    emit this->facePaddingChanged(facePadding);
}

void FaceTrackElement::setFaceMargin(const QRect &faceMargin)
{
    if (this->d->m_faceMargin == faceMargin)
        return;

    this->d->m_faceMargin = faceMargin;
    emit this->faceMarginChanged(faceMargin);
}

void FaceTrackElement::setAspectRatio(const AkFrac &aspectRatio)
{
    if (this->d->m_aspectRatio == aspectRatio)
        return;

    this->d->m_aspectRatio = aspectRatio;
    emit this->aspectRatioChanged(aspectRatio);
}

void FaceTrackElement::setOverrideAspectRatio(bool overrideAspectRatio)
{
    if (this->d->m_overrideAspectRatio == overrideAspectRatio)
        return;

    this->d->m_overrideAspectRatio = overrideAspectRatio;
    emit this->overrideAspectRatioChanged(overrideAspectRatio);
}

void FaceTrackElement::setLockedViewport(bool lockViewport)
{
    if (this->d->m_lockedViewport == lockViewport)
        return;

    this->d->m_lockedViewport = lockViewport;
    emit this->lockedViewportChanged(lockViewport);
}

void FaceTrackElement::setDebugModeEnabled(bool enabled)
{
    if (this->d->m_debugModeEnabled == enabled)
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
    this->setScanSize({160, 120});
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
    this->setFacePadding({20, 50, 0, 80});
}

void FaceTrackElement::resetFaceMargin()
{
    this->setFaceMargin({30, 30, 0, 0});
}

void FaceTrackElement::resetAspectRatio()
{
    this->setAspectRatio({16, 9});
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

QRect FaceTrackElementPrivate::calculateNewBounds(const QRect &targetBounds,
                                                  const QSize &maxCropSize,
                                                  const QSize &srcSize)
{
    // Slowly change the bounds
    // Can't use addition/subtraction, need to use ratios,
    // or we'll pass our target and get jittery
    auto lastBounds = this->m_lastBounds;
    auto xRate = qreal(this->m_expandRate) / 100;
    auto cRate = qreal(-this->m_contractRate) / 100;

    // Apply the expand/contract rates to get the new bounds
    QRect newBounds;
    newBounds.setCoords(lastBounds.left()
                        - ((targetBounds.left() < lastBounds.left()? xRate: cRate)
                           * abs(targetBounds.left() - lastBounds.left())),
                        lastBounds.top()
                        - ((targetBounds.top() < lastBounds.top()? xRate: cRate)
                           * abs(targetBounds.top() - lastBounds.top())),
                        lastBounds.right()
                        + ((targetBounds.right() > lastBounds.right()? xRate: cRate)
                           * abs(targetBounds.right() - lastBounds.right())),
                        lastBounds.bottom()
                        + ((targetBounds.bottom() > lastBounds.bottom()? xRate: cRate)
                           * abs(targetBounds.bottom() - lastBounds.bottom())));

    // Make sure the new bounds are the correct aspect ratio
    auto aspectRatio = this->m_aspectRatio;
    int proposedWidth(qMax<int>(newBounds.width(),
                                newBounds.height() * aspectRatio.value()));
    int proposedHeight(qMax<int>(newBounds.height(),
                                 newBounds.width() / aspectRatio.value()));

    if (proposedWidth > maxCropSize.width()) {
        proposedWidth = maxCropSize.width();
        proposedHeight = maxCropSize.width() / aspectRatio.value();
    }

    if (proposedHeight > maxCropSize.height()) {
        proposedHeight = maxCropSize.height();
        proposedWidth = maxCropSize.height() * aspectRatio.value();
    }

    // Make sure that we pan the image gradually
    QLine centerline(targetBounds.center(), this->m_lastBounds.center());

    // Make sure we correctly center and size the new bounds
    int left = centerline.center().x() - proposedWidth / 2;
    newBounds.setLeft(qMax(0, left));

    int right = proposedWidth + newBounds.left();
    newBounds.setRight(qMin(srcSize.width(), right));

    left = newBounds.left() - (proposedWidth - newBounds.width());
    newBounds.setLeft(left);

    int top = centerline.center().y() - proposedHeight / 2;
    newBounds.setTop(qMax(0, top));

    int bottom = proposedHeight + newBounds.top();
    newBounds.setBottom(qMin(srcSize.height(), bottom));

    top = newBounds.top() - (proposedHeight - newBounds.height());
    newBounds.setTop(top);

    this->m_lastBounds = newBounds;

    return newBounds;
}

void FaceTrackElementPrivate::collectFaces(const QVector<QRect> &vecFaces)
{
    /*
     * Track faces grouped by the inverse of the scan frequency
     * so that we can mitigate the effect of flashing face detection.
     * This will cause each bucket to contain `m_faceBucketSize`
     * number of seconds of face scans grouped together
     */

    int now = QDateTime::currentSecsSinceEpoch() / this->m_faceBucketSize;
    auto size = this->m_scanSize;
    auto pad = this->m_facePadding;
    auto margin = this->m_faceMargin;
    int nextTimeSlot = (now + 1) % this->m_faceBuckets.length();
    int timeSlot = now % this->m_faceBuckets.length();

    for (auto &face: vecFaces) {
        // Add a padding around an incoming face
        QRect head(face);
        // Forehead
        head.setTop(qMax(0, head.top() - int(face.height() * pad.top() / 100)));
        // Neck
        head.setHeight(qMin(size.height(),
                            head.height()
                            + int(face.height() * pad.bottom() / 100)));
        // Left shoulder
        head.setLeft(qMax(0,
                          head.left() - int(face.width() * pad.left() / 100)));
        // Right shoulder
        head.setWidth(qMin(size.width(),
                           head.width()
                           + int(face.width() * pad.right() / 100)));

        if (this->m_faceBuckets[timeSlot].isNull()) {
            this->m_faceBuckets[timeSlot] = head;
        } else {
            /*
             * Include a "margin" option that will only include the new head
             * if it is outside the old bucket + margin. This will further
             * help reduce jittery or very small movements of the crop/pan.
             */
            QRect headMargin;
            auto bucket = this->m_faceBuckets[timeSlot];
            headMargin.setCoords(bucket.left()
                                 - int(head.left() * margin.left() / 100),
                                 bucket.top()
                                 - int(head.top() * margin.top() / 100),
                                 bucket.right()
                                 + int(head.right() * margin.right() / 100),
                                 bucket.bottom()
                                 + int(head.bottom() * margin.bottom() / 100));
            // If `head` is completely inside `headMargin`,
            // then don't merge it with the bucket
            if (!headMargin.contains(head.topLeft())
                || !headMargin.contains(head.bottomRight())) {
                this->m_faceBuckets[timeSlot] =
                        this->m_faceBuckets[timeSlot].united(head);
            }
        }

        // Degrade the last occupied slot by intersecting it with the current one
        if (!this->m_faceBuckets[nextTimeSlot].isNull())
            this->m_faceBuckets[nextTimeSlot] =
                    this->m_faceBuckets[nextTimeSlot].intersected(head);
    }
}

#include "moc_facetrackelement.cpp"
