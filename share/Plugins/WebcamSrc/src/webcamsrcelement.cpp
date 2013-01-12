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
#include <linux/videodev2.h>
#include <stropts.h>
#include <sys/mman.h>

#include "webcamsrcelement.h"

// https://github.com/hatstand/Video-capture.git
// https://www.archlinux.org/packages/extra/x86_64/ffmpeg/files/
// https://www.libav.org/doxygen/master/index.html
// http://ffmpeg.org/trac/ffmpeg/wiki

WebcamSrcElement::WebcamSrcElement(): QbElement()
{
    gst_init(NULL, NULL);

    this->m_pipeline = NULL;

    this->resetDevice();
    this->resetSize();

    QString pipeline = QString("v4l2src name=webcam device=%1 ! "
                               "capsfilter name=webcamcaps "
                               "caps=video/x-raw,width=%2,height=%3 ! "
                               "videoconvert ! "
                               "video/x-raw,format=RGB ! "
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

    if (error)
        qDebug() << error->message;

    if (this->m_pipeline && !error)
    {
        GstElement *appsink = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "output");
        this->m_callBack = g_signal_connect(appsink, "new-sample", G_CALLBACK(WebcamSrcElement::newBuffer), this);
        gst_object_unref(GST_OBJECT(appsink));
    }
}

WebcamSrcElement::~WebcamSrcElement()
{
    if (this->m_pipeline)
    {
        this->setState(ElementStateNull);

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

void WebcamSrcElement::newBuffer(GstElement *appsink, gpointer self)
{
    WebcamSrcElement *element = (WebcamSrcElement *) self;

    element->m_mutex.lock();

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
    GstCaps *caps = gst_sample_get_caps(sample);
    const GstStructure *capsStructure = gst_caps_get_structure(caps, 0);
    int width;
    int height;

    if (gst_structure_get_int(capsStructure, "width", &width) &&
        gst_structure_get_int(capsStructure, "height", &height))
    {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstMapInfo mapInfo;

        if (gst_buffer_map(buffer, &mapInfo, GST_MAP_READ))
        {
            element->m_oFrame = QImage((const uchar *) mapInfo.data,
                                       width,
                                       height,
                                       QImage::Format_RGB888);

            gst_buffer_unmap(buffer, &mapInfo);
        }

        gst_buffer_unref(buffer);
    }

    gst_caps_unref(caps);
    gst_sample_unref(sample);

    QbPacket packet(QString("video/x-raw,format=RGB,width=%1,height=%2").arg(element->m_oFrame.width())
                                                                        .arg(element->m_oFrame.height()),
                    element->m_oFrame.constBits(),
                    element->m_oFrame.byteCount());

    emit element->oStream(packet);

    element->m_mutex.unlock();
}

int WebcamSrcElement::xioctl(int fd, int request, void *arg)
{
    while (true)
    {
        int r = ioctl(fd, request, arg);

        if (r != -1 || errno != EINTR)
            return r;
    }
}

bool WebcamSrcElement::openDevice()
{
    this->m_camera.setFileName(this->m_device);

    return this->m_camera.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

bool WebcamSrcElement::initDevice()
{
    struct v4l2_capability cap;

    if (this->xioctl(this->m_camera.handle(), VIDIOC_QUERYCAP, &cap) == -1)
        return false;

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        return false;

    switch (this->m_ioMethod)
    {
        case IoMethodRead:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
                return false;
        break;

        case IoMethodMmap:
        case IoMethodUserPtr:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
                return false;
        break;
    }

    struct v4l2_cropcap cropcap;
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->xioctl(this->m_camera.handle(), VIDIOC_CROPCAP, &cropcap) == 0)
    {
        struct v4l2_crop crop;
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        this->xioctl(this->m_camera.handle(), VIDIOC_S_CROP, &crop);
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (this->m_forceFormat)
    {
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (this->xioctl(this->m_camera.handle(), VIDIOC_S_FMT, &fmt) == -1)
            return false;
    }
    else if (this->xioctl(this->m_camera.handle(), VIDIOC_G_FMT, &fmt) == -1)
        return false;

    unsigned int min = 2 * fmt.fmt.pix.width;

    if (fmt.fmt.pix.bytesperline < min)
            fmt.fmt.pix.bytesperline = min;

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;

    if (fmt.fmt.pix.sizeimage < min)
            fmt.fmt.pix.sizeimage = min;

    switch (this->m_ioMethod)
    {
        case IoMethodRead:
            return this->initRead(fmt.fmt.pix.sizeimage);
        break;

        case IoMethodMmap:
            return this->initMmap();
        break;

        case IoMethodUserPtr:
            return this->initUserPtr(fmt.fmt.pix.sizeimage);
        break;

        default:
        break;
    }

    return false;
}

bool WebcamSrcElement::initRead(unsigned int bufferSize)
{
    this->m_buffers.reserve(1);

    if (this->m_buffers.isEmpty())
        return false;

    this->m_buffers[0]->length = bufferSize;
    this->m_buffers[0]->start = new unsigned char[bufferSize];

    if (!this->m_buffers[0]->start)
        return false;

    return true;
}

bool WebcamSrcElement::initMmap()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (this->xioctl(this->m_camera.handle(), VIDIOC_REQBUFS, &req) == -1)
        return false;

    if (req.count < 2)
        return false;

    this->m_buffers.reserve(req.count);

    if (this->m_buffers.isEmpty())
        return false;

    for (uint nBuffer = 0; nBuffer < req.count; nBuffer++)
    {
        struct v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));

        buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = nBuffer;

        if (xioctl(this->m_camera.handle(), VIDIOC_QUERYBUF, &buffer) == -1)
            return false;

        this->m_buffers[nBuffer]->length = buffer.length;
        this->m_buffers[nBuffer]->start = (uchar *) mmap(NULL,
                                                         buffer.length,
                                                         PROT_READ | PROT_WRITE,
                                                         MAP_SHARED,
                                                         this->m_camera.handle(),
                                                         buffer.m.offset);

        if (this->m_buffers[nBuffer]->start == MAP_FAILED)
            return false;
    }

    return true;
}

