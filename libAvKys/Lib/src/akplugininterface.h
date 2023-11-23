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

#ifndef AKPLUGININTERFACE_H
#define AKPLUGININTERFACE_H

#include <QFont>
#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRect>
#include <QSize>

#include "akaudiocaps.h"
#include "akaudiopacket.h"
#include "akcaps.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akvideocaps.h"
#include "akvideopacket.h"

#define IAK_UNKNOWN "Ak.Unknown"

class AKCOMMONS_EXPORT IAkUnknown
{
    public:
        virtual void *queryInterface(const QString &interfaceId)
        {
            if (interfaceId == IAK_UNKNOWN)
                return this;

            return nullptr;
        }

        quint64 ref()
        {
            this->m_refCount++;

            return this->m_refCount;
        }

        quint64 unref()
        {
            this->m_refCount--;

            if (this->m_refCount < 1)
                deleteThis(this);

            return this->m_refCount;
        }

    private:
        quint64 m_refCount {1};

    protected:
        virtual void deleteThis(void *userData) const = 0;
};

class IAkCallback
{
    public:
        virtual ~IAkCallback() {

        }
};

class AKCOMMONS_EXPORT IAkCallbacks
{
    public:
        inline void subscribe(IAkCallback *callbacks)
        {
            if (!this->m_callbacks.contains(callbacks))
                this->m_callbacks << callbacks;
        }

        inline void unsubscribe(IAkCallback *callbacks)
        {
            this->m_callbacks.removeAll(callbacks);
        }

    private:
        QVector<IAkCallback *> m_callbacks;

    protected:
        inline QVector<IAkCallback *> callbacks() const
        {
            return this->m_callbacks;
        }
};

enum IAkPropertyMode
{
    IAkPropertyMode_ReadOnly = 0x1,
    IAkPropertyMode_WriteOnly = 0x2,
    IAkPropertyMode_ReadWrite =
        IAkPropertyMode_ReadOnly | IAkPropertyMode_WriteOnly,
    IAkPropertyMode_Const = 0x4,
};

enum IAkPropertyProtection
{
    IAkPropertyProtection_Protected,
};

template <typename T> class IAkPropertyTypeStore;

