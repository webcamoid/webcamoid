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

#include <QtConcurrent>
#include <ak.h>
#include <akfrac.h>
#include <akcaps.h>
#include <akpacket.h>
#include <libuvc/libuvc.h>

#include "capturelibuvc.h"
#include "usbglobals.h"
#include "usbids.h"

Q_GLOBAL_STATIC(UsbIds, usbIds)

#define TIME_OUT 500

#define PROCESSING_UNIT 0
#define CAMERA_TERMINAL 1

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

Q_GLOBAL_STATIC(UsbGlobals, usbGlobals)

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

class CaptureLibUVCPrivate
{
    public:
        QString m_device;
        QList<int> m_streams;
        QMap<quint32, QString> m_devices;
        QMap<QString, QString> m_descriptions;
        QMap<QString, QVariantList> m_devicesCaps;
        QMap<QString, QVariantList> m_imageControls;
        QMap<QString, QVariantList> m_cameraControls;
        QString m_curDevice;
        AkPacket m_curPacket;
        uvc_context_t *m_uvcContext;
        uvc_device_handle_t *m_deviceHnd;
        QThreadPool m_threadPool;
        QWaitCondition m_packetNotReady;
        QMutex m_mutex;
        qint64 m_id;
        AkFrac m_fps;

        CaptureLibUVCPrivate():
            m_uvcContext(nullptr),
            m_deviceHnd(nullptr),
            m_id(-1)
        {
        }

        inline QVariantList controlsList(uvc_device_handle_t *deviceHnd,
                                         uint8_t unit,
                                         uint8_t control,
                                         int controlType) const;
        inline void setControls(uvc_device_handle_t *deviceHnd,
                                uint8_t unit,
                                uint8_t control,
                                int controlType,
                                const QVariantMap &values);
        inline static void frameCallback(struct uvc_frame *frame, void *userData);
        inline QString fourccToStr(const uint8_t *format) const;
};

CaptureLibUVC::CaptureLibUVC(QObject *parent):
    Capture(parent)
{
    this->d = new CaptureLibUVCPrivate;
    auto uvcError = uvc_init(&this->d->m_uvcContext, usbGlobals->context());

    if (uvcError != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(uvcError);

        return;
    }

    QObject::connect(usbGlobals,
                     &UsbGlobals::devicesUpdated,
                     this,
                     &CaptureLibUVC::updateDevices);

    this->updateDevices();
}

CaptureLibUVC::~CaptureLibUVC()
{
    if (this->d->m_uvcContext)
        uvc_exit(this->d->m_uvcContext);

    delete this->d;
}

QStringList CaptureLibUVC::webcams() const
{
    return this->d->m_devices.values();
}

QString CaptureLibUVC::device() const
{
    return this->d->m_device;
}

QList<int> CaptureLibUVC::streams() const
{
    if (!this->d->m_streams.isEmpty())
        return this->d->m_streams;

    QVariantList caps = this->caps(this->d->m_device);

    if (caps.isEmpty())
        return QList<int>();

    return QList<int> {0};
}

QList<int> CaptureLibUVC::listTracks(const QString &mimeType)
{
    if (mimeType != "video/x-raw"
        && !mimeType.isEmpty())
        return QList<int>();

    QVariantList caps = this->caps(this->d->m_device);
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
    return this->d->m_descriptions.value(webcam);
}

