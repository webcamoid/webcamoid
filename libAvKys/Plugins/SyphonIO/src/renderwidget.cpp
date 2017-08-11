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

#include <QtDebug>

#include "renderwidget.h"

#define TEXTURE_TARGET GL_TEXTURE_2D
//#define TEXTURE_TARGET GL_TEXTURE_RECTANGLE_ARB

RenderWidget::RenderWidget():
    QOpenGLWidget()
{
    this->m_initialized = false;
    this->m_fbo = NULL;
}

RenderWidget::~RenderWidget()
{
    if (this->m_fbo)
        delete this->m_fbo;
}

GLuint RenderWidget::texture() const
{
    return this->m_texture;
}

QImage RenderWidget::grabFrame()
{
    this->makeCurrent();

    if (!this->m_initialized) {
        this->initializeGL();
        this->resizeGL(this->width(), this->height());
    }

    if (!this->m_fbo
        || this->m_fbo->width() != this->width()
        || this->m_fbo->height() != this->height()) {
        if (this->m_fbo)
            delete this->m_fbo;

        this->m_fbo = new QOpenGLFramebufferObject(this->size());
        this->resizeGL(this->width(), this->height());
    }

    this->m_fbo->bind();
    this->paintGL();
    auto frame = this->m_fbo->toImage();
    this->m_fbo->release();
    this->m_fbo->bindDefault();
    this->doneCurrent();

    return frame;
}

void RenderWidget::initializeGL()
{
    glEnable(TEXTURE_TARGET);
    glDisable(GL_DEPTH_TEST);
}

void RenderWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void RenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(TEXTURE_TARGET, this->m_texture);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex2f(0, 0);

        glTexCoord2f(0, 1);
        glVertex2f(0, 1);

        glTexCoord2f(1, 1);
        glVertex2f(1, 1);

        glTexCoord2f(1, 0);
        glVertex2f(1, 0);
    glEnd();
}

void RenderWidget::setTexture(GLuint texture)
{
    this->m_texture = texture;
}
