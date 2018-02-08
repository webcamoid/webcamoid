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

#include "objectinterface.h"
#include "utils.h"

AkVCam::ObjectInterface::ObjectInterface():
    m_objectID(0),
    m_classID(0)
{

}

AkVCam::ObjectInterface::~ObjectInterface()
{

}

AkVCam::ObjectProperties AkVCam::ObjectInterface::properties() const
{
    return this->m_properties;
}

AkVCam::ObjectProperties &AkVCam::ObjectInterface::properties()
{
    return this->m_properties;
}

void AkVCam::ObjectInterface::setProperties(const ObjectProperties &properties)
{
    this->m_properties = properties;
}

void AkVCam::ObjectInterface::updateProperties(const ObjectProperties &properties)
{
    this->m_properties.update(properties);
}

CMIOObjectPropertyAddress AkVCam::ObjectInterface::address(CMIOObjectPropertySelector selector,
                                                           CMIOObjectPropertyScope scope,
                                                           CMIOObjectPropertyElement element)
{
    return CMIOObjectPropertyAddress {selector, scope, element};
}

void AkVCam::ObjectInterface::show()
{
    AkObjectLogMethod();

    AkLoggerLog("STUB");
}

Boolean AkVCam::ObjectInterface::hasProperty(const CMIOObjectPropertyAddress *address)
{
    AkObjectLogMethod();

    if (!this->m_properties.getProperty(address->mSelector)) {
        AkLoggerLog("Unknown property ", enumToString(address->mSelector));

        return false;
    }

    AkLoggerLog("Found property ", enumToString(address->mSelector));

    return true;
}

OSStatus AkVCam::ObjectInterface::isPropertySettable(const CMIOObjectPropertyAddress *address,
                                                     Boolean *isSettable)
{
    AkObjectLogMethod();

    if (!this->m_properties.getProperty(address->mSelector)) {
        AkLoggerLog("Unknown property ", enumToString(address->mSelector));

        return kCMIOHardwareUnknownPropertyError;
    }

    bool settable = this->m_properties.isSettable(address->mSelector);

    if (isSettable)
        *isSettable = settable;

    AkLoggerLog("Is property ",
                enumToString(address->mSelector),
                " settable? ",
                (settable? "YES": "NO"));

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::getPropertyDataSize(const CMIOObjectPropertyAddress *address,
                                                      UInt32 qualifierDataSize,
                                                      const void *qualifierData,
                                                      UInt32 *dataSize)
{
    AkObjectLogMethod();
    AkLoggerLog("Getting property size ", enumToString(address->mSelector));

    if (!this->m_properties.getProperty(address->mSelector,
                                        qualifierDataSize,
                                        qualifierData,
                                        0,
                                        dataSize)) {
        AkLoggerLog("Unknown property ", enumToString(address->mSelector));

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::getPropertyData(const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  UInt32 *dataUsed,
                                                  void *data)
{
    AkObjectLogMethod();
    AkLoggerLog("Getting property ", enumToString(address->mSelector));

    if (!this->m_properties.getProperty(address->mSelector,
                                        qualifierDataSize,
                                        qualifierData,
                                        dataSize,
                                        dataUsed,
                                        data)) {
        AkLoggerLog("Unknown property ", enumToString(address->mSelector));

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::ObjectInterface::setPropertyData(const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  const void *data)
{
    AkObjectLogMethod();
    AkLoggerLog("Setting property ", enumToString(address->mSelector));
    UNUSED(qualifierDataSize)
    UNUSED(qualifierData)

    if (!this->m_properties.setProperty(address->mSelector,
                                        dataSize,
                                        data)) {
        AkLoggerLog("Unknown property ", enumToString(address->mSelector));

        return kCMIOHardwareUnknownPropertyError;
    }

    return kCMIOHardwareNoError;
}
