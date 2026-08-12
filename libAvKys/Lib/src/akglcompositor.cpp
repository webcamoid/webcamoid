/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 // * it under the terms of the GNU General Public License as published by
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
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <QOffscreenSurface>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QQmlEngine>
#include <QThread>
#include <QVector>
#include <QMatrix4x4>
#include <chrono>

#include "akglcompositor.h"
#include "ak.h"
#include "akfrac.h"
#include "akglpipeline.h"
#include "akpacket.h"
#include "akplugininfo.h"
#include "akpluginmanager.h"
#include "akvideocaps.h"
#include "akvideoconverter.h"
#include "akvideopacket.h"

class AkGLCompositorSource
{
    public:
        QMutex mutex;
        AkVideoPacket lastPacket;
        bool hasFrame {false};
        QRectF rect {0.0, 0.0, 1.0, 1.0};
        int zOrder {0};
        qreal opacity {1.0};
        Qt::AspectRatioMode aspectRatioMode {Qt::KeepAspectRatio};
        AkVideoConverter converter {{AkVideoCaps::Format_rgba, 0, 0, {}}};
        QOpenGLTexture *uploadTex {nullptr};
        QOpenGLFramebufferObject *entryFbo {nullptr};
        QOpenGLFramebufferObject *effectFbo {nullptr};
        AkGLPipeline pipeline;
        bool glInitialized {false};
};

using AkGLCompositorSourcePtr = QSharedPointer<AkGLCompositorSource>;

struct SourceSnapshot
{
    AkGLCompositorSourcePtr source;
    QRectF rect;
    qreal opacity;
    Qt::AspectRatioMode aspectRatioMode;
    int texW;
    int texH;
};

class AkGLCompositorPrivate
{
    public:
        AkGLCompositor *self;
        QMutex m_effectsMutex;
        QThread *m_renderThread {nullptr};
        QAtomicInt m_packetReaders;
        QOpenGLContext *m_context {nullptr};
        QOffscreenSurface *m_surface {nullptr};

        // Shared geometry (VBO + IBO).
        QOpenGLBuffer *m_vbo {nullptr};
        QOpenGLBuffer *m_ibo {nullptr};

        // Blit shader (upload -> entryFbo).
        QOpenGLShaderProgram *m_blitShader {nullptr};
        int m_blitPosAttr {-1};
        int m_blitTexAttr {-1};
        int m_blitTexUniform {-1};

        // Composite shader (entryFbo -> canvasFbo, transform per source).
        QOpenGLShaderProgram *m_compositeShader {nullptr};
        int m_compPosAttr {-1};
        int m_compTexAttr {-1};
        int m_compTransformUniform {-1};
        int m_compOpacityUniform {-1};
        int m_compTexUniform {-1};

        QVector<SourceSnapshot> m_orderedSources;
        bool m_zOrderDirty {true};

        QOpenGLFramebufferObject *m_canvasFbo {nullptr};
        QOpenGLFramebufferObject *m_effectFbo {nullptr};
        AkVideoPacket m_outputPacket;
        QMutex m_sourcesMutex;
        QHash<qint64, AkGLCompositorSourcePtr> m_sources;
        QMutex m_pendingRemovalsMutex;
        QVector<qint64> m_pendingRemovals;
        QMutex m_stopMutex;
        bool m_stopRequested {false};
        QElapsedTimer m_clock;
        AkVideoCaps m_outputCaps {AkVideoCaps::Format_rgba, 640, 480, {30, 1}};
        QRgb m_canvasColor {qRgba(0, 0, 0, 0)};
        AkGLPipeline m_pipeline;
        AkVideoConverter m_outputConverter;
        QStringList m_availableEffects;
        QMutex m_packetMutex;
        qint64 m_id {-1};

        quint8 *m_uploadBuffer {nullptr};
        size_t m_uploadBufferSize {0};
        quint8 *m_readbackBuffer {nullptr};
        size_t m_readbackBufferSize {0};

        explicit AkGLCompositorPrivate(AkGLCompositor *self);
        void startRenderThread();
        void stopRenderThread();
        void renderGL();
        void initGL();
        void uninitGL();
        void processPendingRemovals();
        void processTick();
        void uploadSource(AkGLCompositorSourcePtr source);
        void compositeSourcesIntoCanvas();
        void readAndEmit(qint64 pts);
        void blitTexture(GLuint tex, int width, int height);
        void ensureFboSize(QOpenGLFramebufferObject *&fbo,
                           int width,
                           int height);
        void bindBlitAttribs();
        void bindCompositeAttribs();
        QMatrix4x4 computeSourceTransform(const SourceSnapshot &snap) const;
};

AkGLCompositor::AkGLCompositor(QObject *parent):
    AkElement(parent)
{
    this->d = new AkGLCompositorPrivate(this);
    this->updateAvailableEffects();
}

