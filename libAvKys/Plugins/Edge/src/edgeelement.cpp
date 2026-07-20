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

#include "edgeelement.h"

class EdgeElementPrivate
{
    public:
        // Downscale PASS (for histogram)
        QOpenGLShaderProgram     *m_downscaleShader {nullptr};
        QOpenGLVertexArrayObject  m_downscaleVao;
        QOpenGLFramebufferObject *m_histFbo {nullptr};

        // Equalization pass (optional)
        QOpenGLShaderProgram     *m_eqShader {nullptr};
        QOpenGLVertexArrayObject  m_eqVao;
        QOpenGLFramebufferObject *m_eqFbo {nullptr}; // equalized image

        // Sobel pass
        QOpenGLShaderProgram     *m_sobelShader {nullptr};
        QOpenGLVertexArrayObject  m_sobelVao;
        QOpenGLFramebufferObject *m_gradFbo {nullptr}; // magnitude (R) + direction (G)

        // Non-maximum suppression pass (only if canny)
        QOpenGLShaderProgram     *m_nmsShader {nullptr};
        QOpenGLVertexArrayObject  m_nmsVao;
        QOpenGLFramebufferObject *m_nmsFbo {nullptr};

        // Threshold pass
        QOpenGLShaderProgram     *m_threshShader {nullptr};
        QOpenGLVertexArrayObject  m_threshVao;

        // Parameters
        qreal m_thLow {0.1};
        qreal m_thHi {0.9};
        bool m_canny {false};
        bool m_equalize {false};
        QRgb m_lineColor {qRgb(255, 255, 255)};
        QRgb m_backgroundColor {qRgb(0, 0, 0)};
};

EdgeElement::EdgeElement(): AkVideoEffect()
{
    this->d = new EdgeElementPrivate;
}

EdgeElement::~EdgeElement()
{
    delete this->d;
}

bool EdgeElement::canny() const
{
    return this->d->m_canny;
}

qreal EdgeElement::thLow() const
{
    return this->d->m_thLow;
}

qreal EdgeElement::thHi() const
{
    return this->d->m_thHi;
}

bool EdgeElement::equalize() const
{
    return this->d->m_equalize;
}

QRgb EdgeElement::lineColor() const
{
    return this->d->m_lineColor;
}

QRgb EdgeElement::backgroundColor() const
{
    return this->d->m_backgroundColor;
}

