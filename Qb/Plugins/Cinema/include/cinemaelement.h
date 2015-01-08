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

#ifndef CINEMAELEMENT_H
#define CINEMAELEMENT_H

#include <cmath>
#include <QColor>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class CinemaElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal stripSize
               READ stripSize
               WRITE setStripSize
               RESET resetStripSize
               NOTIFY stripSizeChanged)
    Q_PROPERTY(QRgb stripColor
               READ stripColor
               WRITE setStripColor
               RESET resetStripColor
               NOTIFY stripColorChanged)

    public:
        explicit CinemaElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE qreal stripSize() const;
        Q_INVOKABLE QRgb stripColor() const;

    private:
        qreal m_stripSize;
        QRgb m_stripColor;

        QbElementPtr m_convert;

    signals:
        void stripSizeChanged();
        void stripColorChanged();

    public slots:
        void setStripSize(qreal stripSize);
        void setStripColor(QRgb stripColor);
        void resetStripSize();
        void resetStripColor();

        QbPacket iStream(const QbPacket &packet);
};

#endif // CINEMAELEMENT_H