QVariantList CaptureLibUVC::caps(const QString &webcam) const
{
    return this->d->m_devicesCaps.value(webcam);
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
    return this->d->m_imageControls.value(this->d->m_device);
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

    uvc_device_handle_t *deviceHnd = nullptr;

    if (this->d->m_deviceHnd) {
        deviceHnd = this->d->m_deviceHnd;
    } else {
        auto deviceVP = this->d->m_devices.key(this->d->m_device);
        auto vendorId = deviceVP >> 16;
        auto productId = deviceVP & 0xFFFF;

        uvc_device_t *device = nullptr;
        auto error = uvc_find_device(this->d->m_uvcContext,
                                     &device,
                                     int(vendorId),
                                     int(productId),
                                     nullptr);

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
                this->d->setControls(deviceHnd,
                                     pu->bUnitID,
                                     control,
                                     PROCESSING_UNIT,
                                     imageControlsDiff);
            }
    }

    if (!this->d->m_deviceHnd)
        uvc_close(deviceHnd);

    QVariantList controls;

    for (const auto &control: this->d->m_imageControls.value(this->d->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (imageControlsDiff.contains(controlName))
            controlParams[6] = imageControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->d->m_imageControls[this->d->m_device] = controls;
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
    return this->d->m_cameraControls.value(this->d->m_device);
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

    uvc_device_handle_t *deviceHnd = nullptr;

    if (this->d->m_deviceHnd) {
        deviceHnd = this->d->m_deviceHnd;
    } else {
        auto deviceVP = this->d->m_devices.key(this->d->m_device);
        auto vendorId = deviceVP >> 16;
        auto productId = deviceVP & 0xFFFF;

        uvc_device_t *device = nullptr;
        auto error = uvc_find_device(this->d->m_uvcContext,
                                     &device,
                                     int(vendorId),
                                     int(productId),
                                     nullptr);

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
                this->d->setControls(deviceHnd,
                                     ca->bTerminalID,
                                     control,
                                     CAMERA_TERMINAL,
                                     cameraControlsDiff);
            }
    }

    if (!this->d->m_deviceHnd)
        uvc_close(deviceHnd);

    QVariantList controls;

    for (const auto &control: this->d->m_cameraControls.value(this->d->m_device)) {
        auto controlParams = control.toList();
        auto controlName = controlParams[0].toString();

        if (cameraControlsDiff.contains(controlName))
            controlParams[6] = cameraControlsDiff[controlName];

        controls << QVariant(controlParams);
    }

    this->d->m_cameraControls[this->d->m_device] = controls;
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
    this->d->m_mutex.lock();

    if (!this->d->m_curPacket)
        if (!this->d->m_packetNotReady.wait(&this->d->m_mutex, TIME_OUT)) {
            this->d->m_mutex.unlock();

            return AkPacket();
        }

    auto packet = this->d->m_curPacket;
    this->d->m_curPacket = AkPacket();

    this->d->m_mutex.unlock();

    return packet;
}

QString CaptureLibUVC::uvcId(quint16 vendorId, quint16 productId) const
{
    return QString("USB\\VID_v%1&PID_d%2")
            .arg(vendorId, 4, 16, QChar('0'))
            .arg(productId, 4, 16, QChar('0'));
}

QVariantList CaptureLibUVCPrivate::controlsList(uvc_device_handle_t *deviceHnd,
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

void CaptureLibUVCPrivate::setControls(uvc_device_handle_t *deviceHnd,
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

void CaptureLibUVCPrivate::frameCallback(uvc_frame *frame, void *userData)
{
    if (!frame || !userData)
        return;

    auto self = reinterpret_cast<CaptureLibUVC *>(userData);

    self->d->m_mutex.lock();

    AkCaps caps;
    caps.setMimeType("video/unknown");
    caps.setProperty("fourcc", fourccToUvc->key(frame->frame_format));
    caps.setProperty("width", frame->width);
    caps.setProperty("height", frame->height);
    caps.setProperty("fps", self->d->m_fps.toString());

    QByteArray buffer(reinterpret_cast<const char *>(frame->data),
                       int(frame->data_bytes));

    auto pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                      * self->d->m_fps.value() / 1e3);

    AkPacket packet(caps, buffer);
    packet.setPts(pts);
    packet.setTimeBase(self->d->m_fps.invert());
    packet.setIndex(0);
    packet.setId(self->d->m_id);

    self->d->m_curPacket = packet;
    self->d->m_packetNotReady.wakeAll();
    self->d->m_mutex.unlock();
}

QString CaptureLibUVCPrivate::fourccToStr(const uint8_t *format) const
{
    char fourcc[5];
    memcpy(fourcc, format, sizeof(quint32));
    fourcc[4] = 0;

    return QString(fourcc);
}

bool CaptureLibUVC::init()
{
    if (this->d->m_devices.isEmpty() || this->d->m_device.isEmpty())
        return false;

    QList<int> streams = this->streams();

    if (streams.isEmpty()) {
        qDebug() << "CaptureLibUVC: No streams available.";

        return false;
    }

    auto deviceVP = this->d->m_devices.key(this->d->m_device);
    auto vendorId = deviceVP >> 16;
    auto productId = deviceVP & 0xFFFF;

    uvc_device_t *device = nullptr;
    auto error = uvc_find_device(this->d->m_uvcContext,
                                 &device,
                                 int(vendorId),
                                 int(productId),
                                 nullptr);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        return false;
    }

    error = uvc_open(device, &this->d->m_deviceHnd);
    uvc_unref_device(device);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        return false;
    }

    QVariantList supportedCaps = this->caps(this->d->m_device);
    AkCaps caps = supportedCaps[streams[0]].value<AkCaps>();
    int fps = qRound(AkFrac(caps.property("fps").toString()).value());

    uvc_stream_ctrl_t streamCtrl;
    error = uvc_get_stream_ctrl_format_size(this->d->m_deviceHnd,
                                            &streamCtrl,
                                            fourccToUvc->value(caps.property("fourcc").toString()),
                                            caps.property("width").toInt(),
                                            caps.property("height").toInt(),
                                            fps);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto init_failed;
    }

    error = uvc_start_streaming(this->d->m_deviceHnd,
                                &streamCtrl,
                                this->d->frameCallback,
                                this,
                                0);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto init_failed;
    }

    this->d->m_curDevice = this->d->m_device;
    this->d->m_id = Ak::id();
    this->d->m_fps = AkFrac(fps, 1);

    return true;

