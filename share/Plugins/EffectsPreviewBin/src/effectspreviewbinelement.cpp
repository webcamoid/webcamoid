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

EffectsPreviewBinElement::EffectsPreviewBinElement(): QbElement()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_curFrameSize = QSize(640, 480);
    this->m_readFrames = true;

    this->resetEffects();
    this->resetFrameSize();
}

EffectsPreviewBinElement::~EffectsPreviewBinElement()
{
    if (this->m_pipeline)
    {
        this->setState(ElementStateNull);

        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
        gst_object_unref(GST_OBJECT(this->m_appsrc));

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
    Q_UNUSED(appsrc)
    Q_UNUSED(size)

    EffectsPreviewBinElement *element = (EffectsPreviewBinElement *) self;

    element->m_readFrames = true;
}

void EffectsPreviewBinElement::enoughData(GstElement *appsrc, gpointer self)
{
    Q_UNUSED(appsrc)

    EffectsPreviewBinElement *element = (EffectsPreviewBinElement *) self;

    element->m_readFrames = false;
}

void EffectsPreviewBinElement::newBuffer(GstElement *appsink, gpointer self)
{
    EffectsPreviewBinElement *element = (EffectsPreviewBinElement *) self;

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

    gchar *name = gst_element_get_name(appsink);
    element->m_oFrame.setText("output", element->nameFromHash(name));
    g_free(name);

    QbPacket packet(QString("video/x-raw,format=RGB,width=%1,height=%2").arg(element->m_oFrame.width())
                                                                        .arg(element->m_oFrame.height()),
                    element->m_oFrame.constBits(),
                    element->m_oFrame.byteCount());

    emit element->oStream(packet);

    element->m_mutex.unlock();
}

void EffectsPreviewBinElement::setEffects(QStringList effects)
{
    if (this->m_pipeline)
    {
        this->setState(ElementStateNull);

        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
        gst_object_unref(GST_OBJECT(this->m_appsrc));

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
                                   "stream-type=stream "
                                   "is-live=true "
                                   "do-timestamp=true "
                                   "min-latency=0 "
                                   "format=time ! "
                                   "videoparse name=parser "
                                   "format=rgb "
                                   "width=%1 "
                                   "height=%2 ! "
                                   "videoconvert ! "
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
                                "videoconvert ! "
                                "%1 ! "
                                "videoconvert ! "
                                "avenc_bmp ! "
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

        if (error)
            qDebug() << error->message;

        if (this->m_pipeline && !error)
        {
            this->m_appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");

            this->m_callBack["need-data"] = g_signal_connect(this->m_appsrc,
                                                             "need-data",
                                                             G_CALLBACK(EffectsPreviewBinElement::needData),
                                                             this);

            this->m_callBack["enough-data"] = g_signal_connect(this->m_appsrc,
                                                               "enough-data",
                                                               G_CALLBACK(EffectsPreviewBinElement::enoughData),
                                                               this);

            foreach (QString effect, this->m_effects)
            {
                QString previewHash = this->hashFromName(effect);

                GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                          previewHash.toUtf8().constData());

                this->m_callBack[previewHash] = g_signal_connect(appsink,
                                                                 "new-sample",
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

    if (this->m_state != ElementStateNull)
        this->setState(ElementStateNull);

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

void EffectsPreviewBinElement::iStream(const QbPacket &packet)
{
    if (!this->m_pipeline ||
        !packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        packet.caps().property("format") != "RGB" ||
        this->m_state != ElementStatePlaying ||
        !this->m_readFrames)
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

void EffectsPreviewBinElement::setState(ElementState state)
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
