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

#include "desktopsrcelement.h"

DesktopSrcElement::DesktopSrcElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;

    this->resetShowPointer();
    this->resetState();

    QString pipeline = "ximagesrc show-pointer=false ! "
                       "ffmpegcolorspace ! "
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
        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                  "output");

        this->m_callBack = g_signal_connect(appsink,
                                            "new-buffer",
                                            G_CALLBACK(DesktopSrcElement::newBuffer),
                                            this);

        gst_object_unref(GST_OBJECT(appsink));
    }
}

DesktopSrcElement::~DesktopSrcElement()
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                  "output");

        g_signal_handler_disconnect(appsink, this->m_callBack);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

bool DesktopSrcElement::showPointer()
{
    return this->m_showPointer;
}

Element::ElementState DesktopSrcElement::state()
{
    return this->m_state;
}

void DesktopSrcElement::newBuffer(GstElement *appsink, gpointer self)
{
    DesktopSrcElement *element = (DesktopSrcElement *) self;

    element->m_mutex.lock();

    GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(appsink));

    element->m_oFrame = QImage::fromData((const uchar *) GST_BUFFER_DATA(buffer),
                                         GST_BUFFER_SIZE(buffer));

    gst_buffer_unref(buffer);

    emit element->oStream((const void *) &element->m_oFrame, 0, "QImage");

    element->m_mutex.unlock();
}

void DesktopSrcElement::setShowPointer(bool showPointer)
{
    this->m_showPointer = showPointer;

    if (this->m_pipeline)
        g_object_set(GST_OBJECT(this->m_pipeline),
                     "show-pointer",
                     showPointer,
                     NULL);
}

void DesktopSrcElement::resetShowPointer()
{
    this->setShowPointer(false);
}

void DesktopSrcElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(data)
    Q_UNUSED(datalen)
    Q_UNUSED(dataType)
}

void DesktopSrcElement::setState(ElementState state)
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

void DesktopSrcElement::resetState()
{
    this->setState(Null);
}
