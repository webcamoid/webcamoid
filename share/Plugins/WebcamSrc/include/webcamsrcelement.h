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

#ifndef WEBCAMSRCELEMENT_H
#define WEBCAMSRCELEMENT_H

#include <QtGui>
#include <gst/gst.h>

#include "qbelement.h"

class WebcamSrcElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(IoMethod)

    Q_PROPERTY(QString device READ device
                              WRITE setDevice
                              RESET resetDevice)

    Q_PROPERTY(QSize size READ size
                          WRITE setSize
                          RESET resetSize)

    public:
        enum IoMethod
        {
            IoMethodRead,
            IoMethodMmap,
            IoMethodUserPtr
        };

        explicit WebcamSrcElement();
        ~WebcamSrcElement();

        Q_INVOKABLE QString device();
        Q_INVOKABLE QSize size();

        Q_INVOKABLE ElementState state();
        Q_INVOKABLE QList<QbElement *> srcs();
        Q_INVOKABLE QList<QbElement *> sinks();

    private:
        struct buffer
        {
            unsigned char *start;
            size_t length;
        };

        QString m_device;
        QSize m_size;

        QMutex m_mutex;
        int m_callBack;
        GstElement *m_pipeline;
        QImage m_oFrame;
        QFile m_camera;
        IoMethod m_ioMethod;
        QList<struct buffer *> m_buffers;
        bool m_forceFormat;

        static void newBuffer(GstElement *appsink, gpointer self);
        int xioctl(int fd, int request, void *arg);
        bool openDevice();
        bool initDevice();
        bool initRead(unsigned int bufferSize);
        bool initMmap();
        bool initUserPtr(unsigned int bufferSize);
        bool startCapturing();
        bool mainLoop();
        bool readFrame();
        void processImage(const void *image, int size);
        void stopCapturing();
        void uninitDevice();
        void closeDevice();

    public slots:
        void setDevice(QString device);
        void setSize(QSize size);
        void resetDevice();
        void resetSize();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);
        void setSrcs(QList<QbElement *> srcs);
        void setSinks(QList<QbElement *> sinks);
        void resetState();
        void resetSrcs();
        void resetSinks();
};

#endif // WEBCAMSRCELEMENT_H
