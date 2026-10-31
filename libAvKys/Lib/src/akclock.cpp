/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <chrono>

#include "akclock.h"
#include "akfrac.h"

#define NSECS_PER_SEC 1000000000LL

qint64 AkClock::nsecs()
{
    using Clock = std::chrono::steady_clock;

    // Initialized once, thread-safe.
    static const auto origin = Clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()
                                                                - origin).count();
}

qint64 AkClock::nsecsToUnits(qint64 nsecs, qint64 den)
{
    if (den < 1)
        return 0;

    // Split in seconds and remainder to avoid overflowing qint64.
    auto sec = nsecs / NSECS_PER_SEC;
    auto rem = nsecs % NSECS_PER_SEC;

    return sec * den + rem * den / NSECS_PER_SEC;
}

qint64 AkClock::unitsToNsecs(qint64 units, qint64 den)
{
    if (den < 1)
        return 0;

    auto sec = units / den;
    auto rem = units % den;

    return sec * NSECS_PER_SEC + rem * NSECS_PER_SEC / den;
}

qint64 AkClock::ptsToNsecs(qint64 pts, const AkFrac &timeBase)
{
    return AkClock::unitsToNsecs(pts * timeBase.num(), timeBase.den());
}
