/* Webcamoid, camera capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QQmlContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

#include "gammaelement.h"

class GammaElementPrivate
{
    public:
        int m_gamma {0};
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
};

GammaElement::GammaElement(): AkVideoEffect()
{
    this->d = new GammaElementPrivate;
}

GammaElement::~GammaElement()
{
    delete this->d;
}

int GammaElement::gamma() const
{
    return this->d->m_gamma;
}

bool GammaElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Gamma/share/shaders/gamma.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Gamma/share/shaders/gamma.frag"))
        return false;

    if (!this->d->m_shader->link())
        return false;

    this->d->m_vao.create();
    this->d->m_vao.bind();
    vbo->bind();
    ibo->bind();

    this->d->m_shader->bind();
    int posAttr = this->d->m_shader->attributeLocation("aPos");
    this->d->m_shader->enableAttributeArray(posAttr);
    this->d->m_shader->setAttributeBuffer(posAttr,
                                          GL_FLOAT,
                                          0,
                                          3,
                                          5 * sizeof(float));

    int texAttr = this->d->m_shader->attributeLocation("aTexCoord");
    this->d->m_shader->enableAttributeArray(texAttr);
    this->d->m_shader->setAttributeBuffer(texAttr,
                                          GL_FLOAT,
                                          3 * sizeof(float),
                                          2,
                                          5 * sizeof(float));
    this->d->m_shader->release();
    this->d->m_vao.release();

    return true;
}

void GammaElement::process(QOpenGLFramebufferObject *inputFbo,
                           QOpenGLFramebufferObject *&outputFbo,
                           qint64 streamId,
                           qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shader || !this->m_gl || !inputFbo)
        return;

    const int width = inputFbo->width();
    const int height = inputFbo->height();

    if (width <= 0 || height <= 0)
        return;

    if (!outputFbo || outputFbo->width() != width || outputFbo->height() != height) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(width, height, fmt);
    }

    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTex", 0);

    // Convert gamma from [-255, 255] to [-1, 1] for the shader
    int gamma = qBound(-255, this->d->m_gamma, 255);
    this->d->m_shader->setUniformValue("uGamma", float(gamma) / 255.0f);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void GammaElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString GammaElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Gamma/share/qml/main.qml");
}

void GammaElement::controlInterfaceConfigure(QQmlContext *context,
                                             const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("Gamma", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void GammaElement::setGamma(int gamma)
{
    if (this->d->m_gamma == gamma)
        return;

    this->d->m_gamma = gamma;
    emit this->gammaChanged(gamma);
}

void GammaElement::resetGamma()
{
    this->setGamma(0);
}

#include "moc_gammaelement.cpp"
