/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QMetaEnum>

#include "akaudiocaps.h"

typedef QMap<AkAudioCaps::SampleFormat, int> BitsPerSampleMap;

inline BitsPerSampleMap initBitsPerSampleMap()
{
    BitsPerSampleMap bitsPerSample;
    bitsPerSample[AkAudioCaps::SampleFormat_s8] = 8;
    bitsPerSample[AkAudioCaps::SampleFormat_u8] = 8;
    bitsPerSample[AkAudioCaps::SampleFormat_s16] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_s16le] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_s16be] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_u16] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_u16le] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_u16be] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_s24] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_s24le] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_s24be] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_u24] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_u24le] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_u24be] = 24;
    bitsPerSample[AkAudioCaps::SampleFormat_s32] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_s32le] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_s32be] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_u32] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_u32le] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_u32be] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_flt] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_fltle] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_fltbe] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_dbl] = 64;
    bitsPerSample[AkAudioCaps::SampleFormat_dblle] = 64;
    bitsPerSample[AkAudioCaps::SampleFormat_dblbe] = 64;
    bitsPerSample[AkAudioCaps::SampleFormat_u8p] = 8;
    bitsPerSample[AkAudioCaps::SampleFormat_s16p] = 16;
    bitsPerSample[AkAudioCaps::SampleFormat_s32p] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_fltp] = 32;
    bitsPerSample[AkAudioCaps::SampleFormat_dblp] = 64;

    return bitsPerSample;
}

Q_GLOBAL_STATIC_WITH_ARGS(BitsPerSampleMap, toBitsPerSample, (initBitsPerSampleMap()))

typedef QMap<AkAudioCaps::ChannelLayout, QString> ChannelLayoutStrMap;

inline ChannelLayoutStrMap initChannelLayoutStrMap()
{
    ChannelLayoutStrMap layoutToStr;
    layoutToStr[AkAudioCaps::Layout_none] = "none";
    layoutToStr[AkAudioCaps::Layout_mono] = "mono";
    layoutToStr[AkAudioCaps::Layout_stereo] = "stereo";
    layoutToStr[AkAudioCaps::Layout_2p1] = "2.1";
    layoutToStr[AkAudioCaps::Layout_3p0] = "3.0";
    layoutToStr[AkAudioCaps::Layout_3p0_back] = "3.0(back)";
    layoutToStr[AkAudioCaps::Layout_3p1] = "3.1";
    layoutToStr[AkAudioCaps::Layout_4p0] = "4.0";
    layoutToStr[AkAudioCaps::Layout_quad] = "quad";
    layoutToStr[AkAudioCaps::Layout_quad_side] = "quad(side)";
    layoutToStr[AkAudioCaps::Layout_4p1] = "4.1";
    layoutToStr[AkAudioCaps::Layout_5p0] = "5.0";
    layoutToStr[AkAudioCaps::Layout_5p0_side] = "5.0(side)";
    layoutToStr[AkAudioCaps::Layout_5p1] = "5.1";
    layoutToStr[AkAudioCaps::Layout_5p1_side] = "5.1(side)";
    layoutToStr[AkAudioCaps::Layout_6p0] = "6.0";
    layoutToStr[AkAudioCaps::Layout_6p0_front] = "6.0(front)";
    layoutToStr[AkAudioCaps::Layout_hexagonal] = "hexagonal";
    layoutToStr[AkAudioCaps::Layout_6p1] = "6.1";
    layoutToStr[AkAudioCaps::Layout_6p1_front] = "6.1(front)";
    layoutToStr[AkAudioCaps::Layout_7p0] = "7.0";
    layoutToStr[AkAudioCaps::Layout_7p0_front] = "7.0(front)";
    layoutToStr[AkAudioCaps::Layout_7p1] = "7.1";
    layoutToStr[AkAudioCaps::Layout_7p1_wide] = "7.1(wide)";
    layoutToStr[AkAudioCaps::Layout_7p1_wide_side] = "7.1(wide-side)";
    layoutToStr[AkAudioCaps::Layout_octagonal] = "octagonal";
    layoutToStr[AkAudioCaps::Layout_hexadecagonal] = "hexadecagonal";
    layoutToStr[AkAudioCaps::Layout_downmix] = "downmix";

    return layoutToStr;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelLayoutStrMap, layoutToStr, (initChannelLayoutStrMap()))

