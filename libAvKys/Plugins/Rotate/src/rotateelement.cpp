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

#include <QMutex>
#include <QQmlContext>
#include <QtMath>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

#include "rotateelement.h"

class RotateElementPrivate
{
    public:
        qreal m_angle {0.0};
        bool m_keep {false};
        QMutex m_mutex;
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
};

RotateElement::RotateElement(): AkVideoEffect()
{
    this->d = new RotateElementPrivate;
}

RotateElement::~RotateElement()
{
    delete this->d;
}

qreal RotateElement::angle() const
{
    return this->d->m_angle;
}

bool RotateElement::keep() const
{
    return this->d->m_keep;
}

bool RotateElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Rotate/share/shaders/rotate.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Rotate/share/shaders/rotate.frag"))
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

void RotateElement::process(QOpenGLFramebufferObject *inputFbo,
                            QOpenGLFramebufferObject *&outputFbo,
                            qint64 streamId,
                            qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shader || !this->m_gl || !inputFbo)
        return;

    const int srcWidth = inputFbo->width();
    const int srcHeight = inputFbo->height();

    if (srcWidth <= 0 || srcHeight <= 0)
        return;

    // Compute output size based on keep mode.
    int dstWidth = srcWidth;
    int dstHeight = srcHeight;

    if (!this->d->m_keep) {
        auto radians = this->d->m_angle * M_PI / 180.0;
        auto c = qAbs(qCos(radians));
        auto s = qAbs(qSin(radians));
        dstWidth  = qRound(srcWidth  * c + srcHeight * s);
        dstHeight = qRound(srcWidth  * s + srcHeight * c);
    }

    if (!outputFbo
        || outputFbo->width()  != dstWidth
        || outputFbo->height() != dstHeight) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(dstWidth, dstHeight, fmt);
    }

    outputFbo->bind();
    this->m_gl->glViewport(0, 0, dstWidth, dstHeight);
    this->m_gl->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTex", 0);
    this->d->m_shader->setUniformValue("uAngle", float(this->d->m_angle));
    this->d->m_shader->setUniformValue("uKeep", this->d->m_keep ? 1 : 0);
    this->d->m_shader->setUniformValue("uSrcWidth", srcWidth);
    this->d->m_shader->setUniformValue("uSrcHeight", srcHeight);
    this->d->m_shader->setUniformValue("uDstWidth", dstWidth);
    this->d->m_shader->setUniformValue("uDstHeight", dstHeight);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void RotateElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString RotateElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Rotate/share/qml/main.qml");
}

void RotateElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("Rotate", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void RotateElement::setAngle(qreal angle)
{
    if (qFuzzyCompare(this->d->m_angle, angle))
        return;

    this->d->m_angle = angle;
    emit this->angleChanged(angle);
}

void RotateElement::setKeep(bool keep)
{
    if (this->d->m_keep == keep)
        return;

    this->d->m_keep = keep;
    emit this->keepChanged(keep);
}

void RotateElement::resetAngle()
{
    this->setAngle(0.0);
}

void RotateElement::resetKeep()
{
    this->setKeep(false);
}

#include "moc_rotateelement.cpp"
