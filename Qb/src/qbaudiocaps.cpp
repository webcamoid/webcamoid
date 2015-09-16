/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QMetaEnum>

#include "qbaudiocaps.h"

class QbAudioCapsPrivate
{
    public:
        bool m_isValid;
        QbAudioCaps::SampleFormat m_format;
        int m_bps;
        int m_channels;
        int m_rate;
        QbAudioCaps::ChannelLayout m_layout;
        int m_samples;
        bool m_align;
};

typedef QMap<QbAudioCaps::ChannelLayout, QString> ChannelLayoutStrMap;

inline ChannelLayoutStrMap initChannelLayoutStrMap()
{
    ChannelLayoutStrMap layoutToStr;
    layoutToStr[QbAudioCaps::Layout_none] = "none";
    layoutToStr[QbAudioCaps::Layout_mono] = "mono";
    layoutToStr[QbAudioCaps::Layout_stereo] = "stereo";
    layoutToStr[QbAudioCaps::Layout_2p1] = "2.1";
    layoutToStr[QbAudioCaps::Layout_3p0] = "3.0";
    layoutToStr[QbAudioCaps::Layout_3p0_back] = "3.0(back)";
    layoutToStr[QbAudioCaps::Layout_3p1] = "3.1";
    layoutToStr[QbAudioCaps::Layout_4p0] = "4.0";
    layoutToStr[QbAudioCaps::Layout_quad] = "quad";
    layoutToStr[QbAudioCaps::Layout_quad_side] = "quad(side)";
    layoutToStr[QbAudioCaps::Layout_4p1] = "4.1";
    layoutToStr[QbAudioCaps::Layout_5p0] = "5.0";
    layoutToStr[QbAudioCaps::Layout_5p0_side] = "5.0(side)";
    layoutToStr[QbAudioCaps::Layout_5p1] = "5.1";
    layoutToStr[QbAudioCaps::Layout_5p1_side] = "5.1(side)";
    layoutToStr[QbAudioCaps::Layout_6p0] = "6.0";
    layoutToStr[QbAudioCaps::Layout_6p0_front] = "6.0(front)";
    layoutToStr[QbAudioCaps::Layout_hexagonal] = "hexagonal";
    layoutToStr[QbAudioCaps::Layout_6p1] = "6.1";
    layoutToStr[QbAudioCaps::Layout_6p1_front] = "6.1(front)";
    layoutToStr[QbAudioCaps::Layout_7p0] = "7.0";
    layoutToStr[QbAudioCaps::Layout_7p0_front] = "7.0(front)";
    layoutToStr[QbAudioCaps::Layout_7p1] = "7.1";
    layoutToStr[QbAudioCaps::Layout_7p1_wide] = "7.1(wide)";
    layoutToStr[QbAudioCaps::Layout_7p1_wide_side] = "7.1(wide-side)";
    layoutToStr[QbAudioCaps::Layout_octagonal] = "octagonal";
    layoutToStr[QbAudioCaps::Layout_downmix] = "downmix";

    return layoutToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutStrMap, layoutToStr, (initChannelLayoutStrMap()))

QbAudioCaps::QbAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new QbAudioCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = SampleFormat_none;
    this->d->m_bps = 0;
    this->d->m_channels = 0;
    this->d->m_rate = 0;
    this->d->m_layout = Layout_none;
    this->d->m_samples = 0;
    this->d->m_align = false;
}

QbAudioCaps::QbAudioCaps(const QVariantMap &caps)
{
    this->d = new QbAudioCapsPrivate();
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format == SampleFormat_none? false: false;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toBool();
}

QbAudioCaps::QbAudioCaps(const QString &caps)
{
    this->d = new QbAudioCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = SampleFormat_none;
    this->d->m_bps = 0;
    this->d->m_channels = 0;
    this->d->m_rate = 0;
    this->d->m_layout = Layout_none;
    this->d->m_samples = 0;
    this->d->m_align = false;
    *this = caps;
}

QbAudioCaps::QbAudioCaps(const QbCaps &caps)
{
    this->d = new QbAudioCapsPrivate();

    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        QString format = "SampleFormat_" + caps.property("format").toString();
        int formatIndex = this->metaObject()->indexOfEnumerator("SampleFormat");
        QMetaEnum formatEnum = this->metaObject()->enumerator(formatIndex);
        int formatInt = formatEnum.keyToValue(format.toStdString().c_str());
        this->d->m_format = static_cast<SampleFormat>(formatInt);

        this->d->m_bps = caps.property("bps").toInt();
        this->d->m_channels = caps.property("channels").toInt();
        this->d->m_rate = caps.property("rate").toInt();

        QString layout = caps.property("layout").toString();
        this->d->m_layout = layoutToStr->key(layout, Layout_none);

        this->d->m_samples = caps.property("samples").toInt();
        this->d->m_align = caps.property("align").toBool();
    } else {
        this->d->m_isValid = false;
        this->d->m_format = SampleFormat_none;
        this->d->m_bps = 0;
        this->d->m_channels = 0;
        this->d->m_rate = 0;
        this->d->m_layout = Layout_none;
        this->d->m_samples = 0;
        this->d->m_align = false;
    }
}

