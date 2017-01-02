/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "capturelibuvc.h"
#include "usbids.h"

Q_GLOBAL_STATIC(UsbIds, usbIds)

#define TIME_OUT 500

#define PROCESSING_UNIT 0
#define CAMERA_TERMINAL 1

template <typename T>
inline void waitLoop(const QFuture<T> &loop)
{
    while (!loop.isFinished()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
    }
}

class UvcControl
{
    public:
        int controlType;
        uint8_t selector;
        QString description;
        QString type;
        bool signd;
        QStringList menu;

        inline static const QVector<UvcControl> &controls()
        {
            static const QVector<UvcControl> controls {
                // Processing Units
                {PROCESSING_UNIT, UVC_PU_CONTROL_UNDEFINED                     , ""                              , ""       , false, {}},
                {PROCESSING_UNIT, UVC_PU_BACKLIGHT_COMPENSATION_CONTROL        , "Backlight Compensation"        , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_BRIGHTNESS_CONTROL                    , "Brightness"                    , "integer", true , {}},
                {PROCESSING_UNIT, UVC_PU_CONTRAST_CONTROL                      , "Contrast"                      , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_GAIN_CONTROL                          , "Gain"                          , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_POWER_LINE_FREQUENCY_CONTROL          , "Power Line Frequency"          , "menu"   , false, {"Disabled",
                                                                                                                                      "50 Hz",
                                                                                                                                      "60 Hz",
                                                                                                                                      "Auto"}},
                {PROCESSING_UNIT, UVC_PU_HUE_CONTROL                           , "Hue"                           , "integer", true , {}},
                {PROCESSING_UNIT, UVC_PU_SATURATION_CONTROL                    , "Saturation"                    , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_SHARPNESS_CONTROL                     , "Sharpness"                     , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_GAMMA_CONTROL                         , "Gamma"                         , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL     , "White Balance Temperature"     , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, "White Balance Temperature Auto", "boolean", false, {}},
                {PROCESSING_UNIT, UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL       , "White Balance Component"       , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL  , "White Balance Component Auto"  , "boolean", false, {}},
                {PROCESSING_UNIT, UVC_PU_DIGITAL_MULTIPLIER_CONTROL            , "Digital Multiplier"            , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL      , "Digital Multiplier Limit"      , "integer", false, {}},
                {PROCESSING_UNIT, UVC_PU_HUE_AUTO_CONTROL                      , "Hue Auto"                      , "boolean", false, {}},
                {PROCESSING_UNIT, UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL         , "Analog Video Standard"         , "menu"   , false, {"None",
                                                                                                                                      "NTSC - 525/60",
                                                                                                                                      "PAL - 625/50",
                                                                                                                                      "SECAM - 625/50",
                                                                                                                                      "NTSC - 625/50",
                                                                                                                                      "PAL - 525/60"}},
                {PROCESSING_UNIT, UVC_PU_ANALOG_LOCK_STATUS_CONTROL            , "Analog Lock Status"            , "menu"   , false, {"Locked",
                                                                                                                                      "Unlocked"}},
                {PROCESSING_UNIT, UVC_PU_CONTRAST_AUTO_CONTROL                 , "Contrast Auto"                 , "boolean", false, {}},

                // Camera Terminals
                {CAMERA_TERMINAL, UVC_CT_CONTROL_UNDEFINED                     , ""                              , ""       , false, {}},
                {CAMERA_TERMINAL, UVC_CT_SCANNING_MODE_CONTROL                 , "Scanning Mode"                 , "boolean", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_AE_MODE_CONTROL                       , "AE Mode"                       , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_AE_PRIORITY_CONTROL                   , "AE Priority"                   , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL        , "Exposure Time Absolute"        , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL        , "Exposure Time Relative"        , "", false, {}},
                {CAMERA_TERMINAL, UVC_CT_FOCUS_ABSOLUTE_CONTROL                , "Focus Absolute"                , "integer", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_FOCUS_RELATIVE_CONTROL                , "Focus Relative"                , "", false, {}},
                {CAMERA_TERMINAL, UVC_CT_FOCUS_AUTO_CONTROL                    , "Focus Auto"                    , "boolean", false, {}},
                {CAMERA_TERMINAL, UVC_CT_IRIS_ABSOLUTE_CONTROL                 , "Iris Absolute"                 , "integer", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_IRIS_RELATIVE_CONTROL                 , "Iris Relative"                 , "", false, {}},
                {CAMERA_TERMINAL, UVC_CT_ZOOM_ABSOLUTE_CONTROL                 , "Zoom Absolute"                 , "integer", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_ZOOM_RELATIVE_CONTROL                 , "Zoom Relative"                 , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_PANTILT_ABSOLUTE_CONTROL              , "Pantilt Absolute"              , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_PANTILT_RELATIVE_CONTROL              , "Pantilt Relative"              , "", false, {}},
                {CAMERA_TERMINAL, UVC_CT_ROLL_ABSOLUTE_CONTROL                 , "Roll Absolute"                 , "integer", true, {}},
                //{CAMERA_TERMINAL, UVC_CT_ROLL_RELATIVE_CONTROL                 , "Roll Relative"                 , "", false, {}},
                {CAMERA_TERMINAL, UVC_CT_PRIVACY_CONTROL                       , "Privacy"                       , "boolean", false, {}},
                {CAMERA_TERMINAL, UVC_CT_FOCUS_SIMPLE_CONTROL                  , "Focus Simple"                  , "menu"   , false, {"Full Range",
                                                                                                                                      "Macro",
                                                                                                                                      "People",
                                                                                                                                      "Scene"}},
                //{CAMERA_TERMINAL, UVC_CT_DIGITAL_WINDOW_CONTROL                , "Digital Window"                , "", false, {}},
                //{CAMERA_TERMINAL, UVC_CT_REGION_OF_INTEREST_CONTROL            , "Region of Interest"            , "", false, {}}
            };

            return controls;
        }

