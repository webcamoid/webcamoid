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

#include <map>

#include "object.h"

namespace AkVCam
{
    enum PropertyType
    {
        PropertyTypeUInt32,
        PropertyTypeFloat64,
        PropertyTypePidT,
        PropertyTypeString,
        PropertyTypeObjectVector,
        PropertyTypeObjectPtrVector,
        PropertyTypeVideoFormat,
        PropertyTypeVideoFormatVector,
        PropertyTypeFloat64Vector,
        PropertyTypeAudioValueRangeVector,
        PropertyTypeClock,
        PropertyTypeAddress
    };

    struct PropertyValue
    {
        PropertyType type;
        bool isSettable;

        union
        {
            UInt32 uint32;
            Float64 float64;
            pid_t pidT;
        } num;

        std::string str;
        std::vector<Object *> objects;
        std::vector<ObjectPtr> objectsPtr;
        std::vector<VideoFormat> videoFormats;
        std::vector<Float64> float64Vector;
        std::vector<AudioValueRange> audioValueRangeVector;
        VideoFormat videoFormat;
        ClockPtr clock;
        CMIOObjectPropertyAddress address;
    };

    class ObjectPropertiesPrivate
    {
        public:
            std::map<UInt32, PropertyValue> m_properties;
    };
}

AkVCam::ObjectProperties::ObjectProperties()
{
    this->d = new ObjectPropertiesPrivate();
}

AkVCam::ObjectProperties::ObjectProperties(const ObjectProperties &other)
{
    this->d = new ObjectPropertiesPrivate();
    this->d->m_properties = other.d->m_properties;
}

AkVCam::ObjectProperties &AkVCam::ObjectProperties::operator =(const ObjectProperties &other)
{
    if (this != &other)
        this->d->m_properties = other.d->m_properties;

    return *this;
}

AkVCam::ObjectProperties::~ObjectProperties()
{
    delete this->d;
}

std::vector<UInt32> AkVCam::ObjectProperties::properties() const
{
    std::vector<UInt32> properties;

    for (auto &property: this->d->m_properties)
        properties.push_back(property.first);

    return properties;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::string &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeString;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].str = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           UInt32 value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeUInt32;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].num.uint32 = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           Float64 value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeFloat64;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].num.float64 = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           pid_t value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypePidT;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].num.pidT = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<Object *> &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeObjectVector;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].objects = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<ObjectPtr> &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeObjectPtrVector;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].objectsPtr = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const VideoFormat &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeVideoFormat;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].videoFormat = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<VideoFormat> &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeVideoFormatVector;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].videoFormats = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<Float64> &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeFloat64Vector;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].float64Vector = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<AudioValueRange> &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeAudioValueRangeVector;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].audioValueRangeVector = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const std::vector<std::pair<double, double> > &value,
                                           bool isSettable)
{
    std::vector<AudioValueRange> valueRanges;

    for (auto &range: value)
        valueRanges.push_back({range.first, range.second});

    return this->setProperty(property, valueRanges, isSettable);
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const ClockPtr &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeClock;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].clock = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           const CMIOObjectPropertyAddress &value,
                                           bool isSettable)
{
    this->d->m_properties[property].type = PropertyTypeAddress;
    this->d->m_properties[property].isSettable = isSettable;
    this->d->m_properties[property].address = value;

    return true;
}

