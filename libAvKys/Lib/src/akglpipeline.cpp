/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QDebug>
#include <QFuture>
#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QQmlEngine>
#include <QThread>
#include <QThreadPool>
#include <QVector>
#include <QWaitCondition>
#include <QtConcurrent>

#include "akglpipeline.h"
#include "akalgorithm.h"
#include "akfrac.h"
#include "akpacket.h"
#include "akplugininfo.h"
#include "akpluginmanager.h"
#include "akvideocaps.h"
#include "akvideoconverter.h"
#include "akvideopacket.h"
#include "iak/akvideoeffect.h"

class AkGLPipelineEffect
{
    public:
        AkVideoEffectPtr element;
        AkPluginInfo info;

        AkGLPipelineEffect();
        AkGLPipelineEffect(const AkVideoEffectPtr &element,
                           const AkPluginInfo &info);
        AkGLPipelineEffect &operator =(const AkGLPipelineEffect &other);
};

class AkGLPipelinePrivate
{
    public:
        AkGLPipeline *self;

        QOpenGLContext *m_shareContext {nullptr};
        QThread *m_renderThread {nullptr};
        QAtomicInt m_packetReaders;

        // GL context and surface - owned exclusively by the render thread.
        QOpenGLContext *m_context {nullptr};
        QOffscreenSurface *m_surface {nullptr};

        // GL objects - created/destroyed on the render thread.
        QOpenGLBuffer *m_vbo {nullptr};
        QOpenGLBuffer *m_ibo {nullptr};
        QOpenGLVertexArrayObject *m_blitVao {nullptr};
        QOpenGLShaderProgram *m_blitShader {nullptr};
        QOpenGLFramebufferObject *m_fbo[2] = {nullptr, nullptr};
        QOpenGLTexture *m_uploadTex {nullptr};

        // Packet queue (written by iVideoStream, read by render thread)
        QMutex m_queueMutex;
        QWaitCondition m_queueCond;
        QVector<AkVideoPacket> m_packetQueue;
        // Maximum packets buffered before iVideoStream blocks.
        static constexpr int kMaxQueue {4};

        // Signals the render thread to exit its processing loop.
        bool m_stopRequested {false};
        QMutex m_stopMutex;

        // Shared effect/pipeline state (read by render thread, written by control thread)
        QMutex m_effectsMutex;
        QVector<AkGLPipelineEffect> m_effects;
        AkGLPipelineEffect m_preview;
        bool m_chainEffects {false};

        // Other state (control thread only)
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgba, 0, 0, {}}};
        QStringList m_availableEffects;
        bool m_preserveNullPlugins {false};

        // Input and output buffers
        char *m_uploadBuffer {nullptr};
        size_t m_uploadBufferSize {0};
        char *m_readbackBuffer {nullptr};
        size_t m_readbackBufferSize {0};

        QThreadPool m_threadPool;
        QFuture<void> m_threadStatus;
        AkPacket m_curPacket;

        explicit AkGLPipelinePrivate(AkGLPipeline *self);

        // Control-thread helpers
        void startRenderThread();
        void stopRenderThread();

        // Render-thread methods (only called from renderGL())
        void renderGL();
        void initGL();
        void uninitGL();
        void initEffects();
        void uninitEffects();
        void initEffect(const AkVideoEffectPtr &effect);
        void processPacket(const AkVideoPacket &packet);
        void blitTexture(GLuint tex, int width, int height);
        void ensureFboSize(QOpenGLFramebufferObject *&fbo,
                           int width,
                           int height);
        void sendPacket(const AkPacket &packet);
};

AkGLPipeline::AkGLPipeline(QObject *parent):
    AkElement(parent)
{
    this->d = new AkGLPipelinePrivate(this);
    this->updateAvailableEffects();
}

AkGLPipeline::~AkGLPipeline()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList AkGLPipeline::availableEffects() const
{
    return this->d->m_availableEffects;
}

QStringList AkGLPipeline::effects() const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);
    QStringList ids;

    for (auto &effect: this->d->m_effects)
        ids << effect.info.id();

    return ids;
}

QString AkGLPipeline::preview() const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    if (!this->d->m_preview.element)
        return {};

    return this->d->m_preview.info.id();
}

AkPluginInfo AkGLPipeline::effectInfo(const QString &effectId) const
{
    return akPluginManager->pluginInfo(effectId);
}

