/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#include <qrgb.h>

#include "frei0relement.h"

Frei0rElement::Frei0rElement(): QbElement()
{
    this->cleanAll();
    this->resetPluginName();
    this->resetFrameSize();
    this->resetFps();
    this->resetIndexMap();
    this->resetParams();
    this->resetFrei0rPaths();

    this->m_convert = Qb::create("VCapsConvert");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    QObject::connect(&this->m_timer,
                     SIGNAL(timeout()),
                     this,
                     SLOT(processFrame()));
}

Frei0rElement::~Frei0rElement()
{
}

QString Frei0rElement::pluginName() const
{
    return this->m_pluginName;
}

QSize Frei0rElement::frameSize() const
{
    return this->m_frameSize;
}

QString Frei0rElement::fps() const
{
    return this->m_fps.toString();
}

QVariantList Frei0rElement::indexMap() const
{
    return this->m_indexMap;
}

QVariantMap Frei0rElement::params()
{
    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStatePaused);

    f0r_instance_t curInstance = this->m_f0rInstance;

    if (!curInstance)
        this->initBuffers();

    QVariantMap params;

    for (int i = 0; i < this->m_info["num_params"].toInt(); i++) {
        f0r_param_info_t info;

        this->f0rGetParamInfo(&info, i);

        if (info.type == F0R_PARAM_BOOL) {
            f0r_param_bool value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = value;
        }
        else if (info.type == F0R_PARAM_DOUBLE) {
            f0r_param_double value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = value;
        }
        else if (info.type == F0R_PARAM_COLOR) {
            f0r_param_color_t value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = qRgb(value.r, value.g, value.b);
        }
        else if (info.type == F0R_PARAM_POSITION) {
            f0r_param_position_t value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = QPoint(value.x, value.y);
        }
        else if (info.type == F0R_PARAM_STRING) {
            f0r_param_string value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = value;
        }
        else
            params[info.name] = QVariant();
    }

    if (!curInstance)
        this->uninitBuffers();

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);

    this->m_params = params;

    return params;
}

QVariantMap Frei0rElement::info()
{
    QVariantMap info = this->m_info;

    if (this->state() == ElementStateNull) {
        this->setState(ElementStatePaused);
        info = this->m_info;
        this->setState(ElementStateNull);
    }

    return info;
}

QStringList Frei0rElement::plugins() const
{
    QStringList plugins;

    foreach (QString path, this->m_frei0rPaths) {
        QDir pluginDir(path);

        foreach (QString plugin, pluginDir.entryList(QDir::Files, QDir::Name)) {
            QString fileName = pluginDir.absoluteFilePath(plugin);

            if (QLibrary::isLibrary(fileName))
                plugins << plugin.replace(QRegExp(".so$"), "");
        }
    }

    return plugins;
}

QStringList Frei0rElement::frei0rPaths() const
{
    return this->m_frei0rPaths;
}

bool Frei0rElement::event(QEvent *event)
{
    bool r;

    if (event->type() == QEvent::ThreadChange) {
        QObject::disconnect(this->m_convert.data(),
                            SIGNAL(oStream(const QbPacket &)),
                            this,
                            SIGNAL(processFrame(const QbPacket &)));

        QObject::disconnect(&this->m_timer,
                            SIGNAL(timeout()),
                            this,
                            SLOT(processFrame()));

        r = QObject::event(event);
        this->m_convert->moveToThread(this->thread());
        this->m_timer.moveToThread(this->thread());

        QObject::connect(this->m_convert.data(),
                         SIGNAL(oStream(const QbPacket &)),
                         this,
                         SIGNAL(processFrame(const QbPacket &)));

        QObject::connect(&this->m_timer,
                         SIGNAL(timeout()),
                         this,
                         SLOT(processFrame()));
    }
    else
        r = QObject::event(event);

    return r;
}

