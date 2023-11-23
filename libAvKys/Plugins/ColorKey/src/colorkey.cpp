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

#include <QImage>
#include <QPainter>
#include <QReadWriteLock>
#include <QStandardPaths>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "colorkey.h"

enum BackgroundType
{
    BackgroundTypeNoBackground,
    BackgroundTypeColor,
    BackgroundTypeImage
};

class ColorKeyPrivate;

class BackgroundImageChangedCallbacks:
    public IAkObjectPropertyCallbacks<QString>
{
    public:
        BackgroundImageChangedCallbacks(ColorKeyPrivate *self);
        void valueChanged(const QString &background) override;

    private:
        ColorKeyPrivate *self;
};

class ColorKeyPrivate
{
    public:
        ColorKey *self {nullptr};
        QString m_description {QObject::tr("Color Key")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyColor m_colorKey {QObject::tr("Color to replace"), qRgb(0, 0, 0)};
        IAkPropertyInt m_colorDiff {QObject::tr("Color difference"), 32};
        IAkPropertyInt m_smoothness {QObject::tr("Smoothness"), 0};
        IAkPropertyBool m_normalize {QObject::tr("Normalize"), false};
        IAkPropertyIntMenu m_backgroundType {QObject::tr("Background type"),
                                             BackgroundTypeNoBackground, {
                                                 {"noBackground", QObject::tr("No background"), BackgroundTypeNoBackground},
                                                 {"color"       , QObject::tr("Color")        , BackgroundTypeColor       },
                                                 {"image"       , QObject::tr("Image")        , BackgroundTypeImage       }
                                             }};
        IAkPropertyColor m_backgroundColor {QObject::tr("Background color"), qRgba(0, 0, 0, 0)};
        IAkPropertyString m_backgroundImage {QObject::tr("Background image")};
        BackgroundImageChangedCallbacks *m_backgroundImageChangedCallbacks {nullptr};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        IAkVideoFilterPtr m_normalizeFilter {akPluginManager->create<IAkVideoFilter>("VideoFilter/Normalize")};
        QImage m_backgroundImg;
        QReadWriteLock m_mutex;

        explicit ColorKeyPrivate(ColorKey *self);
        ~ColorKeyPrivate();
};

ColorKey::ColorKey(QObject *parent):
      QObject(parent)
{
    this->d = new ColorKeyPrivate(this);
    this->registerProperty("colorKey", &this->d->m_colorKey);
    this->registerProperty("colorDiff", &this->d->m_colorDiff);
    this->registerProperty("smoothness", &this->d->m_smoothness);
    this->registerProperty("normalize", &this->d->m_normalize);
    this->registerProperty("backgroundType", &this->d->m_backgroundType);
    this->registerProperty("backgroundColor", &this->d->m_backgroundColor);
    this->registerProperty("backgroundImage", &this->d->m_backgroundImage);

    this->d->m_backgroundImage.subscribe(this->d->m_backgroundImageChangedCallbacks);
}

ColorKey::~ColorKey()
{
    delete this->d;
}

QString ColorKey::description() const
{
    return this->d->m_description;
}

AkElementType ColorKey::type() const
{
    return this->d->m_type;
}

AkElementCategory ColorKey::category() const
{
    return this->d->m_category;
}

void *ColorKey::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *ColorKey::create(const QString &id)
{
    Q_UNUSED(id)

    return new ColorKey;
}

int ColorKey::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/ColorKey",
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

void ColorKey::deleteThis(void *userData) const
{
    delete reinterpret_cast<ColorKey *>(userData);
}

QString ColorKey::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorKey/share/qml/main.qml");
}

void ColorKey::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorKey", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    QStringList picturesPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    context->setContextProperty("picturesPath", picturesPath[0]);
}

