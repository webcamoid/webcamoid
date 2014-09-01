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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef VIGNETTEELEMENT_H
#define VIGNETTEELEMENT_H

#include <QImage>
#include <qb.h>

class VignetteElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(float aspect READ aspect WRITE setAspect RESET resetAspect)
    Q_PROPERTY(float clearCenter READ clearCenter WRITE setClearCenter RESET resetClearCenter)
    Q_PROPERTY(float softness READ softness WRITE setSoftness RESET resetSoftness)

    public:
        explicit VignetteElement();
        Q_INVOKABLE float aspect() const;
        Q_INVOKABLE float clearCenter() const;
        Q_INVOKABLE float softness() const;

    private:
        float m_aspect;
        float m_clearCenter;
        float m_softness;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QVector<float> m_vignette;

        QVector<float> updateVignette(int width, int height);

    public slots:
        void setAspect(float aspect);
        void setClearCenter(float clearCenter);
        void setSoftness(float softness);
        void resetAspect();
        void resetClearCenter();
        void resetSoftness();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // VIGNETTEELEMENT_H