bool Frei0rElement::init()
{
    this->cleanAll();

    QString fileName;

    foreach (QString path, this->m_frei0rPaths) {
        QString filePath = QString("%1%2%3.so").arg(path)
                                               .arg(QDir::separator())
                                               .arg(this->m_pluginName);

        if (QFileInfo(filePath).exists()) {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty())
        return false;

    this->m_library.setFileName(fileName);

    if (!this->m_library.load()) {
        qDebug() << this->m_library.errorString();

        return false;
    }

    this->f0rInit = (f0r_init_t) this->m_library.resolve("f0r_init");

    if (!this->f0rInit || !this->f0rInit()) {
        qDebug() << this->m_library.errorString();

        return false;
    }

    this->f0rGetPluginInfo = (f0r_get_plugin_info_t) this->m_library.resolve("f0r_get_plugin_info");

    if (!this->f0rGetPluginInfo) {
        qDebug() << this->m_library.errorString();

        return false;
    }

    f0r_plugin_info_t infoStruct;

    this->f0rGetPluginInfo(&infoStruct);

    this->m_info["name"] = infoStruct.name;
    this->m_info["author"] = infoStruct.author;

    switch (infoStruct.plugin_type) {
        case F0R_PLUGIN_TYPE_FILTER:
            this->m_info["plugin_type"] = "filter";
        break;

        case F0R_PLUGIN_TYPE_SOURCE:
            this->m_info["plugin_type"] = "source";
        break;

        case F0R_PLUGIN_TYPE_MIXER2:
            this->m_info["plugin_type"] = "mixer2";
        break;

        case F0R_PLUGIN_TYPE_MIXER3:
            this->m_info["plugin_type"] = "mixer3";
        break;

        default:
            this->m_info["plugin_type"] = "";
        break;
    }

    QbCaps caps;

    switch (infoStruct.color_model) {
        case F0R_COLOR_MODEL_BGRA8888:
            this->m_info["color_model"] = "bgra8888";
            caps = QbCaps("video/x-raw,format=bgra");
        break;

        case F0R_COLOR_MODEL_RGBA8888:
            this->m_info["color_model"] = "rgba8888";
            caps = QbCaps("video/x-raw,format=rgba");
        break;

        case F0R_COLOR_MODEL_PACKED32:
            this->m_info["color_model"] = "packed32";
            caps = QbCaps("video/x-raw,format=rgba");
        break;

        default:
            this->m_info["color_model"] = "";
        break;
    }

    if (this->m_info["plugin_type"] == "mixer2" ||
        this->m_info["plugin_type"] == "mixer3") {
        caps.setProperty("width", this->m_frameSize.width());
        caps.setProperty("height", this->m_frameSize.height());
    }

    if (caps.isValid())
        this->m_convert->setProperty("caps", caps.toString());

    this->m_info["frei0r_version"] = infoStruct.frei0r_version;
    this->m_info["major_version"] = infoStruct.major_version;
    this->m_info["minor_version"] = infoStruct.minor_version;
    this->m_info["num_params"] = infoStruct.num_params;
    this->m_info["explanation"] = infoStruct.explanation;

    if (infoStruct.plugin_type == F0R_PLUGIN_TYPE_FILTER ||
        infoStruct.plugin_type == F0R_PLUGIN_TYPE_SOURCE) {
        this->f0rUpdate = (f0r_update_t) this->m_library.resolve("f0r_update");

        if (!this->f0rUpdate)
            return false;
    }
    else {
        this->f0rUpdate2 = (f0r_update2_t) this->m_library.resolve("f0r_update2");

        if (!this->f0rUpdate2)
            return false;
    }

    this->f0rConstruct = (f0r_construct_t) this->m_library.resolve("f0r_construct");
    this->f0rDeinit = (f0r_deinit_t) this->m_library.resolve("f0r_deinit");
    this->f0rGetParamInfo = (f0r_get_param_info_t) this->m_library.resolve("f0r_get_param_info");
    this->f0rDestruct = (f0r_destruct_t) this->m_library.resolve("f0r_destruct");
    this->f0rSetParamValue = (f0r_set_param_value_t) this->m_library.resolve("f0r_set_param_value");
    this->f0rGetParamValue = (f0r_get_param_value_t) this->m_library.resolve("f0r_get_param_value");

    if (!this->f0rConstruct ||
        !this->f0rDeinit ||
        !this->f0rGetParamInfo ||
        !this->f0rDestruct ||
        !this->f0rSetParamValue ||
        !this->f0rGetParamValue)
        return false;

    if (this->m_info["plugin_type"] == "source") {
        this->m_oBuffer.resize(4 * this->m_frameSize.width() *
                                   this->m_frameSize.height());

        this->initBuffers();
    }

    return true;
}

void Frei0rElement::uninit()
{
    if (!this->m_library.isLoaded())
        return;

    if (this->m_f0rInstance) {
        this->f0rDestruct(this->m_f0rInstance);
        this->m_f0rInstance = NULL;
    }

    if (this->m_info["plugin_type"] == "source")
        this->uninitBuffers();

    this->m_info.clear();
    this->f0rDeinit();
    this->m_library.unload();
}

void Frei0rElement::stateChange(QbElement::ElementState from, QbElement::ElementState to)
{
    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused)
        this->init();
    else if (from == QbElement::ElementStatePaused
             && to == QbElement::ElementStateNull)
        this->uninit();
}

bool Frei0rElement::initBuffers()
{
    if (this->m_f0rInstance)
        this->uninitBuffers();

    int width;
    int height;

    if (this->m_info["plugin_type"] == "filter" &&
        this->m_curInputCaps.isValid()) {
        width = this->m_curInputCaps.property("width").toInt();
        height = this->m_curInputCaps.property("height").toInt();
    }
    else {
        width = this->m_frameSize.width();
        height = this->m_frameSize.height();
    }

    if (!this->f0rConstruct)
        return false;

    this->m_f0rInstance = this->f0rConstruct(width, height);

    if (!this->m_f0rInstance)
        return false;

    return true;
}

