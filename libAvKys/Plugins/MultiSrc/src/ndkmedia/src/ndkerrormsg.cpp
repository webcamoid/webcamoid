/* Webcamoid, webcam capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include <QMap>

#include "ndkerrormsg.h"

QString mediaStatusToStr(media_status_t status, const QString &defaultValue)
{
    static const QMap<media_status_t, QString> mediaStatusToStr {
        {AMEDIA_OK                              , "AMEDIA_OK"                              },
        {AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE, "AMEDIACODEC_ERROR_INSUFFICIENT_RESOURCE"},
        {AMEDIACODEC_ERROR_RECLAIMED            , "AMEDIACODEC_ERROR_RECLAIMED"            },
        {AMEDIA_ERROR_BASE                      , "AMEDIA_ERROR_BASE"                      },
        {AMEDIA_ERROR_UNKNOWN                   , "AMEDIA_ERROR_UNKNOWN"                   },
        {AMEDIA_ERROR_MALFORMED                 , "AMEDIA_ERROR_MALFORMED"                 },
        {AMEDIA_ERROR_UNSUPPORTED               , "AMEDIA_ERROR_UNSUPPORTED"               },
        {AMEDIA_ERROR_INVALID_OBJECT            , "AMEDIA_ERROR_INVALID_OBJECT"            },
        {AMEDIA_ERROR_INVALID_PARAMETER         , "AMEDIA_ERROR_INVALID_PARAMETER"         },
        {AMEDIA_ERROR_INVALID_OPERATION         , "AMEDIA_ERROR_INVALID_OPERATION"         },
        {AMEDIA_ERROR_END_OF_STREAM             , "AMEDIA_ERROR_END_OF_STREAM"             },
        {AMEDIA_ERROR_IO                        , "AMEDIA_ERROR_IO"                        },
        {AMEDIA_ERROR_WOULD_BLOCK               , "AMEDIA_ERROR_WOULD_BLOCK"               },
        {AMEDIA_DRM_ERROR_BASE                  , "AMEDIA_DRM_ERROR_BASE"                  },
        {AMEDIA_DRM_NOT_PROVISIONED             , "AMEDIA_DRM_NOT_PROVISIONED"             },
        {AMEDIA_DRM_RESOURCE_BUSY               , "AMEDIA_DRM_RESOURCE_BUSY"               },
        {AMEDIA_DRM_DEVICE_REVOKED              , "AMEDIA_DRM_DEVICE_REVOKED"              },
        {AMEDIA_DRM_SHORT_BUFFER                , "AMEDIA_DRM_SHORT_BUFFER"                },
        {AMEDIA_DRM_SESSION_NOT_OPENED          , "AMEDIA_DRM_SESSION_NOT_OPENED"          },
        {AMEDIA_DRM_TAMPER_DETECTED             , "AMEDIA_DRM_TAMPER_DETECTED"             },
        {AMEDIA_DRM_VERIFY_FAILED               , "AMEDIA_DRM_VERIFY_FAILED"               },
        {AMEDIA_DRM_NEED_KEY                    , "AMEDIA_DRM_NEED_KEY"                    },
        {AMEDIA_DRM_LICENSE_EXPIRED             , "AMEDIA_DRM_LICENSE_EXPIRED"             },
        {AMEDIA_IMGREADER_ERROR_BASE            , "AMEDIA_IMGREADER_ERROR_BASE"            },
        {AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE   , "AMEDIA_IMGREADER_NO_BUFFER_AVAILABLE"   },
        {AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED   , "AMEDIA_IMGREADER_MAX_IMAGES_ACQUIRED"   },
        {AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE     , "AMEDIA_IMGREADER_CANNOT_LOCK_IMAGE"     },
        {AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE   , "AMEDIA_IMGREADER_CANNOT_UNLOCK_IMAGE"   },
        {AMEDIA_IMGREADER_IMAGE_NOT_LOCKED      , "AMEDIA_IMGREADER_IMAGE_NOT_LOCKED"      },
    };

    return mediaStatusToStr.value(status, defaultValue);
}
