/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef RENDERWIDGET_H
#define RENDERWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>

//#define TEXTURE_TARGET GL_TEXTURE_2D
#define TEXTURE_TARGET GL_TEXTURE_RECTANGLE_ARB

class RenderWidget: public QOpenGLWidget
{
    Q_OBJECT

    public:
        explicit RenderWidget();
        ~RenderWidget();

        Q_INVOKABLE GLuint texture() const;
        Q_INVOKABLE QImage grabFrame();

    protected:
        void initializeGL();
        void resizeGL(int width, int height);
        void paintGL();

    private:
        GLuint m_texture;
        bool m_initialized;
        QOpenGLFramebufferObject *m_fbo;

    public slots:
        void setTexture(GLuint texture);
};

#endif // RENDERWIDGET_H
