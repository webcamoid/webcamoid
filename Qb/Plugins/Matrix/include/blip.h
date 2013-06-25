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

#ifndef BLIP_H
#define BLIP_H

#include <QtCore>

class Blip: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(int y READ y WRITE setY RESET resetY)
    Q_PROPERTY(int timer READ timer WRITE setTimer RESET resetTimer)
    Q_PROPERTY(int speed READ speed WRITE setSpeed RESET resetSpeed)

    public:
        explicit Blip(QObject *parent=NULL);
        Blip(const Blip &other);
        Blip &operator =(const Blip &other);

        Q_INVOKABLE int mode() const;
        Q_INVOKABLE int y() const;
        Q_INVOKABLE int timer() const;
        Q_INVOKABLE int speed() const;

    private:
        int m_mode;
        int m_y;
        int m_timer;
        int m_speed;

    public slots:
        void setMode(int mode);
        void setY(int y);
        void setTimer(int timer);
        void setSpeed(int speed);
        void resetMode();
        void resetY();
        void resetTimer();
        void resetSpeed();
};

#endif // BLIP_H