        static inline const UvcControl *bySelector(int controlType,
                                                   uint8_t selector)
        {
            for (int i = 0; i < controls().size(); i++)
                if (controls()[i].controlType == controlType
                    && controls()[i].selector == selector)
                    return &controls()[i];

            // Returns default for control type.
            for (int i = 0; i < controls().size(); i++)
                if (controls()[i].controlType == controlType)
                    return &controls()[i];

            return &controls().first();
        }

        static inline QVector<decltype(selector)> allSelectors(int controlType)
        {
            QVector<decltype(selector)> selectors;

            for (int i = 1; i < controls().size(); i++)
                if (controls()[i].controlType == controlType)
                    selectors << controls()[i].selector;

            return selectors;
        }
};


typedef QMap<QString, uvc_frame_format> PixFmtToUvcMap;

inline PixFmtToUvcMap initPixFmtToUvcMap()
{
    PixFmtToUvcMap fourccToUvc = {
        {"YUY2", UVC_FRAME_FORMAT_YUYV },
        {"UYVY", UVC_FRAME_FORMAT_UYVY },
        {"RGB3", UVC_FRAME_FORMAT_RGB  },
        {"BGR3", UVC_FRAME_FORMAT_BGR  },
        {"MJPG", UVC_FRAME_FORMAT_MJPEG},
        {"Y800", UVC_FRAME_FORMAT_GRAY8},
#ifdef UVC_FRAME_FORMAT_BY8
        {"BY8 ", UVC_FRAME_FORMAT_BY8  },
#endif
    };

    return fourccToUvc;
}

Q_GLOBAL_STATIC_WITH_ARGS(PixFmtToUvcMap, fourccToUvc, (initPixFmtToUvcMap()))

CaptureLibUVC::CaptureLibUVC(QObject *parent):
    Capture(parent),
    m_usbContext(NULL),
    m_uvcContext(NULL),
    m_deviceHnd(NULL),
    m_hotplugCallbackHnd(0),
    m_processsUsbEventsLoop(false),
    m_updateDevices(false),
    m_id(-1)
{
    auto usbError = libusb_init(&this->m_usbContext);

    if (usbError != LIBUSB_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << libusb_strerror(libusb_error(usbError));

        return;
    }

    auto uvcError = uvc_init(&this->m_uvcContext, this->m_usbContext);

    if (uvcError != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(uvcError);

        return;
    }

    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        usbError = libusb_hotplug_register_callback(this->m_usbContext,
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

        this->m_processsUsbEventsLoop = true;
        this->m_processsUsbEvents =
                QtConcurrent::run(&this->m_threadPool,
                                  [this] () {
                                        while (this->m_processsUsbEventsLoop) {
                                            timeval tv {0, 500000};
                                            libusb_handle_events_timeout_completed(this->m_usbContext,
                                                                                   &tv, NULL);
                                        }
                                  });
    }

    this->m_updateDevices = true;

    this->updateDevices();
}

