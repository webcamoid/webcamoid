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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *     Copyright (c) 2012, Tanner Helland
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *     AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *     IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *     FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *     DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlContext>
#include <QtMath>

#include "temperatureelement.h"

class TemperatureElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        qreal m_temperature {6500.0};
        qreal m_kr {1.0};
        qreal m_kg {1.0};
        qreal m_kb {1.0};

        inline void colorFromTemperature(qreal temperature,
                                         qreal *r,
                                         qreal *g,
                                         qreal *b) const;
        inline void updateCoefficients(qreal temperature);
};

TemperatureElement::TemperatureElement(): AkVideoEffect()
{
    this->d = new TemperatureElementPrivate;
    this->d->updateCoefficients(this->d->m_temperature);
}

TemperatureElement::~TemperatureElement()
{
    delete this->d;
}

qreal TemperatureElement::temperature() const
{
    return this->d->m_temperature;
}

bool TemperatureElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Temperature/share/shaders/temperature.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Temperature/share/shaders/temperature.frag"))
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

void TemperatureElement::process(QOpenGLFramebufferObject *inputFbo,
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
        || outputFbo->width()  != width
        || outputFbo->height() != height) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(width, height, fmt);
    }

    // Render
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    // Texture unit 0: source frame
    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    this->m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    this->m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    this->m_gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    this->d->m_shader->setUniformValue("uTex", 0);

    // Temperature coefficients
    this->d->m_shader->setUniformValue("uKr", float(this->d->m_kr));
    this->d->m_shader->setUniformValue("uKg", float(this->d->m_kg));
    this->d->m_shader->setUniformValue("uKb", float(this->d->m_kb));

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void TemperatureElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString TemperatureElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Temperature/share/qml/main.qml");
}

void TemperatureElement::controlInterfaceConfigure(QQmlContext *context,
                                                   const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Temperature", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void TemperatureElement::setTemperature(qreal temperature)
{
    if (qFuzzyCompare(this->d->m_temperature, temperature))
        return;

    this->d->m_temperature = temperature;
    this->d->updateCoefficients(temperature);
    emit this->temperatureChanged(temperature);
}

void TemperatureElement::resetTemperature()
{
    this->setTemperature(6500);
}

void TemperatureElementPrivate::colorFromTemperature(qreal temperature,
                                                     qreal *r,
                                                     qreal *g,
                                                     qreal *b) const
{
    // This algorithm was taken from here:
    // http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/

    // Temperature must fall between 1000 and 40000 degrees
    temperature = qBound<qreal>(1000.0, temperature, 40000.0);

    // All calculations require temperature / 100, so only do the conversion once
    temperature /= 100.0;

    if (temperature <= 66.0)
        *r = 1;
    else
        *r = 1.2929362 * qPow(temperature - 60.0, -0.1332047592);

    if (temperature <= 66.0)
        *g = 0.39008158 * qLn(temperature) - 0.63184144;
    else
        *g = 1.1298909 * qPow(temperature - 60, -0.0755148492);

    if (temperature >= 66)
        *b = 1;
    else if (temperature <= 19)
        *b = 0;
    else
        *b = 0.54320679 * qLn(temperature - 10) - 1.1962541;
}

void TemperatureElementPrivate::updateCoefficients(qreal temperature)
{
    this->colorFromTemperature(temperature,
                               &this->m_kr,
                               &this->m_kg,
                               &this->m_kb);
}

#include "moc_temperatureelement.cpp"
