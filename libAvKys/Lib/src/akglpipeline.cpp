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
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QQmlEngine>
#include <QVector>

#include "akglpipeline.h"
#include "akplugininfo.h"
#include "akpluginmanager.h"

class AkGLPipelineEffect
{
    public:
        AkVideoEffectPtr element;
        AkPluginInfo info;

        AkGLPipelineEffect()
        {
        }

        AkGLPipelineEffect(const AkVideoEffectPtr &element,
                           const AkPluginInfo &info):
            element(element),
            info(info)
        {
        }
};

class AkGLPipelinePrivate
{
    public:
        // Control thread
        QMutex m_requestMutex;
        QVector<AkGLPipelineEffect> m_requestEffects;
        AkGLPipelineEffect m_requestPreview;
        bool m_requestChainEffects {false};
        bool m_requestDirty {false};
        bool m_preserveNullPlugins {false};

        // Render thread
        QVector<AkGLPipelineEffect> m_activeEffects;
        AkGLPipelineEffect m_activePreview;
        bool m_activeChainEffects {false};

        // OpenGL context
        QOpenGLFunctions *m_gl {nullptr};
        QOpenGLBuffer *m_vbo {nullptr};
        QOpenGLBuffer *m_ibo {nullptr};
        QOpenGLFramebufferObject *m_fbo[2] = {nullptr, nullptr};

        void applyPendingRequest();
        void initEffect(const AkVideoEffectPtr &effect);
        void ensureFboSize(QOpenGLFramebufferObject *&fbo,
                           int width,
                           int height);
};

AkGLPipeline::AkGLPipeline(QObject *parent):
    QObject(parent)
{
    this->d = new AkGLPipelinePrivate;
}

AkGLPipeline::~AkGLPipeline()
{
    delete this->d;
}

QStringList AkGLPipeline::effects() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);
    QStringList ids;

    for (auto &effect: this->d->m_requestEffects)
        ids << effect.info.id();

    return ids;
}

QString AkGLPipeline::preview() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (!this->d->m_requestPreview.element)
        return {};

    return this->d->m_requestPreview.info.id();
}

bool AkGLPipeline::chainEffects() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    return this->d->m_requestChainEffects;
}

bool AkGLPipeline::preserveNullPlugins() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    return this->d->m_preserveNullPlugins;
}

bool AkGLPipeline::isEmpty() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    return this->d->m_requestEffects.isEmpty()
           && !this->d->m_requestPreview.element;
}

AkVideoEffectPtr AkGLPipeline::elementAt(int index) const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (index < 0 || index >= this->d->m_requestEffects.size())
        return {};

    return this->d->m_requestEffects.at(index).element;
}

AkVideoEffectPtr AkGLPipeline::previewElement() const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    return this->d->m_requestPreview.element;
}

AkPluginInfo AkGLPipeline::effectInfo(int index) const
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (index < 0 || index >= this->d->m_requestEffects.size())
        return {};

    return this->d->m_requestEffects.at(index).info;
}

void AkGLPipeline::init(QOpenGLFunctions *glFunctions,
                        QOpenGLBuffer *vbo,
                        QOpenGLBuffer *ibo)
{
    this->d->m_gl = glFunctions;
    this->d->m_vbo = vbo;
    this->d->m_ibo = ibo;

    QMutexLocker mutexLocker(&this->d->m_requestMutex);
    this->d->m_requestDirty = true;
}

void AkGLPipeline::uninit()
{
    for (auto &effect: this->d->m_activeEffects)
        if (effect.element)
            effect.element->uninit();

    if (this->d->m_activePreview.element)
        this->d->m_activePreview.element->uninit();

    this->d->m_activeEffects.clear();
    this->d->m_activePreview = {};

    for (auto &fbo: this->d->m_fbo) {
        delete fbo;
        fbo = nullptr;
    }

    this->d->m_gl = nullptr;
    this->d->m_vbo = nullptr;
    this->d->m_ibo = nullptr;
}

void AkGLPipeline::process(QOpenGLFramebufferObject *inputFbo,
                           QOpenGLFramebufferObject *&outputFbo,
                           qint64 streamId,
                           qreal pts)
{
    this->d->applyPendingRequest();

    if (this->d->m_activeEffects.isEmpty()
        && !this->d->m_activePreview.element) {
        if (inputFbo != outputFbo)
            QOpenGLFramebufferObject::blitFramebuffer(outputFbo,
                                                      inputFbo,
                                                      GL_COLOR_BUFFER_BIT,
                                                      GL_NEAREST);

        return;
    }

    auto src = inputFbo;
    int dstIdx = 0;

    auto runStep = [this, &src, &dstIdx, streamId, pts] (const AkVideoEffectPtr &element) {
        this->d->ensureFboSize(this->d->m_fbo[dstIdx], src->width(), src->height());
        element->process(src, this->d->m_fbo[dstIdx], streamId, pts);
        src = this->d->m_fbo[dstIdx];
        dstIdx = 1 - dstIdx;
    };

    if (this->d->m_activeChainEffects || !this->d->m_activePreview.element)
        for (auto &effect: this->d->m_activeEffects)
            if (effect.element)
                runStep(effect.element);

    if (this->d->m_activePreview.element)
        runStep(this->d->m_activePreview.element);

    if (src != outputFbo)
        QOpenGLFramebufferObject::blitFramebuffer(outputFbo,
                                                  src,
                                                  GL_COLOR_BUFFER_BIT,
                                                  GL_NEAREST);
}

