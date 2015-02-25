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

#ifndef EDGEELEMENT_H
#define EDGEELEMENT_H

#include <cmath>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class EdgeElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(bool equalize
               READ equalize
               WRITE setEqualize
               RESET resetEqualize
               NOTIFY equalizeChanged)
    Q_PROPERTY(bool invert
               READ invert
               WRITE setInvert
               RESET resetInvert
               NOTIFY invertChanged)

    public:
        explicit EdgeElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE bool equalize() const;
        Q_INVOKABLE bool invert() const;

    private:
        bool m_equalize;
        bool m_invert;
        QbElementPtr m_convert;

    signals:
        void equalizeChanged();
        void invertChanged();

    public slots:
        void setEqualize(bool equalize);
        void setInvert(bool invert);
        void resetEqualize();
        void resetInvert();
        QbPacket iStream(const QbPacket &packet);
};

#endif // EDGEELEMENT_H