AkGLCompositor::~AkGLCompositor()
{
    this->setState(AkElement::ElementStateNull);

    delete this->d;
}

AkVideoCaps AkGLCompositor::outputCaps() const
{
    return this->d->m_outputCaps;
}

QRgb AkGLCompositor::canvasColor() const
{
    return this->d->m_canvasColor;
}

QStringList AkGLCompositor::availableEffects() const
{
    return this->d->m_availableEffects;
}

AkPluginInfo AkGLCompositor::effectInfo(const QString &effectId) const
{
    return akPluginManager->pluginInfo(effectId);
}

QString AkGLCompositor::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return {};

    auto info = akPluginManager->pluginInfo(effectId);

    if (!info)
        return {};

    return info.description();
}

QStringList AkGLCompositor::effects() const
{
    return this->d->m_pipeline.effects();
}

QString AkGLCompositor::preview() const
{
    return this->d->m_pipeline.preview();
}

AkVideoEffectPtr AkGLCompositor::elementAt(int index) const
{
    return this->d->m_pipeline.elementAt(index);
}

AkVideoEffectPtr AkGLCompositor::previewElement() const
{
    return this->d->m_pipeline.previewElement();
}

AkPluginInfo AkGLCompositor::effectInfo(int index) const
{
    return this->d->m_pipeline.effectInfo(index);
}

bool AkGLCompositor::chainEffects() const
{
    return this->d->m_pipeline.chainEffects();
}

bool AkGLCompositor::preserveNullPlugins() const
{
    return this->d->m_pipeline.preserveNullPlugins();
}

bool AkGLCompositor::isEmpty() const
{
    return this->d->m_pipeline.isEmpty();
}

qint64 AkGLCompositor::addSource()
{
    return this->addSource(Ak::id());
}

qint64 AkGLCompositor::addSource(qint64 id)
{
    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);

        if (this->d->m_sources.contains(id))
            return id;

        this->d->m_sources[id] = AkGLCompositorSourcePtr::create();
        this->d->m_zOrderDirty = true;
    }

    emit this->sourceAdded(id);

    return id;
}

void AkGLCompositor::removeSource(qint64 id)
{
    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);

        if (!this->d->m_sources.contains(id))
            return;
    }

    QMutexLocker mutexLocker(&this->d->m_pendingRemovalsMutex);

    if (this->d->m_renderThread && this->d->m_renderThread->isRunning()) {
        if (!this->d->m_pendingRemovals.contains(id))
            this->d->m_pendingRemovals << id;
    } else {
        this->d->m_sources.remove(id);
    }

    this->d->m_zOrderDirty = true;
}

QVariantList AkGLCompositor::sourceIds() const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    QVariantList ids;

    for (auto it = this->d->m_sources.keyBegin();
         it != this->d->m_sources.keyEnd();
         ++it)
        ids << QVariant::fromValue(*it);

    return ids;
}

QRectF AkGLCompositor::sourceRect(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    QMutexLocker sourceLocker(&source->mutex);

    return source->rect;
}

int AkGLCompositor::sourceZOrder(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return 0;

    QMutexLocker sourceLocker(&source->mutex);

    return source->zOrder;
}

qreal AkGLCompositor::sourceOpacity(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return 0.0;

    QMutexLocker sourceLocker(&source->mutex);

    return source->opacity;
}

Qt::AspectRatioMode AkGLCompositor::sourceAspectRatioMode(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return Qt::IgnoreAspectRatio;

    QMutexLocker sourceLocker(&source->mutex);

    return source->aspectRatioMode;
}

QStringList AkGLCompositor::sourceEffects(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    return source->pipeline.effects();
}

QString AkGLCompositor::sourcePreview(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    return source->pipeline.preview();
}

AkVideoEffectPtr AkGLCompositor::sourceElementAt(qint64 id, int index) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    return source->pipeline.elementAt(index);
}

AkVideoEffectPtr AkGLCompositor::sourcePreviewElement(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    return source->pipeline.previewElement();
}

AkPluginInfo AkGLCompositor::sourceEffectInfo(qint64 id, int index) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return {};

    return source->pipeline.effectInfo(index);
}

bool AkGLCompositor::sourceChainEffects(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return false;

    return source->pipeline.chainEffects();
}

bool AkGLCompositor::sourcePreserveNullPlugins(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return false;

    return source->pipeline.preserveNullPlugins();
}

bool AkGLCompositor::sourceIsEmpty(qint64 id) const
{
    QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
    auto source = this->d->m_sources.value(id, nullptr);

    if (!source)
        return true;

    return source->pipeline.isEmpty();
}

void AkGLCompositor::setOutputCaps(const AkVideoCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_outputCaps = outputCaps;
    emit this->outputCapsChanged(outputCaps);
}

void AkGLCompositor::setCanvasColor(QRgb canvasColor)
{
    if (this->d->m_canvasColor == canvasColor)
        return;

    this->d->m_canvasColor = canvasColor;
    emit this->canvasColorChanged(canvasColor);
}

