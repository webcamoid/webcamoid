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

#ifndef ENUMMEDIATYPES_H
#define ENUMMEDIATYPES_H

#include <vector>
#include <strmif.h>

#include "cunknown.h"

namespace AkVCam
{
    class EnumMediaTypesPrivate;
    class VideoFormat;

    class EnumMediaTypes:
            public IEnumMediaTypes,
            public CUnknown
    {
        public:
            EnumMediaTypes(const std::vector<VideoFormat> &formats={});
            EnumMediaTypes(const EnumMediaTypes &other);
            EnumMediaTypes &operator =(const EnumMediaTypes &other);
            virtual ~EnumMediaTypes();

            std::vector<VideoFormat> &formats();
            std::vector<VideoFormat> formats() const;
            void setFormats(const std::vector<VideoFormat> &formats,
                            bool changed=true);

            DECLARE_IUNKNOWN(IID_IEnumMediaTypes)

            // IEnumMediaTypes
            HRESULT STDMETHODCALLTYPE Next(ULONG cMediaTypes,
                                           AM_MEDIA_TYPE **ppMediaTypes,
                                           ULONG *pcFetched);
            HRESULT STDMETHODCALLTYPE Skip(ULONG cMediaTypes);
            HRESULT STDMETHODCALLTYPE Reset();
            HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes **ppEnum);

        private:
            EnumMediaTypesPrivate *d;
    };
}

#endif // ENUMMEDIATYPES_H
