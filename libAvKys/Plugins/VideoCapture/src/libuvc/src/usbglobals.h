/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef USBGLOBALS_H
#define USBGLOBALS_H

#include <QtConcurrent>
#include <libusb.h>

class UsbGlobals: public QObject
{
    Q_OBJECT

    public:
        explicit UsbGlobals(QObject *parent=NULL);
        ~UsbGlobals();

        libusb_context *context();

    private:
        libusb_context *m_context;
        libusb_hotplug_callback_handle m_hotplugCallbackHnd;
        QThreadPool m_threadPool;
        bool m_processsUsbEventsLoop;
        QFuture<void> m_processsUsbEvents;
        QMutex m_mutex;

        static int hotplugCallback(libusb_context *context,
                                   libusb_device *device,
                                   libusb_hotplug_event event,
                                   void *userData);

    signals:
        void devicesUpdated();

    public slots:
        void startUSBEvents();
        void stopUSBEvents();

    private slots:
        void processUSBEvents();
};

#endif // USBGLOBALS_H
