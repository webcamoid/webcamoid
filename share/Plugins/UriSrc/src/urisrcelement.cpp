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

#include "urisrcelement.h"

UriSrcElement::UriSrcElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;

    this->resetUri();
    this->resetHasAudio();
    this->resetPlayAudio();
    this->resetState();
}

UriSrcElement::~UriSrcElement()
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

QString UriSrcElement::uri()
{
    return this->m_uri;
}

bool UriSrcElement::hasAudio()
{
    return this->m_hasAudio;
}

bool UriSrcElement::playAudio()
{
    return this->m_playAudio;
}

Element::ElementState UriSrcElement::state()
{
    return this->m_state;
}

void UriSrcElement::newBuffer(GstElement *appsink, gpointer self)
{
    UriSrcElement *element = (UriSrcElement *) self;

    element->m_mutex.lock();

    GstBuffer *buffer = gst_app_sink_pull_buffer(GST_APP_SINK(appsink));
    element->m_oFrame = QImage::fromData((const uchar *) GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
    gst_buffer_unref(buffer);

    emit element->oStream((const void *) &element->m_oFrame, 0, "QImage");

    element->m_mutex.unlock();
}

void UriSrcElement::setUri(QString uri)
{
    this->m_uri = uri;

    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        g_signal_handler_disconnect(appsink, this->m_callBack);
        gst_object_unref(GST_OBJECT(appsink));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
        this->m_pipeline = NULL;
    }

    if (!uri.isEmpty())
    {
        QString scheme = QUrl(uri).scheme();
        QString pipeline;

        if (scheme.isEmpty() || scheme == "file")
        {
            uri.replace(QRegExp("^file://"), "");

            if (!QFile::exists(uri))
            {
                this->m_uri = "";

                return;
            }

            pipeline = "filesrc";
        }
        else if (scheme == "mms" || scheme == "mmsu" || scheme == "mmst")
            pipeline = "mmssrc";
        else if (scheme == "rtsp")
            pipeline = "rtspsrc";
        else
            pipeline = "souphttpsrc";

        pipeline += QString(" location=%1 ! "
                            "decodebin name=dec ! "
                            "ffmpegcolorspace ! "
                            "ffenc_bmp ! "
                            "appsink name=output "
                            "emit-signals=true "
                            "max_buffers=1 "
                            "drop=true").arg(uri);

        if (this->m_hasAudio)
        {
            pipeline += " dec. ! ";

            if (this->m_playAudio)
                pipeline += "autoaudiosink";
            else
                pipeline += "fakesink";
        }

        GError *error = NULL;

        this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                            FALSE,
                                                            &error);

        if (this->m_pipeline && !error)
        {
            GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");

            this->m_callBack = g_signal_connect(appsink,
                                                "new-buffer",
                                                G_CALLBACK(UriSrcElement::newBuffer),
                                                this);

            gst_object_unref(GST_OBJECT(appsink));
        }
        else
        {
            this->m_pipeline = NULL;
            this->m_uri = "";
        }
    }
}

void UriSrcElement::setHasAudio(bool hasAudio)
{
    QString uri = this->m_uri;

    this->resetUri();
    this->m_hasAudio = hasAudio;
    this->setUri(uri);
}

void UriSrcElement::setPlayAudio(bool playAudio)
{
    QString uri = this->m_uri;

    this->resetUri();
    this->m_playAudio = playAudio;
    this->setUri(uri);
}

void UriSrcElement::resetUri()
{
    this->setUri("");
}

void UriSrcElement::resetHasAudio()
{
    this->setHasAudio(false);
}

void UriSrcElement::resetPlayAudio()
{
    this->setPlayAudio(false);
}

void UriSrcElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(data)
    Q_UNUSED(datalen)
    Q_UNUSED(dataType)
}

void UriSrcElement::setState(ElementState state)
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

void UriSrcElement::resetState()
{
    this->setState(Null);
}