bool EdgeElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    auto setupVao = [this](QOpenGLVertexArrayObject &vao,
                           QOpenGLBuffer *vbo,
                           QOpenGLBuffer *ibo) {
        vao.create();
        vao.bind();
        vbo->bind();
        ibo->bind();
    };

    // Helper to add shader and link
    auto createShader = [](const QString &vert,
                           const QString &frag) -> QOpenGLShaderProgram * {
        auto prog = new QOpenGLShaderProgram;

        if (!prog->addShaderFromSourceFile(QOpenGLShader::Vertex, vert)
            || !prog->addShaderFromSourceFile(QOpenGLShader::Fragment, frag)
            || !prog->link()) {
            delete prog;

            return nullptr;
        }

        return prog;
    };

    // Equalization shader
    this->d->m_eqShader = createShader(":/Edge/share/shaders/edge.vert",
                                       ":/Edge/share/shaders/equalize.frag");

    if (!this->d->m_eqShader)
        return false;

    setupVao(this->d->m_eqVao, vbo, ibo);
    this->d->m_eqShader->bind();
    this->d->m_eqShader->enableAttributeArray(this->d->m_eqShader->attributeLocation("aPos"));
    this->d->m_eqShader->setAttributeBuffer(this->d->m_eqShader->attributeLocation("aPos"),
                                            GL_FLOAT, 0, 3, 5 * sizeof(float));
    this->d->m_eqShader->enableAttributeArray(this->d->m_eqShader->attributeLocation("aTexCoord"));
    this->d->m_eqShader->setAttributeBuffer(this->d->m_eqShader->attributeLocation("aTexCoord"),
                                            GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    this->d->m_eqShader->release();
    this->d->m_eqVao.release();

    // Sobel shader
    this->d->m_sobelShader = createShader(":/Edge/share/shaders/edge.vert",
                                          ":/Edge/share/shaders/sobel.frag");

    if (!this->d->m_sobelShader)
        return false;

    setupVao(this->d->m_sobelVao, vbo, ibo);
    this->d->m_sobelShader->bind();
    this->d->m_sobelShader->enableAttributeArray(this->d->m_sobelShader->attributeLocation("aPos"));
    this->d->m_sobelShader->setAttributeBuffer(this->d->m_sobelShader->attributeLocation("aPos"),
                                               GL_FLOAT, 0, 3, 5 * sizeof(float));
    this->d->m_sobelShader->enableAttributeArray(this->d->m_sobelShader->attributeLocation("aTexCoord"));
    this->d->m_sobelShader->setAttributeBuffer(this->d->m_sobelShader->attributeLocation("aTexCoord"),
                                               GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    this->d->m_sobelShader->release();
    this->d->m_sobelVao.release();

    // NMS shader
    this->d->m_nmsShader = createShader(":/Edge/share/shaders/edge.vert",
                                        ":/Edge/share/shaders/nms.frag");

    if (!this->d->m_nmsShader)
        return false;

    setupVao(this->d->m_nmsVao, vbo, ibo);
    this->d->m_nmsShader->bind();
    this->d->m_nmsShader->enableAttributeArray(this->d->m_nmsShader->attributeLocation("aPos"));
    this->d->m_nmsShader->setAttributeBuffer(this->d->m_nmsShader->attributeLocation("aPos"),
                                             GL_FLOAT, 0, 3, 5 * sizeof(float));
    this->d->m_nmsShader->enableAttributeArray(this->d->m_nmsShader->attributeLocation("aTexCoord"));
    this->d->m_nmsShader->setAttributeBuffer(this->d->m_nmsShader->attributeLocation("aTexCoord"),
                                             GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    this->d->m_nmsShader->release();
    this->d->m_nmsVao.release();

    // Threshold shader
    this->d->m_threshShader = createShader(":/Edge/share/shaders/edge.vert",
                                           ":/Edge/share/shaders/threshold.frag");

    if (!this->d->m_threshShader)
        return false;

    setupVao(this->d->m_threshVao, vbo, ibo);
    this->d->m_threshShader->bind();
    this->d->m_threshShader->enableAttributeArray(this->d->m_threshShader->attributeLocation("aPos"));
    this->d->m_threshShader->setAttributeBuffer(this->d->m_threshShader->attributeLocation("aPos"),
                                                GL_FLOAT, 0, 3, 5 * sizeof(float));
    this->d->m_threshShader->enableAttributeArray(this->d->m_threshShader->attributeLocation("aTexCoord"));
    this->d->m_threshShader->setAttributeBuffer(this->d->m_threshShader->attributeLocation("aTexCoord"),
                                                GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    this->d->m_threshShader->release();
    this->d->m_threshVao.release();

    // Downscale shader (for histogram)
    this->d->m_downscaleShader = createShader(":/Edge/share/shaders/edge.vert",
                                              ":/Edge/share/shaders/downscale.frag");
    if (!this->d->m_downscaleShader)
        return false;

    setupVao(this->d->m_downscaleVao, vbo, ibo);
    this->d->m_downscaleShader->bind();
    this->d->m_downscaleShader->enableAttributeArray(this->d->m_downscaleShader->attributeLocation("aPos"));
    this->d->m_downscaleShader->setAttributeBuffer(this->d->m_downscaleShader->attributeLocation("aPos"),
                                                   GL_FLOAT, 0, 3, 5 * sizeof(float));
    this->d->m_downscaleShader->enableAttributeArray(this->d->m_downscaleShader->attributeLocation("aTexCoord"));
    this->d->m_downscaleShader->setAttributeBuffer(this->d->m_downscaleShader->attributeLocation("aTexCoord"),
                                                   GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    this->d->m_downscaleShader->release();
    this->d->m_downscaleVao.release();

    return true;
}

void EdgeElement::process(QOpenGLFramebufferObject *inputFbo,
                          QOpenGLFramebufferObject *&outputFbo,
                          qint64 streamId,
                          qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_sobelShader || !this->m_gl || !inputFbo)
        return;

    int width = inputFbo->width();
    int height = inputFbo->height();
    if (width <= 0 || height <= 0)
        return;

    if (!outputFbo
        || outputFbo->width() != width
        || outputFbo->height() != height) {
        delete outputFbo;
        outputFbo = new QOpenGLFramebufferObject(width, height,
                                                 QOpenGLFramebufferObjectFormat());
    }

    auto sourceFbo = inputFbo;

    // Optional equalization step
    if (this->d->m_equalize) {
        // 1. Downscale to 256x256 to compute min/max
        const int histW = 256, histH = 256;

        if (!this->d->m_histFbo
            || this->d->m_histFbo->width() != histW
            || this->d->m_histFbo->height() != histH) {
            delete this->d->m_histFbo;
            QOpenGLFramebufferObjectFormat fmt;
            this->d->m_histFbo = new QOpenGLFramebufferObject(histW, histH, fmt);
        }

        this->d->m_histFbo->bind();
        this->m_gl->glViewport(0, 0, histW, histH);
        this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

        // Downscale to 256x256 using the dedicated VAO and shader
        this->d->m_downscaleVao.bind();
        this->d->m_downscaleShader->bind();
        this->m_gl->glActiveTexture(GL_TEXTURE0);
        this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
        this->d->m_downscaleShader->setUniformValue("uTex", 0);
        this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        this->d->m_downscaleShader->release();
        this->d->m_downscaleVao.release();

        // Read pixels
        QVector<quint8> pixels(histW * histH * 4);
        this->m_gl->glReadPixels(0, 0, histW, histH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        this->d->m_histFbo->release();

        // Compute min/max luminance
        float minL = 1.0f, maxL = 0.0f;

        for (int y = 0; y < histH; ++y) {
            auto row = pixels.constData() + y * histW * 4;

            for (int x = 0; x < histW; ++x) {
                auto p = row + x * 4;
                float luma = (p[0] * 299 + p[1] * 587 + p[2] * 114) / (255.0f * 1000.0f);

                if (luma < minL)
                    minL = luma;

                if (luma > maxL)
                    maxL = luma;
            }
        }

        if (maxL - minL < 0.001f) {
            minL = 0.0f;
            maxL = 1.0f;
        }

        // 2. Render equalized image to a temporary FBO
        if (!this->d->m_eqFbo
            || this->d->m_eqFbo->width() != width
            || this->d->m_eqFbo->height() != height) {
            delete this->d->m_eqFbo;
            QOpenGLFramebufferObjectFormat fmt;
            this->d->m_eqFbo = new QOpenGLFramebufferObject(width, height, fmt);
        }

        this->d->m_eqFbo->bind();
        this->m_gl->glViewport(0, 0, width, height);
        this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

        this->d->m_eqVao.bind();
        this->d->m_eqShader->bind();
        this->m_gl->glActiveTexture(GL_TEXTURE0);
        this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
        this->d->m_eqShader->setUniformValue("uTexture", 0);
        this->d->m_eqShader->setUniformValue("uMin", minL);
        this->d->m_eqShader->setUniformValue("uMax", maxL);
        this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        this->d->m_eqShader->release();
        this->d->m_eqVao.release();
        this->d->m_eqFbo->release();

        sourceFbo = this->d->m_eqFbo;
    }

    // Sobel pass
    if (!this->d->m_gradFbo
        || this->d->m_gradFbo->width() != width
        || this->d->m_gradFbo->height() != height) {
        delete this->d->m_gradFbo;
        QOpenGLFramebufferObjectFormat fmt;
        this->d->m_gradFbo = new QOpenGLFramebufferObject(width, height, fmt);
    }

    this->d->m_gradFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_sobelVao.bind();
    this->d->m_sobelShader->bind();
    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, sourceFbo->texture());
    this->d->m_sobelShader->setUniformValue("uTexture", 0);
    this->d->m_sobelShader->setUniformValue("uTexelSize", 1.0f/width, 1.0f/height);
    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    this->d->m_sobelShader->release();
    this->d->m_sobelVao.release();
    this->d->m_gradFbo->release();

    // NMS pass (only if canny)
    auto nmsOut = this->d->m_gradFbo;

    if (this->d->m_canny) {
        if (!this->d->m_nmsFbo
            || this->d->m_nmsFbo->width() != width
            || this->d->m_nmsFbo->height() != height) {
            delete this->d->m_nmsFbo;
            QOpenGLFramebufferObjectFormat fmt;
            this->d->m_nmsFbo = new QOpenGLFramebufferObject(width, height, fmt);
        }

        this->d->m_nmsFbo->bind();
        this->m_gl->glViewport(0, 0, width, height);
        this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

        this->d->m_nmsVao.bind();
        this->d->m_nmsShader->bind();
        this->m_gl->glActiveTexture(GL_TEXTURE0);
        this->m_gl->glBindTexture(GL_TEXTURE_2D, this->d->m_gradFbo->texture());
        this->d->m_nmsShader->setUniformValue("uGradTex", 0);
        this->d->m_nmsShader->setUniformValue("uTexelSize", 1.0f/width, 1.0f/height);
        this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        this->d->m_nmsShader->release();
        this->d->m_nmsVao.release();
        this->d->m_nmsFbo->release();
        nmsOut = this->d->m_nmsFbo;
    }

    // Threshold pass
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_threshVao.bind();
    this->d->m_threshShader->bind();
    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, nmsOut->texture());
    this->d->m_threshShader->setUniformValue("uInputTex", 0);
    auto thLowNorm = static_cast<float>(qBound(0.0, this->d->m_thLow, 1.0));
    auto thHiNorm = static_cast<float>(qBound(0.0, this->d->m_thHi, 1.0));
    this->d->m_threshShader->setUniformValue("uThLow", thLowNorm);
    this->d->m_threshShader->setUniformValue("uThHi", thHiNorm);
    float rLine = qRed(this->d->m_lineColor) / 255.0f;
    float gLine = qGreen(this->d->m_lineColor) / 255.0f;
    float bLine = qBlue(this->d->m_lineColor) / 255.0f;
    float aLine = qAlpha(this->d->m_lineColor) / 255.0f;
    float rBg = qRed(this->d->m_backgroundColor) / 255.0f;
    float gBg = qGreen(this->d->m_backgroundColor) / 255.0f;
    float bBg = qBlue(this->d->m_backgroundColor) / 255.0f;
    float aBg = qAlpha(this->d->m_backgroundColor) / 255.0f;
    this->d->m_threshShader->setUniformValue("uLineColor", rLine, gLine, bLine, aLine);
    this->d->m_threshShader->setUniformValue("uBgColor", rBg, gBg, bBg, aBg);
    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    this->d->m_threshShader->release();
    this->d->m_threshVao.release();
    outputFbo->release();
}

void EdgeElement::uninit()
{
    if (this->d->m_eqFbo) {
        delete this->d->m_eqFbo;
        this->d->m_eqFbo = nullptr;
    }

    if (this->d->m_gradFbo) {
        delete this->d->m_gradFbo;
        this->d->m_gradFbo = nullptr;
    }

    if (this->d->m_nmsFbo) {
        delete this->d->m_nmsFbo;
        this->d->m_nmsFbo = nullptr;
    }

    if (this->d->m_histFbo) {
        delete this->d->m_histFbo;
        this->d->m_histFbo = nullptr;
    }

    this->d->m_downscaleVao.destroy();
    this->d->m_eqVao.destroy();
    this->d->m_sobelVao.destroy();
    this->d->m_nmsVao.destroy();
    this->d->m_threshVao.destroy();

    if (this->d->m_downscaleShader) {
        delete this->d->m_downscaleShader;
        this->d->m_downscaleShader = nullptr;
    }

    if (this->d->m_eqShader) {
        delete this->d->m_eqShader;
        this->d->m_eqShader = nullptr;
    }

    if (this->d->m_sobelShader) {
        delete this->d->m_sobelShader;
        this->d->m_sobelShader = nullptr;
    }

    if (this->d->m_nmsShader) {
        delete this->d->m_nmsShader;
        this->d->m_nmsShader = nullptr;
    }

    if (this->d->m_threshShader) {
        delete this->d->m_threshShader;
        this->d->m_threshShader = nullptr;
    }
}

QString EdgeElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Edge/share/qml/main.qml");
}

void EdgeElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Edge", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void EdgeElement::setCanny(bool canny)
{
    if (this->d->m_canny == canny)
        return;

    this->d->m_canny = canny;
    emit this->cannyChanged(canny);
}

void EdgeElement::setThLow(qreal thLow)
{
    if (this->d->m_thLow == thLow)
        return;

    this->d->m_thLow = thLow;
    emit this->thLowChanged(thLow);
}

void EdgeElement::setThHi(qreal thHi)
{
    if (this->d->m_thHi == thHi)
        return;

    this->d->m_thHi = thHi;
    emit this->thHiChanged(thHi);
}

void EdgeElement::setEqualize(bool equalize)
{
    if (this->d->m_equalize == equalize)
        return;

    this->d->m_equalize = equalize;
    emit this->equalizeChanged(equalize);
}

void EdgeElement::setLineColor(QRgb lineColor)
{
    if (this->d->m_lineColor == lineColor)
        return;

    this->d->m_lineColor = lineColor;
    emit this->lineColorChanged(lineColor);
}

void EdgeElement::setBackgroundColor(QRgb backgroundColor)
{
    if (this->d->m_backgroundColor == backgroundColor)
        return;

    this->d->m_backgroundColor = backgroundColor;
    emit this->backgroundColorChanged(backgroundColor);
}

void EdgeElement::resetCanny()
{
    this->setCanny(false);
}

void EdgeElement::resetThLow()
{
    this->setThLow(0.1);
}

void EdgeElement::resetThHi()
{
    this->setThHi(0.9);
}

void EdgeElement::resetEqualize()
{
    this->setEqualize(false);
}

void EdgeElement::resetLineColor()
{
    this->setLineColor(qRgb(255, 255, 255));
}

void EdgeElement::resetBackgroundColor()
{
    this->setBackgroundColor(qRgb(0, 0, 0));
}

#include "moc_edgeelement.cpp"
