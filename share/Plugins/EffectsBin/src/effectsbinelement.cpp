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

#include "effectsbinelement.h"

EffectsBinElement::EffectsBinElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_curFrameSize = QSize(640, 480);

    this->resetEffects();
    this->resetState();
}

EffectsBinElement::~EffectsBinElement()
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
        g_signal_handler_disconnect(appsrc, this->m_callBack["need-data"]);
        gst_object_unref(GST_OBJECT(appsrc));

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack["new-buffer"]);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

QStringList EffectsBinElement::effects()
{
    return this->m_effects;
}

Element::ElementState EffectsBinElement::state()
{
    return this->m_state;
}

void EffectsBinElement::needData(GstElement *appsrc, guint size, gpointer self)
{
    Q_UNUSED(size)

    EffectsBinElement *element = (EffectsBinElement *) self;
    QImage iFrame = element->m_iFrame;

    if (!iFrame.size().isValid())
        return;

    if (iFrame.size() != element->m_curFrameSize)
    {
        element->setState(Null);

        g_object_set(GST_OBJECT(appsrc),
                     "caps",
                     gst_caps_new_simple("video/x-raw",
                                         "format", G_TYPE_STRING, "RGB",
                                         "width", G_TYPE_INT, iFrame.size().width(),
                                         "height", G_TYPE_INT, iFrame.size().height(),
                                         "framerate", GST_TYPE_FRACTION, 0, 1,
                                         NULL),
                     NULL);

        element->setState(Playing);

        element->m_curFrameSize = iFrame.size();
    }

    GstBuffer *buffer = gst_buffer_new_and_alloc(iFrame.byteCount());

    memcpy(GST_BUFFER_DATA(buffer),
           iFrame.constBits(),
           iFrame.byteCount());

    GstFlowReturn ret;

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

    if (ret != GST_FLOW_OK)
        element->setState(Null);
}

void EffectsBinElement::newBuffer(GstElement *appsink, gpointer self)
{
    EffectsBinElement *element = (EffectsBinElement *) self;

    element->m_mutex.lock();

    GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(appsink));
    element->m_oFrame = QImage::fromData((const uchar *) GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
    gst_buffer_unref(buffer);

    emit element->oStream((const void *) &element->m_oFrame, 0, "QImage");

    element->m_mutex.unlock();
}

void EffectsBinElement::setEffects(QStringList effects)
{
    this->m_effects = effects;

    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
        g_signal_handler_disconnect(appsrc, this->m_callBack["need-data"]);
        gst_object_unref(GST_OBJECT(appsrc));

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack["new-buffer"]);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
        this->m_pipeline = NULL;
    }

    if (!effects.isEmpty())
    {
        QString pipeline = QString("appsrc name=input "
                                   "caps=video/x-raw,format=RGB,width=%1,heigth=%2,framerate=0/1 "
                                   "stream-type=stream "
                                   "is-live=true "
                                   "do-timestamp=true "
                                   "min-latency=0 "
                                   "format=time ! ").arg(this->m_curFrameSize.width())
                                                    .arg(this->m_curFrameSize.height());

        if (!effects.isEmpty())
            pipeline += effects.join(" ! ffmpegcolorspace ! ") + " ! ";

        pipeline += "ffmpegcolorspace ! "
                    "ffenc_bmp ! "
                    "appsink name=output "
                    "emit-signals=true "
                    "max_buffers=1 "
                    "drop=true";

        GError *error = NULL;

        this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                            FALSE,
                                                            &error);

        if (this->m_pipeline && !error)
        {
            GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
            this->m_callBack["need-data"] = g_signal_connect(appsrc, "need-data", G_CALLBACK(EffectsBinElement::needData), this);
            gst_object_unref(GST_OBJECT(appsrc));

            GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
            this->m_callBack["new-buffer"] = g_signal_connect(appsink, "new-buffer", G_CALLBACK(EffectsBinElement::newBuffer), this);
            gst_object_unref(GST_OBJECT(appsink));
        }
        else
        {
            this->m_pipeline = NULL;
            this->m_effects.clear();
        }
    }
}

void EffectsBinElement::resetEffects()
{
    this->setEffects(QStringList());
}

void EffectsBinElement::iStream(const void *data, int datalen, QString dataType)
{
    if (dataType != "QImage" || this->m_state != Playing)
        return;

    if (this->m_pipeline)
        this->m_iFrame = *((const QImage *) data);
    else
        emit this->oStream(data, datalen, dataType);
}

void EffectsBinElement::setState(ElementState state)
{
    if (!this->m_pipeline)
    {
        this->m_state = Null;

        return;
    }

    switch (state)
    {
        case Null:
            gst_element_set_state(this->m_pipeline, GST_STATE_NULL);
        break;

        case Ready:
            gst_element_set_state(this->m_pipeline, GST_STATE_READY);
        break;

        case Paused:
            gst_element_set_state(this->m_pipeline, GST_STATE_PAUSED);
        break;

        case Playing:
            gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
        break;

        default:
        break;
    }

    this->m_state = state;
}

void EffectsBinElement::resetState()
{
    this->setState(Null);
}
