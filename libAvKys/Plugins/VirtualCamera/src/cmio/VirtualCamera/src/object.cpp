/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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

#include "object.h"
#include "utils.h"

AkVCam::Object::Object(CMIOHardwarePlugInRef pluginInterface,
                       Object *parent):
    ObjectInterface(),
    m_pluginInterface(pluginInterface),
    m_parent(parent),
    m_isCreated(false)
{
    this->m_className = "Object";
    this->m_properties.setProperty(kCMIOObjectPropertyOwnedObjects,
                                   std::vector<Object *>());
    this->m_properties.setProperty(kCMIOObjectPropertyListenerAdded,
                                   this->address());
    this->m_properties.setProperty(kCMIOObjectPropertyListenerRemoved,
                                   this->address());

    if (this->m_parent) {
        this->m_parent->m_childs.push_back(this);
        this->m_parent->childsUpdate();
    }
}

AkVCam::Object::Object(Object *parent):
    ObjectInterface(),
    m_pluginInterface(parent? parent->m_pluginInterface: nullptr),
    m_parent(parent),
    m_isCreated(false)
{
    this->m_className = "Object";
    this->m_properties.setProperty(kCMIOObjectPropertyOwnedObjects,
                                   std::vector<Object *>());
    this->m_properties.setProperty(kCMIOObjectPropertyListenerAdded,
                                   this->address());
    this->m_properties.setProperty(kCMIOObjectPropertyListenerRemoved,
                                   this->address());

    if (this->m_parent) {
        this->m_parent->m_childs.push_back(this);
        this->m_parent->childsUpdate();
    }
}

AkVCam::Object::~Object()
{
    if (this->m_parent) {
        this->m_parent->m_childs.remove(this);
        this->m_parent->childsUpdate();
    }
}

CMIOObjectID AkVCam::Object::objectID() const
{
    return this->m_objectID;
}

UInt32 AkVCam::Object::classID() const
{
    return this->m_classID;
}

OSStatus AkVCam::Object::createObject()
{
    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Object::registerObject(bool regist)
{
    UNUSED(regist)

    return kCMIOHardwareNoError;
}

AkVCam::Object *AkVCam::Object::findObject(CMIOObjectID objectID)
{
    if (this->m_objectID == objectID)
        return this;

    for (auto child: this->m_childs)
        if (auto object = child->findObject(objectID))
            return object;

    return nullptr;
}

OSStatus AkVCam::Object::propertyChanged(UInt32 numberAddresses,
                                         const CMIOObjectPropertyAddress *addresses)
{
    return CMIOObjectPropertiesChanged(this->m_pluginInterface,
                                       this->m_objectID,
                                       numberAddresses,
                                       addresses);
}

OSStatus AkVCam::Object::setPropertyData(const CMIOObjectPropertyAddress *address,
                                         UInt32 qualifierDataSize,
                                         const void *qualifierData,
                                         UInt32 dataSize,
                                         const void *data)
{
    auto status =
            ObjectInterface::setPropertyData(address,
                                             qualifierDataSize,
                                             qualifierData,
                                             dataSize,
                                             data);

    if (status == kCMIOHardwareUnspecifiedError)
        status = this->propertyChanged(1, address);

    return status;
}

void AkVCam::Object::childsUpdate()
{
    std::vector<Object *> objects;

    for (auto &object: this->m_childs)
        objects.push_back(object);

    this->m_properties.setProperty(kCMIOObjectPropertyOwnedObjects, objects);
}
