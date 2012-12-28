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

#include "recordbinelement.h"

RecordBinElement::RecordBinElement(): Element()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;

    this->resetFileName();
    this->resetVideoEncoder();
    this->resetAudioEncoder();
    this->resetMuxer();
    this->resetRecordAudio();
    this->resetFrameSize();
    this->resetState();
}

RecordBinElement::~RecordBinElement()
{
    if (this->m_pipeline)
    {
        this->setState(Null);

        GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");
        g_signal_handler_disconnect(appsrc, this->m_callBack);
        gst_object_unref(GST_OBJECT(appsrc));

        gst_object_unref(GST_OBJECT(this->m_pipeline));
    }
}

QString RecordBinElement::fileName()
{
    return this->m_fileName;
}

QString RecordBinElement::videoEncoder()
{
    return this->m_videoEncoder;
}

QString RecordBinElement::audioEncoder()
{
    return this->m_audioEncoder;
}

QString RecordBinElement::muxer()
{
    return this->m_muxer;
}

bool RecordBinElement::recordAudio()
{
    return this->m_recordAudio;
}

QSize RecordBinElement::frameSize()
{
    return this->m_frameSize;
}

Element::ElementState RecordBinElement::state()
{
    return this->m_state;
}

void RecordBinElement::needData(GstElement *appsrc, guint size, gpointer self)
{
    Q_UNUSED(size)

    RecordBinElement *element = (RecordBinElement *) self;
    QImage iFrame = element->m_iFrame;

    if (!iFrame.size().isValid())
        return;

    GstBuffer *buffer = gst_buffer_new_and_alloc(iFrame.byteCount());

    memcpy(GST_BUFFER_DATA(buffer),
           iFrame.constBits(),
           iFrame.byteCount());

    GstFlowReturn ret;

    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

    if (ret != GST_FLOW_OK)
        element->setState(Null);
}

void RecordBinElement::setFileName(QString fileName)
{
    this->m_fileName = fileName;
}

void RecordBinElement::setVideoEncoder(QString videoEncoder)
{
    this->m_videoEncoder = videoEncoder;
}

void RecordBinElement::setAudioEncoder(QString audioEncoder)
{
    this->m_audioEncoder = audioEncoder;
}

void RecordBinElement::setMuxer(QString muxer)
{
    this->m_muxer = muxer;
}

void RecordBinElement::setRecordAudio(bool recordAudio)
{
    this->m_recordAudio = recordAudio;
}

void RecordBinElement::setFrameSize(QSize frameSize)
{
    this->m_frameSize = frameSize;
}

void RecordBinElement::resetFileName()
{
    this->setFileName("");
}

void RecordBinElement::resetVideoEncoder()
{
    this->setVideoEncoder("");
}

void RecordBinElement::resetAudioEncoder()
{
    this->setAudioEncoder("");
}

void RecordBinElement::resetMuxer()
{
    this->setMuxer("");
}

void RecordBinElement::resetRecordAudio()
{
    this->setRecordAudio(false);
}

void RecordBinElement::resetFrameSize()
{
    this->setFrameSize(QSize());
}

void RecordBinElement::iStream(const void *data, int datalen, QString dataType)
{
    Q_UNUSED(datalen)

    if (dataType != "QImage" || this->m_state != Playing || !this->m_pipeline)
        return;

    this->m_mutex.lock();

    QImage iFrame = *((const QImage *) data);

    if (iFrame.size() == this->m_frameSize)
        this->m_iFrame = iFrame;
    else
    {
        this->m_iFrame = QImage(this->m_frameSize, QImage::Format_RGB888);
        this->m_iFrame.fill(QColor(0, 0, 0));

        QImage scaledFrame(iFrame.scaled(this->m_frameSize, Qt::KeepAspectRatio));
        QPoint point((this->m_frameSize.width() - scaledFrame.width()) >> 1,
                     (this->m_frameSize.height() - scaledFrame.height()) >> 1);

        QPainter painter;

        painter.begin(&this->m_iFrame);
        painter.drawImage(point, scaledFrame);
        painter.end();
    }

    this->m_mutex.unlock();
}

void RecordBinElement::setState(ElementState state)
{
    switch (state)
    {
        case Null:
            if (this->m_pipeline)
            {
                gst_element_set_state(this->m_pipeline, GST_STATE_NULL);

                GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline),
                                                         "input");

                g_signal_handler_disconnect(appsrc, this->m_callBack);
                gst_object_unref(GST_OBJECT(appsrc));

                gst_object_unref(GST_OBJECT(this->m_pipeline));
                this->m_pipeline = NULL;
            }
        break;

        case Ready:
            if (this->m_pipeline)
                gst_element_set_state(this->m_pipeline, GST_STATE_READY);
            else
                state = Null;
        break;

        case Paused:
            if (this->m_pipeline)
                gst_element_set_state(this->m_pipeline, GST_STATE_PAUSED);
            else
                state = Null;
        break;

        case Playing:
        {
            if (this->m_pipeline)
                this->setState(Null);

            QString pipeline = QString("appsrc name=input "
                                       "caps=video/x-raw,format=RGB,width=%1,heigth=%2,framerate=0/1 "
                                       "stream-type=stream "
                                       "is-live=true "
                                       "do-timestamp=true "
                                       "min-latency=0 "
                                       "format=time ! ").arg(this->m_frameSize.width())
                                                        .arg(this->m_frameSize.height());

            pipeline += QString("ffmpegcolorspace ! "
                                "%1 ! queue ! muxer. ").arg(this->m_videoEncoder);

            if (this->m_recordAudio)
                pipeline += QString("autoaudiosrc ! queue ! audioconvert ! "
                                    "queue ! %1 ! queue ! muxer. ").arg(this->m_audioEncoder);

            pipeline += QString("%1 name=muxer ! filesink location=\"%2\"").arg(this->m_muxer)
                                                                           .arg(this->m_fileName);

            GError *error = NULL;

            this->m_pipeline = gst_parse_bin_from_description(pipeline.toUtf8().constData(),
                                                              FALSE,
                                                              &error);

            if (this->m_pipeline && !error)
            {
                GstElement *appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");

                this->m_callBack = g_signal_connect(appsrc,
                                                    "need-data",
                                                    G_CALLBACK(RecordBinElement::needData),
                                                    this);

                gst_object_unref(GST_OBJECT(appsrc));
            }
            else
                this->m_pipeline = NULL;

            gst_element_set_state(this->m_pipeline, GST_STATE_PLAYING);
        }
        break;

        default:
        break;
    }

    this->m_state = state;
}

void RecordBinElement::resetState()
{
    this->setState(Null);
}
