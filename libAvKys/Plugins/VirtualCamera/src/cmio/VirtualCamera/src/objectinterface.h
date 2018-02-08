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

#ifndef OBJECTINTERFACE_H
#define OBJECTINTERFACE_H

#include <string>
#include <CoreMediaIO/CMIOHardwarePlugIn.h>

#include "objectproperties.h"
#include "VCamUtils/src/utils.h"

#define AkObjectLogMethod() \
    AkLoggerLog(this->m_className, \
                "(", \
                this->m_objectID, \
                ")::", \
                __FUNCTION__, \
                "()")

namespace AkVCam
{
    class ObjectInterface
    {
        public:
            ObjectInterface();
            virtual ~ObjectInterface();

            ObjectProperties properties() const;
            ObjectProperties &properties();
            void setProperties(const ObjectProperties &properties);
            void updateProperties(const ObjectProperties &properties);
            static CMIOObjectPropertyAddress address(CMIOObjectPropertySelector selector=0,
                                                     CMIOObjectPropertyScope scope=kCMIOObjectPropertyScopeGlobal,
                                                     CMIOObjectPropertyElement element=kCMIOObjectPropertyElementMaster);

            virtual void show();
            virtual Boolean hasProperty(const CMIOObjectPropertyAddress *address);
            virtual OSStatus isPropertySettable(const CMIOObjectPropertyAddress *address,
                                                Boolean *isSettable);
            virtual OSStatus getPropertyDataSize(const CMIOObjectPropertyAddress *address,
                                                 UInt32 qualifierDataSize,
                                                 const void *qualifierData,
                                                 UInt32 *dataSize);
            virtual OSStatus getPropertyData(const CMIOObjectPropertyAddress *address,
                                             UInt32 qualifierDataSize,
                                             const void *qualifierData,
                                             UInt32 dataSize,
                                             UInt32 *dataUsed,
                                             void *data);
            virtual OSStatus setPropertyData(const CMIOObjectPropertyAddress *address,
                                             UInt32 qualifierDataSize,
                                             const void *qualifierData,
                                             UInt32 dataSize,
                                             const void *data);

        protected:
            CMIOObjectID m_objectID;
            std::string m_className;
            UInt32 m_classID;
            ObjectProperties m_properties;
    };
}

#endif // OBJECTINTERFACE_H