init_failed:
    uvc_close(this->d->m_deviceHnd);
    this->d->m_deviceHnd = nullptr;

    return false;
}

void CaptureLibUVC::uninit()
{
    this->d->m_mutex.lock();

    if (this->d->m_deviceHnd) {
        uvc_stop_streaming(this->d->m_deviceHnd);
        uvc_close(this->d->m_deviceHnd);
        this->d->m_deviceHnd = nullptr;
    }

    this->d->m_curPacket = AkPacket();
    this->d->m_curDevice.clear();
    this->d->m_id = -1;
    this->d->m_fps = AkFrac();
    this->d->m_mutex.unlock();
}

void CaptureLibUVC::setDevice(const QString &device)
{
    if (this->d->m_device == device)
        return;

    this->d->m_device = device;
    emit this->deviceChanged(device);
}

void CaptureLibUVC::setStreams(const QList<int> &streams)
{
    if (streams.isEmpty())
        return;

    int stream = streams[0];

    if (stream < 0)
        return;

    QVariantList supportedCaps = this->caps(this->d->m_device);

    if (stream >= supportedCaps.length())
        return;

    QList<int> inputStreams {stream};

    if (this->streams() == inputStreams)
        return;

    this->d->m_streams = inputStreams;
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
    QVariantList supportedCaps = this->caps(this->d->m_device);
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
    if (!this->d->m_uvcContext)
        return;

    decltype(this->d->m_devices) devicesList;
    decltype(this->d->m_descriptions) descriptions;
    decltype(this->d->m_devicesCaps) devicesCaps;
    decltype(this->d->m_imageControls) imageControls;
    decltype(this->d->m_cameraControls) cameraControls;

    uvc_device_t **devices = nullptr;
    auto error = uvc_get_device_list(this->d->m_uvcContext, &devices);

    if (error != UVC_SUCCESS) {
        qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

        goto updateDevices_failed;
    }

    for (int i = 0; devices[i] != nullptr; i++) {
        uvc_device_descriptor_t *descriptor = nullptr;
        error = uvc_get_device_descriptor(devices[i], &descriptor);

        if (error != UVC_SUCCESS) {
            qDebug() << "CaptureLibUVC:" << uvc_strerror(error);

            continue;
        }

        auto deviceId = this->uvcId(descriptor->idVendor,
                                    descriptor->idProduct);
        uvc_device_handle_t *deviceHnd = nullptr;

        if (this->d->m_deviceHnd && this->d->m_curDevice == deviceId)
            deviceHnd = this->d->m_deviceHnd;
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

            if (!this->d->m_deviceHnd || this->d->m_curDevice != deviceId)
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
            auto fourCC = this->d->fourccToStr(formatDescription->fourccFormat);

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
                    auto controls =
                            this->d->controlsList(deviceHnd,
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
                    auto controls =
                            this->d->controlsList(deviceHnd,
                                                  ca->bTerminalID,
                                                  control,
                                                  CAMERA_TERMINAL);

                    if (!controls.isEmpty())
                        deviceControls << QVariant(controls);
                }
        }

        cameraControls[deviceId] = deviceControls;

        if (!this->d->m_deviceHnd || this->d->m_curDevice != deviceId)
            uvc_close(deviceHnd);

        uvc_free_device_descriptor(descriptor);
    }

updateDevices_failed:
    if (devices)
        uvc_free_device_list(devices, 1);

    this->d->m_descriptions = descriptions;
    this->d->m_devicesCaps = devicesCaps;
    this->d->m_imageControls = imageControls;
    this->d->m_cameraControls = cameraControls;

    if (this->d->m_devices != devicesList) {
        this->d->m_devices = devicesList;
        emit this->webcamsChanged(this->d->m_devices.values());
    }
}

#include "moc_capturelibuvc.cpp"
