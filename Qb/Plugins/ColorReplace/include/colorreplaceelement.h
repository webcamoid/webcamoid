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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef COLORREPLACEELEMENT_H
#define COLORREPLACEELEMENT_H

#include <cmath>
#include <qrgb.h>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class ColorReplaceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QRgb from
               READ from
               WRITE setFrom
               RESET resetFrom
               NOTIFY fromChanged)
    Q_PROPERTY(QRgb to
               READ to
               WRITE setTo
               RESET resetTo
               NOTIFY toChanged)
    Q_PROPERTY(qreal radius
               READ radius
               WRITE setRadius
               RESET resetRadius
               NOTIFY radiusChanged)
    Q_PROPERTY(bool disable
               READ disable
               WRITE setDisable
               RESET resetDisable
               NOTIFY disableChanged)

    public:
        explicit ColorReplaceElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QRgb from() const;
        Q_INVOKABLE QRgb to() const;
        Q_INVOKABLE qreal radius() const;
        Q_INVOKABLE bool disable() const;

    private:
        QRgb m_from;
        QRgb m_to;
        qreal m_radius;
        bool m_disable;

        QbElementPtr m_convert;

    signals:
        void fromChanged();
        void toChanged();
        void radiusChanged();
        void disableChanged();

    public slots:
        void setFrom(QRgb from);
        void setTo(QRgb to);
        void setRadius(qreal radius);
        void setDisable(bool disable);
        void resetFrom();
        void resetTo();
        void resetRadius();
        void resetDisable();
        QbPacket iStream(const QbPacket &packet);
};

#endif // COLORREPLACEELEMENT_H
