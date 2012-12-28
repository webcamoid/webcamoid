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

#include "effectspreviewbinelement.h"

EffectsPreviewBinElement::EffectsPreviewBinElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_curFrameSize = QSize(640, 480);

    this->resetEffects();
    this->resetFrameSize();
    this->resetState();
}

EffectsPreviewBinElement::~EffectsPreviewBinElement()
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
        g_signal_handler_disconnect(appsrc, this->m_callBack["need-data"]);
        gst_object_unref(GST_OBJECT(appsrc));

        foreach (QString effect, this->m_effects)
        {
            QString previewHash = this->hashFromName(effect);

            GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                      previewHash.toUtf8().constData());

            g_signal_handler_disconnect(appsink, this->m_callBack[previewHash]);
            gst_object_unref(GST_OBJECT(appsink));
        }

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

QStringList EffectsPreviewBinElement::effects()
{
    return this->m_effects;
}

QSize EffectsPreviewBinElement::frameSize()
{
    return this->m_frameSize;
}

Element::ElementState EffectsPreviewBinElement::state()
{
    return this->m_state;
}

QString EffectsPreviewBinElement::hashFromName(QString name)
{
    return QString("x") + name.toUtf8().toHex();
}

QString EffectsPreviewBinElement::nameFromHash(QString hash)
{
    return QByteArray::fromHex(hash.mid(1).toUtf8());
}

void EffectsPreviewBinElement::needData(GstElement *appsrc, guint size, gpointer self)
{
    Q_UNUSED(size)

    EffectsPreviewBinElement *element = (EffectsPreviewBinElement *) self;
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

void EffectsPreviewBinElement::newBuffer(GstElement *appsink, gpointer self)
{
    EffectsPreviewBinElement *element = (EffectsPreviewBinElement *) self;

    element->m_mutex.lock();

    GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(appsink));
    element->m_oFrame = QImage::fromData((const uchar *) GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
    gst_buffer_unref(buffer);

    gchar *name = gst_element_get_name(appsink);
    element->m_oFrame.setText("output", element->nameFromHash(name));
    g_free(name);

    emit element->oStream((const void *) &element->m_oFrame, 0, "QImage");

    element->m_mutex.unlock();
}

void EffectsPreviewBinElement::setEffects(QStringList effects)
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
        g_signal_handler_disconnect(appsrc, this->m_callBack["need-data"]);
        gst_object_unref(GST_OBJECT(appsrc));

        foreach (QString effect, this->m_effects)
        {
            QString previewHash = this->hashFromName(effect);

            GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                      previewHash.toUtf8().constData());

            g_signal_handler_disconnect(appsink, this->m_callBack[previewHash]);
            gst_object_unref(GST_OBJECT(appsink));
        }

        gst_object_unref(GST_OBJECT(this->m_pipeline));
        this->m_pipeline = NULL;
    }

    this->m_effects = effects;

    if (!effects.isEmpty())
    {
        QString pipeline = QString("appsrc name=input "
                                   "caps=video/x-raw,format=RGB,width=%1,heigth=%2,framerate=0/1 "
                                   "stream-type=stream "
                                   "is-live=true "
                                   "do-timestamp=true "
                                   "min-latency=0 "
                                   "format=time ! "
                                   "ffmpegcolorspace ! "
                                   "videoscale ! "
                                   "capsfilter name=scalecaps "
                                   "caps=video/x-raw,format=RGB,width=%3,height=%4 ! "
                                   "tee name=preview").arg(this->m_curFrameSize.width())
                                                      .arg(this->m_curFrameSize.height())
                                                      .arg(this->m_frameSize.width())
                                                      .arg(this->m_frameSize.height());

        foreach (QString effect, this->m_effects)
        {
            QString previewHash = this->hashFromName(effect);

            pipeline += QString(" preview. ! "
                                "queue ! "
                                "ffmpegcolorspace ! "
                                "%1 ! "
                                "ffmpegcolorspace ! "
                                "ffenc_bmp ! "
                                "appsink name=%2 "
                                "emit-signals=true "
                                "max_buffers=1 "
                                "drop=true").arg(effect)
                                            .arg(previewHash);
        }

        GError *error = NULL;

        this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                          FALSE,
                                                          &error);

        if (this->m_pipeline && !error)
        {
            GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");

            this->m_callBack["need-data"] = g_signal_connect(appsrc,
                                                             "need-data",
                                                             G_CALLBACK(EffectsPreviewBinElement::needData),
                                                             this);

            gst_object_unref(GST_OBJECT(appsrc));

            foreach (QString effect, this->m_effects)
            {
                QString previewHash = this->hashFromName(effect);

                GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                          previewHash.toUtf8().constData());

                this->m_callBack[previewHash] = g_signal_connect(appsink,
                                                                 "new-buffer",
                                                                 G_CALLBACK(EffectsPreviewBinElement::newBuffer),
                                                                 this);

                gst_object_unref(GST_OBJECT(appsink));
            }
        }
        else
        {
            this->m_pipeline = NULL;
            this->m_effects.clear();
        }
    }
}

void EffectsPreviewBinElement::setFrameSize(QSize frameSize)
{
    this->m_frameSize = frameSize;

    if (!this->m_pipeline)
        return;

    ElementState state = this->m_state;

    if (this->m_state != Null)
        this->setState(Null);

    GstElement *scalecaps = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "scalecaps");

    g_object_set(GST_OBJECT(scalecaps),
                 "caps",
                 gst_caps_new_simple("video/x-raw",
                                     "format", G_TYPE_STRING, "RGB",
                                     "width", G_TYPE_INT, frameSize.width(),
                                     "height", G_TYPE_INT, frameSize.height(),
                                     NULL),
                 NULL);

    gst_object_unref(GST_OBJECT(scalecaps));
    this->setState(state);
}

void EffectsPreviewBinElement::resetEffects()
{
    this->setEffects(QStringList());
}

void EffectsPreviewBinElement::resetFrameSize()
{
    this->setFrameSize(QSize(128, 96));
}

void EffectsPreviewBinElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(datalen)

    if (!this->m_pipeline || dataType != "QImage" || this->m_state != Playing)
        return;

    this->m_iFrame = *((const QImage *) data);
}

void EffectsPreviewBinElement::setState(ElementState state)
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

void EffectsPreviewBinElement::resetState()
{
    this->setState(Null);
}
