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

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQmlContext>

#include "aspectratioelement.h"

class AspectRatioElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        int m_width {16};
        int m_height {9};
};

AspectRatioElement::AspectRatioElement(): AkVideoEffect()
{
    this->d = new AspectRatioElementPrivate;
}

AspectRatioElement::~AspectRatioElement()
{
    delete this->d;
}

int AspectRatioElement::width() const
{
    return this->d->m_width;
}

int AspectRatioElement::height() const
{
    return this->d->m_height;
}

bool AspectRatioElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/AspectRatio/share/shaders/aspectratio.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/AspectRatio/share/shaders/aspectratio.frag"))
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

void AspectRatioElement::process(QOpenGLFramebufferObject *inputFbo,
                                   QOpenGLFramebufferObject *&outputFbo,
                                   qint64 streamId,
                                   qreal pts)
{
    Q_UNUSED(streamId)
    Q_UNUSED(pts)

    if (!this->d->m_shader || !this->m_gl || !inputFbo)
        return;

    const int inWidth  = inputFbo->width();
    const int inHeight = inputFbo->height();

    if (inWidth <= 0 || inHeight <= 0)
        return;

    // Compute output size maintaining aspect ratio
    int oWidth = qRound(qreal(inHeight)
                        * qMax(this->d->m_width, 1)
                        / qMax(this->d->m_height, 1));
    oWidth = qMin(oWidth, inWidth);
    int oHeight = qRound(qreal(inWidth)
                         * qMax(this->d->m_height, 1)
                         / qMax(this->d->m_width, 1));
    oHeight = qMin(oHeight, inHeight);

    // Reallocate output FBO if size changed
    if (!outputFbo
        || outputFbo->width()  != oWidth
        || outputFbo->height() != oHeight) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(oWidth, oHeight, fmt);
    }

    // Compute source crop for AspectRatioMode_Expanding:
    // Scale to fill the output, cropping excess.
    qreal inputAspect = qreal(inWidth) / qreal(inHeight);
    qreal outputAspect = qreal(oWidth) / qreal(oHeight);

    float srcCropX, srcCropY, srcCropW, srcCropH;

    if (inputAspect > outputAspect) {
        // Input is wider: crop left/right sides
        int cropW = qRound(inHeight * outputAspect);
        int srcX = (inWidth - cropW) / 2;
        srcCropX = float(srcX) / float(inWidth);
        srcCropY = 0.0f;
        srcCropW = float(cropW) / float(inWidth);
        srcCropH = 1.0f;
    } else {
        // Input is taller: crop top/bottom
        int cropH = qRound(inWidth / outputAspect);
        int srcY = (inHeight - cropH) / 2;
        srcCropX = 0.0f;
        srcCropY = 1.0f - float(srcY + cropH) / float(inHeight);  // Invert Y for OpenGL
        srcCropW = 1.0f;
        srcCropH = float(cropH) / float(inHeight);
    }

    // Render
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, oWidth, oHeight);
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
    this->d->m_shader->setUniformValue("uTex",     0);
    this->d->m_shader->setUniformValue("uSrcCrop", srcCropX, srcCropY, srcCropW, srcCropH);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void AspectRatioElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString AspectRatioElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AspectRatio/share/qml/main.qml");
}

void AspectRatioElement::controlInterfaceConfigure(QQmlContext *context,
                                                     const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AspectRatio", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void AspectRatioElement::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void AspectRatioElement::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void AspectRatioElement::resetWidth()
{
    this->setWidth(16);
}

void AspectRatioElement::resetHeight()
{
    this->setHeight(9);
}

#include "moc_aspectratioelement.cpp"
