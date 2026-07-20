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

#include "adjusthslelement.h"

class AdjustHSLElementPrivate
{
    public:
        int m_hue {0};
        int m_saturation {0};
        int m_luminance {0};

        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
};

AdjustHSLElement::AdjustHSLElement(): AkVideoEffect()
{
    this->d = new AdjustHSLElementPrivate;
}

AdjustHSLElement::~AdjustHSLElement()
{
    delete this->d;
}

int AdjustHSLElement::hue() const
{
    return this->d->m_hue;
}

int AdjustHSLElement::saturation() const
{
    return this->d->m_saturation;
}

int AdjustHSLElement::luminance() const
{
    return this->d->m_luminance;
}

bool AdjustHSLElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/AdjustHSL/share/shaders/adjusthsl.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/AdjustHSL/share/shaders/adjusthsl.frag"))
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

void AdjustHSLElement::process(QOpenGLFramebufferObject *inputFbo,
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

    // Fast path: no adjustments, just blit
    if (this->d->m_hue == 0
        && this->d->m_saturation == 0
        && this->d->m_luminance == 0) {
        outputFbo->bind();
        this->m_gl->glViewport(0, 0, width, height);
        this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

        this->m_gl->glActiveTexture(GL_TEXTURE0);
        this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());

        this->d->m_vao.bind();
        this->d->m_shader->bind();
        this->d->m_shader->setUniformValue("uTex", 0);
        this->d->m_shader->setUniformValue("uHue", 0.0f);
        this->d->m_shader->setUniformValue("uSaturation", 0.0f);
        this->d->m_shader->setUniformValue("uLuminance", 0.0f);

        this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        this->d->m_shader->release();
        this->d->m_vao.release();
        outputFbo->release();

        return;
    }

    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTex", 0);

    // Convert int offsets to shader scale:
    // hue: degrees, saturation/luminance: [-255,255] -> [-1,1]
    this->d->m_shader->setUniformValue("uHue", float(this->d->m_hue));
    this->d->m_shader->setUniformValue("uSaturation", float(this->d->m_saturation) / 255.0f);
    this->d->m_shader->setUniformValue("uLuminance", float(this->d->m_luminance) / 255.0f);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void AdjustHSLElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString AdjustHSLElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)
    return QString("qrc:/AdjustHSL/share/qml/main.qml");
}

void AdjustHSLElement::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)
    context->setContextProperty("AdjustHSL", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void AdjustHSLElement::setHue(int hue)
{
    if (this->d->m_hue == hue)
        return;

    this->d->m_hue = hue;
    emit this->hueChanged(hue);
}

void AdjustHSLElement::setSaturation(int saturation)
{
    if (this->d->m_saturation == saturation)
        return;

    this->d->m_saturation = saturation;
    emit this->saturationChanged(saturation);
}

void AdjustHSLElement::setLuminance(int luminance)
{
    if (this->d->m_luminance == luminance)
        return;

    this->d->m_luminance = luminance;
    emit this->luminanceChanged(luminance);
}

void AdjustHSLElement::resetHue()
{
    this->setHue(0);
}

void AdjustHSLElement::resetSaturation()
{
    this->setSaturation(0);
}

void AdjustHSLElement::resetLuminance()
{
    this->setLuminance(0);
}

#include "moc_adjusthslelement.cpp"
