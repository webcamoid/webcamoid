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

#ifndef VIGNETTEELEMENT_H
#define VIGNETTEELEMENT_H

#include <cmath>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class VignetteElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(qreal aspect
               READ aspect
               WRITE setAspect
               RESET resetAspect
               NOTIFY aspectChanged)
    Q_PROPERTY(qreal clearCenter
               READ clearCenter
               WRITE setClearCenter
               RESET resetClearCenter
               NOTIFY clearCenterChanged)
    Q_PROPERTY(qreal softness
               READ softness
               WRITE setSoftness
               RESET resetSoftness
               NOTIFY softnessChanged)

    public:
        explicit VignetteElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE qreal aspect() const;
        Q_INVOKABLE qreal clearCenter() const;
        Q_INVOKABLE qreal softness() const;

    private:
        qreal m_aspect;
        qreal m_clearCenter;
        qreal m_softness;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QVector<qreal> m_vignette;

        QVector<qreal> updateVignette(int width, int height);

    signals:
        void aspectChanged();
        void clearCenterChanged();
        void softnessChanged();

    public slots:
        void setAspect(qreal aspect);
        void setClearCenter(qreal clearCenter);
        void setSoftness(qreal softness);
        void resetAspect();
        void resetClearCenter();
        void resetSoftness();
        QbPacket iStream(const QbPacket &packet);
};

#endif // VIGNETTEELEMENT_H
