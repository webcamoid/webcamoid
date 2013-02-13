/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
    this->m_iData = NULL;
    this->m_oData = NULL;
    this->m_resampleContext = NULL;

    this->resetCaps();

    this->m_formatToFF["U8"] = AV_SAMPLE_FMT_U8;
    this->m_formatToFF["S16LE"] = AV_SAMPLE_FMT_S16;
    this->m_formatToFF["S32LE"] = AV_SAMPLE_FMT_S32;
    this->m_formatToFF["F32LE"] = AV_SAMPLE_FMT_FLT;
    this->m_formatToFF["F64LE"] = AV_SAMPLE_FMT_DBL;
}

ACapsConvertElement::~ACapsConvertElement()
{
    this->cleanAll();
}

QList<QbCaps> ACapsConvertElement::oCaps()
{
    QList<QbCaps> caps;

    if (!this->m_srcs.isEmpty())
    {
        foreach (QbElement *src, this->m_srcs)
            foreach (QbCaps cap, src->oCaps())
                if (cap.mimeType() == this->m_caps.mimeType())
                {
                    caps << cap.update(this->m_caps);

                    return caps;
                }
    }

    return caps;
}

QString ACapsConvertElement::caps()
{
    return this->m_caps.toString();
}

bool ACapsConvertElement::init()
{
    // create resampler context
    this->m_resampleContext = swr_alloc();

    if (!this->m_resampleContext)
        return false;

    // Input Format
    this->m_iSampleFormat = this->m_formatToFF[this->m_curInputCaps.property("format").toString()];
    this->m_iNChannels = this->m_curInputCaps.property("channels").toInt();
    int64_t iChannelLayout = av_get_channel_layout(this->m_curInputCaps.property("layout").toString().toUtf8().constData());
    this->m_iSampleRate = this->m_curInputCaps.property("rate").toInt();
    this->m_iNSamples = this->m_curInputCaps.property("samples").toInt();

    if (this->m_iNSamples < 1)
        this->m_iNSamples = 1024;

    // Output Format
    if (this->m_curInputCaps.mimeType() == this->m_caps.mimeType())
    {
        if (this->m_caps.dynamicPropertyNames().contains("format"))
            this->m_oSampleFormat = this->m_formatToFF[this->m_caps.property("format").toString()];
        else
            this->m_oSampleFormat = this->m_iSampleFormat;

        if (this->m_caps.dynamicPropertyNames().contains("channels"))
            this->m_oNChannels = this->m_caps.property("channels").toInt();
        else
            this->m_oNChannels = this->m_iNChannels;

        if (this->m_caps.dynamicPropertyNames().contains("layout"))
            this->m_oChannelLayout = av_get_channel_layout(this->m_caps.property("layout").toString().toUtf8().constData());
        else
            this->m_oChannelLayout = iChannelLayout;

        if (this->m_caps.dynamicPropertyNames().contains("rate"))
            this->m_oSampleRate = this->m_caps.property("rate").toInt();
        else
            this->m_oSampleRate = this->m_iSampleRate;
    }
    else
    {
        this->m_oSampleFormat = this->m_iSampleFormat;
        this->m_oNChannels = this->m_iNChannels;
        this->m_oChannelLayout = iChannelLayout;
        this->m_oSampleRate = this->m_iSampleRate;
    }

    // set options
    av_opt_set_int(this->m_resampleContext, "in_channel_layout", iChannelLayout, 0);
    av_opt_set_int(this->m_resampleContext, "in_sample_rate", this->m_iSampleRate, 0);
    av_opt_set_int(this->m_resampleContext, "in_sample_fmt", this->m_iSampleFormat, 0);

    av_opt_set_int(this->m_resampleContext, "out_channel_layout", this->m_oChannelLayout, 0);
    av_opt_set_int(this->m_resampleContext, "out_sample_rate", this->m_oSampleRate, 0);
    av_opt_set_int(this->m_resampleContext, "out_sample_fmt", this->m_oSampleFormat, 0);

    // initialize the resampling context
    if (swr_init(this->m_resampleContext) < 0)
        return false;

    // allocate source and destination samples buffers
    this->m_iData = NULL;
    int iLineSize;
    int iNPlanes = av_sample_fmt_is_planar(this->m_iSampleFormat)? this->m_iNChannels: 1;
    this->m_iData = (uint8_t **) av_malloc(sizeof(this->m_iData) * iNPlanes);

    if (!this->m_iData)
        return false;

    if (av_samples_alloc(this->m_iData,
                         &iLineSize,
                         this->m_iNChannels,
                         this->m_iNSamples,
                         this->m_iSampleFormat,
                         0) < 0)
    {
        av_freep(this->m_iData);

        return false;
    }

    // compute the number of converted samples: buffering is avoided
    // ensuring that the output buffer will contain at least all the
    // converted input samples
    int oNSamples = av_rescale_rnd(this->m_iNSamples,
                                   this->m_oSampleRate,
                                   this->m_iSampleRate,
                                   AV_ROUND_UP);

    this->m_oMaxNSamples = oNSamples;

    // buffer is going to be directly written to a rawaudio file, no alignment
    this->m_oData = NULL;

    int oNPlanes = av_sample_fmt_is_planar(this->m_oSampleFormat)? this->m_oNChannels: 1;
    this->m_oData = (uint8_t **) av_malloc(sizeof(this->m_oData) * oNPlanes);

    if (!this->m_oData)
    {
        av_freep(this->m_iData[0]);
        av_freep(this->m_iData);

        return false;
    }

    int oLineSize;

    if (av_samples_alloc(this->m_oData,
                     &oLineSize,
                     this->m_oNChannels,
                     oNSamples,
                     this->m_oSampleFormat,
                     0) < 0)
    {
        av_freep(this->m_iData[0]);
        av_freep(this->m_iData);

        return false;
    }

    return true;
}