class AKCOMMONS_EXPORT IAkProperty:
    public IAkUnknown,
    public IAkCallbacks
{
    public:
        IAkProperty()
        {

        }

        template <typename T>
        IAkProperty(const QString &description,
                    const T &value=T(),
                    IAkPropertyMode mode=IAkPropertyMode_ReadWrite):
            m_description(description),
            m_mode(mode)
        {
            this->m_data =
                QSharedPointer<IAkProperty>(new IAkPropertyTypeStore<T>(description,
                                                                    value,
                                                                    mode),
                                            [] (IAkPropertyTypeStore<T> *obj) {
                                                delete obj;
                                            });
        }

        virtual ~IAkProperty()
        {

        }

        template <typename T>
        operator T () const
        {
            return this->value<T>();
        }

        template <typename T>
        IAkProperty &operator =(const T &value)
        {
            this->setValue(value);

            return *this;
        }

        inline QString description() const
        {
            return this->m_description;
        }

        inline IAkPropertyMode mode() const
        {
            return this->m_mode;
        }

        virtual const std::type_info &typeInfo() const
        {
            if (!this->m_data)
                return typeid(void);

            return this->m_data->typeInfo();
        }

        virtual void *data()
        {
            if (!this->m_data)
                return nullptr;

            return this->m_data->data();
        }

        template <typename T>
        inline T defaultValue() const
        {
            if (!this->m_data)
                return T();

            return dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_defaultValue;
        }

        template <typename T>
        inline T value() const
        {
            if (this->typeInfo() != typeid(T))
                return T();

            return dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value;
        }

        template <typename T, IAkPropertyProtection protection>
        inline T value()
        {
            this->m_mutex.lock();
            auto value = this->value<T>();
            this->m_mutex.unlock();

            return value;
        }

        template <typename T>
        inline T toNumeric() const
        {
            if (!this->m_data)
                return T();

            T value = {T()};

            if (this->m_data->typeInfo() != typeid(T)) {
                if (this->m_data->isNumeric() && IAkProperty::isNumericType<T>()) {
                    if (this->m_data->typeInfo() == typeid(bool)) {
                        value = T(*reinterpret_cast<bool *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int8_t)) {
                        value = T(*reinterpret_cast<int8_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int16_t)) {
                        value = T(*reinterpret_cast<int16_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int32_t)) {
                        value = T(*reinterpret_cast<int32_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int64_t)) {
                        value = T(*reinterpret_cast<int64_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint8_t)) {
                        value = T(*reinterpret_cast<uint8_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint16_t)) {
                        value = T(*reinterpret_cast<uint16_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint32_t)) {
                        value = T(*reinterpret_cast<uint32_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint64_t)) {
                        value = T(*reinterpret_cast<uint64_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(double)) {
                        value = T(*reinterpret_cast<double *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(float)) {
                        value = T(*reinterpret_cast<float *>(this->m_data->data()));
                    }
                } else if (this->m_data->isString()) {
                    auto str = this->toString<T>().toLower();

                    if (str == "false") {
                        value = 0;
                    } else if (str == "true") {
                        value = 1;
                    } else {
                        bool ok = false;
                        str.toDouble(&ok);

                        if (ok) {
                            if (typeid(T) == typeid(bool)) {
                                value = T(str.toInt());
                            } else if (typeid(T) == typeid(int8_t)) {
                                value = T(str.toInt());
                            } else if (typeid(T) == typeid(int16_t)) {
                                value = T(str.toInt());
                            } else if (typeid(T) == typeid(int32_t)) {
                                value = T(str.toInt());
                            } else if (typeid(T) == typeid(int64_t)) {
                                value = T(str.toLongLong());
                            } else if (typeid(T) == typeid(uint8_t)) {
                                value = T(str.toUInt());
                            } else if (typeid(T) == typeid(uint16_t)) {
                                value = T(str.toUInt());
                            } else if (typeid(T) == typeid(uint32_t)) {
                                value = T(str.toUInt());
                            } else if (typeid(T) == typeid(uint64_t)) {
                                value = T(str.toULongLong());
                            } else if (typeid(T) == typeid(float)) {
                                value = T(str.toFloat());
                            } else if (typeid(T) == typeid(double)) {
                                value = T(str.toDouble());
                            }
                        }
                    }
                }
            } else {
                value = dynamic_cast<const IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value;
            }

            return value;
        }

        template <typename T, IAkPropertyProtection protection>
        inline T toNumeric()
        {
            this->m_mutex.lock();
            auto value = this->toNumeric<T>();
            this->m_mutex.unlock();

            return value;
        }

        template <typename T>
        inline T toPointer() const
        {
            if (!this->m_data)
                return T();

            T value = {T()};

            if (this->m_data->typeInfo() != typeid(T)
                || this->m_data->isPointer() && IAkProperty::isPointerType<T>()) {
                    value = *reinterpret_cast<T *>(this->m_data->data());
            } else {
                value = dynamic_cast<const IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value;
            }

            return value;
        }

        template <typename T, IAkPropertyProtection protection>
        inline T toPointer()
        {
            this->m_mutex.lock();
            auto value = this->toPointer<T>();
            this->m_mutex.unlock();

            return value;
        }

        template <typename T>
        inline QString toString() const
        {
            if (!this->m_data)
                return {};

            QString value;

            if (this->m_data->typeInfo() != typeid(QString)) {
                if (this->m_data->isCString())
                    value = {*reinterpret_cast<char **>(this->m_data->data())};
                else if (this->m_data->isString())
                    value = QString::fromStdString(*reinterpret_cast<std::string *>(this->m_data->data()));
                else if (this->m_data->isNumeric()) {
                    if (this->m_data->typeInfo() == typeid(bool)) {
                        value = QString("%1").arg(*reinterpret_cast<bool *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int8_t)) {
                        value = QString("%1").arg(*reinterpret_cast<int8_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int16_t)) {
                        value = QString("%1").arg(*reinterpret_cast<int16_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int32_t)) {
                        value = QString("%1").arg(*reinterpret_cast<int32_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(int64_t)) {
                        value = QString("%1").arg(*reinterpret_cast<int64_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint8_t)) {
                        value = QString("%1").arg(*reinterpret_cast<uint8_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint16_t)) {
                        value = QString("%1").arg(*reinterpret_cast<uint16_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint32_t)) {
                        value = QString("%1").arg(*reinterpret_cast<uint32_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(uint64_t)) {
                        value = QString("%1").arg(*reinterpret_cast<uint64_t *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(double)) {
                        value = QString("%1").arg(*reinterpret_cast<double *>(this->m_data->data()));
                    } else if (this->m_data->typeInfo() == typeid(float)) {
                        value = QString("%1").arg(*reinterpret_cast<float *>(this->m_data->data()));
                    }
                }
            } else {
                value = *reinterpret_cast<QString *>(this->m_data->data());
            }

            return value;
        }

        template <typename T, IAkPropertyProtection protection>
        inline QString toString()
        {
            this->m_mutex.lock();
            auto value = this->toString<T>();
            this->m_mutex.unlock();

            return value;
        }

        template <typename T>
        inline bool setValue(const T &value)
        {
            if (!(this->mode() & IAkPropertyMode_WriteOnly)) {
                qWarning() << "Can't set a read-only property";

                return false;
            }

            return this->setValueUnlocked<T>(value);
        }

        template <typename T, IAkPropertyProtection protection>
        inline bool setValue(const T &value)
        {
            if (!(this->mode() & IAkPropertyMode_WriteOnly)) {
                qWarning() << "Can't set a read-only property";

                return false;
            }

            return  this->setValueUnlocked<T, protection>(value);
        }

        template <typename T>
        inline void resetValue()
        {
            this->setValue<T>(this->defaultValue<T>());
        }

        template <typename T, IAkPropertyProtection protection>
        inline void resetValue()
        {
            this->setValue<T, protection>(this->defaultValue<T>());
        }

    private:
        QString m_description;
        IAkPropertyMode m_mode {IAkPropertyMode_ReadWrite};
        QSharedPointer<IAkProperty> m_data;
        QMutex m_mutex;

        template <typename T>
        inline static bool isNumericType()
        {
            return std::is_floating_point<T>::value
                   || std::is_integral<T>::value
                   || std::is_enum<T>::value;
        }

        template <typename T>
        inline static bool isPointerType()
        {
            return std::is_pointer<T>::value;
        }

    protected:
        template <typename T>
        inline bool setValueUnlocked(const T &value)
        {
            if (!this->m_data)
                return false;

            if (this->m_mode & IAkPropertyMode_Const) {
                qWarning() << "Can't set a constant property";

                return false;
            }

            if (this->typeInfo() != typeid(T)) {
                qWarning() << "Can't set incompatible type";

                return false;
            }

            if (dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value == value)
                return false;

            dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value = value;

            return true;
        }

        template <typename T, IAkPropertyProtection protection>
        inline bool setValueUnlocked(const T &value)
        {
            if (this->m_mode & IAkPropertyMode_Const) {
                qWarning() << "Can't set a constant property";

                return false;
            }

            if (this->typeInfo() != typeid(T)) {
                qWarning() << "Can't set incompatible type";

                return false;
            }

            this->m_mutex.lock();

            if (!this->m_data) {
                this->m_mutex.unlock();

                return false;
            }

            if (dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value == value) {
                this->m_mutex.unlock();

                return false;
            }

            dynamic_cast<IAkPropertyTypeStore<T> *>(this->m_data.get())->m_value = value;

            this->m_mutex.unlock();

            return true;
        }

        void deleteThis(void *userData) const override
        {
            delete reinterpret_cast<IAkProperty *>(userData);
        }

        virtual bool isNumeric() const
        {
            return false;
        }

        virtual bool isPointer() const
        {
            return false;
        }

        virtual bool isCString() const
        {
            return false;
        }

        virtual bool isString() const
        {
            return false;
        }
};

using IAkPropertyPtr = QSharedPointer<IAkProperty>;

template <typename T>
class AKCOMMONS_EXPORT IAkPropertyTypeStore:
    public IAkProperty
{
    private:
        T m_value {T()};
        T m_defaultValue {T()};

        IAkPropertyTypeStore():
            IAkProperty()
        {

        }

        IAkPropertyTypeStore(const QString &description,
                         const T &value=T(),
                         IAkPropertyMode mode=IAkPropertyMode_ReadWrite):
            IAkProperty(description, value, mode)
        {

        }

        ~IAkPropertyTypeStore() override
        {
        }

        void *data() override
        {
            return &this->m_value;
        }

        const std::type_info &typeInfo() const override
        {
            return typeid(T);
        }

    protected:
        bool isNumeric() const override
        {
            return std::is_floating_point<T>::value
                   || std::is_integral<T>::value
                   || std::is_enum<T>::value;
        }

        bool isPointer() const override
        {
            return std::is_pointer<T>::value;
        }

        bool isCString() const override
        {
            return typeid(T) == typeid(char *)
                   || typeid(T) == typeid(const char *);
        }

        bool isString() const override
        {
            return typeid(T) == typeid(std::string)
                    || typeid(T) == typeid(QString)
                    || typeid(T) == typeid(char *)
                    || typeid(T) == typeid(const char *);
        }

    friend class IAkProperty;
};

template <typename T>
class AKCOMMONS_EXPORT IAkObjectPropertySetter
{
    public:
        using SetValueFunc = std::function<bool (const T &value)>;

        IAkObjectPropertySetter()
        {

        }

        IAkObjectPropertySetter(SetValueFunc func):
            m_func(func)
        {

        }

        inline bool setValue(const T &value)
        {
            return this->m_func(value);
        }

        inline void setValueFunc(SetValueFunc func)
        {
            if (!this->m_func)
                this->m_func = func;
        }

    private:
        SetValueFunc m_func;
};

template <typename T>
class AKCOMMONS_EXPORT IAkObjectPropertyCallbacks:
    public IAkCallback
{
    public:
        virtual void valueChanged(const T &value)
        {
            Q_UNUSED(value)
        }
};

template <typename T>
class AKCOMMONS_EXPORT IAkObjectProperty:
    public IAkProperty
{
    public:
        IAkObjectProperty(const QString &description={},
                          const T &value=T(),
                          IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                          IAkObjectPropertySetter<T> *propertySetter=nullptr):
            IAkProperty(description,
                        value,
                        mode)
        {
            this->m_propertySetter = propertySetter;

            if (this->m_propertySetter)
                propertySetter->setValueFunc([this] (const T &value) -> bool {
                    return this->setValueUnlocked(value);
                });
        }

        IAkObjectProperty &operator =(const T &value)
        {
            this->setValue(value);

            return *this;
        }

        operator T()
        {
            return this->value();
        }

        inline T defaultValue() const
        {
            return IAkProperty::defaultValue<T>();
        }

        inline T value()
        {
            return IAkProperty::value<T, IAkPropertyProtection_Protected>();
        }

        virtual bool setValue(const T &value)
        {
            auto ok = IAkProperty::setValue<T, IAkPropertyProtection_Protected>(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }

        inline void resetValue()
        {
            this->setValue(this->defaultValue());
        }

    private:
        IAkObjectPropertySetter<T> *m_propertySetter {nullptr};

        bool setValueUnlocked(const T &value)
        {
            auto ok = IAkProperty::setValueUnlocked<T, IAkPropertyProtection_Protected>(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }

    protected:
        void deleteThis(void *userData) const override
        {
            delete reinterpret_cast<IAkObjectProperty<T> *>(userData);
        }

        inline void valueChanged(const T &value) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkObjectPropertyCallbacks<T> *>(callback);

                if (cbk)
                    cbk->valueChanged(value);
            }
        }
};

template <typename T>
class AKCOMMONS_EXPORT IAkNumericPropertySetter
{
    public:
        using SetValueFunc = std::function<bool (T value)>;

        IAkNumericPropertySetter()
        {

        }

        IAkNumericPropertySetter(SetValueFunc func):
            m_func(func)
        {

        }

        inline bool setValue(T value)
        {
            return this->m_func(value);
        }

        inline void setValueFunc(SetValueFunc func)
        {
            if (!this->m_func)
                this->m_func = func;
        }

    private:
        SetValueFunc m_func;
};

template <typename T>
class AKCOMMONS_EXPORT IAkNumericPropertyCallbacks:
    public IAkCallback
{
    public:
        virtual void valueChanged(T value)
        {
            Q_UNUSED(value)
        }
};

template <typename T>
class AKCOMMONS_EXPORT IAkNumericProperty:
    public IAkProperty
{
    public:
        IAkNumericProperty(const QString &description={},
                           T value=T(),
                           IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                           IAkNumericPropertySetter<T> *propertySetter=nullptr):
            IAkProperty(description, value, mode)
        {
            this->m_propertySetter = propertySetter;

            if (this->m_propertySetter)
                propertySetter->setValueFunc([this] (T value) -> bool {
                    return this->setValueUnlocked(value);
                });
        }

        IAkNumericProperty &operator =(T value)
        {
            this->setValue(value);

            return *this;
        }

        operator T() const
        {
            return IAkProperty::toNumeric<T>();
        }

        inline T value() const
        {
            return IAkProperty::toNumeric<T>();
        }

        virtual bool setValue(T value)
        {
            auto ok = IAkProperty::setValue(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }

        inline void resetValue()
        {
            this->setValue(this->defaultValue<T>());
        }

    private:
        IAkNumericPropertySetter<T> *m_propertySetter {nullptr};

        bool setValueUnlocked(T value)
        {
            auto ok = IAkProperty::setValueUnlocked<T>(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }

    protected:
        void deleteThis(void *userData) const override
        {
            delete reinterpret_cast<IAkNumericProperty<T> *>(userData);
        }

        inline void valueChanged(T value) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkNumericPropertyCallbacks<T> *>(callback);

                if (cbk)
                    cbk->valueChanged(value);
            }
        }
};

template <typename T>
class AKCOMMONS_EXPORT IAkFloatingProperty:
    public IAkNumericProperty<T>
{
    public:
        IAkFloatingProperty(const QString &description={},
                            T value=T(),
                            IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                            IAkNumericPropertySetter<T> *propertySetter=nullptr):
            IAkNumericProperty<T>(description,
                                  value,
                                  mode,
                                  propertySetter)
        {
            if (propertySetter)
                propertySetter->setValueFunc([this] (T value) -> bool {
                    return this->setValueUnlocked(value);
                });
        }

        bool setValue(T value) override
        {
            if (qFuzzyCompare(IAkProperty::toNumeric<T>(), value))
                return false;

            auto ok = IAkProperty::setValue(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }

    private:
        bool setValueUnlocked(T value)
        {
            if (qFuzzyCompare(IAkProperty::toNumeric<T>(), value))
                return false;

            auto ok = IAkProperty::setValueUnlocked<T>(value);

            if (ok)
                this->valueChanged(value);

            return ok;
        }
};

#define IAK_LIMITED_NUMERIC_PROPERTY "Ak.LimitedNumericProperty"

template <typename T>
class AKCOMMONS_EXPORT IAkLimitedNumericProperty:
    public IAkNumericProperty<T>
{
    public:
        IAkLimitedNumericProperty(const QString &description,
                                  T value,
                                  T min,
                                  T max,
                                  T step,
                                  IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                                  IAkNumericPropertySetter<T> *propertySetter=nullptr):
            IAkNumericProperty<T>(description,
                                  value,
                                  mode,
                                  propertySetter),
            m_min(min),
            m_max(max),
            m_step(step)
        {

        }

        void *queryInterface(const QString &interfaceId) override
        {
            if (interfaceId == IAK_LIMITED_NUMERIC_PROPERTY)
                return this;

            return IAkNumericProperty<T>::queryInterface(interfaceId);
        }

        inline T min() const
        {
            return this->m_min;
        }

        inline T max() const
        {
            return this->m_max;
        }

        inline T step() const
        {
            return this->m_step;
        }

    private:
        T m_min {T()};
        T m_max {T()};
        T m_step {T()};
};

template <typename T>
class AKCOMMONS_EXPORT IAkLimitedFloatingProperty:
    public IAkFloatingProperty<T>
{
        public:
};

template <typename T>
class AKCOMMONS_EXPORT IAkMenuOption
{
    public:
        IAkMenuOption(const QString &id={},
                      const QString &description={},
                      const T &value=T()):
            m_id(id),
            m_description(description),
            m_value(value)
        {

        }

        IAkMenuOption(const IAkMenuOption &other):
            m_id(other.m_id),
            m_description(other.m_description),
            m_value(other.m_value)
        {

        }

        IAkMenuOption &operator =(const IAkMenuOption &other)
        {
            if (this != &other) {
                this->m_id = other.m_id;
                this->m_description = other.m_description;
                this->m_value = other.m_value;
            }

            return *this;
        }

        inline QString id() const
        {
            return this->m_id;
        }

        inline QString description() const
        {
            return this->m_description;
        }

        inline T value() const
        {
            return this->m_value;
        }

    private:
        QString m_id;
        QString m_description;
        T m_value {T()};
};

template <typename T>
class AKCOMMONS_EXPORT IAkPropertyMenu
{
    public:
        inline QVector<IAkMenuOption<T>> options() const
        {
            return this->m_options;
        }

        inline IAkMenuOption<T> optionById(const QString &id) const
        {
            for (auto &option: this->m_options)
                if (option.id() == id)
                    return option;

            return {};
        }

    protected:
        QVector<IAkMenuOption<T>> m_options;
};

#define IAK_NUMERIC_PROPERTY_MENU "Ak.NumericPropertyMenu"

template <typename T>
class AKCOMMONS_EXPORT IAkNumericPropertyMenu:
    public IAkPropertyMenu<T>,
    public IAkNumericProperty<T>
{
    public:
        IAkNumericPropertyMenu(const QString &description,
                               T value,
                               const QVector<IAkMenuOption<T>> &options,
                               IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                               IAkNumericPropertySetter<T> *propertySetter=nullptr):
              IAkNumericProperty<T>(description,
                                    value,
                                    mode,
                                    propertySetter)
        {
            this->m_options = options;
        }

        void *queryInterface(const QString &interfaceId) override
        {
            if (interfaceId == IAK_NUMERIC_PROPERTY_MENU)
                return this;

            return IAkNumericProperty<T>::queryInterface(interfaceId);
        }
};

#define IAK_OBJECT_PROPERTY_MENU "Ak.ObjectPropertyMenu"

template <typename T>
class AKCOMMONS_EXPORT IAkObjectPropertyMenu:
    public IAkPropertyMenu<T>,
    public IAkObjectProperty<T>
{
    public:
        IAkObjectPropertyMenu(const QString &description,
                        const T &value,
                        const QVector<IAkMenuOption<T>> &options,
                        IAkPropertyMode mode=IAkPropertyMode_ReadWrite,
                              IAkObjectPropertySetter<T> *propertySetter=nullptr):
              IAkObjectProperty<T>(description,
                                   value,
                                   mode,
                                   propertySetter)
        {
            this->m_options = options;
        }

        void *queryInterface(const QString &interfaceId) override
        {
            if (interfaceId == IAK_OBJECT_PROPERTY_MENU)
                return this;

            return IAkObjectProperty<T>::queryInterface(interfaceId);
        }
};

using IAkPropertyBool = IAkNumericProperty<bool>;
using IAkPropertyInt8 = IAkNumericProperty<qint8>;
using IAkPropertyLimitedInt8 = IAkLimitedNumericProperty<qint8>;
using IAkPropertyChar = IAkPropertyInt8;
using IAkPropertyInt16 = IAkNumericProperty<qint16>;
using IAkPropertyLimitedInt16 = IAkLimitedNumericProperty<qint16>;
using IAkPropertyShort = IAkPropertyInt16;
using IAkPropertyInt32 = IAkNumericProperty<qint32>;
using IAkPropertyInt32List = IAkObjectProperty<QVector<qint32>>;
using IAkPropertyInt32Menu = IAkNumericPropertyMenu<qint32>;
using IAkPropertyLimitedInt32 = IAkLimitedNumericProperty<qint32>;
using IAkPropertyInt = IAkPropertyInt32;
using IAkPropertyIntList = IAkPropertyInt32List;
using IAkPropertyIntMenu = IAkPropertyInt32Menu;
using IAkPropertyInt64 = IAkNumericProperty<qint64>;
using IAkPropertyLimitedInt64 = IAkLimitedNumericProperty<qint64>;
using IAkPropertyLong = IAkPropertyInt64;
using IAkPropertyUInt8 = IAkNumericProperty<quint8>;
using IAkPropertyLimitedUInt8 = IAkLimitedNumericProperty<quint8>;
using IAkPropertyUChar = IAkPropertyUInt8;
using IAkPropertyUInt16 = IAkNumericProperty<quint16>;
using IAkPropertyLimitedUInt16 = IAkLimitedNumericProperty<quint16>;
using IAkPropertyUShort = IAkPropertyUInt16;
using IAkPropertyUInt32 = IAkNumericProperty<quint32>;
using IAkPropertyLimitedUInt32 = IAkLimitedNumericProperty<quint32>;
using IAkPropertyUInt = IAkPropertyUInt32;
using IAkPropertyUInt64 = IAkNumericProperty<quint64>;
using IAkPropertyLimitedUInt64 = IAkLimitedNumericProperty<quint64>;
using IAkPropertyULong = IAkPropertyUInt64;
using IAkPropertyFloat = IAkFloatingProperty<float>;
using IAkPropertyLimitedFloat = IAkLimitedFloatingProperty<float>;
using IAkPropertyDouble = IAkFloatingProperty<qreal>;
using IAkPropertyLimitedDouble = IAkLimitedFloatingProperty<qreal>;
using IAkPropertyDoubleList = IAkObjectProperty<QVector<qreal>>;
using IAkPropertyString = IAkObjectProperty<QString>;
using IAkPropertyStringMenu = IAkObjectPropertyMenu<QString>;
using IAkPropertyStringList = IAkObjectProperty<QStringList>;
using IAkPropertyColor = IAkNumericProperty<QRgb>;
using IAkPropertyColorList = IAkObjectProperty<QVector<QRgb>>;
using IAkPropertySize = IAkObjectProperty<QSize>;
using IAkPropertyRect = IAkObjectProperty<QRect>;
using IAkPropertyFont = IAkObjectProperty<QFont>;
using IAkPropertyFrac = IAkObjectProperty<AkFrac>;
using IAkPropertyAudioFormat = IAkObjectProperty<AkAudioCaps>;
using IAkPropertyVideoFormat = IAkObjectProperty<AkVideoCaps>;
using IAkPropertyPixelFormat = IAkNumericProperty<AkVideoCaps::PixelFormat>;
using IAkPropertyPixelFormatList = IAkObjectProperty<AkVideoCaps::PixelFormatList>;

#define IAK_PROPERTIES "Ak.Properties"

class AKCOMMONS_EXPORT IAkProperties
{
    public:
        inline QStringList properties() const
        {
            return this->m_properties.keys();
        }

        template <class T=IAkProperty>
        inline T *property(const QString &key) const
        {
            return dynamic_cast<T *>(this->m_properties.value(key));
        }

    private:
        QMap<QString, IAkProperty *> m_properties;

    protected:
        inline void registerProperty(const QString &key, IAkProperty *property)
        {
            this->m_properties[key] = property;
        }
};

#define IAK_METHODS "Ak.Methods"

class AKCOMMONS_EXPORT IAkMethods
{
    public:
        using CallableFunc = std::function<void (QVariantList *,
                                                 const QVariantList &)>;

        inline QStringList methods() const
        {
            return this->m_functions.keys();
        }

        inline bool call(const QString &methodName,
                         QVariantList *results=nullptr,
                         const QVariantList &args={}) const
        {
            for (auto it = this->m_functions.begin();
                 it != this->m_functions.end();
                 it++)
                if (it.key() == methodName) {
                    it.value()(results, args);

                    return true;
                }

            return false;
        }

        inline bool call(const QString &methodName,
                         const QVariantList &args) const
        {
            return this->call(methodName, nullptr, args);
        }

    private:
        QMap<QString, CallableFunc> m_functions;

    protected:
        inline void registerMethod(const QString &methodName,
                                   const CallableFunc &method)
        {
            this->m_functions[methodName] = method;
        }
};

enum AkElementType
{
    AkElementType_Unknown,
    AkElementType_AudioSource = 0x1,
    AkElementType_VideoSource = 0x2,
    AkElementType_AudioSink = 0x4,
    AkElementType_VideoSink = 0x8,
    AkElementType_MultimediaSource =
       AkElementType_AudioSource | AkElementType_VideoSource,
    AkElementType_MultimediaSink =
       AkElementType_AudioSink | AkElementType_VideoSink,
    AkElementType_AudioFilter =
       AkElementType_AudioSource | AkElementType_AudioSink,
    AkElementType_VideoFilter =
       AkElementType_VideoSource | AkElementType_VideoSink,
    AkElementType_MultimediaFilter =
       AkElementType_MultimediaSource | AkElementType_MultimediaSink,
    AkElementType_DeviceProvider = 0x10,
    AkElementType_Other = 0x20,
};

enum AkElementCategory
{
    AkElementCategory_Unknown,
    AkElementCategory_Audio_Start = 1,
    AkElementCategory_AudioSource,
    AkElementCategory_AudioMicrophone,
    AkElementCategory_AudioSpeaker,
    AkElementCategory_AudioFilter,
    AkElementCategory_AudioEncoder,
    AkElementCategory_AudioDecoder,
    AkElementCategory_Audio_End,
    AkElementCategory_Video_Start = 1000,
    AkElementCategory_VideoSource,
    AkElementCategory_VideoCamera,
    AkElementCategory_VideoScreenCapture,
    AkElementCategory_VideoVirtualCamera,
    AkElementCategory_VideoStreaming,
    AkElementCategory_VideoFilter,
    AkElementCategory_VideoEncoder,
    AkElementCategory_VideoDecoder,
    AkElementCategory_Video_End,
    AkElementCategory_Multimedia_Start = 2000,
    AkElementCategory_MultimediaPlayback,
    AkElementCategory_MultimediaRecording,
    AkElementCategory_Multimedia_End,
    AkElementCategory_Other_Start = 3000,
    AkElementCategory_OtherUnknown,
    AkElementCategory_OtherScripting,
    AkElementCategory_Other_End,
};

#define IAK_ELEMENT "Ak.Element"

class AKCOMMONS_EXPORT IAkElement: public IAkUnknown
{
    public:
        virtual QString description() const = 0;
        virtual AkElementType type() const = 0;
        virtual AkElementCategory category() const = 0;
        void *queryInterface(const QString &interfaceId) override
        {
            if (interfaceId == IAK_ELEMENT)
                return this;

            return IAkUnknown::queryInterface(interfaceId);
        }
};

class AKCOMMONS_EXPORT IAkStreamFormatCallbacks:
    public IAkCallback
{
    public:
        virtual void formatChanged(const AkCaps &caps)
        {
            Q_UNUSED(caps)
        }
};

#define IAK_STREAM_FORMAT "Ak.StreamFormat"

class AKCOMMONS_EXPORT IAkStreamFormat:
    public IAkCallbacks
{
    public:
        virtual AkCaps preferredFormat() const = 0;

        virtual AkCaps format() const
        {
            return this->m_format;
        }

        virtual void setFormat(const AkCaps &caps)
        {
            if (this->m_format == caps)
                return;

            this->m_format = caps;
            this->formatChanged(caps);
        }

        inline void resetFormat()
        {
            this->setFormat(this->preferredFormat());
        }

    protected:
        AkCaps m_format;

        inline void formatChanged(const AkCaps &caps) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkStreamFormatCallbacks *>(callback);

                if (cbk)
                    cbk->formatChanged(caps);
            }
        }
};

using AkCapsList = QVector<AkCaps>;

#define IAK_SUPPORTED_STREAM_FORMATS "Ak.SupportedStreamFormats"

class AKCOMMONS_EXPORT IAkSupportedStreamFormats
{
    public:
        virtual AkCapsList supportedFormats() = 0;
};

#define IAK_SINK_STREAM "Ak.SinkStream"

class AKCOMMONS_EXPORT IAkSinkStream
{
    public:
        virtual IAkStreamFormat *sinkStreamFormat() const
        {
            return nullptr;
        }

        virtual AkPacket iStream(const AkPacket &packet) = 0;
};

using IAkSinkStreamPtr = QSharedPointer<IAkSinkStream>;

#define IAK_SOURCE_STREAM "Ak.SourceStream"

class AKCOMMONS_EXPORT IAkSourceStream
{
    public:
        virtual IAkStreamFormat *sourceStreamFormat() const
        {
            return nullptr;
        }

        inline void link(const IAkSinkStream *sink)
        {
            if (!this->m_sinkStreams.contains(sink))
                this->m_sinkStreams << const_cast<IAkSinkStream *>(sink);
        }

        inline void unlink(const IAkSinkStream *sink)
        {
            this->m_sinkStreams.removeAll(sink);
        }

    private:
        QVector<IAkSinkStream *> m_sinkStreams;

    protected:
        inline void oStream(const AkPacket &packet) const
        {
            for (auto stream: this->m_sinkStreams)
                stream->iStream(packet);
        }
};

using IAkSourceStreamPtr = QSharedPointer<IAkSourceStream>;

#define IAK_AUDIO_SINK_STREAM "Ak.AudioSinkStream"

class AKCOMMONS_EXPORT IAkAudioSinkStream: public IAkSinkStream
{
    public:
        AkPacket iStream(const AkPacket &packet) override
        {
            if (packet.type() == AkPacket::PacketAudio)
                return this->iAudioStream(packet);

            return {};
        }

    protected:
        virtual AkPacket iAudioStream(const AkAudioPacket &packet) = 0;
};

#define IAK_VIDEO_SINK_STREAM "Ak.VideoSinkStream"

class AKCOMMONS_EXPORT IAkVideoSinkStream: public IAkSinkStream
{
    public:
        AkPacket iStream(const AkPacket &packet) override
        {
            if (packet.type() == AkPacket::PacketVideo)
                return this->iVideoStream(packet);

            return {};
        }

    protected:
        virtual AkPacket iVideoStream(const AkVideoPacket &packet) = 0;
};

#define IAK_VIDEO_FILTER "Ak.VideoFilter"

class AKCOMMONS_EXPORT IAkVideoFilter:
    public IAkVideoSinkStream,
    public IAkSourceStream,
    public IAkProperties,
    public IAkMethods
{
    public:
};

using IAkVideoFilterPtr = QSharedPointer<IAkVideoFilter>;

#define IAK_AUDIO_STREAM_SUPPORTED_FORMATS "Ak.AudioStreamSupportedFormats"

class AKCOMMONS_EXPORT IAkAudioStreamSupportedFormats
{
    public:
        virtual QList<AkAudioCaps::SampleFormat> supportedFormats() const = 0;
        virtual QList<AkAudioCaps::ChannelLayout> supportedChannelLayouts() const = 0;
        virtual QList<int> supportedSampleRates() const = 0;
};

class AKCOMMONS_EXPORT IAkCodecSettingsCallbacks:
    public IAkCallback
{
    public:
        virtual void codecChanged(const QString &codec)
        {
            Q_UNUSED(codec)
        }

        virtual void bitrateChanged(int bitrate)
        {
            Q_UNUSED(bitrate)
        }
};

#define IAK_CODEC_SETTINGS "Ak.CodecSettings"

class AKCOMMONS_EXPORT IAkCodecSettings:
    public IAkUnknown,
    public IAkCallbacks
{
    public:
        virtual QString codec() const
        {
            return this->m_codec;
        }

        virtual void setCodec(const QString &codec)
        {
            if (this->m_codec == codec)
                return;

            this->m_codec = codec;
            this->codecChanged(codec);
        }

        virtual int bitrate() const
        {
            return this->m_bitrate;
        }

        virtual void setBitrate(int bitrate)
        {
            if (this->m_bitrate == bitrate)
                return;

            this->m_bitrate = bitrate;
            this->bitrateChanged(bitrate);
        }

        virtual QStringList codecsBlackList() const = 0;
        virtual QStringList supportedCodecs(const QString &format,
                                            AkCaps::CapsType type=AkCaps::CapsUnknown) const = 0;
        virtual QString defaultCodec(const QString &format,
                                     AkCaps::CapsType type) const = 0;
        virtual QString codecDescription(const QString &codec) const = 0;
        virtual AkCaps::CapsType codecType(const QString &codec) const = 0;
        virtual const IAkProperties *defaultCodecParams(const QString &codec) const = 0;
        void *queryInterface(const QString &interfaceId) override
        {
            if (interfaceId == IAK_CODEC_SETTINGS)
                return this;

            return IAkUnknown::queryInterface(interfaceId);
        }

    protected:
        QString m_codec;
        int m_bitrate {0};

        inline void codecChanged(const QString &codec) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkCodecSettingsCallbacks *>(callback);

                if (cbk)
                    cbk->codecChanged(codec);
            }
        }

        inline void bitrateChanged(int bitrate) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkCodecSettingsCallbacks *>(callback);

                if (cbk)
                    cbk->bitrateChanged(bitrate);
            }
        }
};

