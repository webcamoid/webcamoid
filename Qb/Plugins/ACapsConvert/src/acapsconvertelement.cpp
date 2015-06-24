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

#include "acapsconvertelement.h"

ACapsConvertElement::ACapsConvertElement(): QbElement()
{
    this->m_resampleContext = NULL;
    this->resetCaps();
}

ACapsConvertElement::~ACapsConvertElement()
{
    this->deleteSwrContext();
}

QString ACapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

void ACapsConvertElement::deleteSwrContext()
{
    if (this->m_resampleContext) {
        swr_free(&this->m_resampleContext);
        this->m_resampleContext = NULL;
    }
}

void ACapsConvertElement::setCaps(const QString &format)
{
    this->m_caps = QbCaps(format);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps("");
}

QbPacket ACapsConvertElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "audio/x-raw")
        return QbPacket();

    // Input Format
    AVSampleFormat iSampleFormat = av_get_sample_fmt(packet.caps().property("format").toString().toStdString().c_str());
    int iNChannels = packet.caps().property("channels").toInt();
    qint64 iChannelLayout = av_get_channel_layout(packet.caps().property("layout").toString().toStdString().c_str());
    int iNPlanes = av_sample_fmt_is_planar(iSampleFormat)? iNChannels: 1;
    int iSampleRate = packet.caps().property("rate").toInt();
    int iNSamples = packet.caps().property("samples").toInt();
    bool iAlign = packet.caps().property("align").toBool();

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

    qint64 oChannelLayout = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("layout"))?
                                 av_get_channel_layout(this->m_caps.property("layout").toString().toStdString().c_str()):
                                 iChannelLayout;

    int oSampleRate = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("rate"))?
                          this->m_caps.property("rate").toInt():
                          iSampleRate;

    bool oAlign = (sameMimeType && this->m_caps.dynamicPropertyNames().contains("align"))?
                      this->m_caps.property("align").toBool():
                      iAlign;

    QVector<uint8_t *> iData(iNPlanes);
    int iLineSize;

    if (av_samples_fill_arrays(&iData.data()[0],
                               &iLineSize,
                               (const uint8_t *) packet.buffer().data(),
                               iNChannels,
                               iNSamples,
                               iSampleFormat,
                               iAlign? 0: 1) < 0)
        return QbPacket();

    QbCaps caps1(packet.caps());
    QbCaps caps2(this->m_curInputCaps);

    caps1.setProperty("samples", QVariant());
    caps2.setProperty("samples", QVariant());
    caps1.setProperty("align", QVariant());
    caps2.setProperty("align", QVariant());

    if (caps1 != caps2) {
        this->deleteSwrContext();

            // create resampler context
        this->m_resampleContext = swr_alloc();

        if (!this->m_resampleContext)
            return QbPacket();

        // set options
        av_opt_set_int(this->m_resampleContext, "in_channel_layout", iChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext, "in_sample_rate", iSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext, "in_sample_fmt", iSampleFormat, 0);

        av_opt_set_int(this->m_resampleContext, "out_channel_layout", oChannelLayout, 0);
        av_opt_set_int(this->m_resampleContext, "out_sample_rate", oSampleRate, 0);
        av_opt_set_sample_fmt(this->m_resampleContext, "out_sample_fmt", oSampleFormat, 0);

        // initialize the resampling context
        if (swr_init(this->m_resampleContext) < 0)
            return QbPacket();

        this->m_curInputCaps = packet.caps();
    }

    // compute destination number of samples
    int oNSamples = av_rescale_rnd(swr_get_delay(this->m_resampleContext,
                                                 iSampleRate) +
                                   iNSamples,
                                   oSampleRate,
                                   iSampleRate,
                                   AV_ROUND_UP);

    // buffer is going to be directly written to a rawaudio file, no alignment
    int oNPlanes = av_sample_fmt_is_planar(oSampleFormat)? oNChannels: 1;
    QVector<uint8_t *> oData(oNPlanes);

    int oBps = av_get_bytes_per_sample(oSampleFormat);
    int oLineSize;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 oNChannels,
                                                 oNSamples,
                                                 oSampleFormat,
                                                 oAlign? 0: 1);

    QbBufferPtr oBuffer(new char[oBufferSize]);

    if (!oBuffer)
        return QbPacket();

    if (av_samples_fill_arrays(&oData.data()[0],
                               &oLineSize,
                               (const uint8_t *) oBuffer.data(),
                               oNChannels,
                               oNSamples,
                               oSampleFormat,
                               oAlign? 0: 1) < 0)
        return QbPacket();

    // convert to destination format
    oNSamples = swr_convert(this->m_resampleContext,
                            oData.data(),
                            oNSamples,
                            (const uint8_t **) iData.data(),
                            iNSamples);

    if (oNSamples < 1)
        return QbPacket();

    const char *format = av_get_sample_fmt_name(oSampleFormat);
    char layout[256];

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 oNChannels,
                                 oChannelLayout);

    QString caps = QString("audio/x-raw,"
                           "format=%1,"
                           "bps=%2,"
                           "channels=%3,"
                           "rate=%4,"
                           "layout=%5,"
                           "samples=%6,"
                           "align=%7").arg(format)
                                      .arg(oBps)
                                      .arg(oNChannels)
                                      .arg(oSampleRate)
                                      .arg(layout)
                                      .arg(oNSamples)
                                      .arg(oAlign);

    int bufferSize = oBps * oNChannels * oNSamples;

    if (bufferSize < oBufferSize)
        oBufferSize = bufferSize;

    QbPacket oPacket(caps,
                     oBuffer,
                     oBufferSize);

    oPacket.setPts(packet.pts());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    qbSend(oPacket)
}
