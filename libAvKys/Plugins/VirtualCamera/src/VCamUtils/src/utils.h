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

#ifndef AKVCAMUTILS_UTILS_H
#define AKVCAMUTILS_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

#ifndef UNUSED
    #define UNUSED(x) (void)(x);
#endif

#define GLOBAL_STATIC(type, variableName) \
    type *variableName() \
    { \
        static type _##variableName; \
        \
        return &_##variableName; \
    }

#define GLOBAL_STATIC_WITH_ARGS(type, variableName, ...) \
    type *variableName() \
    { \
        static type _##variableName {__VA_ARGS__}; \
        \
        return &_##variableName; \
    }

#define AKVCAM_CALLBACK(CallbackName, ...) \
    typedef void (*CallbackName##CallbackT)(void *userData, __VA_ARGS__); \
    typedef std::pair<void *, CallbackName##CallbackT> CallbackName##Callback;

#define AKVCAM_CALLBACK_NOARGS(CallbackName) \
    typedef void (*CallbackName##CallbackT)(void *userData); \
    typedef std::pair<void *, CallbackName##CallbackT> CallbackName##Callback;

#define AKVCAM_SIGNAL(CallbackName, ...) \
    public: \
        typedef void (*CallbackName##CallbackT)(void *userData, __VA_ARGS__); \
        typedef std::pair<void *, CallbackName##CallbackT> CallbackName##Callback; \
        \
        void connect##CallbackName(void *userData, \
                                   CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto &func: this->m_##CallbackName##Callback) \
                if (func.first == userData \
                    && func.second == callback) \
                    return; \
            \
            this->m_##CallbackName##Callback.push_back({userData, callback});\
        } \
        \
        void disconnect##CallbackName(void *userData, \
                                      CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto it = this->m_##CallbackName##Callback.begin(); \
                it != this->m_##CallbackName##Callback.end(); \
                it++) \
                if (it->first == userData \
                    && it->second == callback) { \
                    this->m_##CallbackName##Callback.erase(it); \
                    \
                    return; \
                } \
        } \
    \
    private: \
        std::vector<CallbackName##Callback> m_##CallbackName##Callback;

#define AKVCAM_SIGNAL_NOARGS(CallbackName) \
    public: \
        typedef void (*CallbackName##CallbackT)(void *userData); \
        typedef std::pair<void *, CallbackName##CallbackT> CallbackName##Callback; \
        \
        void connect##CallbackName(void *userData, \
                                   CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto &func: this->m_##CallbackName##Callback) \
                if (func.first == userData \
                    && func.second == callback) \
                    return; \
            \
            this->m_##CallbackName##Callback.push_back({userData, callback});\
        } \
        \
        void disconnect##CallbackName(void *userData, \
                                      CallbackName##CallbackT callback) \
        { \
            if (!callback) \
                return; \
            \
            for (auto it = this->m_##CallbackName##Callback.begin(); \
                it != this->m_##CallbackName##Callback.end(); \
                it++) \
                if (it->first == userData \
                    && it->second == callback) { \
                    this->m_##CallbackName##Callback.erase(it); \
                    \
                    return; \
                } \
        } \
    \
    private: \
        std::vector<CallbackName##Callback> m_##CallbackName##Callback;

#define AKVCAM_EMIT(owner, CallbackName, ...) \
    for (auto &callback: owner->m_##CallbackName##Callback) \
        if (callback.second) \
            callback.second(callback.first, __VA_ARGS__); \

#define AKVCAM_EMIT_NOARGS(owner, CallbackName) \
    for (auto &callback: owner->m_##CallbackName##Callback) \
        if (callback.second) \
            callback.second(callback.first); \

namespace AkVCam
{
    uint64_t id();
    std::string timeStamp();
    std::string replace(const std::string &str,
                        const std::string &from,
                        const std::string &to);
    std::wstring replace(const std::wstring &str,
                         const std::wstring &from,
                         const std::wstring &to);
    bool isEqualFile(const std::wstring &file1, const std::wstring &file2);
}

#endif // AKVCAMUTILS_UTILS_H
