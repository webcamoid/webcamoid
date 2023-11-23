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

#include <QRandomGenerator>
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "shagadelic.h"

class ShagadelicPrivate
{
    public:
        Shagadelic *self {nullptr};
        QString m_description {QObject::tr("Psychedelic")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoPacket m_ripple;
        AkVideoPacket m_spiral;
        QSize m_curSize;
        quint32 m_mask {0xffffff};
        int m_rx {0};
        int m_ry {0};
        int m_bx {0};
        int m_by {0};
        int m_rvx {0};
        int m_rvy {0};
        int m_bvx {0};
        int m_bvy {0};
        uchar m_phase {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit ShagadelicPrivate(Shagadelic *self);
        AkVideoPacket makeRipple(const QSize &size) const;
        AkVideoPacket makeSpiral(const QSize &size) const;
        void init(const QSize &size);
};

Shagadelic::Shagadelic(QObject *parent):
      QObject(parent)
{
    this->d = new ShagadelicPrivate(this);
}

Shagadelic::~Shagadelic()
{
    delete this->d;
}

QString Shagadelic::description() const
{
    return this->d->m_description;
}

AkElementType Shagadelic::type() const
{
    return this->d->m_type;
}

AkElementCategory Shagadelic::category() const
{
    return this->d->m_category;
}

void *Shagadelic::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Shagadelic::create(const QString &id)
{
    Q_UNUSED(id)

    return new Shagadelic;
}

int Shagadelic::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Shagadelic",
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

quint32 Shagadelic::mask() const
{
    return this->d->m_mask;
}

void Shagadelic::deleteThis(void *userData) const
{
    delete reinterpret_cast<Shagadelic *>(userData);
}

QString Shagadelic::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Shagadelic/share/qml/main.qml");
}

void Shagadelic::controlInterfaceConfigure(QQmlContext *context,
                                                  const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Shagadelic", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Shagadelic::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_curSize) {
        this->d->init(frameSize);
        this->d->m_curSize = frameSize;
    }

    for (int y = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        auto rLine = this->d->m_ripple.constLine(0, y + this->d->m_ry);
        auto gLine = this->d->m_spiral.constLine(0, y);
        auto bLine = this->d->m_ripple.constLine(0, y + this->d->m_by);

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = iLine[x];

            // Color saturation
            int r = qRed(pixel) > 127? 255: 0;
            int g = qGreen(pixel) > 127? 255: 0;
            int b = qBlue(pixel) > 127? 255: 0;
            int a = qAlpha(pixel);

            int pr = char(rLine[x + this->d->m_rx] + this->d->m_phase * 2) >> 7;
            int pg = char(gLine[x] + this->d->m_phase * 3) >> 7;
            int pb = char(bLine[x + this->d->m_by] - this->d->m_phase) >> 7;

            oLine[x] = qRgba(r, g, b, a) & qRgb(pr, pg, pb)
                       & (this->d->m_mask | 0xff000000);
        }
    }

    this->d->m_phase -= 8;

    if ((this->d->m_rx + this->d->m_rvx) < 0
        || (this->d->m_rx + this->d->m_rvx) >= src.caps().width())
        this->d->m_rvx = -this->d->m_rvx;

    if ((this->d->m_ry + this->d->m_rvy) < 0
        || (this->d->m_ry + this->d->m_rvy) >= src.caps().height())
        this->d->m_rvy = -this->d->m_rvy;

    if ((this->d->m_bx + this->d->m_bvx) < 0
        || (this->d->m_bx + this->d->m_bvx) >= src.caps().width())
        this->d->m_bvx = -this->d->m_bvx;

    if ((this->d->m_by + this->d->m_bvy) < 0
        || (this->d->m_by + this->d->m_bvy) >= src.caps().height())
        this->d->m_bvy = -this->d->m_bvy;

    this->d->m_rx += this->d->m_rvx;
    this->d->m_ry += this->d->m_rvy;
    this->d->m_bx += this->d->m_bvx;
    this->d->m_by += this->d->m_bvy;

    if (dst)
        this->oStream(dst);

    return dst;
}

void Shagadelic::setMask(quint32 mask)
{
    if (this->d->m_mask == mask)
        return;

    this->d->m_mask = mask;
    emit this->maskChanged(mask);
}

void Shagadelic::resetMask()
{
    this->setMask(0xffffff);
}

ShagadelicPrivate::ShagadelicPrivate(Shagadelic *self):
      self(self)
{

}

AkVideoPacket ShagadelicPrivate::makeRipple(const QSize &size) const
{
    AkVideoPacket ripple({AkVideoCaps::Format_y8,
                          2 * size.width(),
                          2 * size.height(),
                          {}});

    for (int y = 0; y < ripple.caps().height(); y++) {
        int yy = (y - size.width()) / size.width();
        auto oLine = reinterpret_cast<quint8 *>(ripple.line(0, y));

        for (int x = 0; x < ripple.caps().width(); x++) {
            int xx = (x - size.width()) / size.width();
            oLine[x] = uint(3000 * sqrt(xx * xx + yy * yy)) & 255;
        }
    }

    return ripple;
}

AkVideoPacket ShagadelicPrivate::makeSpiral(const QSize &size) const
{
    AkVideoPacket spiral({AkVideoCaps::Format_y8,
                          size.width(),
                          size.height(),
                          {}});
    int yc = spiral.caps().height() / 2;

    for (int y = 0; y < spiral.caps().height(); y++) {
        qreal yy = qreal(y - yc) / spiral.caps().width();
        auto oLine = reinterpret_cast<quint8 *>(spiral.line(0, y));

        for (int x = 0; x < spiral.caps().width(); x++) {
            qreal xx = qreal(x) / spiral.caps().width() - 0.5;
            oLine[x] = uint(256 * 9 * atan2(xx, yy) / M_PI
                            + 1800 * sqrt(xx * xx + yy * yy))
                       & 255;
        }
    }

    return spiral;
}

void ShagadelicPrivate::init(const QSize &size)
{
    this->m_ripple = this->makeRipple(size);
    this->m_spiral = this->makeSpiral(size);

    this->m_rx = QRandomGenerator::global()->bounded(size.width());
    this->m_ry = QRandomGenerator::global()->bounded(size.height());
    this->m_bx = QRandomGenerator::global()->bounded(size.width());
    this->m_by = QRandomGenerator::global()->bounded(size.height());

    this->m_rvx = -2;
    this->m_rvy = -2;
    this->m_bvx = 2;
    this->m_bvy = 2;

    this->m_phase = 0;
}

#include "moc_shagadelic.cpp"
