/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef SHAGADELICELEMENT_H
#define SHAGADELICELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>
#include <akutils.h>

class ShagadelicElement: public AkElement
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

        QImage m_ripple;
        QImage m_spiral;
        QSize m_curSize;

        QImage makeRipple(const QSize &size) const;
        QImage makeSpiral(const QSize &size) const;
        void init(const QSize &size);

    signals:
        void maskChanged(quint32 mask);

    public slots:
        void setMask(quint32 mask);
        void resetMask();
        AkPacket iStream(const AkPacket &packet);
};

#endif // SHAGADELICELEMENT_H