AkPluginInfo AkGLPipeline::effectInfo(int index) const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    return this->d->m_effects.at(index).info;
}

QString AkGLPipeline::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return {};

    auto info = akPluginManager->pluginInfo(effectId);

    if (!info)
        return {};

    return info.description();
}

bool AkGLPipeline::chainEffects() const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    return this->d->m_chainEffects;
}

bool AkGLPipeline::preserveNullPlugins() const
{
    return this->d->m_preserveNullPlugins;
}

bool AkGLPipeline::isEmpty() const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    return this->d->m_effects.isEmpty() && !this->d->m_preview.element;
}

AkVideoEffectPtr AkGLPipeline::elementAt(int index) const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    return this->d->m_effects.at(index).element;
}

AkVideoEffectPtr AkGLPipeline::previewElement() const
{
    QMutexLocker mutexLocker(&this->d->m_effectsMutex);

    return this->d->m_preview.element;
}

void AkGLPipeline::setShareContext(QOpenGLContext *shareContext)
{
    this->d->m_shareContext = shareContext;
}

void AkGLPipeline::addPacketReader()
{
    this->d->m_packetReaders++;
}

void AkGLPipeline::removePacketReader()
{
    this->d->m_packetReaders--;
}

void AkGLPipeline::setEffects(const QStringList &effects)
{
    if (this->effects() == effects)
        return;

    bool wasEmpty = this->isEmpty();
    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_effects.clear();

        for (auto &effectId: effects) {
            auto effect = akPluginManager->create<AkVideoEffect>(effectId);

            if (effect) {
                this->d->m_effects << AkGLPipelineEffect(effect,
                                                         akPluginManager->pluginInfo(effectId));
            } else if (this->d->m_preserveNullPlugins) {
                this->d->m_effects << AkGLPipelineEffect();
            }
        }
    }

    if (isRunning)
        this->d->startRenderThread();

    emit this->effectsChanged(effects);

    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

void AkGLPipeline::setPreview(const QString &preview)
{
    if (this->preview() == preview)
        return;

    bool wasEmpty = this->isEmpty();
    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_preview.element = akPluginManager->create<AkVideoEffect>(preview);
        this->d->m_preview.info = akPluginManager->pluginInfo(preview);
    }

    if (isRunning)
        this->d->startRenderThread();

    emit this->previewChanged(preview);
    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

bool AkGLPipeline::setState(AkElement::ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull:
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->startRenderThread();
            return AkElement::setState(state);
        case AkElement::ElementStateNull:
            break;
        }
        break;

    case AkElement::ElementStatePaused:
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->stopRenderThread();
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }
        break;

    case AkElement::ElementStatePlaying:
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->stopRenderThread();
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }
        break;
    }

    return false;
}

void AkGLPipeline::setChainEffects(bool chainEffects)
{
    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);

        if (this->d->m_chainEffects == chainEffects)
            return;

        this->d->m_chainEffects = chainEffects;
    }

    // chainEffects only affects processing logic, no GL reinit needed.
    emit this->chainEffectsChanged(chainEffects);
}

void AkGLPipeline::setPreserveNullPlugins(bool preserveNullPlugins)
{
    if (this->d->m_preserveNullPlugins == preserveNullPlugins)
        return;

    this->d->m_preserveNullPlugins = preserveNullPlugins;
    emit this->preserveNullPluginsChanged(preserveNullPlugins);
}

void AkGLPipeline::resetEffects()
{
    this->setEffects({});
}

void AkGLPipeline::resetPreview()
{
    this->setPreview({});
}

void AkGLPipeline::resetChainEffects()
{
    this->setChainEffects(false);
}

void AkGLPipeline::resetPreserveNullPlugins()
{
    this->setPreserveNullPlugins(false);
}

void AkGLPipeline::applyPreview()
{
    bool wasEmpty = this->isEmpty();
    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    bool applied = false;

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);

        if (this->d->m_preview.element) {
            if (this->d->m_chainEffects && !this->d->m_effects.isEmpty())
                this->d->m_effects << this->d->m_preview;
            else {
                this->d->m_effects.clear();
                this->d->m_effects << this->d->m_preview;
            }

            this->d->m_preview = {};
            applied = true;
        }
    }

    if (!applied)
        return;

    if (isRunning)
        this->d->startRenderThread();

    emit this->previewChanged({});
    emit this->effectsChanged(this->effects());

    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