typedef QMap<AkAudioCaps::ChannelLayout, int> ChannelCountMap;

inline ChannelCountMap initChannelCountMap()
{
    ChannelCountMap channelCountMap;
    channelCountMap[AkAudioCaps::Layout_none] = 0;
    channelCountMap[AkAudioCaps::Layout_mono] = 1;
    channelCountMap[AkAudioCaps::Layout_stereo] = 2;
    channelCountMap[AkAudioCaps::Layout_2p1] = 3;
    channelCountMap[AkAudioCaps::Layout_3p0] = 3;
    channelCountMap[AkAudioCaps::Layout_3p0_back] = 3;
    channelCountMap[AkAudioCaps::Layout_3p1] = 4;
    channelCountMap[AkAudioCaps::Layout_4p0] = 4;
    channelCountMap[AkAudioCaps::Layout_quad] = 4;
    channelCountMap[AkAudioCaps::Layout_quad_side] = 4;
    channelCountMap[AkAudioCaps::Layout_4p1] = 5;
    channelCountMap[AkAudioCaps::Layout_5p0] = 5;
    channelCountMap[AkAudioCaps::Layout_5p0_side] = 5;
    channelCountMap[AkAudioCaps::Layout_5p1] = 6;
    channelCountMap[AkAudioCaps::Layout_5p1_side] = 6;
    channelCountMap[AkAudioCaps::Layout_6p0] = 6;
    channelCountMap[AkAudioCaps::Layout_6p0_front] = 6;
    channelCountMap[AkAudioCaps::Layout_hexagonal] = 6;
    channelCountMap[AkAudioCaps::Layout_6p1] = 7;
    channelCountMap[AkAudioCaps::Layout_6p1_front] = 7;
    channelCountMap[AkAudioCaps::Layout_7p0] = 7;
    channelCountMap[AkAudioCaps::Layout_7p0_front] = 7;
    channelCountMap[AkAudioCaps::Layout_7p1] = 8;
    channelCountMap[AkAudioCaps::Layout_7p1_wide] = 8;
    channelCountMap[AkAudioCaps::Layout_7p1_wide_side] = 8;
    channelCountMap[AkAudioCaps::Layout_octagonal] = 8;
    channelCountMap[AkAudioCaps::Layout_hexadecagonal] = 16;
    channelCountMap[AkAudioCaps::Layout_downmix] = 2;

    return channelCountMap;
}

Q_GLOBAL_STATIC_WITH_ARGS(ChannelCountMap, channelCountMap, (initChannelCountMap()))

class AkAudioCapsPrivate
{
    public:
        bool m_isValid;
        AkAudioCaps::SampleFormat m_format;
        int m_bps;
        int m_channels;
        int m_rate;
        AkAudioCaps::ChannelLayout m_layout;
        int m_samples;
        bool m_align;
};

AkAudioCaps::AkAudioCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = SampleFormat_none;
    this->d->m_bps = 0;
    this->d->m_channels = 0;
    this->d->m_rate = 0;
    this->d->m_layout = Layout_none;
    this->d->m_samples = 0;
    this->d->m_align = false;
}

AkAudioCaps::AkAudioCaps(const QVariantMap &caps)
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format == SampleFormat_none? false: false;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toBool();
}

