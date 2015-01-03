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

#ifndef LIFEELEMENT_H
#define LIFEELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class LifeElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)

    public:
        explicit LifeElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int threshold() const;

    private:
        int m_threshold;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_background;
        QImage m_diff;
        QImage m_diff2;
        QImage m_field;
        quint8 *m_field1;
        quint8 *m_field2;

        void createImg(QImage &src);
        QImage imageBgSubtractUpdateY(const QImage &src);
        QImage imageDiffFilter(const QImage &diff);

    signals:
        void thresholdChanged();

    public slots:
        void setThreshold(int threshold);
        void resetThreshold();
        void clearField();

        QbPacket iStream(const QbPacket &packet);
};

#endif // LIFEELEMENT_H