class AKCOMMONS_EXPORT IAkVideoCodecSettingsCallbacks:
    public IAkCallback
{
    public:
        virtual void gopChanged(int gop)
        {
            Q_UNUSED(gop)
        }
};

#define IAK_VIDEO_CODEC_SETTINGS "Ak.VideoCodecSettings"

class AKCOMMONS_EXPORT IAkVideoCodecSettings:
    public IAkCodecSettings
{
    public:
        virtual int gop() const
        {
            return this->m_gop;
        }

        virtual void setGop(int gop)
        {
            if (this->m_gop == gop)
                return;

            this->m_gop = gop;
            this->gopChanged(gop);
        }

    protected:
        int m_gop {0};

        inline void gopChanged(int gop) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkVideoCodecSettingsCallbacks *>(callback);

                if (cbk)
                    cbk->gopChanged(gop);
            }
        }
};

class AKCOMMONS_EXPORT IAkRecordingCallbacks:
    public IAkCallback
{
    public:
        virtual void outputFileNameChanged(const QString &outputFileName)
        {
            Q_UNUSED(outputFileName)
        }

        virtual void outputFormatChanged(const QString &outputFormat)
        {
            Q_UNUSED(outputFormat)
        }

        virtual void formatsBlackListChanged(const QStringList &formatsBlackList)
        {
            Q_UNUSED(formatsBlackList)
        }

        virtual void streamIndexChanged(AkCaps::CapsType type,
                                        int streamIndex)
        {
            Q_UNUSED(type)
            Q_UNUSED(streamIndex)
        }
};

