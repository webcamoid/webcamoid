/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef IOCTLDEFS_H
#define IOCTLDEFS_H

#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_V4LUTILS
#include <libv4l2.h>

#define x_ioctl v4l2_ioctl
#define x_open v4l2_open
#define x_close v4l2_close
#define x_read v4l2_read
#define x_mmap v4l2_mmap
#define x_munmap v4l2_munmap
#else
#include <unistd.h>
#include <sys/ioctl.h>

#define x_ioctl ioctl
#define x_open open
#define x_close close
#define x_read read
#define x_mmap mmap
#define x_munmap munmap
#endif

#endif // IOCTLDEFS_H