bool AkGLCompositor::setState(AkElement::ElementState state)
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

void AkGLCompositor::resetOutputCaps()
{
    this->setOutputCaps({AkVideoCaps::Format_rgba, 640, 480, {30, 1}});
}

void AkGLCompositor::resetCanvasColor()
{
    this->setCanvasColor(qRgba(0, 0, 0, 0));
}

void AkGLCompositor::setEffects(const QStringList &effects)
{
    bool wasEmpty = this->isEmpty();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.setEffects(effects);
    }

    emit this->effectsChanged(effects);
    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

void AkGLCompositor::setPreview(const QString &preview)
{
    bool wasEmpty = this->isEmpty();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.setPreview(preview);
    }

    emit this->previewChanged(preview);

    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

void AkGLCompositor::setChainEffects(bool chainEffects)
{
    this->d->m_pipeline.setChainEffects(chainEffects);
    emit this->chainEffectsChanged(chainEffects);
}

void AkGLCompositor::setPreserveNullPlugins(bool preserveNullPlugins)
{
    this->d->m_pipeline.setPreserveNullPlugins(preserveNullPlugins);
    emit this->preserveNullPluginsChanged(preserveNullPlugins);
}

void AkGLCompositor::resetEffects()
{
    this->setEffects({});
}

void AkGLCompositor::resetPreview()
{
    this->setPreview({});
}

void AkGLCompositor::resetChainEffects()
{
    this->setChainEffects(false);
}

void AkGLCompositor::resetPreserveNullPlugins()
{
    this->setPreserveNullPlugins(false);
}

void AkGLCompositor::moveEffect(int from, int to)
{
    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.moveEffect(from, to);
    }

    emit this->effectsChanged(this->effects());
}

void AkGLCompositor::removeEffect(int index)
{
    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.removeEffect(index);
    }

    emit this->effectsChanged(this->effects());
}

void AkGLCompositor::removeAllEffects()
{
    if (this->isEmpty())
        return;

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.removeAllEffects();
    }

    emit this->effectsChanged({});
    emit this->isEmptyChanged(true);
}

void AkGLCompositor::applyPreview()
{
    bool wasEmpty = this->isEmpty();

    {
        QMutexLocker mutexLocker(&this->d->m_effectsMutex);
        this->d->m_pipeline.applyPreview();
    }

    emit this->previewChanged({});
    emit this->effectsChanged(this->effects());

    bool isEmpty = this->isEmpty();

    if (wasEmpty != isEmpty)
        emit this->isEmptyChanged(isEmpty);
}

void AkGLCompositor::setSourceRect(qint64 id, const QRectF &rect)
{
    bool changed = false;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        {
            QMutexLocker sourceLocker(&source->mutex);

            if (source->rect == rect)
                return;

            source->rect = rect;
            changed = true;
        }
    }

    if (changed)
        emit this->sourceRectChanged(id, rect);
}

void AkGLCompositor::setSourceZOrder(qint64 id, int zOrder)
{
    bool changed = false;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        {
            QMutexLocker sourceLocker(&source->mutex);

            if (source->zOrder == zOrder)
                return;

            source->zOrder = zOrder;
            changed = true;
        }

        if (changed)
            this->d->m_zOrderDirty = true;
    }

    if (changed)
        emit this->sourceZOrderChanged(id, zOrder);
}

void AkGLCompositor::setSourceOpacity(qint64 id, qreal opacity)
{
    bool changed = false;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        {
            QMutexLocker sourceLocker(&source->mutex);

            if (qFuzzyCompare(source->opacity, opacity))
                return;

            source->opacity = opacity;
            changed = true;
        }
    }

    if (changed)
        emit this->sourceOpacityChanged(id, opacity);
}

void AkGLCompositor::setSourceAspectRatioMode(qint64 id,
                                              Qt::AspectRatioMode mode)
{
    bool changed = false;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        {
            QMutexLocker sourceLocker(&source->mutex);

            if (source->aspectRatioMode == mode)
                return;

            source->aspectRatioMode = mode;
            changed = true;
        }
    }

    if (changed)
        emit this->sourceAspectRatioModeChanged(id, mode);
}

void AkGLCompositor::setSourceEffects(qint64 id, const QStringList &effects)
{
    bool wasEmpty = false;
    bool isEmpty = effects.isEmpty();

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        wasEmpty = source->pipeline.isEmpty();
        source->pipeline.setEffects(effects);
    }

    emit this->sourceEffectsChanged(id, effects);

    if (wasEmpty != isEmpty)
        emit this->sourceIsEmptyChanged(id, isEmpty);
}