AkPacket ColorKey::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket *srcPtr = nullptr;
    AkVideoPacket normalized;

    if (this->d->m_normalize) {
        normalized = this->d->m_normalizeFilter->iStream(src);
        srcPtr = &normalized;
    } else {
        srcPtr = &src;
    }

    auto colorKey = QRgb(this->d->m_colorKey);
    auto ckR = qRed(colorKey);
    auto ckG = qGreen(colorKey);
    auto ckB = qBlue(colorKey);

    auto colorDiff = int(this->d->m_colorDiff);
    auto colorDiff2 = colorDiff * colorDiff;

    auto alphaContrast = 128 - int(this->d->m_smoothness);

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < srcPtr->caps().height(); ++y) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto nLine = reinterpret_cast<const QRgb *>(srcPtr->constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < srcPtr->caps().width(); ++x) {
            auto &npixel = nLine[x];
            auto diffR = ckR - qRed(npixel);
            auto diffG = ckG - qGreen(npixel);
            auto diffB = ckB - qBlue(npixel);

            auto diffR2 = diffR * diffR;
            auto diffG2 = diffG * diffG;
            auto diffB2 = diffB * diffB;

            auto k = diffR2 + diffG2 + diffB2;
            int alpha = 255;

            if (colorDiff > 0)
                alpha = qMin(255 * k / colorDiff2, 255);

            if (alphaContrast > 0) {
                if (alpha < 127)
                    alpha = qMax(alpha - alphaContrast, 0);
                else
                    alpha = qMin(alpha + alphaContrast, 255);
            }

            auto &pixel = iLine[x];
            oLine[x] = qRgba(qRed(pixel),
                             qGreen(pixel),
                             qBlue(pixel),
                             (alpha * qAlpha(pixel)) >> 8);
        }
    }

    if (this->d->m_backgroundType == BackgroundTypeImage
        && !this->d->m_backgroundImage.value().isEmpty()) {
        QImage image(dst.caps().width(),
                     dst.caps().height(),
                     QImage::Format_ARGB32);
        auto lineSize = qMin<size_t>(dst.lineSize(0), image.bytesPerLine());

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = dst.constLine(0, y);
            auto dstLine = image.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        this->d->m_mutex.lockForRead();

        if (!this->d->m_backgroundImage.value().isNull()) {
            auto scaled =
                this->d->m_backgroundImg.scaled(src.caps().width(),
                                                src.caps().height(),
                                                Qt::KeepAspectRatioByExpanding);

            QPainter painter;
            painter.begin(&scaled);
            painter.drawImage(0, 0, image);
            painter.end();

            lineSize = qMin<size_t>(scaled.bytesPerLine(), dst.lineSize(0));

            for (int y = 0; y < dst.caps().height(); y++) {
                auto srcLine = scaled.constScanLine(y);
                auto dstLine = dst.line(0, y);
                memcpy(dstLine, srcLine, lineSize);
            }
        }

        this->d->m_mutex.unlock();
    } else if (this->d->m_backgroundType == BackgroundTypeColor
             && qAlpha(QRgb(this->d->m_backgroundColor)) != 0) {
        QImage background(dst.caps().width(),
                          dst.caps().height(),
                          QImage::Format_ARGB32);
        background.fill(QRgb(this->d->m_backgroundColor));

        QImage image(dst.caps().width(),
                     dst.caps().height(),
                     QImage::Format_ARGB32);
        auto lineSize = qMin<size_t>(dst.lineSize(0), image.bytesPerLine());

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = dst.constLine(0, y);
            auto dstLine = image.scanLine(y);
            memcpy(dstLine, srcLine, lineSize);
        }

        QPainter painter;
        painter.begin(&background);
        painter.drawImage(0, 0, image);
        painter.end();

        lineSize = qMin<size_t>(background.bytesPerLine(), dst.lineSize(0));

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = background.constScanLine(y);
            auto dstLine = dst.line(0, y);
            memcpy(dstLine, srcLine, lineSize);
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

BackgroundImageChangedCallbacks::BackgroundImageChangedCallbacks(ColorKeyPrivate *self):
      self(self)
{

}

void BackgroundImageChangedCallbacks::valueChanged(const QString &background)
{
    self->m_mutex.lockForWrite();

    if (background.isEmpty())
        self->m_backgroundImg = {};
    else
        self->m_backgroundImg =
            QImage(background).convertedTo(QImage::Format_ARGB32);

    self->m_mutex.unlock();
}

ColorKeyPrivate::ColorKeyPrivate(ColorKey *self):
    self(self)
{
    this->m_backgroundImageChangedCallbacks =
        new BackgroundImageChangedCallbacks(this);
}

ColorKeyPrivate::~ColorKeyPrivate()
{
    delete this->m_backgroundImageChangedCallbacks;
}

#include "moc_colorkey.cpp"
