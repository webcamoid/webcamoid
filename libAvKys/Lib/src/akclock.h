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

#ifndef AKCLOCK_H
#define AKCLOCK_H

#include "akcommons.h"

class AkFrac;

/* Process-wide monotonic clock.
 *
 * Every element that timestamps the data sent to the outputs (recording,
 * streaming, etc.) must take its time from here, so the audio and the video
 * share the same time base and origin. The origin is fixed the first time the
 * clock is read and it never changes while the process is running.
 *
 * Sources (cameras, files, microphones) can keep their own clocks to pace
 * themselves, but the packets must be re-timed with this clock when they
 * leave the compositor (video) and the mixer (audio).
 */
class AKCOMMONS_EXPORT AkClock
{
    public:
        // Time in nanoseconds elapsed since the clock origin.
        static qint64 nsecs();

        // Convert nanoseconds to units of 1/den seconds (den > 0).
        static qint64 nsecsToUnits(qint64 nsecs, qint64 den);

        // Convert units of 1/den seconds to nanoseconds (den > 0).
        static qint64 unitsToNsecs(qint64 units, qint64 den);

        // Convert a pts expressed in the given time base to nanoseconds.
        static qint64 ptsToNsecs(qint64 pts, const AkFrac &timeBase);
};

#endif // AKCLOCK_H
