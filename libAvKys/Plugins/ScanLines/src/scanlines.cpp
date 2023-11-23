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

#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "scanlines.h"

class ScanLinesPrivate
{
    public:
        ScanLines *self {nullptr};
        QString m_description {QObject::tr("Scan Lines")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        int m_showSize {1};
        int m_hideSize {4};
        QRgb m_hideColor {qRgb(0, 0, 0)};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        qint64 *m_aiMultTable {nullptr};
        qint64 *m_aoMultTable {nullptr};
        qint64 *m_alphaDivTable {nullptr};

        explicit ScanLinesPrivate(ScanLines *self);
};

ScanLines::ScanLines(QObject *parent):
      QObject(parent)
{
    this->d = new ScanLinesPrivate(this);
    constexpr qint64 maxAi = 255;
    qint64 maxAi2 = maxAi * maxAi;
    constexpr qint64 alphaMult = 1 << 16;
    this->d->m_aiMultTable = new qint64 [alphaMult];
    this->d->m_aoMultTable = new qint64 [alphaMult];
    this->d->m_alphaDivTable = new qint64 [alphaMult];

    for (qint64 ai = 0; ai < 256; ai++)
        for (qint64 ao = 0; ao < 256; ao++) {
            auto alphaMask = (ai << 8) | ao;
            auto a = maxAi2 - (maxAi - ai) * (maxAi - ao);
            this->d->m_aiMultTable[alphaMask] = a? alphaMult * ai * maxAi / a: 0;
            this->d->m_aoMultTable[alphaMask] = a? alphaMult * ao * (maxAi - ai) / a: 0;
            this->d->m_alphaDivTable[alphaMask] = a / maxAi;
        }
}

ScanLines::~ScanLines()
{
    delete [] this->d->m_aiMultTable;
    delete [] this->d->m_aoMultTable;
    delete [] this->d->m_alphaDivTable;
    delete this->d;
}

QString ScanLines::description() const
{
    return this->d->m_description;
}

AkElementType ScanLines::type() const
{
    return this->d->m_type;
}

AkElementCategory ScanLines::category() const
{
    return this->d->m_category;
}

void *ScanLines::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *ScanLines::create(const QString &id)
{
    Q_UNUSED(id)

    return new ScanLines;
}

int ScanLines::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/ScanLines",
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

int ScanLines::showSize() const
{
    return this->d->m_showSize;
}

int ScanLines::hideSize() const
{
    return this->d->m_hideSize;
}

QRgb ScanLines::hideColor() const
{
    return this->d->m_hideColor;
}

void ScanLines::deleteThis(void *userData) const
{
    delete reinterpret_cast<ScanLines *>(userData);
}

QString ScanLines::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ScanLines/share/qml/main.qml");
}

void ScanLines::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ScanLines", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket ScanLines::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto dst = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!dst)
        return {};

    int showSize = this->d->m_showSize;
    int hideSize = this->d->m_hideSize;
    int stripeSize = showSize + hideSize;

    if (stripeSize < 1) {
        showSize = 1;
        stripeSize = 2;
    }

    auto hideColor = this->d->m_hideColor;
    auto ri = qRed(hideColor);
    auto gi = qGreen(hideColor);
    auto bi = qBlue(hideColor);
    auto ai = qAlpha(hideColor);
    int i = 0;

    for (int y = 0; y < dst.caps().height(); y++) {
        if (i >= showSize) {
            auto line = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < dst.caps().width(); x++) {
                auto &pixel = line[x];

                qint64 ro = qRed(pixel);
                qint64 go = qGreen(pixel);
                qint64 bo = qBlue(pixel);
                qint64 ao = qAlpha(pixel);

                auto alphaMask = (ai << 8) | ao;
                qint64 rt = (ri * this->d->m_aiMultTable[alphaMask] + ro * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 gt = (gi * this->d->m_aiMultTable[alphaMask] + go * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 bt = (bi * this->d->m_aiMultTable[alphaMask] + bo * this->d->m_aoMultTable[alphaMask]) >> 16;
                qint64 &at = this->d->m_alphaDivTable[alphaMask];

                pixel = qRgba(int(rt), int(gt), int(bt), int(at));
            }
        }

        i = (i + 1) % stripeSize;
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

void ScanLines::setShowSize(int showSize)
{
    if (this->d->m_showSize == showSize)
        return;

    this->d->m_showSize = showSize;
    emit this->showSizeChanged(showSize);
}

void ScanLines::setHideSize(int hideSize)
{
    if (this->d->m_hideSize == hideSize)
        return;

    this->d->m_hideSize = hideSize;
    emit this->hideSizeChanged(hideSize);
}

void ScanLines::setHideColor(QRgb hideColor)
{
    if (this->d->m_hideColor == hideColor)
        return;

    this->d->m_hideColor = hideColor;
    emit this->hideColorChanged(hideColor);
}

void ScanLines::resetShowSize()
{
    this->setShowSize(1);
}

void ScanLines::resetHideSize()
{
    this->setHideSize(4);
}

void ScanLines::resetHideColor()
{
    this->setHideColor(qRgb(0, 0, 0));
}

ScanLinesPrivate::ScanLinesPrivate(ScanLines *self):
      self(self)
{

}

#include "moc_scanlines.cpp"
