/* Webcamoid, camera capture application.
 * Copyright (C) 2023  Gonzalo Exequiel Pedone
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

#include "cropelement.h"

class CropElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;
        bool m_editMode {false};
        bool m_relative {false};
        bool m_keepResolution {false};
        qreal m_left {0.0};
        qreal m_right {639.0};
        qreal m_top {0.0};
        qreal m_bottom {479.0};
        QRgb m_fillColor {qRgba(0, 0, 0, 0)};
        int m_frameWidth {640};
        int m_frameHeight {480};
};

CropElement::CropElement(): AkVideoEffect()
{
    this->d = new CropElementPrivate;
}

CropElement::~CropElement()
{
    delete this->d;
}

bool CropElement::editMode() const
{
    return this->d->m_editMode;
}

bool CropElement::relative() const
{
    return this->d->m_relative;
}

bool CropElement::keepResolution() const
{
    return this->d->m_keepResolution;
}

qreal CropElement::left() const
{
    return this->d->m_left;
}

qreal CropElement::right() const
{
    return this->d->m_right;
}

qreal CropElement::top() const
{
    return this->d->m_top;
}

qreal CropElement::bottom() const
{
    return this->d->m_bottom;
}

QRgb CropElement::fillColor() const
{
    return this->d->m_fillColor;
}

int CropElement::frameWidth() const
{
    return this->d->m_frameWidth;
}

int CropElement::frameHeight() const
{
    return this->d->m_frameHeight;
}

bool CropElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Crop/share/shaders/crop.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Crop/share/shaders/crop.frag"))
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

void CropElement::process(QOpenGLFramebufferObject *inputFbo,
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

    // Update frame size properties
    if (width != this->d->m_frameWidth) {
        this->d->m_frameWidth = width;
        emit this->frameWidthChanged(width);
    }

    if (height != this->d->m_frameHeight) {
        this->d->m_frameHeight = height;
        emit this->frameHeightChanged(height);
    }

    // Compute crop rectangle (same logic as original)
    int rightMax = width - 1;
    int left = this->d->m_relative?
                   qRound(rightMax * this->d->m_left / 100.0):
                   qRound(this->d->m_left);
    int right = this->d->m_relative?
                    qRound(rightMax * this->d->m_right / 100.0):
                    qRound(this->d->m_right);
    left = qBound(0, left, rightMax);
    right = qBound(0, right, rightMax);

    if (left >= right) {
        if (left + 1 <= rightMax) {
            right = left + 1;
        } else {
            left = rightMax - 1;
            right = rightMax;
        }
    }

    int bottomMax = height - 1;
    int top = this->d->m_relative?
                  qRound(bottomMax * this->d->m_top / 100.0):
                  qRound(this->d->m_top);
    int bottom = this->d->m_relative?
                     qRound(bottomMax * this->d->m_bottom / 100.0):
                     qRound(this->d->m_bottom);
    top = qBound(0, top, bottomMax);
    bottom = qBound(0, bottom, bottomMax);

    if (top >= bottom) {
        if (top + 1 <= bottomMax) {
            bottom = top + 1;
        } else {
            top = bottomMax - 1;
            bottom = bottomMax;
        }
    }

    QRect srcRect(left, top, right - left + 1, bottom - top + 1);

    // Determine output size and UV mapping
    int outWidth = width;
    int outHeight = height;
    float srcCropX = float(srcRect.left()) / float(width);
    float srcCropY = 1.0f - float(srcRect.bottom() + 1) / float(height);
    float srcCropW = float(srcRect.width()) / float(width);
    float srcCropH = float(srcRect.height()) / float(height);
    float dstCropX = 0.0f;
    float dstCropY = 0.0f;
    float dstCropW = 1.0f;
    float dstCropH = 1.0f;

    if (!this->d->m_editMode) {
        if (this->d->m_keepResolution) {
            // Keep original resolution, center the crop
            QRect dstRect;
            if (width * srcRect.height() <= height * srcRect.width()) {
                int dstHeight = width * srcRect.height() / srcRect.width();
                dstRect = {0,
                           (height - dstHeight) / 2,
                           width,
                           dstHeight};
            } else {
                int dstWidth = height * srcRect.width() / srcRect.height();
                dstRect = {(width - dstWidth) / 2,
                           0,
                           dstWidth,
                           height};
            }
            dstCropX = float(dstRect.left()) / float(width);
            dstCropY = float(dstRect.top()) / float(height);
            dstCropW = float(dstRect.width()) / float(width);
            dstCropH = float(dstRect.height()) / float(height);
        } else {
            // Change resolution to crop size
            outWidth = srcRect.width();
            outHeight = srcRect.height();
            dstCropX = 0.0f;
            dstCropY = 0.0f;
            dstCropW = 1.0f;
            dstCropH = 1.0f;
        }
    }

    // Reallocate output FBO if size changed
    if (!outputFbo
        || outputFbo->width()  != outWidth
        || outputFbo->height() != outHeight) {
        delete outputFbo;
        QOpenGLFramebufferObjectFormat fmt;
        outputFbo = new QOpenGLFramebufferObject(outWidth, outHeight, fmt);
    }

    // Fill color normalized
    float fillR = float(qRed(this->d->m_fillColor))   / 255.0f;
    float fillG = float(qGreen(this->d->m_fillColor)) / 255.0f;
    float fillB = float(qBlue(this->d->m_fillColor))  / 255.0f;
    float fillA = float(qAlpha(this->d->m_fillColor)) / 255.0f;

    // Render
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, outWidth, outHeight);
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
    this->d->m_shader->setUniformValue("uTex",       0);
    this->d->m_shader->setUniformValue("uSrcCrop",   srcCropX, srcCropY, srcCropW, srcCropH);
    this->d->m_shader->setUniformValue("uDstCrop",   dstCropX, dstCropY, dstCropW, dstCropH);
    this->d->m_shader->setUniformValue("uFillColor", fillR, fillG, fillB, fillA);
    this->d->m_shader->setUniformValue("uEditMode",  this->d->m_editMode ? 1.0f : 0.0f);
    this->d->m_shader->setUniformValue("uEditColor", 1.0f, 0.0f, 0.0f, 1.0f); // Red
    this->d->m_shader->setUniformValue("uWidth",     outWidth);
    this->d->m_shader->setUniformValue("uHeight",    outHeight);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void CropElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

QString CropElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Crop/share/qml/main.qml");
}

void CropElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Crop", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void CropElement::setEditMode(bool editMode)
{
    if (this->d->m_editMode == editMode)
        return;

    this->d->m_editMode = editMode;
    emit this->editModeChanged(this->d->m_editMode);
}

void CropElement::setRelative(bool relative)
{
    if (this->d->m_relative == relative)
        return;

    this->d->m_relative = relative;
    qreal left = 0.0;
    qreal right = 0.0;
    qreal top = 0.0;
    qreal bottom = 0.0;
    int rightMax = qMax(this->d->m_frameWidth - 1, 1);
    int bottomMax = qMax(this->d->m_frameHeight - 1, 1);

    if (relative) {
        left = 100.0 * this->d->m_left / rightMax;
        right = 100.0 * this->d->m_right / rightMax;
        top = 100.0 * this->d->m_top / bottomMax;
        bottom = 100.0 * this->d->m_bottom / bottomMax;
    } else {
        left = this->d->m_left * rightMax / 100.0;
        right = this->d->m_right * rightMax / 100.0;
        top = this->d->m_top * bottomMax / 100.0;
        bottom = this->d->m_bottom * bottomMax / 100.0;
    }

    emit this->relativeChanged(this->d->m_relative);
    this->setLeft(left);
    this->setRight(right);
    this->setTop(top);
    this->setBottom(bottom);
}

void CropElement::setKeepResolution(bool keepResolution)
{
    if (this->d->m_keepResolution == keepResolution)
        return;

    this->d->m_keepResolution = keepResolution;
    emit this->keepResolutionChanged(this->d->m_keepResolution);
}

void CropElement::setLeft(qreal left)
{
    if (qFuzzyCompare(this->d->m_left, left))
        return;

    this->d->m_left = left;
    emit this->leftChanged(this->d->m_left);
}

void CropElement::setRight(qreal right)
{
    if (qFuzzyCompare(this->d->m_right, right))
        return;

    this->d->m_right = right;
    emit this->rightChanged(this->d->m_right);
}

void CropElement::setTop(qreal top)
{
    if (qFuzzyCompare(this->d->m_top, top))
        return;

    this->d->m_top = top;
    emit this->topChanged(this->d->m_top);
}

void CropElement::setBottom(qreal bottom)
{
    if (qFuzzyCompare(this->d->m_bottom, bottom))
        return;

    this->d->m_bottom = bottom;
    emit this->bottomChanged(this->d->m_bottom);
}

void CropElement::setFillColor(QRgb fillColor)
{
    if (this->d->m_fillColor == fillColor)
        return;

    this->d->m_fillColor = fillColor;
    emit this->fillColorChanged(this->d->m_fillColor);
}

void CropElement::resetEditMode()
{
    this->setEditMode(false);
}

void CropElement::resetRelative()
{
    this->setRelative(false);
}

void CropElement::resetKeepResolution()
{
    this->setKeepResolution(false);
}

void CropElement::resetLeft()
{
    this->setLeft(0.0);
}

void CropElement::resetRight()
{
    this->setRight(this->d->m_frameWidth);
}

void CropElement::resetTop()
{
    this->setTop(0.0);
}

void CropElement::resetBottom()
{
    this->setBottom(this->d->m_frameHeight);
}

void CropElement::resetFillColor()
{
    this->setFillColor(qRgba(0, 0, 0, 0));
}

void CropElement::reset()
{
    this->resetEditMode();
    this->resetRelative();
    this->resetKeepResolution();
    this->resetLeft();
    this->resetRight();
    this->resetTop();
    this->resetBottom();
    this->resetFillColor();
}

#include "moc_cropelement.cpp"
