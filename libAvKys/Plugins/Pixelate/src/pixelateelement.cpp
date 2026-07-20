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
#include <QMutex>

#include "pixelateelement.h"

class PixelateElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        QSize m_blockSize {8, 8};
        QMutex m_mutex;
};

PixelateElement::PixelateElement(): AkVideoEffect()
{
    this->d = new PixelateElementPrivate;
}

PixelateElement::~PixelateElement()
{
    delete this->d;
}

QSize PixelateElement::blockSize() const
{
    return this->d->m_blockSize;
}

bool PixelateElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
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
                                 GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
        prog->release();
        vao.release();
    };

    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Pixelate/share/shaders/pixelate.vert")
        || !this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                       ":/Pixelate/share/shaders/pixelate.frag")
        || !this->d->m_shader->link()) {
        qCritical() << "Pixelate shader link failed";

        return false;
    }

    setupVao(this->d->m_vao, vbo, ibo, this->d->m_shader);

    return true;
}

void PixelateElement::process(QOpenGLFramebufferObject *inputFbo,
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

    this->d->m_mutex.lock();
    QSize blockSize = this->d->m_blockSize;
    this->d->m_mutex.unlock();

    blockSize.setWidth(qMax(1, blockSize.width()));
    blockSize.setHeight(qMax(1, blockSize.height()));

    if (!outputFbo
        || outputFbo->width() != width
        || outputFbo->height() != height) {
        delete outputFbo;
        outputFbo = new QOpenGLFramebufferObject(width,
                                                 height,
                                                 QOpenGLFramebufferObjectFormat());
    }

    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTexture", 0);
    this->d->m_shader->setUniformValue("uBlockSize", (float)blockSize.width(), (float)blockSize.height());
    this->d->m_shader->setUniformValue("uOutputSize", (float)width, (float)height);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void PixelateElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString PixelateElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Pixelate/share/qml/main.qml");
}

void PixelateElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Pixelate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void PixelateElement::setBlockSize(const QSize &blockSize)
{
    if (blockSize == this->d->m_blockSize)
        return;

    this->d->m_mutex.lock();
    this->d->m_blockSize = blockSize;
    this->d->m_mutex.unlock();
    emit this->blockSizeChanged(blockSize);
}

void PixelateElement::resetBlockSize()
{
    this->setBlockSize({8, 8});
}

#include "moc_pixelateelement.cpp"