CaptureLibUVC::~CaptureLibUVC()
{
    if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        this->m_processsUsbEventsLoop = false;
        waitLoop(this->m_processsUsbEvents);

        libusb_hotplug_deregister_callback(this->m_usbContext,
                                           this->m_hotplugCallbackHnd);
    }

    if (this->m_uvcContext)
        uvc_exit(this->m_uvcContext);

    if (this->m_usbContext)
        libusb_exit(this->m_usbContext);
}

QStringList CaptureLibUVC::webcams() const
{
    return this->m_devices.values();
}

QString CaptureLibUVC::device() const
{
    return this->m_device;
}

QList<int> CaptureLibUVC::streams() const
{
    if (!this->m_streams.isEmpty())
        return this->m_streams;

    QVariantList caps = this->caps(this->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureLibUVC::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->m_device);
    QList<int> streams;

    for (int i = 0; i < caps.count(); i++)
        streams << i;

    return streams;
}

QString CaptureLibUVC::ioMethod() const
{
    return QString();
}

int CaptureLibUVC::nBuffers() const
{
    return 0;
}

QString CaptureLibUVC::description(const QString &webcam) const
{
    return this->m_descriptions.value(webcam);
}

QVariantList CaptureLibUVC::caps(const QString &webcam) const
{
    return this->m_devicesCaps.value(webcam);
}

QString CaptureLibUVC::capsDescription(const AkCaps &caps) const
{
    if (caps.mimeType() != "video/unknown")
        return QString();

    AkFrac fps = caps.property("fps").toString();

    return QString("%1, %2x%3, %4 FPS")
                .arg(caps.property("fourcc").toString())
                .arg(caps.property("width").toString())
                .arg(caps.property("height").toString())
                .arg(qRound(fps.value()));
}

QVariantList CaptureLibUVC::imageControls() const
{
    return this->m_imageControls.value(this->m_device);
}

