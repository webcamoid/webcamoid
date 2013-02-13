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

#include "frei0relement.h"

Frei0rElement::Frei0rElement(): QbElement()
{
    this->resetPluginName();
    this->resetParams();
    this->resetFrei0rPaths();
}

Frei0rElement::~Frei0rElement()
{
}

QString Frei0rElement::pluginName() const
{
    return this->m_pluginName;
}

QVariantMap Frei0rElement::params() const
{
    return this->m_params;
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

QStringList Frei0rElement::frei0rPaths() const
{
    return this->m_frei0rPaths;
}

bool Frei0rElement::init()
{
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

    switch (infoStruct.color_model)
    {
        case F0R_COLOR_MODEL_BGRA8888:
            this->m_info["color_model"] = "bgra8888";
        break;

        case F0R_COLOR_MODEL_RGBA8888:
            this->m_info["color_model"] = "rgba8888";
        break;

        case F0R_COLOR_MODEL_PACKED32:
            this->m_info["color_model"] = "packed32";
        break;

        default:
            this->m_info["color_model"] = "";
        break;
    }

    this->m_info["frei0r_version"] = infoStruct.frei0r_version;
    this->m_info["major_version"] = infoStruct.major_version;
    this->m_info["minor_version"] = infoStruct.minor_version;
    this->m_info["num_params"] = infoStruct.num_params;
    this->m_info["explanation"] = infoStruct.explanation;

    this->f0rDeinit = (f0r_deinit_t) this->m_library.resolve("f0r_deinit");
    this->f0rGetParamInfo = (f0r_get_param_info_t) this->m_library.resolve("f0r_get_param_info");
    this->f0rConstruct = (f0r_construct_t) this->m_library.resolve("f0r_construct");
    this->f0rDestruct = (f0r_destruct_t) this->m_library.resolve("f0r_destruct");
    this->f0rSetParamValue = (f0r_set_param_value_t) this->m_library.resolve("f0r_set_param_value");
    this->f0rGetParamValue = (f0r_get_param_value_t) this->m_library.resolve("f0r_get_param_value");
    this->f0rUpdate = (f0r_update_t) this->m_library.resolve("f0r_update");
    this->f0rUpdate2 = (f0r_update2_t) this->m_library.resolve("f0r_update2");

    return true;
}

void Frei0rElement::uninit()
{
    if (!this->m_library.isLoaded())
        return;

    this->m_info.clear();
    this->f0rDeinit();
}

void Frei0rElement::cleanAll()
{
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

void Frei0rElement::setParams(QVariantMap params)
{
    this->m_params = params;
}

void Frei0rElement::setFrei0rPaths(QStringList frei0rPaths)
{
    this->m_frei0rPaths = frei0rPaths;
}

void Frei0rElement::resetPluginName()
{
    this->setPluginName("");
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
    Q_UNUSED(packet)
}

void Frei0rElement::setState(ElementState state)
{
    ElementState preState = this->state();

    switch (state)
    {
        case ElementStateNull:
            switch (preState)
            {
                case ElementStatePaused:
                case ElementStatePlaying:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                    this->uninit();
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStateReady:
            switch (preState)
            {
                case ElementStateNull:
                    if (this->init())
                        this->m_state = state;
                    else
                        this->m_state = ElementStateNull;
                break;

                case ElementStatePlaying:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePaused:
            switch (preState)
            {
                case ElementStateNull:
                    this->setState(ElementStateReady);

                    if (this->state() != ElementStateReady)
                        return;

                case ElementStateReady:
                    this->m_state = state;
                break;

                case ElementStatePlaying:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        case ElementStatePlaying:
            switch (preState)
            {
                case ElementStateNull:
                case ElementStateReady:
                    this->setState(ElementStatePaused);

                    if (this->state() != ElementStatePaused)
                        return;

                case ElementStatePaused:
                    this->m_state = state;
                break;

                default:
                break;
            }
        break;

        default:
        break;
    }
}
