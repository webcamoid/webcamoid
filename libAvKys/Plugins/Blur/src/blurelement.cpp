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

#include <QtMath>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlContext>

#include "blurelement.h"

#define MAX_RADIUS 16

class BlurElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shaderH {nullptr};    // Horizontal pass
        QOpenGLShaderProgram *m_shaderV {nullptr};    // Vertical pass
        QOpenGLShaderProgram *m_shaderCopy {nullptr}; // radius == 0 fast path
        QOpenGLVertexArrayObject m_vao;

        // Intermediate FBO that holds the result of the horizontal pass
        // before the vertical pass consumes it.
        QOpenGLFramebufferObject *m_intermediateFbo {nullptr};

        int m_radius {8};

        void setupAttributes(QOpenGLShaderProgram *shader,
                             QOpenGLBuffer *vbo,
                             QOpenGLBuffer *ibo) const;
};

BlurElement::BlurElement(): AkVideoEffect()
{
    this->d = new BlurElementPrivate;
}

BlurElement::~BlurElement()
{
    delete this->d;
}

int BlurElement::radius() const
{
    return this->d->m_radius;
}

bool BlurElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shaderH = new QOpenGLShaderProgram;

    if (!this->d->m_shaderH->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                     ":/Blur/share/shaders/blur.vert"))
        return false;

    if (!this->d->m_shaderH->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                     ":/Blur/share/shaders/blurh.frag"))
        return false;

    // Pin attribute locations so they match the vertical-pass program below;
    // a single VAO is reused for both draw calls, and the driver is not
    // required to assign the same locations to two separately linked
    // programs unless we fix them explicitly.
    this->d->m_shaderH->bindAttributeLocation("aPos", 0);
    this->d->m_shaderH->bindAttributeLocation("aTexCoord", 1);

    if (!this->d->m_shaderH->link())
        return false;

    this->d->m_shaderV = new QOpenGLShaderProgram;

    if (!this->d->m_shaderV->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                     ":/Blur/share/shaders/blur.vert"))
        return false;

    if (!this->d->m_shaderV->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                     ":/Blur/share/shaders/blurv.frag"))
        return false;

    this->d->m_shaderV->bindAttributeLocation("aPos", 0);
    this->d->m_shaderV->bindAttributeLocation("aTexCoord", 1);

    if (!this->d->m_shaderV->link())
        return false;

    this->d->m_shaderCopy = new QOpenGLShaderProgram;

    if (!this->d->m_shaderCopy->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                        ":/Blur/share/shaders/blur.vert"))
        return false;

    if (!this->d->m_shaderCopy->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                        ":/Blur/share/shaders/blurcopy.frag"))
        return false;

    this->d->m_shaderCopy->bindAttributeLocation("aPos", 0);
    this->d->m_shaderCopy->bindAttributeLocation("aTexCoord", 1);

    if (!this->d->m_shaderCopy->link())
        return false;

    this->d->m_vao.create();
    this->d->m_vao.bind();
    vbo->bind();
    ibo->bind();

    // Both shaders now expose aPos/aTexCoord at the same fixed locations
    // (0/1), so the attribute setup below is valid for both draw calls that
    // reuse this VAO.
    this->d->setupAttributes(this->d->m_shaderH, vbo, ibo);

    this->d->m_vao.release();

    return true;
}