bool CaptureLibUVC::setImageControls(const QVariantMap &imageControls)
{
    QVariantMap imageControlsDiff;

    for (const auto &control: this->imageControls()) {
        auto params = control.toList();
        auto ctrlName = params[0].toString();

        if (imageControls.contains(ctrlName)
            && imageControls[ctrlName] != params[6]) {
            imageControlsDiff[ctrlName] = imageControls[ctrlName];
        }
    }

    if (imageControlsDiff.isEmpty())
        return false;

    uvc_device_handle_t *deviceHnd = NULL;

    if (this->m_deviceHnd) {
        deviceHnd = this->m_deviceHnd;
    } else {
        auto deviceVP = this->m_devices.key(this->m_device);
        auto vendorId = deviceVP >> 16;
        auto productId = deviceVP & 0xFFFF;

        uvc_device_t *device = NULL;
        auto error = uvc_find_device(this->m_uvcContext,
                                     &device,
                                     int(vendorId),
                                     int(productId),
                                     NULL);

        if (error != UVC_SUCCESS)
            return false;

        error = uvc_open(device, &deviceHnd);
        uvc_unref_device(device);

        if (error != UVC_SUCCESS)
            return false;
    }

    for (auto pu = uvc_get_processing_units(deviceHnd); pu; pu = pu->next) {
        for (auto &control: UvcControl::allSelectors(PROCESSING_UNIT))
            if (pu->bmControls & control) {
                this->setControls(deviceHnd,
                                  pu->bUnitID,
                                  control,
                                  PROCESSING_UNIT,
                                  imageControlsDiff);
            }
    }

    if (!this->m_deviceHnd)
        uvc_close(deviceHnd);

    QVariantList controls;

    for (const auto &control: this->m_imageControls.value(this->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (imageControlsDiff.contains(controlName))
            controlParams[6] = imageControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->m_imageControls[this->m_device] = controls;
    emit this->imageControlsChanged(imageControlsDiff);

    return true;
}

bool CaptureLibUVC::resetImageControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->imageControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setImageControls(controls);
}

QVariantList CaptureLibUVC::cameraControls() const
{
    return this->m_cameraControls.value(this->m_device);
}

bool CaptureLibUVC::setCameraControls(const QVariantMap &cameraControls)
{
    QVariantMap cameraControlsDiff;

    for (const auto &control: this->cameraControls()) {
        auto params = control.toList();
        auto ctrlName = params[0].toString();

        if (cameraControls.contains(ctrlName)
            && cameraControls[ctrlName] != params[6]) {
            cameraControlsDiff[ctrlName] = cameraControls[ctrlName];
        }
    }

    if (cameraControlsDiff.isEmpty())
        return false;

    uvc_device_handle_t *deviceHnd = NULL;

    if (this->m_deviceHnd) {
        deviceHnd = this->m_deviceHnd;
    } else {
        auto deviceVP = this->m_devices.key(this->m_device);
        auto vendorId = deviceVP >> 16;
        auto productId = deviceVP & 0xFFFF;

        uvc_device_t *device = NULL;
        auto error = uvc_find_device(this->m_uvcContext,
                                     &device,
                                     int(vendorId),
                                     int(productId),
                                     NULL);

        if (error != UVC_SUCCESS)
            return false;

        error = uvc_open(device, &deviceHnd);
        uvc_unref_device(device);

        if (error != UVC_SUCCESS)
            return false;
    }

    for (auto ca = uvc_get_input_terminals(deviceHnd); ca; ca = ca->next) {
        for (auto &control: UvcControl::allSelectors(CAMERA_TERMINAL))
            if (ca->bmControls & control) {
                this->setControls(deviceHnd,
                                  ca->bTerminalID,
                                  control,
                                  CAMERA_TERMINAL,
                                  cameraControlsDiff);
            }
    }

    if (!this->m_deviceHnd)
        uvc_close(deviceHnd);

    QVariantList controls;

    for (const auto &control: this->m_cameraControls.value(this->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (cameraControlsDiff.contains(controlName))
            controlParams[6] = cameraControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->m_cameraControls[this->m_device] = controls;
    emit this->cameraControlsChanged(cameraControlsDiff);

    return true;
}

bool CaptureLibUVC::resetCameraControls()
{
    QVariantMap controls;

    for (const QVariant &control: this->cameraControls()) {
        QVariantList params = control.toList();

        controls[params[0].toString()] = params[5].toInt();
    }

    return this->setCameraControls(controls);
}

AkPacket CaptureLibUVC::readFrame()
{
    this->m_mutex.lock();

    if (!this->m_curPacket)
        if (!this->m_packetNotReady.wait(&this->m_mutex, TIME_OUT)) {
            this->m_mutex.unlock();

            return AkPacket();
        }

    auto packet = this->m_curPacket;
    this->m_curPacket = AkPacket();

    this->m_mutex.unlock();

    return packet;
}

QVariantList CaptureLibUVC::controlsList(uvc_device_handle_t *deviceHnd,
                                         uint8_t unit,
                                         uint8_t control,
                                         int controlType) const
{
    auto selector = UvcControl::bySelector(controlType, control);
    int min = 0;
    int max = 0;
    int step = 0;
    int defaultValue = 0;
    int value = 0;

    if (selector->type == "integer") {
        if (selector->signd) {
            int16_t val = 0;

            if (uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_CUR) < 0)
                return QVariantList();

            value = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_MIN);
            min = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_MAX);
            max = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_RES);
            step = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_DEF);
            defaultValue = val;
        } else {
            uint16_t val = 0;

            if (uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_CUR) < 0)
                return QVariantList();

            value = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint16_t), UVC_GET_MIN);
            min = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint16_t), UVC_GET_MAX);
            max = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint16_t), UVC_GET_RES);
            step = val;
            uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint16_t), UVC_GET_DEF);
            defaultValue = val;
        }
    } else if (selector->type == "boolean") {
        uint8_t val = false;

        if (uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_CUR) < 0)
            return QVariantList();

        value = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_MIN);
        min = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_MAX);
        max = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_RES);
        step = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_DEF);
        defaultValue = val;
    } else if (selector->type == "menu") {
        uint8_t val = 0;

        if (uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(int16_t), UVC_GET_CUR) < 0)
            return QVariantList();

        value = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_MIN);
        min = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_MAX);
        max = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_RES);
        step = val;
        uvc_get_ctrl(deviceHnd, unit, control, &val, sizeof(uint8_t), UVC_GET_DEF);
        defaultValue = val;
    }

    return QVariantList {
        selector->description,
        selector->type,
        min,
        max,
        step,
        defaultValue,
        value,
        selector->menu
    };
}

