/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef EDGEELEMENT_H
#define EDGEELEMENT_H

#include <qrgb.h>
#include <iak/akvideoeffect.h>

class EdgeElementPrivate;

class EdgeElement: public AkVideoEffect
{
    Q_OBJECT
    Q_PROPERTY(bool canny
               READ canny
               WRITE setCanny
               RESET resetCanny
               NOTIFY cannyChanged)
    Q_PROPERTY(qreal thLow
               READ thLow
               WRITE setThLow
               RESET resetThLow
               NOTIFY thLowChanged)
    Q_PROPERTY(qreal thHi
               READ thHi
               WRITE setThHi
               RESET resetThHi
               NOTIFY thHiChanged)
    Q_PROPERTY(bool equalize
               READ equalize
               WRITE setEqualize
               RESET resetEqualize
               NOTIFY equalizeChanged)
    Q_PROPERTY(QRgb lineColor
               READ lineColor
               WRITE setLineColor
               RESET resetLineColor
               NOTIFY lineColorChanged)
    Q_PROPERTY(QRgb backgroundColor
               READ backgroundColor
               WRITE setBackgroundColor
               RESET resetBackgroundColor
               NOTIFY backgroundColorChanged)

    public:
        EdgeElement();
        ~EdgeElement();

        Q_INVOKABLE bool canny() const;
        Q_INVOKABLE qreal thLow() const;
        Q_INVOKABLE qreal thHi() const;
        Q_INVOKABLE bool equalize() const;
        Q_INVOKABLE QRgb lineColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE bool init(QOpenGLBuffer *vbo,
                              QOpenGLBuffer *ibo) override;
        Q_INVOKABLE void process(QOpenGLFramebufferObject *inputFbo,
                                 QOpenGLFramebufferObject *&outputFbo,
                                 qint64 streamId,
                                 qreal pts) override;
        Q_INVOKABLE void uninit() override;

    private:
        EdgeElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void cannyChanged(bool canny);
        void thLowChanged(qreal thLow);
        void thHiChanged(qreal thHi);
        void equalizeChanged(bool equalize);
        void lineColorChanged(QRgb lineColor);
        void backgroundColorChanged(QRgb backgroundColor);

    public slots:
        void setCanny(bool canny);
        void setThLow(qreal thLow);
        void setThHi(qreal thHi);
        void setEqualize(bool equalize);
        void setLineColor(QRgb lineColor);
        void setBackgroundColor(QRgb backgroundColor);
        void resetCanny();
        void resetThLow();
        void resetThHi();
        void resetEqualize();
        void resetLineColor();
        void resetBackgroundColor();
};

#endif // EDGEELEMENT_H
