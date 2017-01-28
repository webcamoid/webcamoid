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

#ifndef WAITCONDITION_H
#define WAITCONDITION_H

#include "mutex.h"

class WaitConditionPrivate;

class WaitCondition
{
    public:
        explicit WaitCondition(const std::wstring &name=std::wstring());
        WaitCondition(Mutex *mutex);
        WaitCondition(const WaitCondition &other);
        ~WaitCondition();
        WaitCondition &operator =(const WaitCondition &other);

        bool wait(Mutex *mutex, int timeout=ULONG_MAX);
        void wakeAll();

    private:
        WaitConditionPrivate *d;
};

#endif // WAITCONDITION_H