void CaptureLibUVC::setControls(uvc_device_handle_t *deviceHnd,
                                uint8_t unit,
                                uint8_t control,
                                int controlType,
                                const QVariantMap &values)
{
    auto selector = UvcControl::bySelector(controlType,
                                           control);

    if (!values.contains(selector->description))
        return;

    if (selector->type == "integer") {
        if (selector->signd) {
            auto val = int16_t(values[selector->description].toInt());

            uvc_set_ctrl(deviceHnd,
                         unit,
                         control,
                         &val,
                         sizeof(int16_t));
        } else {
            auto val = uint16_t(values[selector->description].toUInt());

            uvc_set_ctrl(deviceHnd,
                         unit,
                         control,
                         &val,
                         sizeof(uint16_t));
        }
    } else if (selector->type == "boolean") {
        uint8_t val = values[selector->description].toBool();

        uvc_set_ctrl(deviceHnd,
                     unit,
                     control,
                     &val,
                     sizeof(uint8_t));
    } else if (selector->type == "menu") {
        auto val = uint8_t(values[selector->description].toUInt());

        uvc_set_ctrl(deviceHnd,
                     unit,
                     control,
                     &val,
                     sizeof(uint8_t));
    }
}

int CaptureLibUVC::hotplugCallback(libusb_context *context,
                                   libusb_device *device,
                                   libusb_hotplug_event event,
                                   void *userData)
{
    Q_UNUSED(context)
    Q_UNUSED(device)
    Q_UNUSED(event)

    auto self = reinterpret_cast<CaptureLibUVC *>(userData);

    if (self->m_updateDevices)
        QMetaObject::invokeMethod(self, "updateDevices");

    return 0;
}

void CaptureLibUVC::frameCallback(uvc_frame *frame, void *userData)
{
    if (!frame || !userData)
        return;

    auto self = reinterpret_cast<CaptureLibUVC *>(userData);

    self->m_mutex.lock();

    AkCaps caps;
    caps.setMimeType("video/unknown");
    caps.setProperty("fourcc", fourccToUvc->key(frame->frame_format));
    caps.setProperty("width", frame->width);
    caps.setProperty("height", frame->height);
    caps.setProperty("fps", self->m_fps.toString());

    QByteArray buffer(reinterpret_cast<const char *>(frame->data),
                       int(frame->data_bytes));

    auto pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                      * self->m_fps.value() / 1e3);

    AkPacket packet(caps, buffer);
    packet.setPts(pts);
    packet.setTimeBase(self->m_fps.invert());
    packet.setIndex(0);
    packet.setId(self->m_id);

    self->m_curPacket = packet;
    self->m_packetNotReady.wakeAll();
    self->m_mutex.unlock();
}

bool CaptureLibUVC::init()
{
    if (this->m_devices.isEmpty() || this->m_device.isEmpty())
        return false;

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "CaptureLibUVC: No streams available.";

        return false;
    }

    auto deviceVP = this->m_devices.key(this->m_device);
    auto vendorId = deviceVP >> 16;
    auto productId = deviceVP & 0xFFFF;

    uvc_device_t *device = NULL;
    auto error = uvc_find_device(this->m_uvcContext,
                                 &device,
                                 int(vendorId),
                                 int(productId),
                                 NULL);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        return false;
    }

    error = uvc_open(device, &this->m_deviceHnd);
    uvc_unref_device(device);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        return false;
    }

    QVariantList supportedCaps = this->caps(this->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    int fps = qRound(AkFrac(caps.property("fps").toString()).value());

    uvc_stream_ctrl_t streamCtrl;
    error = uvc_get_stream_ctrl_format_size(this->m_deviceHnd,
                                            &streamCtrl,
                                            fourccToUvc->value(caps.property("fourcc").toString()),
                                            caps.property("width").toInt(),
                                            caps.property("height").toInt(),
                                            fps);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto init_failed;
    }

    error = uvc_start_streaming(this->m_deviceHnd,
                                &streamCtrl,
                                this->frameCallback,
                                this,
                                0);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto init_failed;
    }

    this->m_curDevice = this->m_device;
    this->m_id = Ak::id();
    this->m_fps = AkFrac(fps, 1);

    return true;

