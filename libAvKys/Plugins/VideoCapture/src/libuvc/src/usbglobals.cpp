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

#include "usbglobals.h"

template <typename T>
inline void waitLoop(const QFuture<T> &loop)
{
    while (!loop.isFinished()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }
}

UsbGlobals::UsbGlobals(QObject *parent):
    QObject(parent),
    m_context(NULL),
    m_hotplugCallbackHnd(0),
    m_processsUsbEventsLoop(false)
{
    auto usbError = libusb_init(&this->m_context);

    if (usbError != LIBUSB_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << libusb_strerror(libusb_error(usbError));

        return;
    }

    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        usbError =
                libusb_hotplug_register_callback(this->m_context,
                                                 libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                                                                      | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                                 LIBUSB_HOTPLUG_ENUMERATE,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 this->hotplugCallback,
                                                 this,
                                                 &this->m_hotplugCallbackHnd);

        if (usbError != LIBUSB_SUCCESS)
            qDebug() << "CaptureLibUVC:" << libusb_strerror(libusb_error(usbError));
    }

    this->startUSBEvents();
}

UsbGlobals::~UsbGlobals()
{
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
        libusb_hotplug_deregister_callback(this->m_context,
                                           this->m_hotplugCallbackHnd);

    this->stopUSBEvents();

    if (this->m_context)
        libusb_exit(this->m_context);
}

libusb_context *UsbGlobals::context()
{
    return this->m_context;
}

int UsbGlobals::hotplugCallback(libusb_context *context,
                                libusb_device *device,
                                libusb_hotplug_event event,
                                void *userData)
{
    Q_UNUSED(context)
    Q_UNUSED(device)
    Q_UNUSED(event)

    auto self = reinterpret_cast<UsbGlobals *>(userData);
    emit self->devicesUpdated();

    return 0;
}

void UsbGlobals::startUSBEvents()
{
    this->m_mutex.lock();

    if (!this->m_processsUsbEventsLoop) {
        this->m_processsUsbEventsLoop = true;
        this->m_processsUsbEvents =
                QtConcurrent::run(&this->m_threadPool,
                                  this,
                                  &UsbGlobals::processUSBEvents);
    }

    this->m_mutex.unlock();
}

void UsbGlobals::stopUSBEvents()
{
    this->m_mutex.lock();
    this->m_processsUsbEventsLoop = false;
    this->m_mutex.unlock();
    waitLoop(this->m_processsUsbEvents);
}

void UsbGlobals::processUSBEvents()
{
    while (this->m_processsUsbEventsLoop) {
        timeval tv {0, 500000};
        libusb_handle_events_timeout_completed(this->m_context, &tv, NULL);
    }
}