#define IAK_RECORDING "Ak.Recording"

class AKCOMMONS_EXPORT IAkRecording:
    public IAkElement,
    public IAkSinkStream,
    public IAkCallbacks
{
    public:
        virtual QString outputFileName() const
        {
            return this->m_outputFileName;
        }

        virtual void setOutputFileName(const QString &outputFileName)
        {
            if (this->m_outputFileName == outputFileName)
                return;

            this->m_outputFileName = outputFileName;
            this->outputFileNameChanged(outputFileName);
        }

        virtual QString defaultFormat() const = 0;
        virtual QStringList supportedFormats() const = 0;

        virtual QString outputFormat() const
        {
            return this->m_outputFormat;
        }

        virtual void setOutputFormat(const QString &outputFormat)
        {
            if (this->m_outputFormat == outputFormat)
                return;

            this->m_outputFormat = outputFormat;
            this->outputFormatChanged(outputFormat);
        }

        virtual QStringList formatsBlackList() const
        {
            return this->m_formatsBlackList;
        }

        virtual void setFormatsBlackList(const QStringList &formatsBlackList)
        {
            if (this->m_formatsBlackList == formatsBlackList)
                return;

            this->m_formatsBlackList = formatsBlackList;
            this->formatsBlackListChanged(formatsBlackList);
        }

        virtual QStringList fileExtensions(const QString &format) const = 0;
        virtual QString formatDescription(const QString &format) const = 0;
        virtual IAkProperties *formatOptions() const = 0;
        virtual IAkCodecSettings *audioCodecSettings() const = 0;
        virtual IAkVideoCodecSettings *videoCodecSettings() const = 0;

        virtual int streamIndex(AkCaps::CapsType type) const
        {
            return this->m_streamIndexMap.value(type);
        }

        virtual void setStreamIndex(AkCaps::CapsType type,
                                    int streamIndex)
        {
            if (this->m_streamIndexMap.value(type) == streamIndex)
                return;

            this->m_streamIndexMap[type] = streamIndex;
            this->streamIndexChanged(type, streamIndex);
        }

    protected:
        QString m_outputFileName;
        QString m_outputFormat;
        QStringList m_formatsBlackList;
        QMap<AkCaps::CapsType, int> m_streamIndexMap;

        inline void outputFileNameChanged(const QString &outputFileName) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkRecordingCallbacks *>(callback);

                if (cbk)
                    cbk->outputFileNameChanged(outputFileName);
            }
        }

        inline void outputFormatChanged(const QString &outputFormat) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkRecordingCallbacks *>(callback);

                if (cbk)
                    cbk->outputFormatChanged(outputFormat);
            }
        }

        inline void formatsBlackListChanged(const QStringList &formatsBlackList) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkRecordingCallbacks *>(callback);

                if (cbk)
                    cbk->formatsBlackListChanged(formatsBlackList);
            }
        }

        inline void streamIndexChanged(AkCaps::CapsType type,
                                       int streamIndex) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkRecordingCallbacks *>(callback);

                if (cbk)
                    cbk->streamIndexChanged(type, streamIndex);
            }
        }
};

