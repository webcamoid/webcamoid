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
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlContext>

#include "changehslelement.h"

class ChangeHSLElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        QVector<qreal> m_kernel;
};

ChangeHSLElement::ChangeHSLElement(): AkVideoEffect()
{
    this->d = new ChangeHSLElementPrivate;
    this->d->m_kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };
}

ChangeHSLElement::~ChangeHSLElement()
{
    delete this->d;
}

QVariantList ChangeHSLElement::kernel() const
{
    QVariantList kernel;

    for (auto &e : this->d->m_kernel)
        kernel << e;

    return kernel;
}

bool ChangeHSLElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/ChangeHSL/share/shaders/changehsl.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/ChangeHSL/share/shaders/changehsl.frag"))
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

void ChangeHSLElement::process(QOpenGLFramebufferObject *inputFbo,
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

    // Build kernel row values with fallback to identity
    float matrix[12] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };
    auto matrixSize = qMin(this->d->m_kernel.size(), 12);

    for (int i = 0; i < matrixSize; ++i)
        matrix[i] = static_cast<float>(this->d->m_kernel[i]);

    this->d->m_shader->setUniformValue("uKernelRow0", QVector4D(matrix[0], matrix[1], matrix[2] , matrix[3] ));
    this->d->m_shader->setUniformValue("uKernelRow1", QVector4D(matrix[4], matrix[5], matrix[6] , matrix[7] ));
    this->d->m_shader->setUniformValue("uKernelRow2", QVector4D(matrix[8], matrix[9], matrix[10], matrix[11]));

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void ChangeHSLElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString ChangeHSLElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)
    return QString("qrc:/ChangeHSL/share/qml/main.qml");
}

void ChangeHSLElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("ChangeHSL", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void ChangeHSLElement::setKernel(const QVariantList &kernel)
{
    QVector<qreal> k;

    for (const QVariant &e: kernel)
        k << e.toReal();

    if (this->d->m_kernel == k)
        return;

    this->d->m_kernel = k;
    emit this->kernelChanged(this->kernel());
}

void ChangeHSLElement::resetKernel()
{
    static const QVariantList kernel = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0
    };

    this->setKernel(kernel);
}

#include "moc_changehslelement.cpp"
