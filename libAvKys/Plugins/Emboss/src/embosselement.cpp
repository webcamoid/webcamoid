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

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QQmlContext>
#include <QtMath>

#include "embosselement.h"

class EmbossElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        qreal m_factor {1.0};
        qreal m_bias {128.0};
};

EmbossElement::EmbossElement(): AkVideoEffect()
{
    this->d = new EmbossElementPrivate;
}

EmbossElement::~EmbossElement()
{
    delete this->d;
}

qreal EmbossElement::factor() const
{
    return this->d->m_factor;
}

qreal EmbossElement::bias() const
{
    return this->d->m_bias;
}

bool EmbossElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    auto setupVao = [this](QOpenGLVertexArrayObject &vao,
                           QOpenGLBuffer *vbo, QOpenGLBuffer *ibo,
                           QOpenGLShaderProgram *prog) {
        vao.create();
        vao.bind();
        vbo->bind();
        ibo->bind();
        prog->bind();
        prog->enableAttributeArray(prog->attributeLocation("aPos"));
        prog->setAttributeBuffer(prog->attributeLocation("aPos"),
                                 GL_FLOAT, 0, 3, 5 * sizeof(float));
        prog->enableAttributeArray(prog->attributeLocation("aTexCoord"));
        prog->setAttributeBuffer(prog->attributeLocation("aTexCoord"),
                                 GL_FLOAT,
                                 3 * sizeof(float),
                                 2,
                                 5 * sizeof(float));
        prog->release();
        vao.release();
    };

    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Emboss/share/shaders/emboss.vert")
        || !this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                       ":/Emboss/share/shaders/emboss.frag")
        || !this->d->m_shader->link()) {
        qCritical() << "Emboss shader link failed";

        return false;
    }

    setupVao(this->d->m_vao, vbo, ibo, this->d->m_shader);

    return true;
}

void EmbossElement::process(QOpenGLFramebufferObject *inputFbo,
                            QOpenGLFramebufferObject *&outputFbo,
                            qint64 streamId,
                            qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shader || !this->m_gl || !inputFbo)
        return;

    int width = inputFbo->width();
    int height = inputFbo->height();

    if (width <= 0 || height <= 0)
        return;

    // Crear FBO de salida si es necesario
    if (!outputFbo
        || outputFbo->width() != width
        || outputFbo->height() != height) {
        if (outputFbo)
            delete outputFbo;

        outputFbo = new QOpenGLFramebufferObject(width,
                                                 height,
                                                 QOpenGLFramebufferObjectFormat());
    }

    // Emboss effect render
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTexture", 0);
    this->d->m_shader->setUniformValue("uFactor", float(this->d->m_factor));
    this->d->m_shader->setUniformValue("uBias", float(this->d->m_bias / 255.0f));
    this->d->m_shader->setUniformValue("uTexelSize", 1.0f/width, 1.0f/height);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void EmbossElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString EmbossElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Emboss/share/qml/main.qml");
}

void EmbossElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Emboss", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void EmbossElement::setFactor(qreal factor)
{
    if (qFuzzyCompare(this->d->m_factor, factor))
        return;

    this->d->m_factor = factor;
    emit this->factorChanged(factor);
}

void EmbossElement::setBias(qreal bias)
{
    if (qFuzzyCompare(this->d->m_bias, bias))
        return;

    this->d->m_bias = bias;
    emit this->biasChanged(bias);
}

void EmbossElement::resetFactor()
{
    this->setFactor(1);
}

void EmbossElement::resetBias()
{
    this->setBias(128);
}

#include "moc_embosselement.cpp"