void AkGLPipeline::setEffects(const QStringList &effectIds)
{
    {
        QMutexLocker mutexLocker(&this->d->m_requestMutex);
        QStringList curIds;

        for (auto &effect: this->d->m_requestEffects)
            curIds << effect.info.id();

        if (curIds == effectIds)
            return;

        QVector<AkGLPipelineEffect> newEffects;

        for (auto &effectId: effectIds) {
            auto effect = akPluginManager->create<AkVideoEffect>(effectId);

            if (effect)
                newEffects << AkGLPipelineEffect(effect,
                                                 akPluginManager->pluginInfo(effectId));
            else if (this->d->m_preserveNullPlugins)
                newEffects << AkGLPipelineEffect();
        }

        this->d->m_requestEffects = newEffects;
        this->d->m_requestDirty = true;
    }

    emit this->effectsChanged(effectIds);
}

void AkGLPipeline::setPreview(const QString &effectId)
{
    {
        QMutexLocker mutexLocker(&this->d->m_requestMutex);

        if (this->d->m_requestPreview.info.id() == effectId)
            return;

        if (effectId.isEmpty()) {
            this->d->m_requestPreview = {};
        } else {
            auto effect = akPluginManager->create<AkVideoEffect>(effectId);
            this->d->m_requestPreview =
                    AkGLPipelineEffect(effect, akPluginManager->pluginInfo(effectId));
        }

        this->d->m_requestDirty = true;
    }

    emit this->previewChanged(effectId);
}

void AkGLPipeline::setChainEffects(bool chainEffects)
{
    {
        QMutexLocker mutexLocker(&this->d->m_requestMutex);

        if (this->d->m_requestChainEffects == chainEffects)
            return;

        this->d->m_requestChainEffects = chainEffects;
        this->d->m_requestDirty = true;
    }

    emit this->chainEffectsChanged(chainEffects);
}

void AkGLPipeline::setPreserveNullPlugins(bool preserveNullPlugins)
{
    {
        QMutexLocker mutexLocker(&this->d->m_requestMutex);

        if (this->d->m_preserveNullPlugins == preserveNullPlugins)
            return;

        this->d->m_preserveNullPlugins = preserveNullPlugins;
    }

    emit this->preserveNullPluginsChanged(preserveNullPlugins);
}

void AkGLPipeline::resetEffects()
{
    this->setEffects({});
}

void AkGLPipeline::resetPreview()
{
    this->setPreview("");
}

void AkGLPipeline::resetChainEffects()
{
    this->setChainEffects(false);
}

void AkGLPipeline::resetPreserveNullPlugins()
{
    this->setPreserveNullPlugins(false);
}

void AkGLPipeline::moveEffect(int from, int to)
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (from == to
        || from < 0
        || from >= this->d->m_requestEffects.size()
        || to < 0
        || to >= this->d->m_requestEffects.size())
        return;

    this->d->m_requestEffects.move(from, to);
    this->d->m_requestDirty = true;
}

void AkGLPipeline::removeEffect(int index)
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (index < 0 || index >= this->d->m_requestEffects.size())
        return;

    this->d->m_requestEffects.removeAt(index);
    this->d->m_requestDirty = true;
}

void AkGLPipeline::removeAllEffects()
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (this->d->m_requestEffects.isEmpty())
        return;

    this->d->m_requestEffects.clear();
    this->d->m_requestDirty = true;
}

void AkGLPipeline::applyPreview()
{
    QMutexLocker mutexLocker(&this->d->m_requestMutex);

    if (!this->d->m_requestPreview.element)
        return;

    if (this->d->m_requestChainEffects
        && !this->d->m_requestEffects.isEmpty()) {
        bool alreadyInChain = false;

        for (auto &effect: this->d->m_requestEffects)
            if (effect.info.id() == this->d->m_requestPreview.info.id()) {
                alreadyInChain = true;

                break;
            }

        if (!alreadyInChain)
            this->d->m_requestEffects << this->d->m_requestPreview;
    } else {
        this->d->m_requestEffects.clear();
        this->d->m_requestEffects << this->d->m_requestPreview;
    }

    this->d->m_requestPreview = {};
    this->d->m_requestDirty = true;
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

void AkGLPipelinePrivate::applyPendingRequest()
{
    QVector<AkGLPipelineEffect> newEffects;
    AkGLPipelineEffect newPreview;
    bool chainEffects;

    {
         QMutexLocker mutexLocker(&this->m_requestMutex);

         if (!this->m_requestDirty)
             return;

         newEffects = this->m_requestEffects;
         newPreview = this->m_requestPreview;
         chainEffects = this->m_requestChainEffects;
         this->m_requestDirty = false;
     }

     for (auto &effect: this->m_activeEffects)
         if (effect.element)
             effect.element->uninit();

     if (this->m_activePreview.element)
         this->m_activePreview.element->uninit();

     for (auto &effect: newEffects)
         if (effect.element)
             this->initEffect(effect.element);

     if (newPreview.element)
         this->initEffect(newPreview.element);

     this->m_activeEffects = newEffects;
     this->m_activePreview = newPreview;
     this->m_activeChainEffects = chainEffects;
}

void AkGLPipelinePrivate::initEffect(const AkVideoEffectPtr &effect)
{
    effect->setGLFunctions(this->m_gl);

    if (!effect->init(this->m_vbo, this->m_ibo))
        qWarning() << "AkGLPipeline: effect failed to initialize:" << effect.get();
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

#include "moc_akglpipeline.cpp"