class AKCOMMONS_EXPORT IAkPlaybackCallbacks:
    public IAkCallback
{
    public:
        virtual void inputFileNameChanged(const QString &inputFileName)
        {
            Q_UNUSED(inputFileName)
        }

        virtual void loopChanged(bool loop)
        {
            Q_UNUSED(loop)
        }

        virtual void trackChanged(AkCaps::CapsType type, int track)
        {
            Q_UNUSED(type)
            Q_UNUSED(track)
        }
};

using AkMediaTracks = QVector<int>;

#define IAK_PLAYBACK "Ak.Playback"

class AKCOMMONS_EXPORT IAkPlayback:
    public IAkElement,
    public IAkSourceStream,
    public IAkCallbacks
{
    public:
        virtual QString inputFileName() const
        {
            return this->m_inputFileName;
        }

        virtual void setInputFileName(const QString &inputFileName)
        {
            if (this->m_inputFileName == inputFileName)
                return;

            this->m_inputFileName = inputFileName;
            this->inputFileNameChanged(inputFileName);
        }

        virtual bool loop() const
        {
            return this->m_loop;
        }

        virtual void setLoop(bool loop)
        {
            if (this->m_loop == loop)
                return;

            this->m_loop = loop;
            this->loopChanged(loop);
        }

        virtual QString trackLanguage(int track) const = 0;
        virtual int defaultTrack(AkCaps::CapsType type) const = 0;
        virtual AkMediaTracks tracks(AkCaps::CapsType type) const = 0;

        virtual int track(AkCaps::CapsType type) const
        {
            return this->m_trackMap.value(type);
        }

        virtual void setTrack(AkCaps::CapsType type, int track)
        {
            if (this->m_trackMap.value(type) == track)
                return;

            this->m_trackMap[type] = track;
            this->trackChanged(type, track);
        }

        virtual qint64 durationMSecs() const = 0;
        virtual qint64 currentTimeMSecs() const = 0;

    protected:
        QString m_inputFileName;
        bool m_loop {false};
        QMap<AkCaps::CapsType, int> m_trackMap;

        inline void inputFileNameChanged(const QString &inputFileName) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkPlaybackCallbacks *>(callback);

                if (cbk)
                    cbk->inputFileNameChanged(inputFileName);
            }
        }

        inline void loopChanged(bool loop) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkPlaybackCallbacks *>(callback);

                if (cbk)
                    cbk->loopChanged(loop);
            }
        }

        inline void trackChanged(AkCaps::CapsType type, int track) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkPlaybackCallbacks *>(callback);

                if (cbk)
                    cbk->trackChanged(type, track);
            }
        }
};

