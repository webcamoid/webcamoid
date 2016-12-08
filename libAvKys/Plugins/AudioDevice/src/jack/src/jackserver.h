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

#ifndef JACKSERVER_H
#define JACKSERVER_H

#include <QVariant>
#include <QLibrary>

#include "jackservertypedefs.h"

#define DECL_FUNC(ret, func, ...) \
    private: \
        typedef ret func##_t(__VA_ARGS__); \
        ret func(__VA_ARGS__) const; \
        func##_t *m_##func;

#define LOAD_FUNC(library, func) \
    m_##func = reinterpret_cast<func##_t *>(library.resolve(#func))

typedef bool (*on_device_acquire_t)(const char *device_name);
typedef void (*on_device_release_t)(const char *device_name);

class JackServer: public QObject
{
    Q_OBJECT

    public:
        explicit JackServer(on_device_acquire_t onDeviceAcquire=NULL,
                            on_device_release_t onDeviceRelease=NULL,
                            QObject *parent=NULL);
        ~JackServer();

        Q_INVOKABLE bool start(jackctl_driver_t *driver);
        Q_INVOKABLE bool start(const QString &driver);
        Q_INVOKABLE bool stop();
        Q_INVOKABLE QList<jackctl_driver_t *> drivers() const;
        Q_INVOKABLE QStringList driversByName() const;
        Q_INVOKABLE jackctl_driver_t *driverByName(const QString &name) const;
        Q_INVOKABLE QString name(jackctl_driver_t *driver) const;
        Q_INVOKABLE bool switchMaster(jackctl_driver_t *driver);
        Q_INVOKABLE QList<jackctl_parameter_t *> parameters() const;
        Q_INVOKABLE QList<jackctl_parameter_t *> parameters(jackctl_driver_t *driver) const;
        Q_INVOKABLE QStringList parametersByName() const;
        Q_INVOKABLE QStringList parametersByName(jackctl_driver_t *driver) const;
        Q_INVOKABLE jackctl_parameter_t *parameterByName(const QString &name) const;
        Q_INVOKABLE jackctl_parameter_t *parameterByName(jackctl_driver_t *driver,
                                                         const QString &name) const;
        Q_INVOKABLE QString name(jackctl_parameter_t *parameter) const;
        Q_INVOKABLE QString description(jackctl_parameter_t *parameter) const;
        Q_INVOKABLE QString longDescription(jackctl_parameter_t *parameter) const;
        Q_INVOKABLE bool isSet(jackctl_parameter_t *parameter) const;
        Q_INVOKABLE QVariant value(jackctl_parameter_t *parameter,
                                   bool defaultValue=false) const;
        Q_INVOKABLE bool setValue(jackctl_parameter_t *parameter,
                                  const QVariant &value);
        Q_INVOKABLE bool resetValue(jackctl_parameter_t *parameter);

    private:
        QLibrary m_library;
        jackctl_server_t *m_server;

        DECL_FUNC(jackctl_server_t *,
                  jackctl_server_create,
                  on_device_acquire_t on_device_acquire,
                  on_device_release_t on_device_release)
        DECL_FUNC(void,
                  jackctl_server_destroy,
                  jackctl_server_t *server)
        DECL_FUNC(bool,
                  jackctl_server_start,
                  jackctl_server_t *server,
                  jackctl_driver_t *driver)
        DECL_FUNC(bool,
                  jackctl_server_stop,
                  jackctl_server_t *server)
        DECL_FUNC(const JSList *,
                  jackctl_server_get_drivers_list,
                  jackctl_server_t *server)
        DECL_FUNC(const JSList *,
                  jackctl_server_get_parameters,
                  jackctl_server_t *server)
        DECL_FUNC(bool,
                  jackctl_server_switch_master,
                  jackctl_server_t *server,
                  jackctl_driver_t *driver)
        DECL_FUNC(const char *,
                  jackctl_driver_get_name,
                  jackctl_driver_t *driver)
        DECL_FUNC(const JSList *,
                  jackctl_driver_get_parameters,
                  jackctl_driver_t *driver)
        DECL_FUNC(const char *,
                  jackctl_parameter_get_name,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(const char *,
                  jackctl_parameter_get_short_description,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(const char *,
                  jackctl_parameter_get_long_description,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(jackctl_param_type_t,
                  jackctl_parameter_get_type,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(bool,
                  jackctl_parameter_is_set,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(bool,
                  jackctl_parameter_reset,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(jackctl_parameter_value_t,
                  jackctl_parameter_get_value,
                  jackctl_parameter_t *parameter)
        DECL_FUNC(bool,
                  jackctl_parameter_set_value,
                  jackctl_parameter_t *parameter,
                  const jackctl_parameter_value_t *value_ptr)
        DECL_FUNC(jackctl_parameter_value_t,
                  jackctl_parameter_get_default_value,
                  jackctl_parameter_t *parameter)
};

#endif // JACKSERVER_H