QbAudioCaps::QbAudioCaps(const QbAudioCaps &other):
    QObject()
{
    this->d = new QbAudioCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_bps = other.d->m_bps;
    this->d->m_channels = other.d->m_channels;
    this->d->m_rate = other.d->m_rate;
    this->d->m_layout = other.d->m_layout;
    this->d->m_samples = other.d->m_samples;
    this->d->m_align = other.d->m_align;
}

QbAudioCaps::~QbAudioCaps()
{
    delete this->d;
}

QbAudioCaps &QbAudioCaps::operator =(const QbAudioCaps &other)
{
    if (this != &other) {
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_format = other.d->m_format;
        this->d->m_bps = other.d->m_bps;
        this->d->m_channels = other.d->m_channels;
        this->d->m_rate = other.d->m_rate;
        this->d->m_layout = other.d->m_layout;
        this->d->m_samples = other.d->m_samples;
        this->d->m_align = other.d->m_align;
    }

    return *this;
}

QbAudioCaps &QbAudioCaps::operator =(const QbCaps &caps)
{
    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        QString format = "SampleFormat_" + caps.property("format").toString();
        int formatIndex = this->metaObject()->indexOfEnumerator("SampleFormat");
        QMetaEnum formatEnum = this->metaObject()->enumerator(formatIndex);
        int formatInt = formatEnum.keyToValue(format.toStdString().c_str());
        this->d->m_format = static_cast<SampleFormat>(formatInt);

        this->d->m_bps = caps.property("bps").toInt();
        this->d->m_channels = caps.property("channels").toInt();
        this->d->m_rate = caps.property("rate").toInt();

        QString layout = caps.property("layout").toString();
        this->d->m_layout = layoutToStr->key(layout, Layout_none);

        this->d->m_samples = caps.property("samples").toInt();
        this->d->m_align = caps.property("align").toBool();
    } else {
        this->d->m_isValid = false;
        this->d->m_format = SampleFormat_none;
        this->d->m_bps = 0;
        this->d->m_channels = 0;
        this->d->m_rate = 0;
        this->d->m_layout = Layout_none;
        this->d->m_samples = 0;
        this->d->m_align = false;
    }

    return *this;
}

QbAudioCaps &QbAudioCaps::operator =(const QString &caps)
{
    return this->operator =(QbCaps(caps));
}

bool QbAudioCaps::operator ==(const QbAudioCaps &other) const
{
    if (this->d->m_isValid == other.d->m_isValid
        && this->d->m_format == other.d->m_format
        && this->d->m_bps == other.d->m_bps
        && this->d->m_channels == other.d->m_channels
        && this->d->m_rate == other.d->m_rate
        && this->d->m_layout == other.d->m_layout
        && this->d->m_samples == other.d->m_samples
        && this->d->m_align == other.d->m_align)
        return true;

    return false;
}

bool QbAudioCaps::operator !=(const QbAudioCaps &other) const
{
    return !(*this == other);
}

bool QbAudioCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &QbAudioCaps::isValid()
{
    return this->d->m_isValid;
}

QbAudioCaps::SampleFormat QbAudioCaps::format() const
{
    return this->d->m_format;
}

QbAudioCaps::SampleFormat &QbAudioCaps::format()
{
    return this->d->m_format;
}

int QbAudioCaps::bps() const
{
    return this->d->m_bps;
}

int &QbAudioCaps::bps()
{
    return this->d->m_bps;
}

int QbAudioCaps::channels() const
{
    return this->d->m_channels;
}

int &QbAudioCaps::channels()
{
    return this->d->m_channels;
}

int QbAudioCaps::rate() const
{
    return this->d->m_rate;
}

int &QbAudioCaps::rate()
{
    return this->d->m_rate;
}

QbAudioCaps::ChannelLayout QbAudioCaps::layout() const
{
    return this->d->m_layout;
}

QbAudioCaps::ChannelLayout &QbAudioCaps::layout()
{
    return this->d->m_layout;
}

int QbAudioCaps::samples() const
{
    return this->d->m_samples;
}

int &QbAudioCaps::samples()
{
    return this->d->m_samples;
}

