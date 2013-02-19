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

#include <QtGui>

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

    this->m_capsConvert = Qb::create("VCapsConvert");

    QObject::connect(this->m_capsConvert.data(),
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

double Frei0rElement::fps() const
{
    return this->m_fps;
}

QVariantList Frei0rElement::indexMap() const
{
    return this->m_indexMap;
}

QVariantMap Frei0rElement::params()
{
    ElementState preState = this->state();

    if (preState == ElementStateNull)
        this->setState(ElementStateReady);

    f0r_instance_t curInstance = this->m_f0rInstance;

    if (!curInstance)
        this->initBuffers();

    QVariantMap params;

    for (int i = 0; i < this->m_info["num_params"].toInt(); i++)
    {
        f0r_param_info_t info;

        this->f0rGetParamInfo(&info, i);

        if (info.type == F0R_PARAM_BOOL)
        {
            f0r_param_bool value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = value;
        }
        else if (info.type == F0R_PARAM_DOUBLE)
        {
            f0r_param_double value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = value;
        }
        else if (info.type == F0R_PARAM_COLOR)
        {
            f0r_param_color_t value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = QColor(value.r, value.g, value.b);
        }
        else if (info.type == F0R_PARAM_POSITION)
        {
            f0r_param_position_t value;

            this->f0rGetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

            params[info.name] = QPoint(value.x, value.y);
        }
        else if (info.type == F0R_PARAM_STRING)
        {
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

    if (this->state() == ElementStateNull)
    {
        this->setState(ElementStateReady);
        info = this->m_info;
        this->setState(ElementStateNull);
    }

    return info;
}

QStringList Frei0rElement::plugins() const
{
    QStringList plugins;

    foreach (QString path, this->m_frei0rPaths)
    {
        QDir pluginDir(path);

        foreach (QString plugin, pluginDir.entryList(QDir::Files, QDir::Name))
        {
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

bool Frei0rElement::init()
{
    this->cleanAll();

    QString fileName;

    foreach (QString path, this->m_frei0rPaths)
    {
        QString filePath = QString("%1%2%3.so").arg(path)
                                               .arg(QDir::separator())
                                               .arg(this->m_pluginName);

        if (QFileInfo(filePath).exists())
        {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty())
        return false;

    this->m_library.setFileName(fileName);

    if (!this->m_library.load())
    {
        qDebug() << this->m_library.errorString();

        return false;
    }

    this->f0rInit = (f0r_init_t) this->m_library.resolve("f0r_init");

    if (!this->f0rInit || !this->f0rInit())
    {
        qDebug() << this->m_library.errorString();

        return false;
    }

    this->f0rGetPluginInfo = (f0r_get_plugin_info_t) this->m_library.resolve("f0r_get_plugin_info");

    if (!this->f0rGetPluginInfo)
    {
        qDebug() << this->m_library.errorString();

        return false;
    }

    f0r_plugin_info_t infoStruct;

    this->f0rGetPluginInfo(&infoStruct);

    this->m_info["name"] = infoStruct.name;
    this->m_info["author"] = infoStruct.author;

    switch (infoStruct.plugin_type)
    {
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

    switch (infoStruct.color_model)
    {
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
        this->m_info["plugin_type"] == "mixer3")
    {
        caps.setProperty("width", this->m_frameSize.width());
        caps.setProperty("height", this->m_frameSize.height());
    }

    if (caps.isValid())
        this->m_capsConvert->setProperty("caps", caps.toString());

    this->m_info["frei0r_version"] = infoStruct.frei0r_version;
    this->m_info["major_version"] = infoStruct.major_version;
    this->m_info["minor_version"] = infoStruct.minor_version;
    this->m_info["num_params"] = infoStruct.num_params;
    this->m_info["explanation"] = infoStruct.explanation;

    if (infoStruct.plugin_type == F0R_PLUGIN_TYPE_FILTER ||
        infoStruct.plugin_type == F0R_PLUGIN_TYPE_SOURCE)
    {
        this->f0rUpdate = (f0r_update_t) this->m_library.resolve("f0r_update");

        if (!this->f0rUpdate)
            return false;
    }
    else
    {
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

    if (this->m_info["plugin_type"] == "source")
    {
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

    if (this->m_info["plugin_type"] == "source")
        this->uninitBuffers();

    this->m_info.clear();
    this->f0rDeinit();
    this->m_library.unload();
}

bool Frei0rElement::initBuffers()
{
    if (this->m_f0rInstance)
        this->uninitBuffers();

    int width;
    int height;

    if (this->m_info["plugin_type"] == "filter" &&
        this->m_curInputCaps.isValid())
    {
        width = this->m_curInputCaps.property("width").toInt();
        height = this->m_curInputCaps.property("height").toInt();
    }
    else
    {
        width = this->m_frameSize.width();
        height = this->m_frameSize.height();
    }

    this->m_f0rInstance = this->f0rConstruct(width, height);

    if (!this->m_f0rInstance)
        return false;

    return true;
}

void Frei0rElement::uninitBuffers()
{
    if (!this->m_f0rInstance)
        return;

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

void Frei0rElement::setFps(double fps)
{
    this->m_fps = (fps <= 0)? 0.001: fps;
    this->m_duration = 1000.0 / this->m_fps;
    this->m_timer.setInterval(this->m_duration);
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
        this->setState(ElementStateReady);

    f0r_instance_t curInstance = this->m_f0rInstance;

    if (!curInstance)
        this->initBuffers();

    for (int i = 0; i < this->m_info["num_params"].toInt(); i++)
    {
        f0r_param_info_t info;

        this->f0rGetParamInfo(&info, i);

        if (!params.contains(info.name))
            continue;

        if (info.type == F0R_PARAM_BOOL)
        {
            f0r_param_bool value = params[info.name].toBool();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);

        }
        else if (info.type == F0R_PARAM_DOUBLE)
        {
            f0r_param_double value = params[info.name].toDouble();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_COLOR)
        {
            f0r_param_color_t value;
            QColor color = params[info.name].value<QColor>();

            value.r = color.red();
            value.g = color.green();
            value.b = color.blue();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_POSITION)
        {
            f0r_param_position_t value;
            QPoint point = params[info.name].toPoint();

            value.x = point.x();
            value.y = point.y();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
        else if (info.type == F0R_PARAM_STRING)
        {
            f0r_param_string value = params[info.name].toString().toUtf8().data();

            this->f0rSetParamValue(this->m_f0rInstance,
                                   &value,
                                   i);
        }
    }

    this->m_params = this->params();

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
    this->setFps(30);
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

    paths << QDir::homePath() + QDir::separator() + ".frei0r-1/lib"
          << "/usr/local/lib/frei0r-1"
          << "/usr/lib/frei0r-1";

    this->setFrei0rPaths(paths);
}

void Frei0rElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying ||
        this->m_info["plugin_type"] == "source")
        return;

    if (!this->m_f0rInstance || packet.caps() != this->m_curInputCaps)
    {
        this->m_curInputCaps = packet.caps();

        int width;
        int height;

        if (this->m_info["plugin_type"] == "filter")
        {
            width = this->m_curInputCaps.property("width").toInt();
            height = this->m_curInputCaps.property("height").toInt();
        }
        else
        {
            width = this->m_frameSize.width();
            height = this->m_frameSize.height();
        }

        this->initBuffers();
        this->setParams(this->m_params);

        if (this->m_info["plugin_type"] == "mixer2" ||
            this->m_info["plugin_type"] == "mixer3")
        {
            this->m_iBuffer0.resize(4 * width * height);
            this->m_iBuffer1.resize(4 * width * height);
        }

        if (this->m_info["plugin_type"] == "mixer3")
            this->m_iBuffer2.resize(4 * width * height);

        this->m_oBuffer.resize(4 * width * height);
    }

    this->m_capsConvert->iStream(packet);
}

void Frei0rElement::setState(ElementState state)
{
    QbElement::setState(state);

    if (this->m_capsConvert)
        this->m_capsConvert->setState(this->state());

    if (this->state() == ElementStateNull ||
        this->state() == ElementStateReady)
    {
        if (this->m_info["plugin_type"] != "source")
            this->uninitBuffers();

        this->m_t = 0;
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
        foreach (QVariant map, this->m_indexMap)
        {
            QVariantList pair = map.toList();

            if (pair[0].toInt() == packet.index())
            {
                if (pair[1].toInt() == 0)
                    memcpy(this->m_iBuffer0.data(),
                           packet.data(),
                           packet.dataSize());

                if (pair[1].toInt() == 1)
                    memcpy(this->m_iBuffer1.data(),
                           packet.data(),
                           packet.dataSize());

                if (pair[1].toInt() == 2 &&
                    this->m_info["plugin_type"] == "mixer3")
                    memcpy(this->m_iBuffer2.data(),
                           packet.data(),
                           packet.dataSize());
            }
        }

    QbCaps caps;
    int64_t dts;
    int64_t pts;
    int duration;
    QbFrac timeBase;
    int index;

    if (this->m_info["plugin_type"] == "source")
    {
        QString format;

        if (this->m_info["color_model"] == "bgra8888")
            format = "bgra";
        else
            format = "rgba";

        caps = QbCaps(QString("video/x-raw,"
                              "format=%1,"
                              "width=%2,"
                              "height=%3").arg(format)
                                          .arg(this->m_frameSize.width())
                                          .arg(this->m_frameSize.height()));

        dts = pts = this->m_t;
        duration = this->m_duration;
        timeBase = QbFrac(1, 1000);
        index = 0;
    }
    else
    {
        caps = packet.caps();
        dts = packet.dts();
        pts = packet.pts();
        duration = packet.duration();
        timeBase = packet.timeBase();
        index = packet.index();
    }

    if (this->m_info["plugin_type"] == "filter")
        this->f0rUpdate(this->m_f0rInstance,
                        pts * timeBase.value(),
                        (uint32_t *) packet.data(),
                        (uint32_t *) this->m_oBuffer.data());
    else if (this->m_info["plugin_type"] == "source")
        this->f0rUpdate(this->m_f0rInstance,
                        this->m_t,
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

    this->m_t += this->m_duration;

    QbPacket oPacket(caps,
                     this->m_oBuffer.constData(),
                     this->m_oBuffer.size());

    oPacket.setDts(dts);
    oPacket.setPts(pts);
    oPacket.setDuration(duration);
    oPacket.setTimeBase(timeBase);
    oPacket.setIndex(index);

    emit this->oStream(oPacket);
}
