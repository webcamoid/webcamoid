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

#include <QtDebug>
#include "jackserver.h"

JackServer::JackServer(on_device_acquire_t onDeviceAcquire,
                       on_device_release_t onDeviceRelease,
                       QObject *parent):
    QObject(parent)
{
    this->m_library.setFileName("jackserver");
    this->m_library.load();

    LOAD_FUNC(this->m_library, jackctl_server_create);
    LOAD_FUNC(this->m_library, jackctl_server_destroy);
    LOAD_FUNC(this->m_library, jackctl_server_start);
    LOAD_FUNC(this->m_library, jackctl_server_stop);
    LOAD_FUNC(this->m_library, jackctl_server_get_drivers_list);
    LOAD_FUNC(this->m_library, jackctl_server_get_parameters);
    LOAD_FUNC(this->m_library, jackctl_server_switch_master);
    LOAD_FUNC(this->m_library, jackctl_driver_get_name);
    LOAD_FUNC(this->m_library, jackctl_driver_get_parameters);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_name);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_short_description);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_long_description);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_type);
    LOAD_FUNC(this->m_library, jackctl_parameter_is_set);
    LOAD_FUNC(this->m_library, jackctl_parameter_reset);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_value);
    LOAD_FUNC(this->m_library, jackctl_parameter_set_value);
    LOAD_FUNC(this->m_library, jackctl_parameter_get_default_value);

    this->m_server = this->jackctl_server_create(onDeviceAcquire,
                                                 onDeviceRelease);
}

JackServer::~JackServer()
{
    if (this->m_server)
        this->jackctl_server_destroy(this->m_server);

    this->m_library.unload();
}

bool JackServer::start(jackctl_driver_t *driver)
{
    return this->jackctl_server_start(this->m_server, driver);
}

bool JackServer::start(const QString &driver)
{
    return this->start(this->driverByName(driver));
}

bool JackServer::stop()
{
    return this->jackctl_server_stop(this->m_server);
}

QList<jackctl_driver_t *> JackServer::drivers() const
{
    QList<jackctl_driver_t *> driversList;
    auto drivers = this->jackctl_server_get_drivers_list(this->m_server);

    for (auto driver = drivers; driver;  driver = driver->next)
        driversList << reinterpret_cast<jackctl_driver_t *>(driver->data);

    return driversList;
}

QStringList JackServer::driversByName() const
{
    QStringList drivers;

    for (auto driver: this->drivers())
        drivers << this->name(driver);

    return drivers;
}

jackctl_driver_t *JackServer::driverByName(const QString &name) const
{
    for (auto driver: this->drivers())
        if (this->name(driver) == name)
            return  driver;

    return NULL;
}

QString JackServer::name(jackctl_driver_t *driver) const
{
    return this->jackctl_driver_get_name(driver);
}

bool JackServer::switchMaster(jackctl_driver_t *driver)
{
    return this->jackctl_server_switch_master(this->m_server, driver);
}

QList<jackctl_parameter_t *> JackServer::parameters() const
{
    QList<jackctl_parameter_t *> parametersList;
    auto parameters = this->jackctl_server_get_parameters(this->m_server);

    for (auto parameter = parameters; parameter;  parameter = parameter->next)
        parametersList << reinterpret_cast<jackctl_parameter_t *>(parameter->data);

    return parametersList;
}

QList<jackctl_parameter_t *> JackServer::parameters(jackctl_driver_t *driver) const
{
    QList<jackctl_parameter_t *> parametersList;
    auto parameters = this->jackctl_driver_get_parameters(driver);

    for (auto parameter = parameters; parameter;  parameter = parameter->next)
        parametersList << reinterpret_cast<jackctl_parameter_t *>(parameter->data);

    return parametersList;
}

QStringList JackServer::parametersByName() const
{
    QStringList parameters;

    for (auto parameter: this->parameters())
        parameters << this->name(parameter);

    return parameters;
}

QStringList JackServer::parametersByName(jackctl_driver_t *driver) const
{
    QStringList parameters;

    for (auto parameter: this->parameters(driver))
        parameters << this->name(parameter);

    return parameters;
}

jackctl_parameter_t *JackServer::parameterByName(const QString &name) const
{
    for (auto parameter: this->parameters())
        if (this->name(parameter) == name)
            return  parameter;

    return NULL;
}

jackctl_parameter_t *JackServer::parameterByName(jackctl_driver_t *driver,
                                                 const QString &name) const
{
    for (auto parameter: this->parameters(driver))
        if (this->name(parameter) == name)
            return  parameter;

    return NULL;
}

QString JackServer::name(jackctl_parameter_t *parameter) const
{
    return QString(this->jackctl_parameter_get_name(parameter));
}

QString JackServer::description(jackctl_parameter_t *parameter) const
{
    return QString(this->jackctl_parameter_get_short_description(parameter));
}

QString JackServer::longDescription(jackctl_parameter_t *parameter) const
{
    return QString(this->jackctl_parameter_get_long_description(parameter));
}

bool JackServer::isSet(jackctl_parameter_t *parameter) const
{
    return jackctl_parameter_is_set(parameter);
}