void AkGLPipeline::moveEffect(int from, int to)
{
    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);

        if (from == to
            || from < 0
            || from >= this->d->m_effects.size()
            || to < 0
            || to >= this->d->m_effects.size())
            return;

        this->d->m_effects.move(from, to);
    }

    if (isRunning)
        this->d->startRenderThread();

    // Order change only - no GL objects need to be recreated.
    emit this->effectsChanged(this->effects());
}

void AkGLPipeline::removeEffect(int index)
{
    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);

        if (index < 0 || index >= this->d->m_effects.size())
            return;

        this->d->m_effects.removeAt(index);
    }

    if (isRunning)
        this->d->startRenderThread();

    emit this->effectsChanged(this->effects());
}

void AkGLPipeline::removeAllEffects()
{
    if (this->isEmpty())
        return;

    bool isRunning = this->d->m_renderThread != nullptr;

    if (isRunning)
        this->d->stopRenderThread();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_effects.clear();
    }

    if (isRunning)
        this->d->startRenderThread();

    emit this->effectsChanged({});
    emit this->isEmptyChanged(true);
}

// Converts the packet to RGBA and enqueues it for the render thread.
AkPacket AkGLPipeline::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    // If there is nothing to process, pass through immediately.
    if (this->isEmpty() || this->state() != AkElement::ElementStatePlaying) {
        emit this->oStream(packet);

        return packet;
    }

    // Convert the frames to RGBA.
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    // Enqueue, blocking if the render thread is falling behind.
    {
        QMutexLocker mutexLocker(&this->d->m_queueMutex);

        while (this->d->m_packetQueue.size() >= AkGLPipelinePrivate::kMaxQueue)
            this->d->m_queueCond.wait(&this->d->m_queueMutex);

        this->d->m_packetQueue << src;
        this->d->m_queueCond.wakeAll();
    }

    return {};
}

void AkGLPipeline::updateAvailableEffects()
{
    auto availableEffects =
            akPluginManager->listPlugins({},
                                         {"VideoFilterGL"},
                                         AkPluginManager::FilterEnabled);
    std::sort(availableEffects.begin(),
              availableEffects.end(),
              [this] (const QString &pluginId1, const QString &pluginId2) {
        return this->effectDescription(pluginId1) < this->effectDescription(pluginId2);
    });

    if (this->d->m_availableEffects != availableEffects) {
        this->d->m_availableEffects = availableEffects;
        emit this->availableEffectsChanged(availableEffects);
    }
}

void AkGLPipeline::registerTypes()
{
    qRegisterMetaType<AkGLPipeline>("AkGLPipeline");
    qmlRegisterSingletonType<AkGLPipeline>("Ak", 1, 0, "AkGLPipeline",
                                           [] (QQmlEngine *qmlEngine,
                                               QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)
        return new AkGLPipeline();
    });
}

AkGLPipelinePrivate::AkGLPipelinePrivate(AkGLPipeline *self):
    self(self)
{
}

void AkGLPipelinePrivate::startRenderThread()
{
    if (this->m_renderThread)
        return;

    this->m_renderThread = QThread::create([this] { this->renderGL(); });
    this->m_renderThread->start();
}

void AkGLPipelinePrivate::stopRenderThread()
{
    if (!this->m_renderThread)
        return;

    // Signal the render thread to stop and wake it in case it is waiting on the queue.
    {
        QMutexLocker mutexLocker(&this->m_stopMutex);
        this->m_stopRequested = true;
    }

    this->m_queueCond.wakeAll();

    this->m_renderThread->wait();
    delete this->m_renderThread;
    this->m_renderThread = nullptr;

    this->m_stopRequested = false;

    QMutexLocker mutexLocker(&this->m_queueMutex);
    this->m_packetQueue.clear();
}

