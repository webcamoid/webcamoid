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

#ifndef CONVERTIO_H
#define CONVERTIO_H

#include <qb.h>

extern "C"
{
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

class ConvertIO: public QObject
{
    Q_OBJECT

    Q_PROPERTY(int iWidth READ iWidth)
    Q_PROPERTY(int iHeight READ iHeight)
    Q_PROPERTY(PixelFormat iFormat READ iFormat)
    Q_PROPERTY(int oWidth READ oWidth)
    Q_PROPERTY(int oHeight READ oHeight)
    Q_PROPERTY(PixelFormat oFormat READ oFormat)
    Q_PROPERTY(QList<int> check READ check)

    public:
        explicit ConvertIO(QObject *parent=NULL);
        ConvertIO(const QbPacket &iPacket, const QbCaps &oCaps);
        ConvertIO(const ConvertIO &other);
        ConvertIO &operator =(const ConvertIO &other);

        Q_INVOKABLE int iWidth() const;
        Q_INVOKABLE int iHeight() const;
        Q_INVOKABLE PixelFormat iFormat() const;
        Q_INVOKABLE int oWidth() const;
        Q_INVOKABLE int oHeight() const;
        Q_INVOKABLE PixelFormat oFormat() const;
        Q_INVOKABLE QList<int> check() const;

    private:
        int m_iWidth;
        int m_iHeight;
        PixelFormat m_iFormat;
        int m_oWidth;
        int m_oHeight;
        PixelFormat m_oFormat;
};

#endif // CONVERTIO_H
