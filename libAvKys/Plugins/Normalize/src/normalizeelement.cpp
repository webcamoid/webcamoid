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

#include <QtMath>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

#include "normalizeelement.h"

#if 0
#define USE_FULLSWING
#endif

#ifdef USE_FULLSWING
    #define MIN_Y 0
    #define MAX_Y 255
#else
    #define MIN_Y 16
    #define MAX_Y 235
#endif

using HistogramType = int;

class NormalizeElementPrivate
{
    public:
        QOpenGLShaderProgram *m_shader {nullptr};
        QOpenGLVertexArrayObject m_vao;

        static void histogram(const quint8 *rgba,
                              int width,
                              int height,
                              HistogramType *table);
        static void limits(int totalPixels,
                           const HistogramType *histogram,
                           int &low,
                           int &high);
};

NormalizeElement::NormalizeElement(): AkVideoEffect()
{
    this->d = new NormalizeElementPrivate;
}

NormalizeElement::~NormalizeElement()
{
    delete this->d;
}

bool NormalizeElement::init(QOpenGLBuffer *vbo, QOpenGLBuffer *ibo)
{
    this->d->m_shader = new QOpenGLShaderProgram;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/Normalize/share/shaders/normalize.vert"))
        return false;

    if (!this->d->m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/Normalize/share/shaders/normalize.frag"))
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

void NormalizeElement::process(QOpenGLFramebufferObject *inputFbo,
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

    // Read back a small thumbnail instead of the full frame to keep the CPU
    // readback cheap, and derive the stats (here, the luma percentile bounds)
    // from it.
    constexpr int thumbW = 160;
    constexpr int thumbH = 120;
    QVector<quint8> thumb(thumbW * thumbH * 4);

    inputFbo->bind();
    this->m_gl->glReadPixels(0, 0,
                             qMin(width, thumbW),
                             qMin(height, thumbH),
                             GL_RGBA, GL_UNSIGNED_BYTE,
                             thumb.data());
    inputFbo->release();

    int readW = qMin(width, thumbW);
    int readH = qMin(height, thumbH);

    HistogramType histogram[256];
    NormalizeElementPrivate::histogram(thumb.constData(), readW, readH, histogram);

    int low  = 0;
    int high = 0;
    NormalizeElementPrivate::limits(readW * readH, histogram, low, high);

    // Render
    outputFbo->bind();
    this->m_gl->glViewport(0, 0, width, height);
    this->m_gl->glClear(GL_COLOR_BUFFER_BIT);

    this->d->m_vao.bind();
    this->d->m_shader->bind();

    this->m_gl->glActiveTexture(GL_TEXTURE0);
    this->m_gl->glBindTexture(GL_TEXTURE_2D, inputFbo->texture());
    this->d->m_shader->setUniformValue("uTex", 0);

    this->d->m_shader->setUniformValue("uYLow",  float(low)  / 255.0f);
    this->d->m_shader->setUniformValue("uYHigh", float(high) / 255.0f);
    this->d->m_shader->setUniformValue("uMinY",  float(MIN_Y) / 255.0f);
    this->d->m_shader->setUniformValue("uMaxY",  float(MAX_Y) / 255.0f);

    this->m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->d->m_shader->release();
    this->d->m_vao.release();
    outputFbo->release();
}

void NormalizeElement::uninit()
{
    this->d->m_vao.destroy();

    if (this->d->m_shader) {
        this->d->m_shader->removeAllShaders();
        delete this->d->m_shader;
        this->d->m_shader = nullptr;
    }
}

void NormalizeElementPrivate::histogram(const quint8 *rgba,
                                        int width,
                                        int height,
                                        HistogramType *table)
{
    memset(table, 0, 256 * sizeof(HistogramType));

    // glReadPixels returns rows bottom-to-top, but that doesn't matter here
    // since we only need the distribution of luma values, not their position.
    for (int y = 0; y < height; y++) {
        auto line = rgba + y * width * 4;

        for (int x = 0; x < width; x++) {
            int r = line[x * 4 + 0];
            int g = line[x * 4 + 1];
            int b = line[x * 4 + 2];

            // Same BT.601 studio-swing luma used by normalize.frag's rgb2y().
            int luma = 16 + qRound((65.481 * r + 128.553 * g + 24.966 * b) / 255.0);
            luma = qBound(0, luma, 255);
            table[luma]++;
        }
    }
}

void NormalizeElementPrivate::limits(int totalPixels,
                                     const HistogramType *histogram,
                                     int &low,
                                     int &high)
{
    // The lowest and highest levels must occupy at least 0.1 % of the image.
    auto thresholdIntensity = totalPixels / 1000;
    int intensity = 0;

    for (low = 0; low < 256; low++) {
        intensity += histogram[low];

        if (intensity > thresholdIntensity)
            break;
    }

    intensity = 0;

    for (high = 255; high > 0; high--) {
        intensity += histogram[high];

        if (intensity > thresholdIntensity)
            break;
    }
}

#include "moc_normalizeelement.cpp"
