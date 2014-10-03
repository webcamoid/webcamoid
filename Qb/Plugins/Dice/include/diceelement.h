/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef DICEELEMENT_H
#define DICEELEMENT_H

#include <qb.h>
#include <qbutils.h>

#define DICEDIR_UP    (char) 0
#define DICEDIR_RIGHT (char) 1
#define DICEDIR_DOWN  (char) 2
#define DICEDIR_LEFT  (char) 3

class DiceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int cubeBits READ cubeBits WRITE setCubeBits RESET resetCubeBits)

    public:
        explicit DiceElement();
        Q_INVOKABLE int cubeBits() const;

    private:
        int m_cubeBits;
        QByteArray m_diceMap;

        int m_curCubeBits;
        QSize m_curSize;
        QbElementPtr m_convert;

        QByteArray makeDiceMap(const QSize &size, int cubeBits) const;
        void init(const QSize &size);

    public slots:
        void setCubeBits(int cubeBits);
        void resetCubeBits();
        QbPacket iStream(const QbPacket &packet);
};

#endif // DICEELEMENT_H