void AkGLPipelinePrivate::renderGL()
{
    this->m_surface = new QOffscreenSurface;
    QSurfaceFormat fmt;
    fmt.setVersion(2, 0);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    this->m_surface->setFormat(fmt);
    this->m_surface->create();

    this->initGL();
    this->initEffects();

    while (true) {
        // Check whether a stop was requested before blocking on the queue.
        {
            QMutexLocker mutexLocker(&this->m_stopMutex);

            if (this->m_stopRequested)
                break;
        }

        // Wait for the next packet from the input queue.
        AkVideoPacket packet;

        {
            QMutexLocker queueMutexLocker(&this->m_queueMutex);

            while (this->m_packetQueue.isEmpty()) {
                // Re-check stop condition while waiting to avoid missing a wakeup.
                {
                    QMutexLocker mutexLocker(&this->m_stopMutex);

                    if (this->m_stopRequested)
                        goto done;
                }

                this->m_queueCond.wait(&this->m_queueMutex, 100);
            }

            if (self->state() != AkElement::ElementStatePlaying)
                continue;

            packet = this->m_packetQueue.takeFirst();
            this->m_queueCond.wakeAll();
        }

        if (packet)
            this->processPacket(packet);
    }

done:
    this->uninitEffects();
    this->uninitGL();

    delete this->m_surface;
    this->m_surface = nullptr;
}

void AkGLPipelinePrivate::initGL()
{
    QSurfaceFormat fmt;
    fmt.setVersion(2, 0);
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    this->m_context = new QOpenGLContext;
    this->m_context->setFormat(fmt);

    if (this->m_shareContext)
        this->m_context->setShareContext(this->m_shareContext);

    this->m_context->create();
    this->m_context->makeCurrent(this->m_surface);

    self->initializeOpenGLFunctions();

    // Full-screen quad in NDC [-1, 1]. UV origin matches OpenGL convention
    // (V=0 at bottom-left), so no Y-flip is required in the blit shader.
    static const float akGLPipelinePrivateQuadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    };

    this->m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->m_vbo->create();
    this->m_vbo->bind();
    this->m_vbo->allocate(akGLPipelinePrivateQuadVertices,
                          sizeof(akGLPipelinePrivateQuadVertices));
    this->m_vbo->release();

    static const unsigned int akGLPipelinePrivateQuadIndices[] = {
        0, 1, 2, 2, 3, 0
    };

    this->m_ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->m_ibo->create();
    this->m_ibo->bind();
    this->m_ibo->allocate(akGLPipelinePrivateQuadIndices,
                          sizeof(akGLPipelinePrivateQuadIndices));
    this->m_ibo->release();

    this->m_blitShader = new QOpenGLShaderProgram;
    this->m_blitVao = new QOpenGLVertexArrayObject;
    this->m_blitVao->create();
    this->m_blitVao->bind();
    this->m_vbo->bind();
    this->m_ibo->bind();

    this->m_blitShader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                ":/Ak/share/shaders/glpipeline/blit.vert");
    this->m_blitShader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                ":/Ak/share/shaders/glpipeline/blit.frag");

    this->m_blitShader->link();
    this->m_blitShader->bind();

    int posAttr = this->m_blitShader->attributeLocation("aPos");
    this->m_blitShader->enableAttributeArray(posAttr);
    this->m_blitShader->setAttributeBuffer(posAttr, GL_FLOAT, 0, 3, 5 * sizeof(float));

    int texAttr = this->m_blitShader->attributeLocation("aTexCoord");
    this->m_blitShader->enableAttributeArray(texAttr);
    this->m_blitShader->setAttributeBuffer(texAttr, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));

    this->m_blitShader->release();
    this->m_blitVao->release();

    this->m_uploadTex = new QOpenGLTexture(QOpenGLTexture::Target2D);
    this->m_uploadTex->setFormat(QOpenGLTexture::RGBA8_UNorm);
    this->m_uploadTex->setMinificationFilter(QOpenGLTexture::Linear);
    this->m_uploadTex->setMagnificationFilter(QOpenGLTexture::Linear);
    this->m_uploadTex->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void AkGLPipelinePrivate::uninitGL()
{
    if (this->m_uploadTex) {
        if (this->m_uploadTex->isCreated())
            this->m_uploadTex->destroy();
        delete this->m_uploadTex;
        this->m_uploadTex = nullptr;
    }

    for (auto *&fbo: this->m_fbo) {
        delete fbo;
        fbo = nullptr;
    }

    if (this->m_blitVao) {
        this->m_blitVao->destroy();
        delete this->m_blitVao;
        this->m_blitVao = nullptr;
    }

    if (this->m_vbo) {
        this->m_vbo->destroy();
        delete this->m_vbo;
        this->m_vbo = nullptr;
    }

    if (this->m_ibo) {
        this->m_ibo->destroy();
        delete this->m_ibo;
        this->m_ibo = nullptr;
    }

    if (this->m_blitShader) {
        delete this->m_blitShader;
        this->m_blitShader = nullptr;
    }

    if (this->m_context) {
        this->m_context->doneCurrent();
        delete this->m_context;
        this->m_context = nullptr;
    }

    this->m_threadPool.waitForDone();
    this->m_curPacket = {};
}

