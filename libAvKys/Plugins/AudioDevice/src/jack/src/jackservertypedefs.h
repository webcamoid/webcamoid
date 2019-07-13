/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef JACKSERVERTYPEDEFS_H
#define JACKSERVERTYPEDEFS_H

#include <cstdint>

#define JACK_PARAM_STRING_MAX  127

struct JSList
{
  void *data;
  JSList *next;
};

typedef union jackctl_parameter_value
{
    uint32_t ui;
    int32_t i;
    char c;
    char str[JACK_PARAM_STRING_MAX + 1];
    bool b;
} jackctl_parameter_value_t;

typedef enum
{
    JackParamInt = 1,
    JackParamUInt,
    JackParamChar,
    JackParamString,
    JackParamBool
} jackctl_param_type_t;

using jackctl_server_t = struct jackctl_server;
using jackctl_driver_t = struct jackctl_driver;
using jackctl_parameter_t = struct jackctl_parameter;

#endif // JACKSERVERTYPEDEFS_H