bool WebcamSrcElement::initUserPtr(unsigned int bufferSize)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (this->xioctl(this->m_camera.handle(), VIDIOC_REQBUFS, &req) == -1)
        return false;

    this->m_buffers.reserve(4);

    if (this->m_buffers.isEmpty())
        return false;

    for (int nBuffer = 0; nBuffer < 4; nBuffer++)
    {
        this->m_buffers[nBuffer]->length = bufferSize;
        this->m_buffers[nBuffer]->start = new unsigned char[bufferSize];

        if (!this->m_buffers[nBuffer]->start)
            return false;
    }

    return true;
}

bool WebcamSrcElement::startCapturing()
{/*
    enum v4l2_buf_type type;

    switch (this->m_ioMethod)
    {
        case IoMethodRead:
        break;

        case IoMethodMmap:
            for (uint i = 0; i < this->m_buffers.length(); i++)
            {
                    struct v4l2_buffer buffer;
                    memset(&buffer, 0, sizeof(buffer));

                    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buffer.memory = V4L2_MEMORY_MMAP;
                    buffer.index = i;

                    if (this->xioctl(this->m_camera.handle(), VIDIOC_QBUF, &buffer) == -1)
                        return false;
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (this->xioctl(this->m_camera.handle(), VIDIOC_STREAMON, &type) == -1)
                return false;
        break;

        case IoMethodUserPtr:
            for (uint i = 0; i < this->m_buffers.length(); i++)
            {
                    struct v4l2_buffer buffer;
                    memset(&buffer, 0, sizeof(buffer));

                    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                    buffer.memory = V4L2_MEMORY_USERPTR;
                    buffer.index = i;
                    buffer.m.userptr = (unsigned long)buffers[i].start;
                    buffer.length = this->m_buffers[i]->length;

                    if (this->xioctl(this->m_camera.handle(), VIDIOC_QBUF, &buffer) == -1)
                        return false;
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (this->xioctl(this->m_camera.handle(), VIDIOC_STREAMON, &type) == -1)
                return false;
        break;
    }

    return true;*/
}