void AkGLCompositor::setSourcePreview(qint64 id, const QString &preview)
{
    bool wasEmpty = false;
    bool isEmpty = false;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        wasEmpty = source->pipeline.isEmpty();
        source->pipeline.setPreview(preview);
        isEmpty = source->pipeline.isEmpty();
    }

    emit this->sourcePreviewChanged(id, preview);

    if (wasEmpty != isEmpty)
        emit this->sourceIsEmptyChanged(id, isEmpty);
}

void AkGLCompositor::setSourceChainEffects(qint64 id, bool chainEffects)
{
    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        source->pipeline.setChainEffects(chainEffects);
    }

    emit this->sourceChainEffectsChanged(id, chainEffects);
}

void AkGLCompositor::setSourcePreserveNullPlugins(qint64 id,
                                                  bool preserveNullPlugins)
{
    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        source->pipeline.setPreserveNullPlugins(preserveNullPlugins);
    }

    emit this->sourcePreserveNullPluginsChanged(id, preserveNullPlugins);
}

void AkGLCompositor::resetSourceEffects(qint64 id)
{
    this->setSourceEffects(id, {});
}

void AkGLCompositor::resetSourcePreview(qint64 id)
{
    this->setSourcePreview(id, {});
}

void AkGLCompositor::resetSourceChainEffects(qint64 id)
{
    this->setSourceChainEffects(id, false);
}

void AkGLCompositor::resetSourcePreserveNullPlugins(qint64 id)
{
    this->setSourcePreserveNullPlugins(id, false);
}

void AkGLCompositor::moveSourceEffect(qint64 id, int from, int to)
{
    QStringList effects;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        source->pipeline.moveEffect(from, to);
        effects = source->pipeline.effects();
    }

    emit this->sourceEffectsChanged(id, effects);
}

void AkGLCompositor::removeSourceEffect(qint64 id, int index)
{
    QStringList effects;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        source->pipeline.removeEffect(index);
        effects = source->pipeline.effects();
    }

    emit this->sourceEffectsChanged(id, effects);
}

void AkGLCompositor::removeAllSourceEffects(qint64 id)
{
    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        if (source->pipeline.isEmpty())
            return;

        source->pipeline.removeAllEffects();
    }

    emit this->sourceEffectsChanged(id, {});
    emit this->sourceIsEmptyChanged(id, true);
}

void AkGLCompositor::applySourcePreview(qint64 id)
{
    bool wasEmpty = false;
    QStringList effects;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        auto source = this->d->m_sources.value(id, nullptr);

        if (!source)
            return;

        wasEmpty = source->pipeline.isEmpty();
        source->pipeline.applyPreview();
        effects = source->pipeline.effects();
    }

    emit this->sourcePreviewChanged(id, {});
    emit this->sourceEffectsChanged(id, effects);

    bool isEmpty = effects.isEmpty();

    if (wasEmpty != isEmpty)
        emit this->sourceIsEmptyChanged(id, isEmpty);
}

void AkGLCompositor::addPacketReader()
{
    this->d->m_packetReaders++;
}

void AkGLCompositor::removePacketReader()
{
    this->d->m_packetReaders--;
}

AkPacket AkGLCompositor::iVideoStream(const AkVideoPacket &videoPacket)
{
    if (!videoPacket)
        return {};

    auto id = videoPacket.id();
    AkGLCompositorSourcePtr source;

    {
        QMutexLocker mutexLocker(&this->d->m_sourcesMutex);
        source = this->d->m_sources.value(id, nullptr);
    }

    if (!source) {
        qWarning() << "AkGLCompositor::iStream: packet with unknown source id"
                   << id;

        return {};
    }

    bool firstFrame = false;

    {
        QMutexLocker sourceLocker(&source->mutex);

        source->converter.begin();
        auto rgbaPacket = source->converter.convert(videoPacket);
        source->converter.end();

        if (!rgbaPacket)
            return {};

        firstFrame = !source->hasFrame;
        source->lastPacket = rgbaPacket;
        source->hasFrame = true;
    }

    if (firstFrame) {
        QMutexLocker compositorLocker(&this->d->m_sourcesMutex);
        this->d->m_zOrderDirty = true;
    }

    return {};
}

void AkGLCompositor::updateAvailableEffects()
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

void AkGLCompositor::registerTypes()
{
    qRegisterMetaType<AkGLCompositor>("AkGLCompositor");
    qmlRegisterSingletonType<AkGLCompositor>("Ak", 1, 0, "AkGLCompositor",
                                              [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkGLCompositor();
    });
}

AkGLCompositorPrivate::AkGLCompositorPrivate(AkGLCompositor *self):
    self(self)
{
}

void AkGLCompositorPrivate::startRenderThread()
{
    if (this->m_renderThread)
        return;

    this->m_renderThread = QThread::create([this] { this->renderGL(); });
    this->m_renderThread->start();
}

void AkGLCompositorPrivate::stopRenderThread()
{
    if (!this->m_renderThread)
        return;

    {
        QMutexLocker mutexLocker(&this->m_stopMutex);
        this->m_stopRequested = true;
    }

    this->m_renderThread->wait();
    delete this->m_renderThread;
    this->m_renderThread = nullptr;

    this->m_stopRequested = false;
}