void BlurElement::process(QOpenGLFramebufferObject *inputFbo,
                          QOpenGLFramebufferObject *&outputFbo,
                          qint64 streamId,
                          qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shaderH
        || !this->d->m_shaderV
        || !this->d->m_shaderCopy
        || !this->m_gl
        || !inputFbo)
        return;

    const int width  = inputFbo->width();
    const int height = inputFbo->height();

    if (width <= 0 || height <= 0)
        return;

    // Reallocate the output FBO when the resolution changes.
    if (!outputFbo
        || outputFbo->width()  != width
        || outputFbo->height() != height) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(width, height, fmt);
    }

    int radius = qBound(0, this->d->m_radius, MAX_RADIUS);

    this->d->m_vao.bind();

    // Fast path: radius == 0 means the blur is a no-op. Don't repurpose the
    // loop-based blurh/blurv shaders as a passthrough - even with uRadius=0
    // that still costs two FBO round-trips plus two 21-iteration loops of
    // skipped taps. Just copy inputFbo straight into outputFbo with a
    // single trivial draw instead, and skip the intermediate FBO entirely.
    if (radius == 0) {
        outputFbo->bind();
        this->m_gl->glViewport(0, 0, width, height);
        this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

        this->d->m_shaderCopy->bind();

        this->m_gl->glActiveTexture(GL_TEXTURE0);
        this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
        this->d->m_shaderCopy->setUniformValue("uTex", 0);

        this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        this->d->m_shaderCopy->release();
        outputFbo->release();

        this->d->m_vao.release();

        return;
    }

    // Reallocate the intermediate FBO (horizontal pass target) when the
    // resolution changes.
    if (!this->d->m_intermediateFbo
        || this->d->m_intermediateFbo->width()  != width
        || this->d->m_intermediateFbo->height() != height) {
        delete this->d->m_intermediateFbo;
        QOpenGLFramebufferObjectFormat fmt;
        this->d->m_intermediateFbo =
                new QOpenGLFramebufferObject(width, height, fmt);
    }

    // Horizontal blur

    this->d->m_intermediateFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_shaderH->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shaderH->setUniformValue("uTex", 0);
    this->d->m_shaderH->setUniformValue("uTexelWidth", float(1.0 / width));
    this->d->m_shaderH->setUniformValue("uRadius", radius);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shaderH->release();
    this->d->m_intermediateFbo->release();

    // Vertical blur

    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_shaderV->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, this->d->m_intermediateFbo->texture());
    this->d->m_shaderV->setUniformValue("uTex", 0);
    this->d->m_shaderV->setUniformValue("uTexelHeight", float(1.0 / height));
    this->d->m_shaderV->setUniformValue("uRadius", radius);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shaderV->release();
    outputFbo->release();

    this->d->m_vao.release();
}

void BlurElement::uninit()
{
    if (this->d->m_intermediateFbo) {
        delete this->d->m_intermediateFbo;
        this->d->m_intermediateFbo = nullptr;
    }

    this->d->m_vao.destroy();

    if (this->d->m_shaderH) {
        this->d->m_shaderH->removeAllShaders();
        delete this->d->m_shaderH;
        this->d->m_shaderH = nullptr;
    }

    if (this->d->m_shaderV) {
        this->d->m_shaderV->removeAllShaders();
        delete this->d->m_shaderV;
        this->d->m_shaderV = nullptr;
    }

    if (this->d->m_shaderCopy) {
        this->d->m_shaderCopy->removeAllShaders();
        delete this->d->m_shaderCopy;
        this->d->m_shaderCopy = nullptr;
    }
}

QString BlurElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Blur/share/qml/main.qml");
}

void BlurElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Blur", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void BlurElement::setRadius(int radius)
{
    if (radius == this->d->m_radius)
        return;

    this->d->m_radius = radius;
    emit this->radiusChanged(radius);
}

void BlurElement::resetRadius()
{
    this->setRadius(8);
}

void BlurElementPrivate::setupAttributes(QOpenGLShaderProgram *shader,
                                         QOpenGLBuffer *vbo,
                                         QOpenGLBuffer *ibo) const
{
    Q_UNUSED(ibo)

    shader->bind();

    int posAttr = shader->attributeLocation("aPos");
    shader->enableAttributeArray(posAttr);
    shader->setAttributeBuffer(posAttr,
                               GL_FLOAT,
                               0,
                               3,
                               5 * sizeof(float));

    int texAttr = shader->attributeLocation("aTexCoord");
    shader->enableAttributeArray(texAttr);
    shader->setAttributeBuffer(texAttr,
                               GL_FLOAT,
                               3 * sizeof(float),
                               2,
                               5 * sizeof(float));

    shader->release();
}

#include "moc_blurelement.cpp"