#define IAK_DEVICE "Ak.Device"

class AKCOMMONS_EXPORT IAkDevice
{
    public:
        virtual QString id() const = 0;
        virtual QString description() const = 0;
};

enum AkCameraTorchMode
{
    AkCameraTorch_Off,
    AkCameraTorch_On,
};

class AKCOMMONS_EXPORT IAkCameraCallbacks:
    public IAkCallback
{
    public:
        virtual void torchModeChanged(AkCameraTorchMode torchMode)
        {
            Q_UNUSED(torchMode)
        }

        virtual void pictureTaken(int index, const AkPacket &picture)
        {
            Q_UNUSED(index)
            Q_UNUSED(picture)
        }
};

#define IAK_CAMERA "Ak.Camera"

class AKCOMMONS_EXPORT IAkCamera:
    public IAkElement,
    public IAkSourceStream,
    public IAkDevice,
    public IAkCallbacks
{
    public:
        virtual const IAkProperties *controls() const = 0;
        virtual bool isTorchSupported() const = 0;

        virtual AkCameraTorchMode torchMode() const
        {
            return this->m_torchMode;
        }

        virtual void setTorchMode(AkCameraTorchMode torchMode)
        {
            if (this->m_torchMode == torchMode)
                return;

            this->m_torchMode = torchMode;
            this->torchModeChanged(torchMode);
        }

        virtual void reset() = 0;
        virtual void takePictures(int count, int delayMsecs=0) = 0;

    protected:
        AkCameraTorchMode m_torchMode {AkCameraTorch_Off};

        inline void torchModeChanged(AkCameraTorchMode torchMode) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkCameraCallbacks *>(callback);

                if (cbk)
                    cbk->torchModeChanged(torchMode);
            }
        }

        inline void pictureTaken(int index, const AkPacket &picture) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkCameraCallbacks *>(callback);

                if (cbk)
                    cbk->pictureTaken(index, picture);
            }
        }
};

