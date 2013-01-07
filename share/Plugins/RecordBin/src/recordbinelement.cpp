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

RecordBinElement::RecordBinElement(): QbElement()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;
    this->m_readFrames = true;

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
        this->setState(ElementStateNull);

        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
        g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
        gst_object_unref(GST_OBJECT(this->m_appsrc));

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

QbElement::ElementState RecordBinElement::state()
{
    return this->m_state;
}

QList<QbElement *> RecordBinElement::srcs()
{
    return this->m_srcs;
}

QList<QbElement *> RecordBinElement::sinks()
{
    return this->m_sinks;
}

void RecordBinElement::needData(GstElement *appsrc, guint size, gpointer self)
{
    Q_UNUSED(appsrc)
    Q_UNUSED(size)

    RecordBinElement *element = (RecordBinElement *) self;

    element->m_readFrames = true;
}

void RecordBinElement::enoughData(GstElement *appsrc, gpointer self)
{
    Q_UNUSED(appsrc)

    RecordBinElement *element = (RecordBinElement *) self;

    element->m_readFrames = false;
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

void RecordBinElement::iStream(const QbPacket &packet)
{
    if (!this->m_pipeline ||
        !packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        packet.caps().property("format") != "RGB" ||
        this->m_state != ElementStatePlaying ||
        !this->m_readFrames)
        return;

    this->m_mutex.lock();

    QImage iFrame((const uchar *) packet.data(),
                  packet.caps().property("width").toInt(),
                  packet.caps().property("height").toInt(),
                  QImage::Format_RGB888);

    if (!iFrame.size().isValid())
        return;

    if (iFrame.size() != this->m_frameSize)
    {
        QImage scaledFrame(iFrame.scaled(this->m_frameSize, Qt::KeepAspectRatio));

        QPoint point((this->m_frameSize.width() - scaledFrame.width()) >> 1,
                     (this->m_frameSize.height() - scaledFrame.height()) >> 1);

        iFrame = QImage(this->m_frameSize, QImage::Format_RGB888);
        iFrame.fill(QColor(0, 0, 0));

        QPainter painter;

        painter.begin(&iFrame);
        painter.drawImage(point, scaledFrame);
        painter.end();
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

    this->m_mutex.unlock();
}

void RecordBinElement::setState(ElementState state)
{
    switch (state)
    {
        case ElementStateNull:
            if (this->m_pipeline)
            {
                gst_element_set_state(this->m_pipeline, GST_STATE_NULL);

                g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["need-data"]);
                g_signal_handler_disconnect(this->m_appsrc, this->m_callBack["enough-data"]);
                gst_object_unref(GST_OBJECT(this->m_appsrc));

                gst_object_unref(GST_OBJECT(this->m_pipeline));
                this->m_pipeline = NULL;
            }
        break;

        case ElementStateReady:
            if (this->m_pipeline)
                gst_element_set_state(this->m_pipeline, GST_STATE_READY);
            else
                state = ElementStateNull;
        break;

        case ElementStatePaused:
            if (this->m_pipeline)
                gst_element_set_state(this->m_pipeline, GST_STATE_PAUSED);
            else
                state = ElementStateNull;
        break;

        case ElementStatePlaying:
        {
            if (this->m_pipeline)
                this->setState(ElementStateNull);

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
                                       "videoconvert ! ").arg(this->m_frameSize.width())
                                                         .arg(this->m_frameSize.height());

            pipeline += QString("videoconvert ! "
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

            if (error)
                qDebug() << error->message;

            if (this->m_pipeline && !error)
            {
                this->m_appsrc = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "input");

                this->m_callBack["need-data"] = g_signal_connect(this->m_appsrc,
                                                                 "need-data",
                                                                 G_CALLBACK(RecordBinElement::needData),
                                                                 this);

                this->m_callBack["enough-data"] = g_signal_connect(this->m_appsrc,
                                                                   "enough-data",
                                                                   G_CALLBACK(RecordBinElement::enoughData),
                                                                   this);
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

void RecordBinElement::setSrcs(QList<QbElement *> srcs)
{
    this->m_srcs = srcs;
}

void RecordBinElement::setSinks(QList<QbElement *> sinks)
{
    this->m_sinks = sinks;
}

void RecordBinElement::resetState()
{
    this->setState(ElementStateNull);
}

void RecordBinElement::resetSrcs()
{
    this->setSrcs(QList<QbElement *>());
}

void RecordBinElement::resetSinks()
{
    this->setSinks(QList<QbElement *>());
}
