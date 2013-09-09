/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <QtCore>

// no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MIN 0.01

// AV sync correction is done if above the maximum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX 0.1

// If a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

// no AV correction is done if too big error
#define AV_NOSYNC_THRESHOLD 10.0

// maximum audio speed change to get correct sync
#define SAMPLE_CORRECTION_PERCENT_MAX 0.1

// we use about AUDIO_DIFF_AVG_NB A-V differences to make the average
#define AUDIO_DIFF_AVG_NB 20

// polls for possible required screen refresh at least this often, should be less than 1/fps
#define REFRESH_RATE 0.01

class Clock: public QObject
{
    Q_OBJECT

    Q_PROPERTY(double clock READ clock WRITE setClock)

    public:
        explicit Clock(QObject *parent=NULL);
        Clock(const Clock &other);
        Clock &operator =(const Clock &other);

        Q_INVOKABLE double clock() const;

    private:
        double m_pts;
        double m_ptsDrift;

    public slots:
        void setClockAt(double pts, double time);
        void setClock(double pts);
        void syncTo(const Clock &slave);
};

#endif // CLOCK_H
