/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include <QPainter>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideoformatspec.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "cropelement.h"

class CropElementPrivate
{
    public:
        bool m_editMode {false};
        bool m_relative {false};
        bool m_keepResolution {false};
        qreal m_left {0.0};
        qreal m_right {639};
        qreal m_top {0.0};
        qreal m_bottom {479};
        QRgb m_fillColor {qRgba(0, 0, 0, 0)};
        int m_frameWidth {640};
        int m_frameHeight {480};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;
};

CropElement::CropElement(): AkElement()
{
    this->d = new CropElementPrivate;
}

CropElement::~CropElement()
{
    delete this->d;
}

bool CropElement::editMode() const
{
    return this->d->m_editMode;
}

bool CropElement::relative() const
{
    return this->d->m_relative;
}

bool CropElement::keepResolution() const
{
    return this->d->m_keepResolution;
}

qreal CropElement::left() const
{
    return this->d->m_left;
}

qreal CropElement::right() const
{
    return this->d->m_right;
}

qreal CropElement::top() const
{
    return this->d->m_top;
}

qreal CropElement::bottom() const
{
    return this->d->m_bottom;
}

QRgb CropElement::fillColor() const
{
    return this->d->m_fillColor;
}

int CropElement::frameWidth() const
{
    return this->d->m_frameWidth;
}

int CropElement::frameHeight() const
{
    return this->d->m_frameHeight;
}

QString CropElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Crop/share/qml/main.qml");
}

void CropElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Crop", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket CropElement::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    if (packet.caps().width() != this->d->m_frameWidth) {
        this->d->m_frameWidth = packet.caps().width();
        emit this->frameWidthChanged(this->d->m_frameWidth);
    }

    if (packet.caps().height() != this->d->m_frameHeight) {
        this->d->m_frameHeight = packet.caps().height();
        emit this->frameHeightChanged(this->d->m_frameHeight);
    }

    int rightMax = packet.caps().width() - 1;
    int left = this->d->m_relative?
                   qRound(rightMax * this->d->m_left / 100):
                   qRound(this->d->m_left);
    int right = this->d->m_relative?
                    qRound(rightMax * this->d->m_right / 100):
                    qRound(this->d->m_right);
    left = qBound(0, left, rightMax);
    right = qBound(0, right, rightMax);

    if (left >= right) {
        if (left + 1 <= rightMax) {
            right = left + 1;
        } else {
            left = rightMax - 1;
            right = rightMax;
        }
    }

    int bottomMax = packet.caps().height() - 1;
    int top = this->d->m_relative?
                  qRound(bottomMax * this->d->m_top / 100):
                  qRound(this->d->m_top);
    int bottom = this->d->m_relative?
                     qRound(bottomMax * this->d->m_bottom / 100):
                     qRound(this->d->m_bottom);
    top = qBound(0, top, bottomMax);
    bottom = qBound(0, bottom, bottomMax);

    if (top >= bottom) {
        if (top + 1 <= bottomMax) {
            bottom = top + 1;
        } else {
            top = bottomMax - 1;
            bottom = bottomMax;
        }
    }

    QRect srcRect(left, top, right - left + 1, bottom - top + 1);

    if (this->d->m_editMode) {
        QImage markImg(packet.caps().width(),
                       packet.caps().height(),
                       QImage::Format_ARGB32);
        markImg.fill(qRgba(0, 0, 0, 0));

        QPen pen;
        pen.setColor(QColor(255, 0, 0));
        pen.setWidth(3);
        pen.setStyle(Qt::SolidLine);

        QPainter painter;
        painter.begin(&markImg);
        painter.setPen(pen);
        painter.drawRect(srcRect);
        painter.end();

        AkVideoPacket mark({AkVideoCaps::Format_argbpack,
                            markImg.width(),
                            markImg.height(),
                            {}});
        auto lineSize = qMin<size_t>(markImg.bytesPerLine(),
                                     mark.lineSize(0));

        for (int y = 0; y < markImg.height(); y++)
            memcpy(mark.line(0, y),
                   markImg.constScanLine(y),
                   lineSize);

        this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_argbpack,
                                                 0,
                                                 0,
                                                 {}});
        this->d->m_videoConverter.setInputRect({});
        this->d->m_videoConverter.begin();
        auto dst = this->d->m_videoConverter.convert(packet);
        this->d->m_videoConverter.end();
        this->d->m_videoMixer.begin(&dst);
        this->d->m_videoMixer.draw(mark);
        this->d->m_videoMixer.end();

        if (dst)
            emit this->oStream(dst);

        return dst;
    }

    QRect dstRect;

    if (this->d->m_keepResolution) {
        if (packet.caps().width() * srcRect.height() <= packet.caps().height() * srcRect.width()) {
            int dstHeight = packet.caps().width() * srcRect.height() / srcRect.width();
            dstRect = {0,
                       (packet.caps().height() - dstHeight) / 2,
                       packet.caps().width(),
                       dstHeight};
        } else {
            int dstWidth = packet.caps().height() * srcRect.width() / srcRect.height();
            dstRect = {(packet.caps().width() - dstWidth) / 2,
                       0,
                       dstWidth,
                       packet.caps().height()};
        }
    } else {
        dstRect = {0, 0, srcRect.width(), srcRect.height()};
    }

    this->d->m_videoConverter.setInputRect(srcRect);
    this->d->m_videoConverter.setOutputCaps({AkVideoCaps::Format_argbpack,
                                             dstRect.width(),
                                             dstRect.height(),
                                             packet.caps().fps()});
    this->d->m_videoConverter.begin();
    auto cropped = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!this->d->m_keepResolution) {
        if (cropped)
            emit this->oStream(cropped);

        return cropped;
    }

    auto caps = packet.caps();
    caps.setFormat(this->d->m_videoConverter.outputCaps().format());

    if (!this->d->m_keepResolution) {
        caps.setWidth(srcRect.width());
        caps.setHeight(srcRect.height());
    }

    AkVideoPacket dst(caps);
    dst.copyMetadata(packet);
    dst.fill(this->d->m_fillColor);

    this->d->m_videoMixer.begin(&dst);
    this->d->m_videoMixer.draw(dstRect.x(), dstRect.y(), cropped);
    this->d->m_videoMixer.end();

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void CropElement::setEditMode(bool editMode)
{
    if (this->d->m_editMode == editMode)
        return;

    this->d->m_editMode = editMode;
    emit this->editModeChanged(this->d->m_editMode);
}

