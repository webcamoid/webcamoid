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
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideomixer.h>
#include <akvideopacket.h>

#include "crop.h"

class CropPrivate;

class RelativeCallbacks: public IAkNumericPropertyCallbacks<bool>
{
    public:
        RelativeCallbacks(CropPrivate *self);
        void valueChanged(bool relative) override;

    private:
        CropPrivate *self;
};

class CropPrivate
{
    public:
        Crop *self {nullptr};
        QString m_description {QObject::tr("Crop")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyBool m_editMode {QObject::tr("Edit mode"), false};
        IAkPropertyBool m_relative {QObject::tr("Relative"), false};
        RelativeCallbacks *m_relativeCallbacks {nullptr};
        IAkPropertyBool m_keepResolution {QObject::tr("Keep resolution"), false};
        IAkPropertyDouble m_left {QObject::tr("Left"), 0.0};
        IAkPropertyDouble m_right {QObject::tr("Right"), 639};
        IAkPropertyDouble m_top {QObject::tr("Top"), 0.0};
        IAkPropertyDouble m_bottom {QObject::tr("Bottom"), 479};
        IAkPropertyColor m_fillColor {QObject::tr("Filling color"), qRgba(0, 0, 0, 0)};
        IAkNumericPropertySetter<qint32> m_frameWidthSetter;
        IAkPropertyInt *m_frameWidth {nullptr};
        IAkNumericPropertySetter<qint32> m_frameHeightSetter;
        IAkPropertyInt *m_frameHeight {nullptr};
        AkVideoConverter m_videoConverter;
        AkVideoMixer m_videoMixer;

        explicit CropPrivate(Crop *self);
        ~CropPrivate();
        void reset();
};

Crop::Crop(QObject *parent):
      QObject(parent)
{
    this->d = new CropPrivate(this);

    this->d->m_frameWidth =
            new IAkPropertyInt(QObject::tr("Frame width"),
                               640,
                               IAkPropertyMode_ReadOnly,
                               &this->d->m_frameWidthSetter);
    this->d->m_frameHeight =
            new IAkPropertyInt(QObject::tr("Frame height"),
                               480,
                               IAkPropertyMode_ReadOnly,
                               &this->d->m_frameHeightSetter);

    this->registerProperty("editMode", &this->d->m_editMode);
    this->registerProperty("relative", &this->d->m_relative);
    this->registerProperty("keepResolution", &this->d->m_keepResolution);
    this->registerProperty("left", &this->d->m_left);
    this->registerProperty("right", &this->d->m_right);
    this->registerProperty("top", &this->d->m_top);
    this->registerProperty("bottom", &this->d->m_bottom);
    this->registerProperty("fillColor", &this->d->m_fillColor);
    this->registerProperty("frameWidth", this->d->m_frameWidth);
    this->registerProperty("frameHeight", this->d->m_frameHeight);

    this->d->m_relative.subscribe(this->d->m_relativeCallbacks);

    this->registerMethod("reset", [this] (QVariantList *results,
                                          const QVariantList &args) {
        Q_UNUSED(results)
        Q_UNUSED(args)

        this->d->reset();
    });
}

Crop::~Crop()
{
    delete this->d->m_frameWidth;
    delete this->d->m_frameHeight;
    delete this->d;
}

QString Crop::description() const
{
    return this->d->m_description;
}

AkElementType Crop::type() const
{
    return this->d->m_type;
}

AkElementCategory Crop::category() const
{
    return this->d->m_category;
}

void *Crop::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Crop::create(const QString &id)
{
    Q_UNUSED(id)

    return new Crop;
}

int Crop::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Crop",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

void Crop::deleteThis(void *userData) const
{
    delete reinterpret_cast<Crop *>(userData);
}

QString Crop::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Crop/share/qml/main.qml");
}

void Crop::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Crop", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Crop::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    this->d->m_frameWidthSetter.setValue(packet.caps().width());
    this->d->m_frameHeightSetter.setValue(packet.caps().height());

    int rightMax = packet.caps().width() - 1;
    int left = this->d->m_relative?
                   qRound(rightMax * qreal(this->d->m_left) / 100):
                   qRound(qreal(this->d->m_left));
    int right = this->d->m_relative?
                    qRound(rightMax * qreal(this->d->m_right) / 100):
                    qRound(qreal(this->d->m_right));
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
                  qRound(bottomMax * qreal(this->d->m_top) / 100):
                  qRound(qreal(this->d->m_top));
    int bottom = this->d->m_relative?
                     qRound(bottomMax * qreal(this->d->m_bottom) / 100):
                     qRound(qreal(this->d->m_bottom));
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
            this->oStream(dst);

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
            this->oStream(cropped);

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
    dst.fill(QRgb(this->d->m_fillColor));

    this->d->m_videoMixer.begin(&dst);
    this->d->m_videoMixer.draw(dstRect.x(), dstRect.y(), cropped);
    this->d->m_videoMixer.end();

    if (dst)
        this->oStream(dst);

    return dst;
}

RelativeCallbacks::RelativeCallbacks(CropPrivate *self):
    self(self)
{
}

void RelativeCallbacks::valueChanged(bool relative)
{
    qreal left = 0.0;
    qreal right = 0.0;
    qreal top = 0.0;
    qreal bottom = 0.0;
    int rightMax = qMax(int(*self->m_frameWidth) - 1, 1);
    int bottomMax = qMax(int(*self->m_frameHeight) - 1, 1);

    if (relative) {
        left = 100.0 * qreal(self->m_left) / rightMax;
        right = 100.0 * qreal(self->m_right) / rightMax;
        top = 100.0 * qreal(self->m_top) / bottomMax;
        bottom = 100.0 * qreal(self->m_bottom) / bottomMax;
    } else {
        left = qreal(self->m_left) * rightMax / 100.0;
        right = qreal(self->m_right) * rightMax / 100.0;
        top = qreal(self->m_top) * bottomMax / 100.0;
        bottom = qreal(self->m_bottom) * bottomMax / 100.0;
    }

    self->m_left.setValue(left);
    self->m_right.setValue(right);
    self->m_top.setValue(top);
    self->m_bottom.setValue(bottom);
}

CropPrivate::CropPrivate(Crop *self):
    self(self)
{
    this->m_relativeCallbacks = new RelativeCallbacks(this);
}

CropPrivate::~CropPrivate()
{
    delete this->m_relativeCallbacks;
}

void CropPrivate::reset()
{
    this->m_editMode.resetValue();
    this->m_relative.resetValue();
    this->m_keepResolution.resetValue();
    this->m_left.resetValue();
    this->m_right.resetValue();
    this->m_top.resetValue();
    this->m_bottom.resetValue();
    this->m_fillColor.resetValue();
}

#include "moc_crop.cpp"