bool QbAudioCaps::align() const
{
    return this->d->m_align;
}

bool &QbAudioCaps::align()
{
    return this->d->m_align;
}

QbAudioCaps &QbAudioCaps::fromMap(const QVariantMap &caps)
{
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format == QbAudioCaps::SampleFormat_none? false: false;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toBool();

    return *this;
}

QbAudioCaps::operator bool() const
{
    return this->d->m_isValid;
}

QbAudioCaps &QbAudioCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap QbAudioCaps::toMap() const
{
    QVariantMap map;
    map["format"] = this->d->m_format;
    map["bps"] = this->d->m_bps;
    map["channels"] = this->d->m_channels;
    map["rate"] = this->d->m_rate;
    map["layout"] = this->d->m_layout;
    map["samples"] = this->d->m_samples;
    map["align"] = this->d->m_align;

    return map;
}

QString QbAudioCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    int formatIndex = this->metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = this->metaObject()->enumerator(formatIndex);
    QString sampleFormat(formatEnum.valueToKey(this->d->m_format));
    sampleFormat.remove("SampleFormat_");

    QString layout = layoutToStr->value(this->d->m_layout, "none");

    return QString("audio/x-raw,"
                   "format=%1,"
                   "bps=%2,"
                   "channels=%3,"
                   "rate=%4,"
                   "layout=%5,"
                   "samples=%6,"
                   "align=%7").arg(sampleFormat)
                              .arg(this->d->m_bps)
                              .arg(this->d->m_channels)
                              .arg(this->d->m_rate)
                              .arg(layout)
                              .arg(this->d->m_samples)
            .arg(this->d->m_align);
}

QbAudioCaps &QbAudioCaps::update(const QbCaps &caps)
{
    if (caps.mimeType() != "audio/x-raw")
        return *this;

    if (caps.contains("format")) {
        QString format = "SampleFormat_" + caps.property("format").toString();
        int formatIndex = this->metaObject()->indexOfEnumerator("SampleFormat");
        QMetaEnum formatEnum = this->metaObject()->enumerator(formatIndex);
        int formatInt = formatEnum.keyToValue(format.toStdString().c_str());
        this->d->m_format = static_cast<SampleFormat>(formatInt);
    }

    if (caps.contains("bps"))
        this->d->m_bps = caps.property("bps").toInt();

    if (caps.contains("channels"))
        this->d->m_channels = caps.property("channels").toInt();

    if (caps.contains("rate"))
        this->d->m_rate = caps.property("rate").toInt();

    if (caps.contains("layout")) {
        QString layout = caps.property("layout").toString();
        this->d->m_layout = layoutToStr->key(layout, Layout_none);
    }

    if (caps.contains("samples"))
        this->d->m_samples = caps.property("samples").toInt();

    if (caps.contains("align"))
        this->d->m_align = caps.property("align").toBool();

    return *this;
}

QbCaps QbAudioCaps::toCaps() const
{
    return QbCaps(this->toString());
}

void QbAudioCaps::setFormat(QbAudioCaps::SampleFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void QbAudioCaps::setBps(int bps)
{
    if (this->d->m_bps == bps)
        return;

    this->d->m_bps = bps;
    emit this->bpsChanged(bps);
}

void QbAudioCaps::setChannels(int channels)
{
    if (this->d->m_channels == channels)
        return;

    this->d->m_channels = channels;
    emit this->channelsChanged(channels);
}

void QbAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void QbAudioCaps::setLayout(QbAudioCaps::ChannelLayout layout)
{
    if (this->d->m_layout == layout)
        return;

    this->d->m_layout = layout;
    emit this->layoutChanged(layout);
}

void QbAudioCaps::setSamples(int samples)
{
    if (this->d->m_samples == samples)
        return;

    this->d->m_samples = samples;
    emit this->samplesChanged(samples);
}

void QbAudioCaps::setAlign(bool align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    emit this->alignChanged(align);
}

void QbAudioCaps::resetFormat()
{
    this->setFormat(SampleFormat_none);
}

void QbAudioCaps::resetBps()
{
    this->setBps(0);
}

void QbAudioCaps::resetChannels()
{
    this->setChannels(0);
}

void QbAudioCaps::resetRate()
{
    this->setRate(0);
}

void QbAudioCaps::resetLayout()
{
    this->setLayout(Layout_none);
}

void QbAudioCaps::resetSamples()
{
    this->setSamples(0);
}

void QbAudioCaps::resetAlign()
{
    this->setAlign(false);
}

QDebug operator <<(QDebug debug, const QbAudioCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, QbAudioCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const QbAudioCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
