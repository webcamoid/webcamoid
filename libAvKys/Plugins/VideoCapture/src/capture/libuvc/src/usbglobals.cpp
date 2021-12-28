/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include <QtConcurrent>

#include "usbglobals.h"

class UsbGlobalsPrivate
{
    public:
        libusb_context *m_context {nullptr};
        libusb_hotplug_callback_handle m_hotplugCallbackHnd {0};
        QThreadPool m_threadPool;
        bool m_processsUsbEventsLoop {false};
        QFuture<void> m_processsUsbEvents;
        QMutex m_mutex;

        void processUSBEvents();
        static int hotplugCallback(libusb_context *context,
                                   libusb_device *device,
                                   libusb_hotplug_event event,
                                   void *userData);
        template <typename T>
        inline void waitLoop(const QFuture<T> &loop)
        {
            while (!loop.isFinished()) {
                auto eventDispatcher = QThread::currentThread()->eventDispatcher();

                if (eventDispatcher)
                    eventDispatcher->processEvents(QEventLoop::AllEvents);
            }
        }
};

UsbGlobals::UsbGlobals(QObject *parent):
    QObject(parent)
{
    this->d = new UsbGlobalsPrivate;
    auto usbError = libusb_init(&this->d->m_context);

    if (usbError != LIBUSB_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << libusb_strerror(libusb_error(usbError));

        return;
    }

#if QT_VERSION_CHECK(LIBUSB_MAJOR, LIBUSB_MINOR, LIBUSB_MICRO) >= QT_VERSION_CHECK(1, 0, 15)
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        usbError =
                libusb_hotplug_register_callback(this->d->m_context,
                                                 libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED
                                                                      | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                                 LIBUSB_HOTPLUG_ENUMERATE,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 LIBUSB_HOTPLUG_MATCH_ANY,
                                                 UsbGlobalsPrivate::hotplugCallback,
                                                 this,
                                                 &this->d->m_hotplugCallbackHnd);

        if (usbError != LIBUSB_SUCCESS)
            qDebug() << "CaptureLibUVC:" << libusb_strerror(libusb_error(usbError));
    }
#endif

    this->startUSBEvents();
}

UsbGlobals::~UsbGlobals()
{
#if QT_VERSION_CHECK(LIBUSB_MAJOR, LIBUSB_MINOR, LIBUSB_MICRO) >= QT_VERSION_CHECK(1, 0, 15)
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG))
        libusb_hotplug_deregister_callback(this->d->m_context,
                                           this->d->m_hotplugCallbackHnd);
#endif

    this->stopUSBEvents();

    if (this->d->m_context)
        libusb_exit(this->d->m_context);

    delete this->d;
}

libusb_context *UsbGlobals::context() const
{
    return this->d->m_context;
}

void UsbGlobals::startUSBEvents()
{
    this->d->m_mutex.lock();

    if (!this->d->m_processsUsbEventsLoop) {
        this->d->m_processsUsbEventsLoop = true;
        this->d->m_processsUsbEvents =
                QtConcurrent::run(&this->d->m_threadPool,
                                  this->d,
                                  &UsbGlobalsPrivate::processUSBEvents);
    }

    this->d->m_mutex.unlock();
}

void UsbGlobals::stopUSBEvents()
{
    this->d->m_mutex.lock();
    this->d->m_processsUsbEventsLoop = false;
    this->d->m_mutex.unlock();
    this->d->waitLoop(this->d->m_processsUsbEvents);
}

void UsbGlobalsPrivate::processUSBEvents()
{
    while (this->m_processsUsbEventsLoop) {
        timeval tv {0, 500000};
        libusb_handle_events_timeout_completed(this->m_context, &tv, nullptr);
    }
}

int UsbGlobalsPrivate::hotplugCallback(libusb_context *context,
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

#include "moc_usbglobals.cpp"
