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

#ifndef TEMPERATUREELEMENT_H
#define TEMPERATUREELEMENT_H

#include <iak/akvideoeffect.h>

class TemperatureElementPrivate;

class TemperatureElement: public AkVideoEffect
{
    Q_OBJECT
    Q_PROPERTY(qreal temperature
               READ temperature
               WRITE setTemperature
               RESET resetTemperature
               NOTIFY temperatureChanged)

    public:
        TemperatureElement();
        ~TemperatureElement();

        Q_INVOKABLE qreal temperature() const;
        Q_INVOKABLE bool init(QOpenGLBuffer *vbo,
                              QOpenGLBuffer *ibo) override;
        Q_INVOKABLE void process(QOpenGLFramebufferObject *inputFbo,
                                 QOpenGLFramebufferObject *&outputFbo,
                                 qint64 streamId,
                                 qreal pts) override;
        Q_INVOKABLE void uninit() override;

    private:
        TemperatureElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void temperatureChanged(qreal temperature);

    public slots:
        void setTemperature(qreal temperature);
        void resetTemperature();
};

#endif // TEMPERATUREELEMENT_H
