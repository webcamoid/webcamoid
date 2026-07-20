/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef SWAPRBELEMENT_H
#define SWAPRBELEMENT_H

#include <iak/akvideoeffect.h>

class SwapRBElementPrivate;

class SwapRBElement: public AkVideoEffect
{
    Q_OBJECT

    public:
        SwapRBElement();
        ~SwapRBElement();

        Q_INVOKABLE bool init(QOpenGLBuffer *vbo,
                              QOpenGLBuffer *ibo) override;
        Q_INVOKABLE void process(QOpenGLFramebufferObject *inputFbo,
                                 QOpenGLFramebufferObject *&outputFbo,
                                 qint64 streamId,
                                 qreal pts) override;
        Q_INVOKABLE void uninit() override;

    private:
        SwapRBElementPrivate *d;
};

#endif // SWAPRBELEMENT_H