AkAudioCaps::AkAudioCaps(const QString &caps)
{
    this->d = new AkAudioCapsPrivate();
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

AkAudioCaps::AkAudioCaps(const AkCaps &caps)
{
    this->d = new AkAudioCapsPrivate();

    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        this->d->m_format = this->sampleFormatFromString(caps.property("format").toString());
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

AkAudioCaps::AkAudioCaps(const AkAudioCaps &other):
    QObject()
{
    this->d = new AkAudioCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_bps = other.d->m_bps;
    this->d->m_channels = other.d->m_channels;
    this->d->m_rate = other.d->m_rate;
    this->d->m_layout = other.d->m_layout;
    this->d->m_samples = other.d->m_samples;
    this->d->m_align = other.d->m_align;
}

AkAudioCaps::~AkAudioCaps()
{
    delete this->d;
}

AkAudioCaps &AkAudioCaps::operator =(const AkAudioCaps &other)
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

AkAudioCaps &AkAudioCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "audio/x-raw") {
        this->d->m_isValid = caps.isValid();

        this->d->m_format = this->sampleFormatFromString(caps.property("format").toString());
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

AkAudioCaps &AkAudioCaps::operator =(const QString &caps)
{
    return this->operator =(AkCaps(caps));
}

bool AkAudioCaps::operator ==(const AkAudioCaps &other) const
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

bool AkAudioCaps::operator !=(const AkAudioCaps &other) const
{
    return !(*this == other);
}

AkAudioCaps::operator AkCaps() const
{
    return this->toCaps();
}

bool AkAudioCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &AkAudioCaps::isValid()
{
    return this->d->m_isValid;
}

AkAudioCaps::SampleFormat AkAudioCaps::format() const
{
    return this->d->m_format;
}

AkAudioCaps::SampleFormat &AkAudioCaps::format()
{
    return this->d->m_format;
}

int AkAudioCaps::bps() const
{
    return this->d->m_bps;
}

int &AkAudioCaps::bps()
{
    return this->d->m_bps;
}

int AkAudioCaps::channels() const
{
    return this->d->m_channels;
}

int &AkAudioCaps::channels()
{
    return this->d->m_channels;
}

int AkAudioCaps::rate() const
{
    return this->d->m_rate;
}

int &AkAudioCaps::rate()
{
    return this->d->m_rate;
}

AkAudioCaps::ChannelLayout AkAudioCaps::layout() const
{
    return this->d->m_layout;
}

AkAudioCaps::ChannelLayout &AkAudioCaps::layout()
{
    return this->d->m_layout;
}

int AkAudioCaps::samples() const
{
    return this->d->m_samples;
}

int &AkAudioCaps::samples()
{
    return this->d->m_samples;
}

bool AkAudioCaps::align() const
{
    return this->d->m_align;
}

bool &AkAudioCaps::align()
{
    return this->d->m_align;
}

AkAudioCaps &AkAudioCaps::fromMap(const QVariantMap &caps)
{
    this->d->m_format = caps["format"].value<SampleFormat>();
    this->d->m_isValid = this->d->m_format == AkAudioCaps::SampleFormat_none? false: false;
    this->d->m_bps = caps["bps"].toInt();
    this->d->m_channels = caps["channels"].toInt();
    this->d->m_rate = caps["rate"].toInt();
    this->d->m_layout = caps["layout"].value<ChannelLayout>();
    this->d->m_samples = caps["samples"].toInt();
    this->d->m_align = caps["align"].toBool();

    return *this;
}

AkAudioCaps::operator bool() const
{
    return this->d->m_isValid;
}

AkAudioCaps &AkAudioCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap AkAudioCaps::toMap() const
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

QString AkAudioCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString sampleFormat = this->sampleFormatToString(this->d->m_format);
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

AkAudioCaps &AkAudioCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "audio/x-raw")
        return *this;

    if (caps.contains("format"))
        this->d->m_format = this->sampleFormatFromString(caps.property("format").toString());

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

AkCaps AkAudioCaps::toCaps() const
{
    return AkCaps(this->toString());
}

int AkAudioCaps::bitsPerSample(AkAudioCaps::SampleFormat sampleFormat)
{
    return toBitsPerSample->value(sampleFormat, 0);
}

int AkAudioCaps::bitsPerSample(const QString &sampleFormat)
{
    return AkAudioCaps::bitsPerSample(AkAudioCaps::sampleFormatFromString(sampleFormat));
}

QString AkAudioCaps::sampleFormatToString(AkAudioCaps::SampleFormat sampleFormat)
{
    AkAudioCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(sampleFormat));
    format.remove("SampleFormat_");

    return format;
}

