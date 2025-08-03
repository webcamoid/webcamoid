/* Webcamoid, camera capture application.
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

#ifndef AKCOMMONS_H
#define AKCOMMONS_H

#include <QtGlobal>

#if defined(AKCOMMONS_LIBRARY)
#  define AKCOMMONS_EXPORT Q_DECL_EXPORT
#else
#  define AKCOMMONS_EXPORT Q_DECL_IMPORT
#endif

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    #define AK_MAKE_FOURCC_LE(a, b, c, d) \
          (((quint32(a) & 0xff)) \
         | ((quint32(b) & 0xff) <<  8) \
         | ((quint32(c) & 0xff) << 16) \
         |  (quint32(d) & 0xff) << 24)

    #define AK_MAKE_FOURCC_BE(a, b, c, d) \
        (((quint32(a) & 0xff) << 24) \
         | ((quint32(b) & 0xff) << 16) \
         | ((quint32(c) & 0xff) <<  8) \
         |  (quint32(d) & 0xff))
    #define AK_MAKE_FOURCC(a, b, c, d) AK_MAKE_FOURCC_LE(a, b, c, d)
#else
    #define AK_MAKE_FOURCC_LE(a, b, c, d) \
          (((quint32(a) & 0xff) << 24) \
         | ((quint32(b) & 0xff) << 16) \
         | ((quint32(c) & 0xff) <<  8) \
         |  (quint32(d) & 0xff))
    #define AK_MAKE_FOURCC_BE(a, b, c, d) \
          (((quint32(a) & 0xff)) \
         | ((quint32(b) & 0xff) <<  8) \
         | ((quint32(c) & 0xff) << 16) \
         |  (quint32(d) & 0xff) << 24)
    #define AK_MAKE_FOURCC(a, b, c, d) AK_MAKE_FOURCC_BE(a, b, c, d)
#endif

#endif // AKCOMMONS_H