class AKCOMMONS_EXPORT IAkAudioStreamLatencyCallbacks:
    public IAkCallback
{
    public:
        virtual void latencyChanged(int latency)
        {
            Q_UNUSED(latency)
        }
};

#define IAK_AUDIO_STREAM_LATENCY "Ak.AudioStreamLatency"

class AKCOMMONS_EXPORT IAkAudioStreamLatency:
    public IAkCallbacks
{
    public:
        virtual int latency() const
        {
            return this->m_latency;
        }

        virtual void setLatency(int latency)
        {
            if (this->m_latency == latency)
                return;

            this->m_latency = latency;
            this->latencyChanged(latency);
        }

    protected:
        int m_latency {0};

        inline void latencyChanged(int latency) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkAudioStreamLatencyCallbacks *>(callback);

                if (cbk)
                    cbk->latencyChanged(latency);
            }
        }
};

class AKCOMMONS_EXPORT IAkVideoStreamFrameRateCallbacks:
    public IAkCallback
{
    public:
        virtual void fpsChanged(const AkFrac &fps)
        {
            Q_UNUSED(fps)
        }
};

#define IAK_VIDEO_STREAM_FRAME_RATE "Ak.VideoStreamFrameRate"

class AKCOMMONS_EXPORT IAkVideoStreamFrameRate:
    public IAkCallbacks
{
    public:
        virtual AkFrac fps() const
        {
            return this->m_fps;
        }

        virtual void setFps(const AkFrac &fps)
        {
            if (this->m_fps == fps)
                return;

            this->m_fps = fps;
            this->fpsChanged(fps);
        }

    protected:
        AkFrac m_fps;

        inline void fpsChanged(const AkFrac &fps) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkVideoStreamFrameRateCallbacks *>(callback);

                if (cbk)
                    cbk->fpsChanged(fps);
            }
        }
};

class AKCOMMONS_EXPORT IAkScreenCursorCallbacks:
    public IAkCallback
{
    public:
        virtual void showCursorChanged(bool showCursor)
        {
            Q_UNUSED(showCursor)
        }
};

#define IAK_SCREEN_CURSOR "Ak.ScreenCursor"

class AKCOMMONS_EXPORT IAkScreenCursor:
    public IAkCallbacks
{
    public:
        virtual bool showCursor() const
        {
            return this->m_showCursor;
        }

        virtual void setShowCursor(bool showCursor)
        {
            if (this->m_showCursor == showCursor)
                return;

            this->m_showCursor = showCursor;
            this->showCursorChanged(showCursor);
        }

    protected:
        bool m_showCursor {false};

        inline void showCursorChanged(bool showCursor) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkScreenCursorCallbacks *>(callback);

                if (cbk)
                    cbk->showCursorChanged(showCursor);
            }
        }
};

class AKCOMMONS_EXPORT IAkScreenCursorSizeCallbacks:
    public IAkCallback
{
    public:
        virtual void cursorSizeChanged(int cursorSize)
        {
            Q_UNUSED(cursorSize)
        }
};

#define IAK_SCREEN_CURSOR_SIZE "Ak.ScreenCursorSize"

class AKCOMMONS_EXPORT IAkScreenCursorSize:
    public IAkCallbacks
{
    public:
        virtual int cursorSize() const
        {
            return this->m_cursorSize;
        }

        virtual void setCursorSize(int cursorSize)
        {
            if (this->m_cursorSize == cursorSize)
                return;

            this->m_cursorSize = cursorSize;
            this->cursorSizeChanged(cursorSize);
        }

    protected:
        int m_cursorSize {0};

        inline void cursorSizeChanged(int cursorSize) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkScreenCursorSizeCallbacks *>(callback);

                if (cbk)
                    cbk->cursorSizeChanged(cursorSize);
            }
        }
};