void CropElement::setRelative(bool relative)
{
    if (this->d->m_relative == relative)
        return;

    this->d->m_relative = relative;
    qreal left = 0.0;
    qreal right = 0.0;
    qreal top = 0.0;
    qreal bottom = 0.0;
    int rightMax = qMax(this->d->m_frameWidth - 1, 1);
    int bottomMax = qMax(this->d->m_frameHeight - 1, 1);

    if (relative) {
        left = 100 * this->d->m_left / rightMax;
        right = 100 * this->d->m_right / rightMax;
        top = 100 * this->d->m_top / bottomMax;
        bottom = 100 * this->d->m_bottom / bottomMax;
    } else {
        left = this->d->m_left * rightMax / 100;
        right = this->d->m_right * rightMax / 100;
        top = this->d->m_top * bottomMax / 100;
        bottom = this->d->m_bottom * bottomMax / 100;
    }

    emit this->relativeChanged(this->d->m_relative);
    this->setLeft(left);
    this->setRight(right);
    this->setTop(top);
    this->setBottom(bottom);
}

void CropElement::setKeepResolution(bool keepResolution)
{
    if (this->d->m_keepResolution == keepResolution)
        return;

    this->d->m_keepResolution = keepResolution;
    emit this->keepResolutionChanged(this->d->m_keepResolution);
}

void CropElement::setLeft(qreal left)
{
    if (qFuzzyCompare(this->d->m_left, left))
        return;

    this->d->m_left = left;
    emit this->leftChanged(this->d->m_left);
}

void CropElement::setRight(qreal right)
{
    if (qFuzzyCompare(this->d->m_right, right))
        return;

    this->d->m_right = right;
    emit this->rightChanged(this->d->m_right);
}

void CropElement::setTop(qreal top)
{
    if (qFuzzyCompare(this->d->m_top, top))
        return;

    this->d->m_top = top;
    emit this->topChanged(this->d->m_top);
}

void CropElement::setBottom(qreal bottom)
{
    if (qFuzzyCompare(this->d->m_bottom, bottom))
        return;

    this->d->m_bottom = bottom;
    emit this->bottomChanged(this->d->m_bottom);
}

void CropElement::setFillColor(QRgb fillColor)
{
    if (this->d->m_fillColor == fillColor)
        return;

    this->d->m_fillColor = fillColor;
    emit this->fillColorChanged(this->d->m_fillColor);
}

void CropElement::resetEditMode()
{
    this->setEditMode(false);
}

void CropElement::resetRelative()
{
    this->setRelative(false);
}

void CropElement::resetKeepResolution()
{
    this->setKeepResolution(false);
}

void CropElement::resetLeft()
{
    this->setLeft(0.0);
}

void CropElement::resetRight()
{
    this->setRight(this->d->m_frameWidth);
}

void CropElement::resetTop()
{
    this->setTop(0.0);
}

void CropElement::resetBottom()
{
    this->setBottom(this->d->m_frameHeight);
}

void CropElement::resetFillColor()
{
    this->setFillColor(qRgba(0, 0, 0, 0));
}

void CropElement::reset()
{
    this->resetEditMode();
    this->resetRelative();
    this->resetKeepResolution();
    this->resetLeft();
    this->resetRight();
    this->resetTop();
    this->resetBottom();
    this->resetFillColor();
}

#include "moc_cropelement.cpp"