void Frei0rElement::uninitBuffers()
{
    if (!this->m_f0rInstance)
        return;

    this->f0rDestruct(this->m_f0rInstance);
    this->m_f0rInstance = NULL;
}

void Frei0rElement::cleanAll()
{
    this->m_f0rInstance = NULL;
    this->f0rInit = NULL;
    this->f0rDeinit = NULL;
    this->f0rGetPluginInfo = NULL;
    this->f0rGetParamInfo = NULL;
    this->f0rConstruct = NULL;
    this->f0rDestruct = NULL;
    this->f0rSetParamValue = NULL;
    this->f0rGetParamValue = NULL;
    this->f0rUpdate = NULL;
    this->f0rUpdate2 = NULL;
}

void Frei0rElement::setPluginName(QString pluginName)
{
    if (pluginName == this->pluginName())
        return;

    ElementState preState = this->state();

    this->setState(ElementStateNull);
    this->m_pluginName = pluginName;

    if (!this->pluginName().isEmpty())
        this->setState(preState);
}

void Frei0rElement::setFrameSize(QSize frameSize)
{
    ElementState preState = this->state();

    this->setState(ElementStateNull);
    this->m_frameSize = frameSize;

    if (!this->pluginName().isEmpty())
        this->setState(preState);
}

void Frei0rElement::setFps(QString fps)
{
    this->m_fps = QbFrac(fps);

    if (this->m_fps.num() < 1 || this->m_fps.den() < 1) {
        this->m_fps.setNum(1);
        this->m_fps.setDen(1);
    }

    this->m_timer.setInterval(1e3 / this->m_fps.value());
}

void Frei0rElement::setIndexMap(QVariantList indexMap)
{
    this->m_indexMap = indexMap;
}

void Frei0rElement::setParams(QVariantMap params)
{
    if (params.isEmpty())
        return;

    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStatePaused);

    f0r_instance_t curInstance = this->m_f0rInstance;

    if (!curInstance)
        this->initBuffers();

    for (int i = 0; i < this->m_info["num_params"].toInt(); i++) {
        f0r_param_info_t info;

        this->f0rGetParamInfo(&info, i);

        if (!params.contains(info.name))
            continue;

        if (info.type == F0R_PARAM_BOOL) {
            f0r_param_bool value = params[info.name].toBool();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

        }
        else if (info.type == F0R_PARAM_DOUBLE) {
            f0r_param_double value = params[info.name].toDouble();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_COLOR) {
            f0r_param_color_t value;
            QRgb color = params[info.name].toInt();

            value.r = qRed(color);
            value.g = qGreen(color);
            value.b = qBlue(color);

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_POSITION) {
            f0r_param_position_t value;
            QPoint point = params[info.name].toPoint();

            value.x = point.x();
            value.y = point.y();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_STRING) {
            f0r_param_string value = params[info.name].toString().toUtf8().data();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
    }

    this->m_params = params;

    if (!curInstance)
        this->uninitBuffers();

    if (preState == ElementStateNull)
        this->setState(ElementStateNull);
}

void Frei0rElement::setFrei0rPaths(QStringList frei0rPaths)
{
    this->m_frei0rPaths = frei0rPaths;
}

void Frei0rElement::resetPluginName()
{
    this->setPluginName("");
}

void Frei0rElement::resetFrameSize()
{
    this->setFrameSize(QSize(640, 480));
}

void Frei0rElement::resetFps()
{
    this->setFps("30/1");
}

void Frei0rElement::resetIndexMap()
{
    this->setIndexMap(QVariantList());
}

void Frei0rElement::resetParams()
{
    this->setParams(QVariantMap());
}

void Frei0rElement::resetFrei0rPaths()
{
    QStringList paths;
    QChar separator = QDir::separator();

    paths << QString("%2%1.frei0r-1%1lib").arg(separator).arg(QDir::homePath())
          << QString("%2%1frei0r-1").arg(separator).arg(LOCALLIBDIR)
          << QString("%2%1frei0r-1").arg(separator).arg(LIBDIR);

    this->setFrei0rPaths(paths);
}