AkAudioCaps::SampleFormat AkAudioCaps::sampleFormatFromString(const QString &sampleFormat)
{
    AkAudioCaps caps;
    QString format = "SampleFormat_" + sampleFormat;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SampleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    int formatInt = formatEnum.keyToValue(format.toStdString().c_str());

    return static_cast<SampleFormat>(formatInt);
}

QString AkAudioCaps::channelLayoutToString(AkAudioCaps::ChannelLayout channelLayout)
{
    return layoutToStr->value(channelLayout, "none");
}

AkAudioCaps::ChannelLayout AkAudioCaps::channelLayoutFromString(const QString &channelLayout)
{
    return layoutToStr->key(channelLayout, Layout_none);
}

int AkAudioCaps::channelCount(AkAudioCaps::ChannelLayout channelLayout)
{
    return channelCountMap->value(channelLayout, 0);
}

int AkAudioCaps::channelCount(const QString &channelLayout)
{
    ChannelLayout layout = layoutToStr->key(channelLayout, Layout_none);

    return channelCountMap->value(layout, 0);
}

AkAudioCaps::ChannelLayout AkAudioCaps::defaultChannelLayout(int channelCount)
{
    return channelCountMap->key(channelCount, Layout_none);
}

QString AkAudioCaps::defaultChannelLayoutString(int channelCount)
{
    ChannelLayout layout = channelCountMap->key(channelCount, Layout_none);

    return layoutToStr->value(layout, "none");
}

void AkAudioCaps::setFormat(AkAudioCaps::SampleFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkAudioCaps::setBps(int bps)
{
    if (this->d->m_bps == bps)
        return;

    this->d->m_bps = bps;
    emit this->bpsChanged(bps);
}

void AkAudioCaps::setChannels(int channels)
{
    if (this->d->m_channels == channels)
        return;

    this->d->m_channels = channels;
    emit this->channelsChanged(channels);
}

void AkAudioCaps::setRate(int rate)
{
    if (this->d->m_rate == rate)
        return;

    this->d->m_rate = rate;
    emit this->rateChanged(rate);
}

void AkAudioCaps::setLayout(AkAudioCaps::ChannelLayout layout)
{
    if (this->d->m_layout == layout)
        return;

    this->d->m_layout = layout;
    emit this->layoutChanged(layout);
}

void AkAudioCaps::setSamples(int samples)
{
    if (this->d->m_samples == samples)
        return;

    this->d->m_samples = samples;
    emit this->samplesChanged(samples);
}

void AkAudioCaps::setAlign(bool align)
{
    if (this->d->m_align == align)
        return;

    this->d->m_align = align;
    emit this->alignChanged(align);
}

void AkAudioCaps::resetFormat()
{
    this->setFormat(SampleFormat_none);
}

void AkAudioCaps::resetBps()
{
    this->setBps(0);
}

void AkAudioCaps::resetChannels()
{
    this->setChannels(0);
}

void AkAudioCaps::resetRate()
{
    this->setRate(0);
}

void AkAudioCaps::resetLayout()
{
    this->setLayout(Layout_none);
}

void AkAudioCaps::resetSamples()
{
    this->setSamples(0);
}

void AkAudioCaps::resetAlign()
{
    this->setAlign(false);
}

QDebug operator <<(QDebug debug, const AkAudioCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkAudioCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkAudioCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
