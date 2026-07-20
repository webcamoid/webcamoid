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
#include <QOpenGLExtraFunctions>
#include <QQmlContext>

#include "vignetteelement.h"

class VignetteElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;

        // Vignette parameters
        QRgb m_color {qRgb(0, 0, 0)};
        qreal m_aspect {3.0 / 7.0};
        qreal m_scale {0.5};
        qreal m_softness {0.5};
};

VignetteElement::VignetteElement(): AkVideoEffect()
{
    this->d = new VignetteElementPrivate;
}

VignetteElement::~VignetteElement()
{
    delete this->d;
}

QRgb VignetteElement::color() const
{
    return this->d->m_color;
}

qreal VignetteElement::aspect() const
{
    return this->d->m_aspect;
}

qreal VignetteElement::scale() const
{
    return this->d->m_scale;
}

qreal VignetteElement::softness() const
{
    return this->d->m_softness;
}

bool VignetteElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Vignette/share/shaders/vignette.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Vignette/share/shaders/vignette.frag"))
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

void VignetteElement::process(QOpenGLFramebufferObject *inputFbo,
                              QOpenGLFramebufferObject *&outputFbo,
                              qint64 streamId,
                              qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shader || !this->m_gl || !inputFbo)
        return;

    const int width  = inputFbo->width();
    const int height = inputFbo->height();

    if (width <= 0 || height <= 0)
        return;

    // Reallocate the output FBO when the resolution changes.
    if (!outputFbo
        || outputFbo->width() != width
        || outputFbo->height() != height) {
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

    // Clamp parameters to valid ranges.
    qreal aspect = qBound(0.0, this->d->m_aspect, 1.0);
    qreal rho    = qBound(0.01, this->d->m_aspect, 0.99);
    qreal scale  = this->d->m_scale * qSqrt(1.0 / qPow(rho, 2) + 1.0 / qPow(1.0 - rho, 2));

    // Half-dimensions for ellipse radii (in normalised UV space, 0..1).
    // a and b are the ellipse semi-axes expressed as fractions of the
    // half-width / half-height respectively, so the shader can work directly
    // in UV coordinates centred on (0.5, 0.5).
    float a = static_cast<float>(qMax(0.01, scale * aspect         * 0.5));
    float b = static_cast<float>(qMax(0.01, scale * (1.0 - aspect) * 0.5));

    // Maximum radius to the corner of the UV rectangle (used to normalise k).
    float maxRadius = static_cast<float>(qSqrt(qPow(0.5 / a, 2) + qPow(0.5 / b, 2)));

    // Vignette colour + alpha (pre-multiplied alpha is NOT used here).
    float colorR = qRed(this->d->m_color)   / 255.0f;
    float colorG = qGreen(this->d->m_color) / 255.0f;
    float colorB = qBlue(this->d->m_color)  / 255.0f;
    float colorA = qAlpha(this->d->m_color) / 255.0f;

    float softness = static_cast<float>(2.0 * this->d->m_softness - 1.0);

    this->d->m_shader->setUniformValue("uA", a);
    this->d->m_shader->setUniformValue("uB", b);
    this->d->m_shader->setUniformValue("uMaxRadius", maxRadius);
    this->d->m_shader->setUniformValue("uColor", colorR, colorG, colorB, colorA);
    this->d->m_shader->setUniformValue("uSoftness", softness);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void VignetteElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString VignetteElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Vignette/share/qml/main.qml");
}

void VignetteElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Vignette", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void VignetteElement::setColor(QRgb color)
{
    if (this->d->m_color == color)
        return;

    this->d->m_color = color;
    emit this->colorChanged(color);
}

void VignetteElement::setAspect(qreal aspect)
{
    if (qFuzzyCompare(this->d->m_aspect, aspect))
        return;

    this->d->m_aspect = aspect;
    emit this->aspectChanged(aspect);
}

void VignetteElement::setScale(qreal scale)
{
    if (qFuzzyCompare(this->d->m_scale, scale))
        return;

    this->d->m_scale = scale;
    emit this->scaleChanged(scale);
}

void VignetteElement::setSoftness(qreal softness)
{
    if (qFuzzyCompare(this->d->m_softness, softness))
        return;

    this->d->m_softness = softness;
    emit this->softnessChanged(softness);
}

void VignetteElement::resetColor()
{
    this->setColor(qRgb(0, 0, 0));
}

void VignetteElement::resetAspect()
{
    this->setAspect(3.0 / 7.0);
}

void VignetteElement::resetScale()
{
    this->setScale(0.5);
}

void VignetteElement::resetSoftness()
{
    this->setSoftness(0.5);
}

#include "moc_vignetteelement.cpp"
