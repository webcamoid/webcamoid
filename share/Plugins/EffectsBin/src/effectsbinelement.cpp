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

#include <gst/app/gstappsink.h>
#include <unistd.h>

#include "effectsbinelement.h"

EffectsBinElement::EffectsBinElement(): QbElement()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_curFrameSize = QSize(640, 480);
    this->m_readFrames = true;

    this->resetEffects();
    this->resetState();
}

EffectsBinElement::~EffectsBinElement()
{
    if (this->m_pipeline)
    {
        this->setState(ElementStateNull);

        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
        gst_object_unref(GST_OBJECT(this->m_appsrc));

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack["new-sample"]);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

QStringList EffectsBinElement::effects()
{
    return this->m_effects;
}

QbElement::ElementState EffectsBinElement::state()
{
    return this->m_state;
}

QList<QbElement *> EffectsBinElement::srcs()
{
    return this->m_srcs;
}

QList<QbElement *> EffectsBinElement::sinks()
{
    return this->m_sinks;
}

void EffectsBinElement::needData(GstElement *appsrc, guint size, gpointer self)
{
    Q_UNUSED(appsrc)
    Q_UNUSED(size)

    EffectsBinElement *element = (EffectsBinElement *) self;

    element->m_readFrames = true;
}

void EffectsBinElement::enoughData(GstElement *appsrc, gpointer self)
{
    Q_UNUSED(appsrc)

    EffectsBinElement *element = (EffectsBinElement *) self;

    element->m_readFrames = false;
}

void EffectsBinElement::newBuffer(GstElement *appsink, gpointer self)
{
    EffectsBinElement *element = (EffectsBinElement *) self;

    element->m_mutex.lock();

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstMapInfo mapInfo;

    if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ))
    {
        element->m_oFrame = QImage::fromData((const uchar *) mapInfo.data, mapInfo.size);
        gst_buffer_unmap(buffer, &mapInfo);
    }

    gst_buffer_unref(buffer);
    gst_sample_unref(sample);

    QbPacket packet(QString("video/x-raw,format=RGB,width=%1,height=%2").arg(element->m_oFrame.width())
                                                                        .arg(element->m_oFrame.height()),
                    element->m_oFrame.constBits(),
                    element->m_oFrame.byteCount());

    emit element->oStream(packet);

    element->m_mutex.unlock();
}

void EffectsBinElement::setEffects(QStringList effects)
{
    this->m_effects = effects;

    if (this->m_pipeline)
    {
        this->setState(ElementStateNull);

        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
        gst_object_unref(GST_OBJECT(this->m_appsrc));

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack["new-sample"]);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
        this->m_pipeline = NULL;
    }

    QString pipeline = QString("appsrc name=input "
                               "stream-type=stream "
                               "is-live=true "
                               "do-timestamp=true "
                               "min-latency=0 "
                               "format=time ! "
                               "videoparse name=parser "
                               "format=rgb "
                               "width=%1 "
                               "height=%2 ! ").arg(this->m_curFrameSize.width())
                                              .arg(this->m_curFrameSize.height());

    if (!effects.isEmpty())
        pipeline += "videoconvert ! " + effects.join(" ! videoconvert ! ") + " ! ";

    pipeline += "videoconvert ! "
                "avenc_bmp ! "
                "appsink name=output "
                "emit-signals=true "
                "max_buffers=1 "
                "drop=true";

    GError *error = NULL;

    this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                        FALSE,
                                                        &error);

    if (error)
        qDebug() << error->message;

    if (this->m_pipeline && !error)
    {
        this->m_appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");

        this->m_callBack["need-data"] = g_signal_connect(this->m_appsrc,
                                                         "need-data",
                                                         G_CALLBACK(EffectsBinElement::needData),
                                                         this);

        this->m_callBack["enough-data"] = g_signal_connect(this->m_appsrc,
                                                           "enough-data",
                                                           G_CALLBACK(EffectsBinElement::enoughData),
                                                           this);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");

        this->m_callBack["new-sample"] = g_signal_connect(appsink,
                                                          "new-sample",
                                                          G_CALLBACK(EffectsBinElement::newBuffer),
                                                          this);

        gst_object_unref(GST_OBJECT(appsink));
    }
    else
    {
        this->m_pipeline = NULL;
        this->m_effects.clear();
    }
}

void EffectsBinElement::resetEffects()
{
    this->setEffects(QStringList());
}

void EffectsBinElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        packet.caps().property("format") != "RGB" ||
        this->m_state != ElementStatePlaying)
        return;

    if (this->m_pipeline)
    {
        if (!this->m_readFrames)
            return;

        QImage iFrame((const uchar *) packet.data(),
                      packet.caps().property("width").toInt(),
                      packet.caps().property("height").toInt(),
                      QImage::Format_RGB888);

        if (!iFrame.size().isValid())
            return;

        if (iFrame.size() != this->m_curFrameSize)
        {
            this->setState(ElementStateNull);

            GstElement *parser = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "parser");

            g_object_set(GST_OBJECT(parser),
                         "width", iFrame.size().width(),
                         "height", iFrame.size().height(),
                         NULL);

            gst_object_unref(GST_OBJECT(parser));

            this->setState(ElementStatePlaying);

            this->m_curFrameSize = iFrame.size();
        }

        GstBuffer *buffer = gst_buffer_new_and_alloc(iFrame.byteCount());
        GstMapInfo mapInfo;

        if (gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE))
        {
            memcpy(mapInfo.data,
                   iFrame.constBits(),
                   iFrame.byteCount());

            gst_buffer_unmap(buffer, &mapInfo);
        }

        GstFlowReturn ret;

        g_signal_emit_by_name(this->m_appsrc, "push-buffer", buffer, &ret);

        if (ret != GST_FLOW_OK)
            this->setState(ElementStateNull);
    }
    else
        emit this->oStream(packet);
}

void EffectsBinElement::setState(ElementState state)
{
    if (!this->m_pipeline)
    {
        this->m_state = ElementStateNull;

        return;
    }

    switch (state)
    {
        case ElementStateNull:
            gst_element_set_state(this->m_pipeline, GST_STATE_NULL);
        break;

        case ElementStateReady:
            gst_element_set_state(this->m_pipeline, GST_STATE_READY);
        break;

        case ElementStatePaused:
            gst_element_set_state(this->m_pipeline, GST_STATE_PAUSED);
        break;

        case ElementStatePlaying:
            gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
        break;

        default:
        break;
    }

    this->m_state = state;
}

void EffectsBinElement::setSrcs(QList<QbElement *> srcs)
{
    this->m_srcs = srcs;
}

void EffectsBinElement::setSinks(QList<QbElement *> sinks)
{
    this->m_sinks = sinks;
}

void EffectsBinElement::resetState()
{
    this->setState(ElementStateNull);
}

void EffectsBinElement::resetSrcs()
{
    this->setSrcs(QList<QbElement *>());
}

void EffectsBinElement::resetSinks()
{
    this->setSinks(QList<QbElement *>());
}