bool AkVCam::ObjectProperties::setProperty(UInt32 property,
                                           UInt32 dataSize,
                                           const void *data)
{
    if (!this->d->m_properties.count(property))
        return false;

    bool isSettable = this->d->m_properties[property].isSettable;

    if (!isSettable)
        return false;

    auto propertyType = this->d->m_properties[property].type;
    bool ok = true;

    switch (propertyType) {
        case PropertyTypeAddress:
            if (dataSize == sizeof(CMIOObjectPropertyAddress)) {
                this->d->m_properties[property].type = PropertyTypeAddress;
                this->d->m_properties[property].isSettable = isSettable;
                this->d->m_properties[property].address =
                    *static_cast<const CMIOObjectPropertyAddress *>(data);
            } else {
                ok = false;
            }

            break;

        case PropertyTypeVideoFormat:
            if (dataSize == sizeof(CMFormatDescriptionRef)) {
                this->d->m_properties[property].type = PropertyTypeVideoFormat;
                this->d->m_properties[property].isSettable = isSettable;
                auto videoDescription =
                        *static_cast<const CMFormatDescriptionRef *>(data);
                auto mediaType = CMFormatDescriptionGetMediaType(videoDescription);
                auto dimensions = CMVideoFormatDescriptionGetDimensions(videoDescription);
                this->d->m_properties[property].videoFormat =
                        VideoFormat(formatFromCM(mediaType),
                                    dimensions.width,
                                    dimensions.height);
            } else {
                ok = false;
            }

            break;

        case PropertyTypeUInt32:
            if (dataSize == sizeof(UInt32)) {
                this->d->m_properties[property].type = PropertyTypeUInt32;
                this->d->m_properties[property].isSettable = isSettable;
                this->d->m_properties[property].num.uint32 =
                    *static_cast<const UInt32 *>(data);
            } else {
                ok = false;
            }

            break;

        case PropertyTypeFloat64:
            if (dataSize == sizeof(Float64)) {
                this->d->m_properties[property].type = PropertyTypeFloat64;
                this->d->m_properties[property].isSettable = isSettable;
                this->d->m_properties[property].num.float64 =
                    *static_cast<const Float64 *>(data);
            } else {
                ok = false;
            }

            break;

        case PropertyTypePidT:
            if (dataSize == sizeof(pid_t)) {
                this->d->m_properties[property].type = PropertyTypePidT;
                this->d->m_properties[property].isSettable = isSettable;
                this->d->m_properties[property].num.pidT =
                    *static_cast<const pid_t *>(data);
            } else {
                ok = false;
            }

            break;

        default:
            return false;
    }

    return ok;
}

bool AkVCam::ObjectProperties::getProperty(UInt32 property, UInt32 *value)
{
    if (!value || !this->d->m_properties.count(property))
        return false;

    auto propertyType = this->d->m_properties[property].type;

    if (propertyType != PropertyTypeUInt32)
        return false;

    *value = this->d->m_properties[property].num.uint32;

    return true;
}

bool AkVCam::ObjectProperties::getProperty(UInt32 property, std::string *value)
{
    if (!value || !this->d->m_properties.count(property))
        return false;

    auto propertyType = this->d->m_properties[property].type;

    if (propertyType != PropertyTypeString)
        return false;

    *value = this->d->m_properties[property].str;

    return true;
}