void AkGLCompositorPrivate::renderGL()
{
    this->m_surface = new QOffscreenSurface;
    QSurfaceFormat fmt;

#if defined(Q_OS_ANDROID)
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    fmt.setVersion(2, 0);
#elif defined(Q_OS_MAC)
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setVersion(4, 1);
#else
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setVersion(2, 1);
#endif

    this->m_surface->setFormat(fmt);
    this->m_surface->create();

    this->initGL();

    this->m_pipeline.init(self, this->m_vbo, this->m_ibo);
    emit self->ready();

    this->m_clock.start();
    auto fps = this->m_outputCaps.fps();
    auto fpsValue = fps.value() > 0.0? fps.value(): 30.0;
    auto tickInterval =
            std::chrono::nanoseconds(qint64(1'000'000'000.0 / fpsValue));
    auto nextTick = std::chrono::steady_clock::now();
    this->m_id = Ak::id();

    while (true) {
        {
            QMutexLocker mutexLocker(&this->m_stopMutex);

            if (this->m_stopRequested)
                break;
        }

        nextTick += tickInterval;

        if (self->state() == AkElement::ElementStatePlaying)
            this->processTick();

        auto now = std::chrono::steady_clock::now();
        auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(nextTick - now);

        if (remaining.count() > 0)
            QThread::usleep(static_cast<unsigned long>(remaining.count()));
    }

    this->m_pipeline.uninit();
    {
        QMutexLocker mutexLocker(&this->m_sourcesMutex);

        for (auto &source: this->m_sources) {
            source->pipeline.uninit();
            source->glInitialized = false;
        }
    }

    this->uninitGL();

    delete this->m_surface;
    this->m_surface = nullptr;

    if (this->m_uploadBuffer) {
        delete [] this->m_uploadBuffer;
        this->m_uploadBuffer = nullptr;
    }

    if (this->m_readbackBuffer) {
        delete [] this->m_readbackBuffer;
        this->m_readbackBuffer = nullptr;
    }
}

void AkGLCompositorPrivate::processPendingRemovals()
{
    QVector<qint64> pending;

    {
        QMutexLocker mutexLocker(&this->m_pendingRemovalsMutex);

        if (this->m_pendingRemovals.isEmpty())
            return;

        pending = this->m_pendingRemovals;
        this->m_pendingRemovals.clear();
    }

    for (auto id: pending) {
        AkGLCompositorSourcePtr source;

        {
            QMutexLocker mutexLocker(&this->m_sourcesMutex);
            source = this->m_sources.take(id);
        }

        if (!source)
            continue;

        source->pipeline.uninit();

        if (source->uploadTex) {
            if (source->uploadTex->isCreated())
                source->uploadTex->destroy();

            delete source->uploadTex;
        }

        delete source->entryFbo;
        delete source->effectFbo;

        emit self->sourceRemoved(id);
    }

    this->m_zOrderDirty = true;
}

void AkGLCompositorPrivate::processTick()
{
    this->processPendingRemovals();
    this->ensureFboSize(this->m_canvasFbo,
                        this->m_outputCaps.width(),
                        this->m_outputCaps.height());

    {
        QMutexLocker mutexLocker(&this->m_sourcesMutex);

        for (auto &source: this->m_sources) {
            QMutexLocker sourceLocker(&source->mutex);

            if (!source->glInitialized) {
                source->pipeline.init(self, this->m_vbo, this->m_ibo);
                source->glInitialized = true;
            }

            if (source->hasFrame) {
                this->uploadSource(source);
                this->ensureFboSize(source->effectFbo,
                                    source->entryFbo->width(),
                                    source->entryFbo->height());
                auto pts = source->lastPacket.pts()
                        * source->lastPacket.timeBase().value();
                source->pipeline.process(source->entryFbo,
                                        source->effectFbo,
                                        source->lastPacket.id(),
                                        pts);
            }
        }
    }

    this->compositeSourcesIntoCanvas();
    auto pts = this->m_clock.nsecsElapsed();

    this->ensureFboSize(this->m_effectFbo,
                        this->m_canvasFbo->width(),
                        this->m_canvasFbo->height());
    this->m_pipeline.process(this->m_canvasFbo,
                                this->m_effectFbo,
                                this->m_id,
                                pts / 1e9);
    emit self->outputTextureReady(this->m_effectFbo->texture(),
                                  this->m_effectFbo->size());


    if (this->m_packetReaders > 0)
        this->readAndEmit(pts / 1000);
}

void AkGLCompositorPrivate::uploadSource(AkGLCompositorSourcePtr source)
{
    auto &packet = source->lastPacket;
    int width = packet.caps().width();
    int height = packet.caps().height();

    if (!source->uploadTex)
         source->uploadTex = new QOpenGLTexture(QOpenGLTexture::Target2D);

    if (!source->uploadTex->isCreated()
        || source->uploadTex->width() != width
        || source->uploadTex->height() != height) {
        if (source->uploadTex->isCreated())
            source->uploadTex->destroy();

        source->uploadTex->setSize(width, height);
        source->uploadTex->setFormat(QOpenGLTexture::RGBA8_UNorm);
        source->uploadTex->setMinificationFilter(QOpenGLTexture::Linear);
        source->uploadTex->setMagnificationFilter(QOpenGLTexture::Linear);
        source->uploadTex->setWrapMode(QOpenGLTexture::ClampToEdge);
        source->uploadTex->allocateStorage();
    }

    size_t packetLineSize = packet.lineSize(0);
    size_t srcLineSize = size_t(width) * 4;

    source->uploadTex->bind();

    if (packetLineSize == srcLineSize) {
        self->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                              GL_RGBA, GL_UNSIGNED_BYTE,
                              packet.constData());
    } else {
        size_t uploadBufferSize = srcLineSize * height;

        if (this->m_uploadBufferSize != uploadBufferSize) {
            if (this->m_uploadBuffer)
                delete [] this->m_uploadBuffer;

            this->m_uploadBuffer = new quint8 [uploadBufferSize];
            this->m_uploadBufferSize = uploadBufferSize;
        }

        auto src = reinterpret_cast<const quint8 *>(packet.constData());
        auto dst = this->m_uploadBuffer;
        size_t copyLineSize = qMin<size_t>(srcLineSize, packetLineSize);

        for (int y = 0; y < height; ++y)
            memcpy(dst + y * srcLineSize,
                   src + y * packetLineSize,
                   copyLineSize);

            self->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                                  GL_RGBA, GL_UNSIGNED_BYTE,
                                  dst);
    }

    source->uploadTex->release();

    this->ensureFboSize(source->entryFbo, width, height);
    source->entryFbo->bind();
    this->blitTexture(source->uploadTex->textureId(), width, height);
    source->entryFbo->release();
}