init_failed:
    uvc_close(this->m_deviceHnd);
    this->m_deviceHnd = NULL;

    return false;
}

void CaptureLibUVC::uninit()
{
    this->m_mutex.lock();

    if (this->m_deviceHnd) {
        /* uvc_stop_streaming implementation from uptream hangs when called,
         * following patch is required for making it work properly:
         *
         * https://github.com/ktossell/libuvc/issues/16#issuecomment-101653441
         */
        uvc_stop_streaming(this->m_deviceHnd);
        uvc_close(this->m_deviceHnd);
        this->m_deviceHnd = NULL;
    }

    this->m_curPacket = AkPacket();
    this->m_curDevice.clear();
    this->m_id = -1;
    this->m_fps = AkFrac();
    this->m_mutex.unlock();
}

void CaptureLibUVC::setDevice(const QString &device)
{
    if (this->m_device == device)
        return;

    this->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureLibUVC::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    QVariantList supportedCaps = this->caps(this->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->m_streams = inputStreams;
    emit this->streamsChanged(inputStreams);
}

void CaptureLibUVC::setIoMethod(const QString &ioMethod)
{
    Q_UNUSED(ioMethod)
}

void CaptureLibUVC::setNBuffers(int nBuffers)
{
    Q_UNUSED(nBuffers)
}

void CaptureLibUVC::resetDevice()
{
    this->setDevice("");
}

void CaptureLibUVC::resetStreams()
{
    QVariantList supportedCaps = this->caps(this->m_device);
    QList<int> streams;

    if (!supportedCaps.isEmpty())
        streams << 0;

    this->setStreams(streams);
}

void CaptureLibUVC::resetIoMethod()
{
}

void CaptureLibUVC::resetNBuffers()
{
}

void CaptureLibUVC::reset()
{
    this->resetStreams();
    this->resetImageControls();
    this->resetCameraControls();
}

void CaptureLibUVC::updateDevices()
{
    if (!this->m_uvcContext)
        return;

    decltype(this->m_devices) devicesList;
    decltype(this->m_descriptions) descriptions;
    decltype(this->m_devicesCaps) devicesCaps;
    decltype(this->m_imageControls) imageControls;
    decltype(this->m_cameraControls) cameraControls;

    uvc_device_t **devices = NULL;
    auto error = uvc_get_device_list(this->m_uvcContext, &devices);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto updateDevices_failed;
    }

    for (int i = 0; devices[i] != NULL; i++) {
        uvc_device_descriptor_t *descriptor = NULL;
        error = uvc_get_device_descriptor(devices[i], &descriptor);

        if (error != UVC_SUCCESS) {
            qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

            continue;
        }

        auto deviceId
                = QString("USB\\VID_v%1&PID_d%2")
                    .arg(descriptor->idVendor, 4, 16, QChar('0'))
                    .arg(descriptor->idProduct, 4, 16, QChar('0'));

        uvc_device_handle_t *deviceHnd = NULL;

        if (this->m_deviceHnd && this->m_curDevice == deviceId)
            deviceHnd = this->m_deviceHnd;
        else {
            error = uvc_open(devices[i], &deviceHnd);

            if (error != UVC_SUCCESS) {
                qDebug() << "CaptureLibUVC:" << uvc_strerror(error);
                uvc_free_device_descriptor(descriptor);

                continue;
            }
        }

        auto formatDescription = uvc_get_format_descs(deviceHnd);

        if (!formatDescription) {
            qDebug() << "CaptureLibUVC: Can't read format description";

            if (!this->m_deviceHnd || this->m_curDevice != deviceId)
                uvc_close(deviceHnd);

            uvc_free_device_descriptor(descriptor);

            continue;
        }

        auto description =
                usbIds->description(descriptor->idVendor, descriptor->idProduct);

        if (description.isEmpty()) {
            if (QString(descriptor->manufacturer).isEmpty())
                description += QString("Vendor 0x%1")
                               .arg(descriptor->idVendor, 4, 16, QChar('0'));
            else
                description += QString(descriptor->manufacturer);

            description += ", ";

            if (QString(descriptor->product).isEmpty())
                description += QString("Product 0x%1")
                               .arg(descriptor->idProduct, 4, 16, QChar('0'));
            else
                description += QString(descriptor->product);
        }

        devicesList[quint32((descriptor->idVendor << 16)
                            | descriptor->idProduct)] = deviceId;
        descriptions[deviceId] = description;
        devicesCaps[deviceId] = QVariantList();
        AkCaps videoCaps;
        videoCaps.setMimeType("video/unknown");

        for (; formatDescription; formatDescription = formatDescription->next) {
            auto fourCC = this->fourccToStr(formatDescription->fourccFormat);

            if (!fourccToUvc->contains(fourCC))
                continue;

            videoCaps.setProperty("fourcc", fourCC);

            for (auto description = formatDescription->frame_descs;
                 description;
                 description = description->next) {
                videoCaps.setProperty("width", description->wWidth);
                videoCaps.setProperty("height", description->wHeight);

                if (description->intervals) {
                    int prevInterval = 0;

                    for (auto interval = description->intervals; interval && *interval; interval++) {
                        auto fps = AkFrac(100e5, *interval);
                        auto fpsValue = qRound(fps.value());

                        if (prevInterval != fpsValue) {
                            videoCaps.setProperty("fps", fps.toString());
                            devicesCaps[deviceId] << QVariant::fromValue(videoCaps);
                        }

                        prevInterval = fpsValue;
                    }
                } else if (description->dwFrameIntervalStep > 0
                         && description->dwMinFrameInterval != description->dwMaxFrameInterval) {
                    int prevInterval = 0;

                    for (auto interval = description->dwMinFrameInterval;
                         interval <= description->dwMaxFrameInterval;
                         interval += description->dwFrameIntervalStep) {
                        auto fps = AkFrac(100e5, interval);
                        auto fpsValue = qRound(fps.value());

                        if (prevInterval != fpsValue) {
                            videoCaps.setProperty("fps", fps.toString());
                            devicesCaps[deviceId] << QVariant::fromValue(videoCaps);
                        }

                        prevInterval = fpsValue;
                    }
                } else {
                    auto fps = AkFrac(100e5, description->dwDefaultFrameInterval);
                    videoCaps.setProperty("fps", fps.toString());
                    devicesCaps[deviceId] << QVariant::fromValue(videoCaps);
                }
            }
        }

        QVariantList deviceControls;

        for (auto pu = uvc_get_processing_units(deviceHnd); pu; pu = pu->next) {
            for (auto &control: UvcControl::allSelectors(PROCESSING_UNIT))
                if (pu->bmControls & control) {
                    auto controls = this->controlsList(deviceHnd,
                                                       pu->bUnitID,
                                                       control,
                                                       PROCESSING_UNIT);

                    if (!controls.isEmpty())
                        deviceControls << QVariant(controls);
                }
        }

        imageControls[deviceId] = deviceControls;
        deviceControls.clear();

        for (auto ca = uvc_get_input_terminals(deviceHnd); ca; ca = ca->next) {
            for (auto &control: UvcControl::allSelectors(CAMERA_TERMINAL))
                if (ca->bmControls & control) {
                    auto controls = this->controlsList(deviceHnd,
                                                       ca->bTerminalID,
                                                       control,
                                                       CAMERA_TERMINAL);

                    if (!controls.isEmpty())
                        deviceControls << QVariant(controls);
                }
        }

        cameraControls[deviceId] = deviceControls;

        if (!this->m_deviceHnd || this->m_curDevice != deviceId)
            uvc_close(deviceHnd);

        uvc_free_device_descriptor(descriptor);
    }

updateDevices_failed:
    if (devices)
        uvc_free_device_list(devices, 1);

    this->m_descriptions = descriptions;
    this->m_devicesCaps = devicesCaps;
    this->m_imageControls = imageControls;
    this->m_cameraControls = cameraControls;

    if (this->m_devices != devicesList) {
        this->m_devices = devicesList;
        emit this->webcamsChanged(this->m_devices.values());
    }
}