bool WebcamSrcElement::mainLoop()
{
    while (true)
    {
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(this->m_camera.handle(), &fds);

        struct timeval tv;

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int r = select(this->m_camera.handle() + 1, &fds, NULL, NULL, &tv);

        if (r == -1)
        {
            if (errno == EINTR)
                continue;

            return true;
        }

        if (r == 0)
            return false;

        if (this->readFrame())
            break;
    }

    return true;
}

bool WebcamSrcElement::readFrame()
{/*
    struct v4l2_buffer buffer;
    uint i;

    switch (this->m_ioMethod)
    {
        case IoMethodRead:
            if (this->m_camera.read(this->m_buffers[0]->start, this->m_buffers[0]->length) == -1)
                return false;

            this->processImage(this->m_buffers[0]->start, this->m_buffers[0]->length);
        break;

        case IoMethodMmap:
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_MMAP;

            if (this->xioctl(this->m_camera.handle(), VIDIOC_DQBUF, &buffer) == -1)
                return false;

            if (buffer.index >= this->m_buffers.length())
                return false;

            this->processImage(this->m_buffers[buffer.index]->start, buffer.bytesused);

            if (this->xioctl(this->m_camera.handle(), VIDIOC_QBUF, &buffer) == -1)
                return false;
        break;

        case IoMethodUserPtr:
            memset(&buffer, 0, sizeof(buffer));

            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buffer.memory = V4L2_MEMORY_USERPTR;

            if (this->xioctl(this->m_camera.handle(), VIDIOC_DQBUF, &buffer) == -1)
                return false;

            for (i = 0; i < this->m_buffers.length(); i++)
                if (buffer.m.userptr == (ulong) this->m_buffers[i]->start &&
                    buffer.length == this->m_buffers[i]->length)
                        break;

            if (i >= this->m_buffers.length())
                return false;

            this->processImage((void *) buffer.m.userptr, buffer.bytesused);

            if (this->xioctl(this->m_camera.handle(), VIDIOC_QBUF, &buffer) == -1)
                return false;
        break;
    }

    return true;*/
}

void WebcamSrcElement::processImage(const void *image, int size)
{
}

void WebcamSrcElement::stopCapturing()
{/*
    enum v4l2_buf_type type;

    switch (this->m_ioMethod)
    {
        case IoMethodRead:
        break;

        case IoMethodMmap:
        case IoMethodUserPtr:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (xioctl(this->m_camera.handle(), VIDIOC_STREAMOFF, &type) == -1)
                return false;
        break;
    }

    return true;*/
}

void WebcamSrcElement::uninitDevice()
{
    switch (this->m_ioMethod)
    {
        case IoMethodRead:
            delete this->m_buffers[0]->start;
        break;

        case IoMethodMmap:
            foreach (struct buffer *buffer, this->m_buffers)
                if (munmap(buffer->start, buffer->length) == -1)
                    break;
        break;

        case IoMethodUserPtr:
            foreach (struct buffer *buffer, this->m_buffers)
                delete buffer->start;
        break;
    }

    this->m_buffers.clear();
}

void WebcamSrcElement::closeDevice()
{
    this->m_camera.close();
}

void WebcamSrcElement::setDevice(QString device)
{
    this->m_device = device;

    if (!this->m_pipeline)
        return;

    ElementState state = this->m_state;

    if (state != ElementStateNull)
        this->setState(ElementStateNull);

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

    if (state != ElementStateNull)
        this->setState(ElementStateNull);

    GstElement *webcamcaps = gst_bin_get_by_name(GST_BIN(this->m_pipeline), "webcamcaps");

    g_object_set(GST_OBJECT(webcamcaps),
                 "caps",
                 gst_caps_new_simple("video/x-raw",
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

void WebcamSrcElement::setState(ElementState state)
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