void AkGLCompositorPrivate::compositeSourcesIntoCanvas()
{
    this->m_canvasFbo->bind();
    self->glViewport(0, 0,
                     this->m_canvasFbo->width(),
                     this->m_canvasFbo->height());
    self->glClearColor(qRed(this->m_canvasColor) / 255.0f,
                       qGreen(this->m_canvasColor) / 255.0f,
                       qBlue(this->m_canvasColor) / 255.0f,
                       qAlpha(this->m_canvasColor) / 255.0f);
    self->glClear(GL_COLOR_BUFFER_BIT);

    self->glEnable(GL_BLEND);
    self->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (this->m_zOrderDirty) {
        QMutexLocker mutexLocker(&this->m_sourcesMutex);
        this->m_orderedSources.clear();

        for (auto &source: this->m_sources) {
            QMutexLocker sourceLocker(&source->mutex);

            if (source->hasFrame && source->entryFbo) {
                SourceSnapshot snap;
                snap.source = source;
                snap.rect = source->rect;
                snap.opacity = source->opacity;
                snap.aspectRatioMode = source->aspectRatioMode;
                snap.texW = source->entryFbo ? source->entryFbo->width() : 0;
                snap.texH = source->entryFbo ? source->entryFbo->height() : 0;
                this->m_orderedSources << snap;
            }
        }

        std::sort(this->m_orderedSources.begin(),
                  this->m_orderedSources.end(),
                  [] (const SourceSnapshot &a, const SourceSnapshot &b) {
                      return a.source->zOrder < b.source->zOrder;
                  });

        this->m_zOrderDirty = false;
    }

    this->m_compositeShader->bind();
    this->bindCompositeAttribs();

    for (const auto &snap: std::as_const(this->m_orderedSources)) {
        auto transform = this->computeSourceTransform(snap);
        self->glUniformMatrix4fv(this->m_compTransformUniform,
                                 1,
                                 GL_FALSE,
                                 transform.constData());
        self->glUniform1f(this->m_compOpacityUniform, GLfloat(snap.opacity));
        self->glActiveTexture(GL_TEXTURE0);
        self->glBindTexture(GL_TEXTURE_2D, snap.source->effectFbo->texture());
        self->glUniform1i(this->m_compTexUniform, 0);
        self->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    self->glDisableVertexAttribArray(this->m_compPosAttr);
    self->glDisableVertexAttribArray(this->m_compTexAttr);
    self->glBindTexture(GL_TEXTURE_2D, 0);
    self->glDisable(GL_BLEND);

    this->m_canvasFbo->release();
}

QMatrix4x4 AkGLCompositorPrivate::computeSourceTransform(const SourceSnapshot &snap) const
{
    float canvasW = float(this->m_outputCaps.width());
    float canvasH = float(this->m_outputCaps.height());

    float rx = float(snap.rect.x());
    float ry = float(snap.rect.y());
    float rw = float(snap.rect.width());
    float rh = float(snap.rect.height());
    float fittedW = rw;
    float fittedH = rh;

    if (snap.aspectRatioMode != Qt::IgnoreAspectRatio
        && snap.texW > 0 && snap.texH > 0) {
        float texAspect = float(snap.texW) / float(snap.texH);
        float rectPixelW = rw * canvasW;
        float rectPixelH = rh * canvasH;
        float rectAspect = rectPixelW / rectPixelH;

        if (snap.aspectRatioMode == Qt::KeepAspectRatio) {
            if (texAspect > rectAspect) {
                fittedW = rw;
                fittedH = fittedW * canvasW / (texAspect * canvasH);
            } else {
                fittedH = rh;
                fittedW = fittedH * texAspect * canvasH / canvasW;
            }
        } else if (snap.aspectRatioMode == Qt::KeepAspectRatioByExpanding) {
            if (texAspect > rectAspect) {
                fittedH = rh;
                fittedW = fittedH * texAspect * canvasH / canvasW;
            } else {
                fittedW = rw;
                fittedH = fittedW * canvasW / (texAspect * canvasH);
            }
        }
    }

    float cx = rx + rw / 2.0f;
    float cy = ry + rh / 2.0f;
    float ndcCx = cx * 2.0f - 1.0f;
    float ndcCy = cy * 2.0f - 1.0f;

    QMatrix4x4 transform;
    transform.setToIdentity();
    transform.translate(ndcCx, ndcCy, 0.0f);
    transform.scale(fittedW, fittedH, 1.0f);

    return transform;
}

void AkGLCompositorPrivate::readAndEmit(qint64 pts)
{
    auto outSize = this->m_canvasFbo->size();
    auto currentFps = this->m_outputCaps.fps();

    if (!this->m_outputPacket
        || this->m_outputPacket.caps().width() != outSize.width()
        || this->m_outputPacket.caps().height() != outSize.height()
        || this->m_outputPacket.caps().fps() != currentFps) {
        AkVideoCaps rgbaCaps(AkVideoCaps::Format_rgba,
                             outSize.width(),
                             outSize.height(),
                             currentFps);
        this->m_outputPacket = AkVideoPacket(rgbaCaps);
    }

    this->m_outputPacket.setPts(pts);
    this->m_outputPacket.setTimeBase({1, 1'000'000});

    size_t packetLineSize = this->m_outputPacket.lineSize(0);
    int outWidth = outSize.width();
    int outHeight = outSize.height();

    this->m_effectFbo->bind();
    size_t outLineSize =  4 * outWidth;

    if (packetLineSize == outLineSize) {
        self->glReadPixels(0, 0, outWidth, outHeight,
                           GL_RGBA, GL_UNSIGNED_BYTE,
                           this->m_outputPacket.data());
    } else {
        size_t readbackBufferSize = outLineSize * outHeight;

        if (this->m_readbackBufferSize != readbackBufferSize) {
            if (this->m_readbackBuffer)
                delete [] this->m_readbackBuffer;

            this->m_readbackBuffer = new quint8 [readbackBufferSize];
            this->m_readbackBufferSize = readbackBufferSize;
        }

        self->glReadPixels(0, 0, outWidth, outHeight,
                           GL_RGBA, GL_UNSIGNED_BYTE,
                           this->m_readbackBuffer);
        auto dst = reinterpret_cast<quint8 *>(this->m_outputPacket.data());
        size_t copyBytes = qMin<size_t>(outLineSize, packetLineSize);

        for (int y = 0; y < outHeight; ++y)
            memcpy(dst + y * packetLineSize,
                   this->m_readbackBuffer + y * outLineSize,
                   copyBytes);
    }

    this->m_effectFbo->release();

    this->m_outputConverter.setOutputCaps(this->m_outputCaps);
    this->m_outputConverter.begin();
    auto dst = this->m_outputConverter.convert(this->m_outputPacket);
    this->m_outputConverter.end();

    if (!dst)
        return;

    if (this->m_packetMutex.tryLock()) {
        emit self->oStream(dst);
        this->m_packetMutex.unlock();
    }
}

void AkGLCompositorPrivate::blitTexture(GLuint tex, int width, int height)
{
    self->glViewport(0, 0, width, height);

    this->m_blitShader->bind();
    this->bindBlitAttribs();

    self->glActiveTexture(GL_TEXTURE0);
    self->glBindTexture(GL_TEXTURE_2D, tex);
    self->glUniform1i(this->m_blitTexUniform, 0);

    self->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    self->glDisableVertexAttribArray(this->m_blitPosAttr);
    self->glDisableVertexAttribArray(this->m_blitTexAttr);
    self->glBindTexture(GL_TEXTURE_2D, 0);
}

void AkGLCompositorPrivate::ensureFboSize(QOpenGLFramebufferObject *&fbo,
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

void AkGLCompositorPrivate::bindBlitAttribs()
{
    this->m_vbo->bind();
    this->m_ibo->bind();

    self->glEnableVertexAttribArray(this->m_blitPosAttr);
    self->glVertexAttribPointer(this->m_blitPosAttr, 3, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float), reinterpret_cast<void *>(0));

    self->glEnableVertexAttribArray(this->m_blitTexAttr);
    self->glVertexAttribPointer(this->m_blitTexAttr, 2, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float),
                                reinterpret_cast<void *>(3 * sizeof(float)));
}