bool AkVCam::ObjectProperties::getProperty(UInt32 property,
                                           UInt32 qualifierDataSize,
                                           const void *qualifierData,
                                           UInt32 dataSize,
                                           UInt32 *dataUsed,
                                           void *data)
{
    if (!this->d->m_properties.count(property))
        return false;

    bool ok = true;
    auto propertyType = this->d->m_properties[property].type;

    switch (propertyType) {
        case PropertyTypeString:
            if (dataUsed) {
                *dataUsed = sizeof(CFStringRef);

                if (data)
                    ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto value = this->d->m_properties[property].str;
                *static_cast<CFStringRef *>(data) =
                        CFStringCreateWithCString(kCFAllocatorDefault,
                                                  value.c_str(),
                                                  kCFStringEncodingUTF8);
            }

            break;

        case PropertyTypeObjectVector: {
            auto &objects = this->d->m_properties[property].objects;
            auto objectList = static_cast<CMIOObjectID *>(data);
            size_t i = 0;

            for (auto &object: objects) {
                if (qualify(property,
                            qualifierDataSize,
                            qualifierData,
                            object)) {
                    if (data)
                        objectList[i] = object->objectID();

                    i++;
                }
            }

            if (dataUsed)
                *dataUsed = UInt32(i * sizeof(CMIOObjectID));

            break;
        }

        case PropertyTypeObjectPtrVector: {
            auto &objects = this->d->m_properties[property].objectsPtr;
            auto objectList = static_cast<CMIOObjectID *>(data);
            size_t i = 0;

            for (auto &object: objects) {
                if (qualify(property,
                            qualifierDataSize,
                            qualifierData,
                            &object)) {
                    if (data)
                        objectList[i] = object->objectID();

                    i++;
                }
            }

            if (dataUsed)
                *dataUsed = UInt32(i * sizeof(CMIOObjectID));

            break;
        }

        case PropertyTypeVideoFormat: {
            if (dataUsed) {
                *dataUsed = sizeof(CMFormatDescriptionRef);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto videoFormat = this->d->m_properties[property].videoFormat;
                auto status =
                        CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
                                                       formatToCM(videoFormat.fourcc()),
                                                       videoFormat.width(),
                                                       videoFormat.height(),
                                                       nullptr,
                                                       static_cast<CMFormatDescriptionRef *>(data));

                 if (status != noErr)
                     ok = false;
            }

            break;
        }

        case PropertyTypeVideoFormatVector: {
            if (dataUsed) {
                *dataUsed = sizeof(CFArrayRef);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto videoFormats = this->d->m_properties[property].videoFormats;
                std::vector<CMFormatDescriptionRef> formats;

                for (auto &format: videoFormats) {
                    CMFormatDescriptionRef formatRef = nullptr;
                    auto status =
                            CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
                                                           formatToCM(format.fourcc()),
                                                           format.width(),
                                                           format.height(),
                                                           nullptr,
                                                           &formatRef);

                    if (status == noErr)
                        formats.push_back(formatRef);
                }

                CFArrayRef array = nullptr;

                if (!formats.empty())
                    array = CFArrayCreate(kCFAllocatorDefault,
                                          reinterpret_cast<const void **>(formats.data()),
                                          UInt32(formats.size()),
                                          nullptr);

                *static_cast<CFArrayRef *>(data) = array;
            }

            break;
        }

        case PropertyTypeFloat64Vector: {
            auto &values = this->d->m_properties[property].float64Vector;
            auto valueList = static_cast<Float64 *>(data);
            size_t i = 0;

            for (auto &value: values) {
                if (qualify(property,
                            qualifierDataSize,
                            qualifierData,
                            &value)) {
                    if (data)
                        valueList[i] = value;

                    i++;
                }
            }

            if (dataUsed)
                *dataUsed = UInt32(i * sizeof(Float64));

            break;
        }

        case PropertyTypeAudioValueRangeVector: {
            auto &values = this->d->m_properties[property].audioValueRangeVector;
            auto valueList = static_cast<AudioValueRange *>(data);
            size_t i = 0;

            for (auto &value: values) {
                if (qualify(property,
                            qualifierDataSize,
                            qualifierData,
                            &value)) {
                    if (data)
                        valueList[i] = value;

                    i++;
                }
            }

            if (dataUsed)
                *dataUsed = UInt32(i * sizeof(AudioValueRange));

            break;
        }

        case PropertyTypeClock:
            if (dataUsed) {
                *dataUsed = sizeof(CFTypeRef);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto value = this->d->m_properties[property].clock;
                *static_cast<CFTypeRef *>(data) = value->ref();
                CFRetain(value->ref());
            }

            break;

        case PropertyTypeUInt32:
            if (dataUsed) {
                *dataUsed = sizeof(UInt32);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto value = this->d->m_properties[property].num.uint32;
                *static_cast<UInt32 *>(data) = value;
            }

            break;

        case PropertyTypeFloat64:
            if (dataUsed) {
                *dataUsed = sizeof(Float64);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto value = this->d->m_properties[property].num.float64;
                *static_cast<Float64 *>(data) = value;
            }

            break;

        case PropertyTypePidT:
            if (dataUsed) {
                *dataUsed = sizeof(pid_t);
                ok = dataSize == *dataUsed;
            }

            if (ok && data) {
                auto value = this->d->m_properties[property].num.pidT;
                *static_cast<pid_t *>(data) = value;
            }

            break;

        default:
            return false;
    }

    return ok;
}

void AkVCam::ObjectProperties::removeProperty(UInt32 property)
{
    this->d->m_properties.erase(property);
}

void AkVCam::ObjectProperties::update(const ObjectProperties &other)
{
    for (auto &property: other.d->m_properties)
        this->d->m_properties[property.first] = property.second;
}

bool AkVCam::ObjectProperties::isSettable(UInt32 property)
{
    if (this->d->m_properties.count(property))
        return this->d->m_properties[property].isSettable;

    return true;
}

bool AkVCam::ObjectProperties::qualify(UInt32 property,
                                       UInt32 qualifierDataSize,
                                       const void *qualifierData,
                                       const void *data)
{
    if (qualifierDataSize && qualifierData && data)
        switch (property) {
            case kCMIOObjectPropertyOwnedObjects: {
                auto object = static_cast<const Object *>(data);
                auto qualifier = static_cast<const UInt32 *>(qualifierData);

                for (UInt32 i = 0; i < qualifierDataSize; i++)
                    if (qualifier[i] == object->classID())
                        return true;

                return false;
            }

            case kCMIOStreamPropertyFrameRates:
            case kCMIOStreamPropertyFrameRateRanges:
                // Not implemented.
                break;

            default:
                break;
        }

    return true;
}