void AkGLPipelinePrivate::initEffects()
{
    QMutexLocker mutexLocker(&this->m_effectsMutex);

    for (auto &effect: this->m_effects)
        if (effect.element)
            this->initEffect(effect.element);

    if (this->m_preview.element)
        this->initEffect(this->m_preview.element);

    this->m_uploadBufferSize = 0;

    if (this->m_uploadBuffer) {
        delete this->m_uploadBuffer;
        this->m_uploadBuffer = nullptr;
    }

    this->m_readbackBufferSize = 0;

    if (this->m_readbackBuffer) {
        delete this->m_readbackBuffer;
        this->m_readbackBuffer = nullptr;
    }
}

void AkGLPipelinePrivate::uninitEffects()
{
    QMutexLocker mutexLocker(&this->m_effectsMutex);

    for (auto &effect: this->m_effects)
        if (effect.element)
            effect.element->uninit();

    if (this->m_preview.element)
        this->m_preview.element->uninit();
}

void AkGLPipelinePrivate::initEffect(const AkVideoEffectPtr &effect)
{
    effect->setGLFunctions(self);

    if (!effect->init(this->m_vbo, this->m_ibo))
        qWarning() << "Effect failed to initialize:" << effect.get();
}

void AkGLPipelinePrivate::processPacket(const AkVideoPacket &packet)
{
    int width = packet.caps().width();
    int height = packet.caps().height();

    // Reallocate the upload texture when the frame resolution changes.
    if (!this->m_uploadTex->isCreated()
        || this->m_uploadTex->width() != width
        || this->m_uploadTex->height() != height) {
        if (this->m_uploadTex->isCreated())
            this->m_uploadTex->destroy();

        this->m_uploadTex->setSize(width, height);
        this->m_uploadTex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        this->m_uploadTex->setMinificationFilter(QOpenGLTexture::Linear);
        this->m_uploadTex->setMagnificationFilter(QOpenGLTexture::Linear);
        this->m_uploadTex->setWrapMode(QOpenGLTexture::ClampToEdge);
        this->m_uploadTex->allocateStorage();
    }

    // Copy the packed RGBA frame data into the upload texture.
    this->m_uploadTex->bind();
    size_t packetLineSize = packet.lineSize(0);
    size_t gpuLineSize = 4 * width;
    size_t gpuBufferSize = gpuLineSize * height;
    size_t copyLineSize = qMin(packetLineSize, gpuLineSize);

    if (this->m_uploadBufferSize < gpuBufferSize) {
        if (this->m_uploadBuffer)
            delete this->m_uploadBuffer;

        this->m_uploadBuffer = new char[gpuBufferSize];
        this->m_uploadBufferSize = gpuBufferSize;
    }

    if (packetLineSize == gpuLineSize)
        memcpy(this->m_uploadBuffer,
               packet.constData(),
               this->m_uploadBufferSize);
    else
        for (int y = 0; y < height; ++y)
            memcpy(this->m_uploadBuffer + y * gpuLineSize,
                packet.constLine(0, y),
                copyLineSize);

    self->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                          GL_RGBA, GL_UNSIGNED_BYTE,
                          this->m_uploadBuffer);
    this->m_uploadTex->release();

    // Blit into fbo[0] as pipeline entry point.
    this->ensureFboSize(this->m_fbo[0], width, height);
    this->ensureFboSize(this->m_fbo[1], width, height);
    this->m_fbo[0]->bind();
    self->glViewport(0, 0, width, height);
    this->blitTexture(this->m_uploadTex->textureId(), width, height);
    this->m_fbo[0]->release();

    // Run the effect chain.
    int srcIdx = 0;
    int dstIdx = 1;
    auto pts = packet.pts() * packet.timeBase().value();

    {
        QMutexLocker mutexLocker(&this->m_effectsMutex);

        for (auto &effect: this->m_effects)
            if (effect.element) {
                this->ensureFboSize(this->m_fbo[dstIdx], width, height);
                effect.element->process(this->m_fbo[srcIdx],
                                        this->m_fbo[dstIdx],
                                        packet.id(),
                                        pts);
                std::swap(srcIdx, dstIdx);
            }

        if (this->m_preview.element
            && (!this->m_chainEffects || this->m_effects.isEmpty())) {
            this->ensureFboSize(this->m_fbo[dstIdx], width, height);
            this->m_preview.element->process(this->m_fbo[srcIdx],
                                             this->m_fbo[dstIdx],
                                             packet.id(),
                                             pts);
            std::swap(srcIdx, dstIdx);
        }
    }

    emit self->outputTextureReady(this->m_fbo[srcIdx]->texture(),
                                  this->m_fbo[srcIdx]->size());

    if (this->m_packetReaders == 0)
        return;

    // Read back.
    auto outSize = this->m_fbo[srcIdx]->size();
    AkVideoCaps dstCaps(packet.caps().format(),
                        outSize.width(),
                        outSize.height(),
                        packet.caps().fps());
    AkVideoPacket dst(dstCaps);
    dst.copyMetadata(packet);

    gpuLineSize = 4 * outSize.width();
    gpuBufferSize = gpuLineSize * outSize.height();

    if (this->m_readbackBufferSize < gpuBufferSize) {
        if (this->m_readbackBuffer)
            delete this->m_readbackBuffer;

        this->m_readbackBuffer = new char[gpuBufferSize];
        this->m_readbackBufferSize = gpuBufferSize;
    }

    this->m_fbo[srcIdx]->bind();
    self->glReadPixels(0, 0,
                       outSize.width(), outSize.height(),
                       GL_RGBA, GL_UNSIGNED_BYTE,
                       this->m_readbackBuffer);
    this->m_fbo[srcIdx]->release();

    copyLineSize = qMin<size_t>(gpuLineSize, dst.lineSize(0));

    if (gpuLineSize == dst.lineSize(0))
        memcpy(dst.data(),
               this->m_readbackBuffer,
               this->m_readbackBufferSize);
    else
        for (int y = 0; y < outSize.height(); ++y)
            memcpy(dst.line(0, y),
                   this->m_readbackBuffer + y * gpuLineSize,
                   copyLineSize);

    if (!this->m_threadStatus.isRunning()) {
        this->m_curPacket = dst;

        this->m_threadStatus =
        QtConcurrent::run(&this->m_threadPool,
                          &AkGLPipelinePrivate::sendPacket,
                          this,
                          this->m_curPacket);
    }
}

