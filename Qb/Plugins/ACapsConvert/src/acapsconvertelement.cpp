/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "acapsconvertelement.h"

ACapsConvertElement::ACapsConvertElement(): QbElement()
{
    this->resetCaps();
}

ACapsConvertElement::~ACapsConvertElement()
{
}

QString ACapsConvertElement::caps()
{
    return this->m_caps.toString();
}

void ACapsConvertElement::deleteSwrContext(SwrContext *context)
{
    swr_free(&context);
}

void ACapsConvertElement::setCaps(QString format)
{
    this->m_caps = QbCaps(format);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps("");
}

void ACapsConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "audio/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    // Input Format
    AVSampleFormat iSampleFormat = av_get_sample_fmt(packet.caps().property("format").toString().toStdString().c_str());
    int iNChannels = packet.caps().property("channels").toInt();
    int64_t iChannelLayout = av_get_channel_layout(packet.caps().property("layout").toString().toStdString().c_str());
    int iNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    int iSampleRate = packet.caps().property("rate").toInt();
    int iNSamples = packet.caps().property("samples").toInt();

    if (iNSamples < 1)
        iNSamples = 1024;

    bool sameMimeType = packet.caps().mimeType() == this->m_caps.mimeType();

    // Output Format
    AVSampleFormat oSampleFormat = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("format"))?
                                        av_get_sample_fmt(this->m_caps.property("format").toString().toStdString().c_str()):
                                        iSampleFormat;

    int oNChannels = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("channels"))?
                         this->m_caps.property("channels").toInt():
                         iNChannels;

    int64_t oChannelLayout = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("layout"))?
                                 av_get_channel_layout(this->m_caps.property("layout").toString().toStdString().c_str()):
                                 iChannelLayout;

    int oSampleRate = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("rate"))?
                          this->m_caps.property("rate").toInt():
                          iSampleRate;

    QVector<uint8_t *> iData(iNPlanes);
    int iLineSize;

    if (av_samples_fill_arrays(&iData.data()[0],
                               &iLineSize,
                               (const uint8_t *) packet.buffer().data(),
                               iNChannels,
                               iNSamples,
                               iSampleFormat,
                               1) < 0)
        return;

    QbCaps caps1(packet.caps());
    QbCaps caps2(this->m_curInputCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());

    if (caps1 != caps2)
    {
        // create resampler context
        this->m_resampleContext = SwrContextPtr(swr_alloc(), this->deleteSwrContext);

        if (!this->m_resampleContext)
            return;

        // set options
        av_opt_set_int(this->m_resampleContext.data(), "in_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "in_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "in_sample_fmt", iSampleFormat, 0);

        av_opt_set_int(this->m_resampleContext.data(), "out_channel_layout", oChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext.data(), "out_sample_rate", oSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext.data(), "out_sample_fmt", oSampleFormat, 0);

        // initialize the resampling context
        if (swr_init(this->m_resampleContext.data()) < 0)
            return;

        this->m_curInputCaps = packet.caps();
    }

    // compute destination number of samples
    int oNSamples = av_rescale_rnd(swr_get_delay(this->m_resampleContext.data(),
                                                 iSampleRate) +
                                   iNSamples,
                                   oSampleRate,
                                   iSampleRate,
                                   AV_ROUND_UP);

    // buffer is going to be directly written to a rawaudio file, no alignment
    int oNPlanes = av_sample_fmt_is_planar(oSampleFormat)? oNChannels: 1;
    QVector<uint8_t *> oData(oNPlanes);

    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 oNChannels,
                                                 oNSamples,
                                                 oSampleFormat,
                                                 1);

    QSharedPointer<uchar> oBuffer(new uchar[oBufferSize]);

    if (!oBuffer)
        return;

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               oNChannels,
                               oNSamples,
                               oSampleFormat,
                               1) < 0)
        return;

    // convert to destination format
    if (swr_convert(this->m_resampleContext.data(),
                    oData.data(),
                    oNSamples,
                    (const uint8_t **) iData.data(),
                    iNSamples) < 0)
        return;

    const char *format = av_get_sample_fmt_name(oSampleFormat);
    char layout[256];

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 oNChannels,
                                 oChannelLayout);

    QString caps = QString("audio/x-raw,"
                           "format=%1,"
                           "channels=%2,"
                           "rate=%3,"
                           "layout=%4,"
                           "samples=%5").arg(format)
                                        .arg(oNChannels)
                                        .arg(oSampleRate)
                                        .arg(layout)
                                        .arg(oNSamples);

    QbPacket oPacket(caps,
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
