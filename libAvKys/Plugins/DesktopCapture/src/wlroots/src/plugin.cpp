/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QGuiApplication>
#include <wayland-client.h>

#include "plugin.h"
#include "wlrootsdev.h"
#include "wlr-screencopy-unstable-v1-client-protocol.h"

class PluginPrivate
{
    public:
        static void probeRegistryGlobal(void *userData,
                                        wl_registry *registry,
                                        uint32_t id,
                                        const char *interface,
                                        uint32_t version);
        static void probeRegistryGlobalRemove(void *data,
                                              wl_registry *registry,
                                              uint32_t id);
};

bool Plugin::canLoad()
{
    if (qApp->platformName() != "wayland")
        return false;

    auto display = wl_display_connect(nullptr);

    if (!display)
        return false;

    bool hasScreencopy = false;
    auto registry = wl_display_get_registry(display);
    const wl_registry_listener listener {
        PluginPrivate::probeRegistryGlobal,
        PluginPrivate::probeRegistryGlobalRemove,
    };
    wl_registry_add_listener(registry, &listener, &hasScreencopy);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return hasScreencopy;
}

QObject *Plugin::create()
{
    return new WlrootsDev();
}

void PluginPrivate::probeRegistryGlobal(void *userData,
                                        wl_registry *registry,
                                        uint32_t id,
                                        const char *interface,
                                        uint32_t version)
{
    Q_UNUSED(registry)
    Q_UNUSED(id)
    Q_UNUSED(version)

    if (strcmp(interface, zwlr_screencopy_manager_v1_interface.name) == 0)
        *reinterpret_cast<bool *>(userData) = true;
}

void PluginPrivate::probeRegistryGlobalRemove(void *data,
                                              wl_registry *registry,
                                              uint32_t id)
{
    Q_UNUSED(data)
    Q_UNUSED(registry)
    Q_UNUSED(id)
}

#include "moc_plugin.cpp"