void AkGLPipelinePrivate::blitTexture(GLuint tex, int width, int height)
{
    self->glViewport(0, 0, width, height);
    self->glClear(GL_COLOR_BUFFER_BIT);

    this->m_blitVao->bind();
    this->m_blitShader->bind();

    self->glActiveTexture(GL_TEXTURE0);
    self->glBindTexture(GL_TEXTURE_2D, tex);
    this->m_blitShader->setUniformValue("uTex", 0);

    self->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    this->m_blitShader->release();
    this->m_blitVao->release();
    self->glBindTexture(GL_TEXTURE_2D, 0);
}

void AkGLPipelinePrivate::ensureFboSize(QOpenGLFramebufferObject *&fbo,
                                        int width,
                                        int height)
{
    if (fbo && fbo->width() == width && fbo->height() == height)
        return;

    QOpenGLFramebufferObjectFormat fmt;
    fmt.setAttachment(QOpenGLFramebufferObject::NoAttachment);

    delete fbo;
    fbo = new QOpenGLFramebufferObject(width, height, fmt);
}

void AkGLPipelinePrivate::sendPacket(const AkPacket &packet)
{
    emit self->oStream(packet);
}

AkGLPipelineEffect::AkGLPipelineEffect()
{
}

AkGLPipelineEffect::AkGLPipelineEffect(const AkVideoEffectPtr &element,
                                       const AkPluginInfo &info):
    element(element),
    info(info)
{
}

AkGLPipelineEffect &AkGLPipelineEffect::operator =(const AkGLPipelineEffect &other)
{
    if (this != &other) {
        this->element = other.element;
        this->info = other.info;
    }

    return *this;
}

#include "moc_akglpipeline.cpp"