void Frei0rElement::iStream(const QbPacket &packet)
{
    if (packet.caps().mimeType() != "video/x-raw" ||
        this->m_info["plugin_type"] == "source")
        return;

    if (!this->m_f0rInstance || packet.caps() != this->m_curInputCaps) {
        this->m_curInputCaps = packet.caps();

        int width;
        int height;

        if (this->m_info["plugin_type"] == "filter") {
            width = this->m_curInputCaps.property("width").toInt();
            height = this->m_curInputCaps.property("height").toInt();
        }
        else {
            width = this->m_frameSize.width();
            height = this->m_frameSize.height();
        }

        this->initBuffers();
        this->setParams(this->m_params);

        if (this->m_info["plugin_type"] == "mixer2" ||
            this->m_info["plugin_type"] == "mixer3") {
            this->m_iBuffer0.resize(4 * width * height);
            this->m_iBuffer1.resize(4 * width * height);
        }

        if (this->m_info["plugin_type"] == "mixer3")
            this->m_iBuffer2.resize(4 * width * height);

        this->m_oBuffer.resize(4 * width * height);
    }

    this->m_fps = QbFrac(this->m_curInputCaps.property("fps").toString());

    this->m_convert->iStream(packet);
}

void Frei0rElement::setState(QbElement::ElementState state)
{
    QbElement::setState(state);

    if (this->m_convert)
        this->m_convert->setState(this->state());

    if (this->state() == ElementStateNull) {
        if (this->m_info["plugin_type"] != "source")
            this->uninitBuffers();

        this->m_pts = 0;
    }

    if (this->state() == ElementStatePaused)
        this->m_timer.stop();

    if (this->state() == ElementStatePlaying &&
        this->m_info["plugin_type"] == "source")
        this->m_timer.start();
}

void Frei0rElement::processFrame(const QbPacket &packet)
{
    if (!this->m_f0rInstance)
        return;

    if (this->m_info["plugin_type"] == "mixer2" ||
        this->m_info["plugin_type"] == "mixer3")
        foreach (QVariant map, this->m_indexMap) {
            QVariantList pair = map.toList();

            if (pair[0].toInt() == packet.index()) {
                if (pair[1].toInt() == 0)
                    memcpy(this->m_iBuffer0.data(),
                           packet.buffer().data(),
                           packet.bufferSize());

                if (pair[1].toInt() == 1)
                    memcpy(this->m_iBuffer1.data(),
                           packet.buffer().data(),
                           packet.bufferSize());

                if (pair[1].toInt() == 2 &&
                    this->m_info["plugin_type"] == "mixer3")
                    memcpy(this->m_iBuffer2.data(),
                           packet.buffer().data(),
                           packet.bufferSize());
            }
        }

    QbCaps caps;
    qint64 pts = 0;
    QbFrac timeBase;
    int index = 0;

    if (this->m_info["plugin_type"] == "source") {
        QString format;

        if (this->m_info["color_model"] == "bgra8888")
            format = "bgra";
        else
            format = "rgba";

        caps = packet.caps();
        caps.setProperty("format", format);
        caps.setProperty("width", this->m_frameSize.width());
        caps.setProperty("height", this->m_frameSize.height());
        caps.setProperty("fps", this->m_fps.toString());

        pts = this->m_pts;
        timeBase = this->m_fps.invert();
        index = 0;
    }
    else {
        caps = packet.caps();
        pts = packet.pts();
        timeBase = packet.timeBase();
        index = packet.index();
    }

    if (this->m_oBuffer.isEmpty())
        return;

    if (this->m_info["plugin_type"] == "filter")
        this->f0rUpdate(this->m_f0rInstance,
                        pts * timeBase.value(),
                        (uint32_t *) packet.buffer().data(),
                        (uint32_t *) this->m_oBuffer.data());
    else if (this->m_info["plugin_type"] == "source")
        this->f0rUpdate(this->m_f0rInstance,
                        this->m_pts,
                        NULL,
                        (uint32_t *) this->m_oBuffer.data());
    else if (this->m_info["plugin_type"] == "mixer2")
        this->f0rUpdate2(this->m_f0rInstance,
                         pts * timeBase.value(),
                         (uint32_t *) this->m_iBuffer0.data(),
                         (uint32_t *) this->m_iBuffer1.data(),
                         NULL,
                         (uint32_t *) this->m_oBuffer.data());
    else if (this->m_info["plugin_type"] == "mixer3")
        this->f0rUpdate2(this->m_f0rInstance,
                         pts * timeBase.value(),
                         (uint32_t *) this->m_iBuffer0.data(),
                         (uint32_t *) this->m_iBuffer1.data(),
                         (uint32_t *) this->m_iBuffer2.data(),
                         (uint32_t *) this->m_oBuffer.data());

    this->m_pts++;

    QbBufferPtr oBuffer(new char[this->m_oBuffer.size()]);
    memcpy(oBuffer.data(), this->m_oBuffer.constData(), this->m_oBuffer.size());

    QbPacket oPacket(caps,
                     oBuffer,
                     this->m_oBuffer.size());

    oPacket.setPts(pts);
    oPacket.setTimeBase(timeBase);
    oPacket.setIndex(index);

    emit this->oStream(oPacket);
}
