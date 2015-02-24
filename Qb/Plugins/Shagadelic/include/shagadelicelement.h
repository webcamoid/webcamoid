/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef SHAGADELICELEMENT_H
#define SHAGADELICELEMENT_H

#include <cmath>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class ShagadelicElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(quint32 mask
               READ mask
               WRITE setMask
               RESET resetMask
               NOTIFY maskChanged)

    public:
        explicit ShagadelicElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE quint32 mask() const;

    private:
        quint32 m_mask;
        int m_rx;
        int m_ry;
        int m_bx;
        int m_by;
        int m_rvx;
        int m_rvy;
        int m_bvx;
        int m_bvy;
        uchar m_phase;

        QByteArray m_ripple;
        QByteArray m_spiral;

        QSize m_curSize;
        QbElementPtr m_convert;

        QByteArray makeRipple(const QSize &size) const;
        QByteArray makeSpiral(const QSize &size) const;
        void init(const QSize &size);

    signals:
        void maskChanged();

    public slots:
        void setMask(quint32 mask);
        void resetMask();
        QbPacket iStream(const QbPacket &packet);
};

#endif // SHAGADELICELEMENT_H