QVariant JackServer::value(jackctl_parameter_t *parameter,
                           bool defaultValue) const
{
    auto value = defaultValue?
                     this->jackctl_parameter_get_default_value(parameter):
                     this->jackctl_parameter_get_value(parameter);

    switch (this->jackctl_parameter_get_type(parameter)) {
    case JackParamInt:
        return QVariant(value.i);
    case JackParamUInt:
        return QVariant(value.ui);
    case JackParamChar:
        return QVariant(value.c);
    case JackParamString:
        return QVariant(value.str);
    case JackParamBool:
        return QVariant(value.b);
    }

    return QVariant();
}

bool JackServer::setValue(jackctl_parameter_t *parameter, const QVariant &value)
{
    jackctl_parameter_value_t jvalue = {};

    switch (this->jackctl_parameter_get_type(parameter)) {
    case JackParamInt:
        jvalue.i = value.value<decltype(jvalue.i)>();

        break;
    case JackParamUInt:
        jvalue.ui = value.value<decltype(jvalue.ui)>();

        break;
    case JackParamChar:
        jvalue.c = value.value<decltype(jvalue.c)>();

        break;
    case JackParamString: {
        auto str = value.toString();
        memcpy(jvalue.str,
               str.toStdString().c_str(),
               size_t(std::min(str.size(), JACK_PARAM_STRING_MAX)));

        break;
    }
    case JackParamBool:
        jvalue.b = value.value<decltype(jvalue.b)>();

        break;
    }

    return this->jackctl_parameter_set_value(parameter, &jvalue);
}

bool JackServer::resetValue(jackctl_parameter_t *parameter)
{
    return this->jackctl_parameter_reset(parameter);
}

jackctl_server_t *JackServer::jackctl_server_create(on_device_acquire_t on_device_acquire,
                                                    on_device_release_t on_device_release) const
{
    if (this->m_jackctl_server_create)
        return this->m_jackctl_server_create(on_device_acquire, on_device_release);

    return NULL;
}

void JackServer::jackctl_server_destroy(jackctl_server_t *server) const
{
    if (m_jackctl_server_destroy)
        this->m_jackctl_server_destroy(server);
}

bool JackServer::jackctl_server_start(jackctl_server_t *server,
                                      jackctl_driver_t *driver) const
{
    if (this->m_jackctl_server_start)
        return this->m_jackctl_server_start(server, driver);

    return false;
}

bool JackServer::jackctl_server_stop(jackctl_server_t *server) const
{
    if (this->m_jackctl_server_stop)
        return this->m_jackctl_server_stop(server);

    return true;
}

const JSList *JackServer::jackctl_server_get_drivers_list(jackctl_server_t *server) const
{
    if (this->m_jackctl_server_get_drivers_list)
        return this->m_jackctl_server_get_drivers_list(server);

    return NULL;
}

const JSList *JackServer::jackctl_server_get_parameters(jackctl_server_t *server) const
{
    if (this->m_jackctl_server_get_parameters)
        return this->m_jackctl_server_get_parameters(server);

    return NULL;
}

bool JackServer::jackctl_server_switch_master(jackctl_server_t *server,
                                              jackctl_driver_t *driver) const
{
    if (this->m_jackctl_server_switch_master)
        return this->m_jackctl_server_switch_master(server, driver);

    return false;
}

const char *JackServer::jackctl_driver_get_name(jackctl_driver_t *driver) const
{
    if (this->m_jackctl_driver_get_name)
        return this->m_jackctl_driver_get_name(driver);

    return NULL;
}

const JSList *JackServer::jackctl_driver_get_parameters(jackctl_driver_t *driver) const
{
    if (this->m_jackctl_driver_get_parameters)
        return this->m_jackctl_driver_get_parameters(driver);

    return NULL;
}

const char *JackServer::jackctl_parameter_get_name(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_name)
        return this->m_jackctl_parameter_get_name(parameter);

    return NULL;
}

const char *JackServer::jackctl_parameter_get_short_description(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_short_description)
        return this->m_jackctl_parameter_get_short_description(parameter);

    return NULL;
}

const char *JackServer::jackctl_parameter_get_long_description(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_long_description)
        return this->m_jackctl_parameter_get_long_description(parameter);

    return NULL;
}

jackctl_param_type_t JackServer::jackctl_parameter_get_type(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_type)
        return this->m_jackctl_parameter_get_type(parameter);

    return jackctl_param_type_t(0);
}

bool JackServer::jackctl_parameter_is_set(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_is_set)
        return this->m_jackctl_parameter_is_set(parameter);

    return false;
}

bool JackServer::jackctl_parameter_reset(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_reset)
        return this->m_jackctl_parameter_reset(parameter);

    return false;
}

jackctl_parameter_value_t JackServer::jackctl_parameter_get_value(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_value)
        return this->m_jackctl_parameter_get_value(parameter);

    return {};
}

bool JackServer::jackctl_parameter_set_value(jackctl_parameter_t *parameter,
                                             const jackctl_parameter_value_t *value_ptr) const
{
    if (this->m_jackctl_parameter_set_value)
        return this->m_jackctl_parameter_set_value(parameter, value_ptr);

    return false;
}

jackctl_parameter_value_t JackServer::jackctl_parameter_get_default_value(jackctl_parameter_t *parameter) const
{
    if (this->m_jackctl_parameter_get_default_value)
        return this->m_jackctl_parameter_get_default_value(parameter);

    return {};
}
