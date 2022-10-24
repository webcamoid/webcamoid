/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef AKVIDEOMIXER_H
#define AKVIDEOMIXER_H

#include <QObject>

#include "akcommons.h"

class AkVideoMixerPrivate;
class AkVideoPacket;

class AKCOMMONS_EXPORT AkVideoMixer: public QObject
{
    Q_OBJECT

    public:
        AkVideoMixer(QObject *parent=nullptr);
        AkVideoMixer(const AkVideoMixer &other);
        ~AkVideoMixer();
        AkVideoMixer &operator =(const AkVideoMixer &other);

        Q_INVOKABLE static QObject *create();

        Q_INVOKABLE bool begin(AkVideoPacket *packet);
        Q_INVOKABLE void end();
        Q_INVOKABLE void draw(const AkVideoPacket &packet);
        Q_INVOKABLE bool draw(int x, int y, const AkVideoPacket &packet);

    private:
        AkVideoMixerPrivate *d;

    public Q_SLOTS:
        void reset();
        static void registerTypes();
};

Q_DECLARE_METATYPE(AkVideoMixer)

#endif // AKFRAC_H