void ACapsConvertElement::uninit()
{
    if (this->m_iData)
    {
        av_freep(&this->m_iData[0]);
        av_freep(this->m_iData);
        this->m_iData = NULL;
    }

    if (this->m_oData)
    {
        av_freep(&this->m_oData[0]);
        av_freep(&this->m_oData);
        this->m_oData = NULL;
    }

    if (this->m_resampleContext)
    {
        swr_free(&this->m_resampleContext);
        this->m_resampleContext = NULL;
    }
}

void ACapsConvertElement::cleanAll()
{
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

    if (packet.caps() != this->m_curInputCaps)
    {
        this->uninit();
        this->m_curInputCaps = packet.caps();
        this->init();
    }

    int iLineSize;

    if (av_samples_fill_arrays(&this->m_iData[0],
                               &iLineSize,
                               (const uint8_t *) packet.data(),
                               this->m_iNChannels,
                               this->m_iNSamples,
                               this->m_iSampleFormat,
                               1) < 0)
        return;

    // compute destination number of samples
    int oNSamples = av_rescale_rnd(swr_get_delay(this->m_resampleContext,
                                                 this->m_iSampleRate) +
                                   this->m_iNSamples,
                                   this->m_oSampleRate,
                                   this->m_iSampleRate,
                                   AV_ROUND_UP);

    int oLineSize;

    if (oNSamples > this->m_oMaxNSamples)
    {
        av_free(this->m_oData[0]);

        if (av_samples_alloc(this->m_oData,
                             &oLineSize,
                             this->m_oNChannels,
                             oNSamples,
                             this->m_oSampleFormat,
                             1) < 0)
            return;

       this->m_oMaxNSamples = oNSamples;
    }

    oNSamples = swr_convert(this->m_resampleContext,
                            this->m_oData,
                            oNSamples,
                            (const uint8_t **) this->m_iData,
                            this->m_iNSamples);

    // convert to destination format
    if (oNSamples < 0)
        return;

    int oBufferSize = av_samples_get_buffer_size(&oLineSize,
                                                 this->m_oNChannels,
                                                 oNSamples,
                                                 this->m_oSampleFormat,
                                                 1);

    this->m_oFrame = QByteArray((const char *) this->m_oData[0], oBufferSize);

    QString format = this->m_formatToFF.key(this->m_oSampleFormat);

    char layout[256];

    av_get_channel_layout_string(layout,
                                 sizeof(layout),
                                 this->m_oNChannels,
                                 this->m_oChannelLayout);

    QString caps = QString("audio/x-raw,"
                           "format=%1,"
                           "channels=%2,"
                           "rate=%3,"
                           "layout=%4,"
                           "samples=%5").arg(format)
                                        .arg(this->m_oNChannels)
                                        .arg(this->m_oSampleRate)
                                        .arg(layout)
                                        .arg(oNSamples);

    QbPacket oPacket(caps,
                     this->m_oFrame.constData(),
                     this->m_oFrame.size());

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