enum AkElementState
{
    AkElementState_Stopped,
    AkElementState_Paused,
    AkElementState_Playing
};

class AKCOMMONS_EXPORT IAkElementStateCallbacks:
    public IAkCallback
{
    public:
        virtual void stateChanged(AkElementState state)
        {
            Q_UNUSED(state)
        }
};

#define IAK_ELEMENT_STATE "Ak.ElementState"

class AKCOMMONS_EXPORT IAkElementState:
    public IAkCallbacks
{
    public:
        virtual AkElementState state() const
        {
            return this->m_state;
        }

        virtual bool setState(AkElementState state)
        {
            if (this->m_state == state)
                return false;

            this->m_state = state;
            this->stateChanged(state);

            return true;
        }

    protected:
        AkElementState m_state {AkElementState_Stopped};

        inline void stateChanged(AkElementState state) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkElementStateCallbacks *>(callback);

                if (cbk)
                    cbk->stateChanged(state);
            }
        }
};

#define IAK_UI_QML "Ak.UIQml"

class AKCOMMONS_EXPORT IAkUIQml
{
    public:
        QObject *controlInterface(QQmlEngine *engine,
                                  const QString &controlId) const
        {
            Q_UNUSED(controlId)

            if (!engine)
                return nullptr;

            auto qmlFile = this->controlInterfaceProvide(controlId);

            if (qmlFile.isEmpty())
                return nullptr;

            // Load the UI from the plugin.
            QQmlComponent component(engine, qmlFile);

            if (component.isError()) {
                qDebug() << "Error in file "
                         << qmlFile
                         << ":"
                         << component.errorString();

                return nullptr;
            }

            // Create a context for the plugin.
            auto context = new QQmlContext(engine->rootContext());
            this->controlInterfaceConfigure(context, controlId);

            // Create an item with the plugin context.
            QObject *item = component.create(context);

            if (!item) {
                delete context;

                return nullptr;
            }

            QQmlEngine::setObjectOwnership(item, QQmlEngine::JavaScriptOwnership);
            context->setParent(item);

            return item;
        }

    protected:
        virtual QString controlInterfaceProvide(const QString &controlId) const = 0;
        virtual void controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const = 0;
};

#define IAK_ELEMENT_PROVIDER "Ak.ElementProvider"

class AKCOMMONS_EXPORT IAkElementProvider:
    public IAkElement
{
    public:
        virtual IAkElement *create(const QString &id={}) = 0;
        template<typename T>
        inline QSharedPointer<T> create(const QString &id={})
        {
            auto element = this->create(id);

            if (!element)
                return {};

            return QSharedPointer<T>(reinterpret_cast<T *>(element),
                                     [] (T *element) {
                                         reinterpret_cast<IAkUnknown *>(element)->unref();
                                     });
        }
};

class AKCOMMONS_EXPORT IAkDeviceProviderCallbacks:
    public IAkCallback
{
    public:
        virtual void devicesUpdated(const QString &devices)
        {
            Q_UNUSED(devices)
        }
};

#define IAK_DEVICE_PROVIDER "Ak.DeviceProvider"

class AKCOMMONS_EXPORT IAkDeviceProvider:
    public IAkElementProvider,
    public IAkCallbacks
{
    public:
        virtual QStringList devices() const = 0;

    protected:
        inline void devicesUpdated(const QString &devices) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkDeviceProviderCallbacks *>(callback);

                if (cbk)
                    cbk->devicesUpdated(devices);
            }
        }
};

class AKCOMMONS_EXPORT IAkVirtualCameraProviderCallbacks:
    public IAkCallback
{
    public:
        virtual void pictureChanged(const QString &picture)
        {
            Q_UNUSED(picture)
        }
};

#define IAK_VIRTUAL_CAMERA_PROVIDER "Ak.VirtualCameraProvider"

class AKCOMMONS_EXPORT IAkVirtualCameraProvider:
    public IAkDeviceProvider
{
    public:
        virtual AkVideoCaps::PixelFormatList supportedOutputPixelFormats() const = 0;
        virtual AkVideoCaps::PixelFormat defaultOutputPixelFormat() const = 0;
        virtual QString addWebcam(const QString &description,
                                  const AkVideoCapsList &formats) = 0;
        virtual bool editWebcam(const QString &webcam,
                                const QString &description,
                                const AkVideoCapsList &formats) = 0;
        virtual bool changeDescription(const QString &webcam,
                                       const QString &description={}) = 0;
        virtual bool removeWebcam(const QString &webcam) = 0;
        virtual bool removeAllWebcams() = 0;
        virtual QList<quint64> clientsPids() const = 0;
        virtual QString clientExe(quint64 pid) const = 0;
        virtual bool driverInstalled() const = 0;
        virtual QString driverVersion() const = 0;

        virtual QString picture() const
        {
            return this->m_picture;
        }

        virtual void setPicture(const QString &picture)
        {
            if (this->m_picture == picture)
                return;

            this->m_picture = picture;
            this->pictureChanged(picture);
        }

        virtual bool canEditVCamDescription() const = 0;
        virtual bool applyPicture() = 0;

    protected:
        QString m_picture;

        inline void pictureChanged(const QString &picture) const
        {
            for (auto callback: this->callbacks()) {
                auto cbk = dynamic_cast<IAkVirtualCameraProviderCallbacks *>(callback);

                if (cbk)
                    cbk->pictureChanged(picture);
            }
        }
};

enum AkDeviceType
{
    AkDeviceType_Source,
    AkDeviceType_Sink,
};

#define IAK_AUDIO_DEVICE_PROVIDER "Ak.AudioDeviceProvider"

class AKCOMMONS_EXPORT IAkAudioDeviceProvider: public IAkDeviceProvider
{
    public:
        virtual QString defaultDevice(AkDeviceType deviceType) const = 0;
        virtual QStringList sources() const = 0;
        virtual QStringList sinks() const = 0;

        inline AkDeviceType deviceType(const QString &device) const
        {
            if (this->sources().contains(device))
                return AkDeviceType_Source;

            return AkDeviceType_Sink;
        }
};

using IAkAudioDeviceProviderPtr = QSharedPointer<IAkAudioDeviceProvider>;

#define IAK_AUDIO_DEVICE_SOURCE "Ak.AudioDeviceSource"

class AKCOMMONS_EXPORT IAkAudioDeviceSource:
    public IAkElement,
    public IAkSourceStream,
    public IAkAudioStreamSupportedFormats,
    public IAkElementState,
    public IAkAudioStreamLatency,
    public IAkProperties,
    public IAkMethods
{
    public:
};

using IAkAudioDeviceSourcePtr = QSharedPointer<IAkAudioDeviceSource>;

#define IAK_AUDIO_DEVICE_SINK "Ak.AudioDeviceSink"

class AKCOMMONS_EXPORT IAkAudioDeviceSink:
    public IAkElement,
    public IAkSinkStream,
    public IAkAudioStreamSupportedFormats,
    public IAkElementState,
    public IAkAudioStreamLatency,
    public IAkProperties,
    public IAkMethods
{
    public:
};

using IAkAudioDeviceSinkPtr = QSharedPointer<IAkAudioDeviceSink>;

#endif // AKPLUGININTERFACE_H
