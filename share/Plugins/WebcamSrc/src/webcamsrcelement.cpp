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

#include "webcamsrcelement.h"

WebcamSrcElement::WebcamSrcElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;

    this->resetDevice();
    this->resetSize();
    this->resetState();

    QString pipeline = QString("v4l2src name=webcam device=%1 ! "
                               "capsfilter name=webcamcaps "
                               "caps=video/x-raw,format=YUV,width=%2,height=%3 ! "
                               "ffmpegcolorspace ! "
                               "ffenc_bmp ! "
                               "appsink name=output "
                               "emit-signals=true "
                               "max_buffers=1 "
                               "drop=true").arg(this->m_device)
                                           .arg(this->m_size.width())
                                           .arg(this->m_size.height());

    GError *error = NULL;

    this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                      FALSE,
                                                      &error);

    if (this->m_pipeline && !error)
    {
        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        this->m_callBack = g_signal_connect(appsink, "new-buffer", G_CALLBACK(WebcamSrcElement::newBuffer), this);
        gst_object_unref(GST_OBJECT(appsink));
    }
}

WebcamSrcElement::~WebcamSrcElement()
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

QString WebcamSrcElement::device()
{
    return this->m_device;
}

QSize WebcamSrcElement::size()
{
    return this->m_size;
}

Element::ElementState WebcamSrcElement::state()
{
    return this->m_state;
}

void WebcamSrcElement::newBuffer(GstElement *appsink, gpointer self)
{
    WebcamSrcElement *element = (WebcamSrcElement *) self;

    element->m_mutex.lock();

    GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(appsink));
    element->m_oFrame = QImage::fromData((const uchar *) GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
    gst_buffer_unref(buffer);

    emit element->oStream((const void *) &element->m_oFrame, 0, "QImage");

    element->m_mutex.unlock();
}

void WebcamSrcElement::setDevice(QString device)
{
    this->m_device = device;

    if (!this->m_pipeline)
        return;

    ElementState state = this->m_state;

    if (state != Null)
        this->setState(Null);

    GstElement *webcam = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "webcam");
    g_object_set(GST_OBJECT(webcam), "device", device.toUtf8().constData(), NULL);
    gst_object_unref(GST_OBJECT(webcam));

    this->setState(state);
}

void WebcamSrcElement::setSize(QSize size)
{
    this->m_size = size;

    if (!this->m_pipeline)
        return;

    ElementState state = this->m_state;

    if (state != Null)
        this->setState(Null);

    GstElement *webcamcaps = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "webcamcaps");

    g_object_set(GST_OBJECT(webcamcaps),
                 "caps",
                 gst_caps_new_simple("video/x-raw-yuv",
                                     "width", G_TYPE_INT, size.width(),
                                     "height", G_TYPE_INT, size.height(),
                                     NULL),
                 NULL);

    gst_object_unref(GST_OBJECT(webcamcaps));

    this->setState(state);
}

void WebcamSrcElement::resetDevice()
{
    this->setDevice("/dev/video0");
}

void WebcamSrcElement::resetSize()
{
    this->setSize(QSize(640, 480));
}

void WebcamSrcElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(data)
    Q_UNUSED(datalen)
    Q_UNUSED(dataType)
}

void WebcamSrcElement::setState(ElementState state)
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

void WebcamSrcElement::resetState()
{
    this->setState(Null);
}