void AkGLCompositorPrivate::bindCompositeAttribs()
{
    this->m_vbo->bind();
    this->m_ibo->bind();

    self->glEnableVertexAttribArray(this->m_compPosAttr);
    self->glVertexAttribPointer(this->m_compPosAttr, 3, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float), reinterpret_cast<void *>(0));

    self->glEnableVertexAttribArray(this->m_compTexAttr);
    self->glVertexAttribPointer(this->m_compTexAttr, 2, GL_FLOAT, GL_FALSE,
                                5 * sizeof(float),
                                reinterpret_cast<void *>(3 * sizeof(float)));
}

void AkGLCompositorPrivate::initGL()
{
    QSurfaceFormat fmt;
    fmt.setVersion(2, 0);
    fmt.setProfile(QSurfaceFormat::CoreProfile);

    this->m_context = new QOpenGLContext;
    this->m_context->setFormat(this->m_surface->format());
    this->m_context->create();
    this->m_context->makeCurrent(this->m_surface);

    self->initializeOpenGLFunctions();

    // Full-screen quad.
    static const float akGLCompositorQuadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    };

    this->m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    this->m_vbo->create();
    this->m_vbo->bind();
    this->m_vbo->allocate(akGLCompositorQuadVertices,
                          sizeof(akGLCompositorQuadVertices));
    this->m_vbo->release();

    static const unsigned int akGLCompositorQuadIndices[] = {
        0, 1, 2, 2, 3, 0
    };

    this->m_ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    this->m_ibo->create();
    this->m_ibo->bind();
    this->m_ibo->allocate(akGLCompositorQuadIndices,
                          sizeof(akGLCompositorQuadIndices));
    this->m_ibo->release();

    // Blit shader
    this->m_blitShader = new QOpenGLShaderProgram;

    this->m_blitShader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                ":/Ak/share/shaders/glpipeline/blit.vert");
    this->m_blitShader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                ":/Ak/share/shaders/glpipeline/blit.frag");
    this->m_blitShader->link();

    this->m_blitPosAttr = this->m_blitShader->attributeLocation("aPos");
    this->m_blitTexAttr = this->m_blitShader->attributeLocation("aTexCoord");
    this->m_blitTexUniform = this->m_blitShader->uniformLocation("uTex");

    // Composite shader
    this->m_compositeShader = new QOpenGLShaderProgram;

    this->m_compositeShader->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                     ":/Ak/share/shaders/glpipeline/composite.vert");
    this->m_compositeShader->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                     ":/Ak/share/shaders/glpipeline/composite.frag");
    this->m_compositeShader->link();

    this->m_compPosAttr = this->m_compositeShader->attributeLocation("aPos");
    this->m_compTexAttr = this->m_compositeShader->attributeLocation("aTexCoord");
    this->m_compTransformUniform = this->m_compositeShader->uniformLocation("uTransform");
    this->m_compOpacityUniform = this->m_compositeShader->uniformLocation("uOpacity");
    this->m_compTexUniform = this->m_compositeShader->uniformLocation("uTex");

    this->ensureFboSize(this->m_canvasFbo,
                        this->m_outputCaps.width(),
                        this->m_outputCaps.height());
}

void AkGLCompositorPrivate::uninitGL()
{
    {
        QMutexLocker mutexLocker(&this->m_sourcesMutex);

        for (auto &source: this->m_sources) {
            source->pipeline.uninit();

            if (source->uploadTex) {
                if (source->uploadTex->isCreated())
                    source->uploadTex->destroy();

                delete source->uploadTex;
                source->uploadTex = nullptr;
            }

            delete source->entryFbo;
            source->entryFbo = nullptr;
            delete source->effectFbo;
            source->effectFbo = nullptr;
        }
    }

    delete this->m_canvasFbo;
    this->m_canvasFbo = nullptr;
    delete this->m_effectFbo;
    this->m_effectFbo = nullptr;

    if (this->m_compositeShader) {
        delete this->m_compositeShader;
        this->m_compositeShader = nullptr;
    }

    if (this->m_blitShader) {
        delete this->m_blitShader;
        this->m_blitShader = nullptr;
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

    if (this->m_context) {
        this->m_context->doneCurrent();
        delete this->m_context;
        this->m_context = nullptr;
    }
}

#include "moc_akglcompositor.cpp"
