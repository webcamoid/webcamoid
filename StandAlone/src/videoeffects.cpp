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

#include <QMutex>
#include <QSettings>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <akcaps.h>
#include <akglpipeline.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "videoeffects.h"
#include "videodisplay.h"

enum EffectType
{
    EffectType_Cpu,
    EffectType_Gpu
};

class VideoEffect
{
    public:
        EffectType type {EffectType_Cpu};
        AkElementPtr element;
        AkVideoEffectPtr glElement;
        AkPluginInfo info;

        VideoEffect();
        VideoEffect(const AkElementPtr &element,
                    const AkPluginInfo &info);
        VideoEffect(const AkVideoEffectPtr &glElement,
                    const AkPluginInfo &info);
        VideoEffect &operator =(const VideoEffect &other);
};

class VideoEffectsPrivate
{
    public:
        VideoEffects *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_availableEffects;
        QVector<VideoEffect> m_effects;
        QStringList m_cpuEffectIds;
        QStringList m_gpuEffectIds;
        VideoEffect m_preview;
        QVector<AkElementPtr> m_cpuPipeline;
        AkGLPipeline m_glPipeline;
        QMutex m_mutex;
        AkElement::ElementState m_state {AkElement::ElementStateNull};
        bool m_chainEffects {false};

        explicit VideoEffectsPrivate(VideoEffects *self);
        void updateChainEffects();
        void updateEffects();
        void updateEffectsProperties();
        void saveChainEffects(bool chainEffects);
        void saveEffects();
        void saveEffectsProperties();
        void linkPreview();
        void unlinkPreview();
        void linkGLPreview();
        void unlinkGLPreview();
};

VideoEffects::VideoEffects(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoEffectsPrivate(this);
    this->setQmlEngine(engine);
    this->updateAvailableEffects();
    this->d->updateChainEffects();
    this->d->updateEffects();
}

VideoEffects::~VideoEffects()
{
    this->setState(AkElement::ElementStateNull);
    this->d->saveEffectsProperties();
    delete this->d;
}

QStringList VideoEffects::availableEffects() const
{
    return this->d->m_availableEffects;
}

QStringList VideoEffects::effects() const
{
    QStringList effects;

    for (auto &effect: this->d->m_effects)
        effects << effect.info.id();

    return effects;
}

QString VideoEffects::preview() const
{
    if (this->d->m_preview.element)
        return this->d->m_preview.info.id();

    return this->d->m_glPipeline.preview();
}

AkPluginInfo VideoEffects::effectInfo(const QString &effectId) const
{
    return akPluginManager->pluginInfo(effectId);
}

QString VideoEffects::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return {};

    auto info = akPluginManager->pluginInfo(effectId);

    if (!info)
        return {};

    return info.description();
}

AkElement::ElementState VideoEffects::state() const
{
    return this->d->m_state;
}

bool VideoEffects::isGpuEffect(const QString &effectId) const
{
    if (effectId.isEmpty())
        return false;

    auto info = akPluginManager->pluginInfo(effectId);

    if (!info)
        return false;

    return info.id().startsWith("VideoFilterGL/");
}

bool VideoEffects::chainEffects() const
{
    return this->d->m_chainEffects;
}

bool VideoEffects::embedControls(const QString &where,
                                 int effectIndex,
                                 const QString &name) const
{
    auto effect = this->d->m_effects.value(effectIndex);
    QObject *interface = nullptr;

    if (effect.type == EffectType_Cpu) {
        if (!effect.element)
            return false;

        interface = effect.element->controlInterface(this->d->m_engine,
                                                     effect.info.id());
    } else {
        if (!effect.glElement)
            return false;

        interface = effect.glElement->controlInterface(this->d->m_engine,
                                                       effect.info.id());
    }

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        if (!interfaceItem)
            continue;

        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

bool VideoEffects::embedPreviewControls(const QString &where,
                                        const QString &name) const
{
    QObject *interface = nullptr;
    auto glPreview = this->d->m_glPipeline.previewElement();

    if (glPreview) {
        auto info = this->d->m_glPipeline.effectInfo(this->d->m_glPipeline.preview());
        interface = glPreview->controlInterface(this->d->m_engine, info.id());
    } else {
        auto &preview = this->d->m_preview;

        if (!preview.element)
            return false;

        interface = preview.element->controlInterface(this->d->m_engine,
                                                      preview.info.id());
    }

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        if (!interfaceItem)
            continue;

        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

void VideoEffects::removeInterface(const QString &where) const
{
    if (!this->d->m_engine)
        return;

    for (auto &obj: this->d->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        auto childItems = item->childItems();

        for (auto &child: childItems) {
            child->setParentItem(nullptr);
            child->setParent(nullptr);

            delete child;
        }
    }
}

void VideoEffects::setEffects(const QStringList &effects)
{
    if (this->effects() == effects)
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    // Remove old effects
    if (!this->d->m_cpuPipeline.isEmpty()) {
        auto lastElement = this->d->m_cpuPipeline.last();

        if (!this->d->m_glPipeline.isEmpty())
            lastElement->unlink(&this->d->m_glPipeline);

        if (this->d->m_preview.element)
            lastElement->unlink(this->d->m_preview.element);

        this->d->m_cpuPipeline.clear();
    }

    this->d->m_effects.clear();
    this->d->m_cpuEffectIds.clear();
    this->d->m_gpuEffectIds.clear();

    // Populate the effects
    QStringList gpuEffects;

    for (auto &effectId: effects)
        if (this->isGpuEffect(effectId))
            gpuEffects << effectId;

    this->d->m_glPipeline.setEffects(gpuEffects);
    AkElementPtr prevEffect;
    int gpuEffectsIndex = 0;

    for (auto &effectId: effects)
        if (this->isGpuEffect(effectId)) {
            if (auto effect = this->d->m_glPipeline.elementAt(gpuEffectsIndex)) {
                this->d->m_effects << VideoEffect(effect,
                                                  akPluginManager->pluginInfo(effectId));
                this->d->m_gpuEffectIds << effectId;
            }

            gpuEffectsIndex++;
        } else {
            if (auto effect = akPluginManager->create<AkElement>(effectId)) {
                this->d->m_effects << VideoEffect(effect,
                                                  akPluginManager->pluginInfo(effectId));
                this->d->m_cpuPipeline << effect;
                this->d->m_cpuEffectIds << effectId;

                if (prevEffect)
                    prevEffect->link(effect, Qt::DirectConnection);
                else
                    prevEffect = effect;
            }
        }

    // Link the effects to the outputs
    if (!this->d->m_cpuPipeline.isEmpty()) {
        auto lastElement = this->d->m_cpuPipeline.last();

        if (this->d->m_chainEffects && this->d->m_preview.element) {
            // If there is a chained preview, the last element goes connected
            // to the preview
            lastElement->link(this->d->m_preview.element, Qt::DirectConnection);
        } else {
            // Else, connect it to the pipeline
            lastElement->link(this->d->m_glPipeline, Qt::DirectConnection);
        }
    }

    // Re-link the CPU preview if it exists
    if (this->d->m_preview.element)
        this->d->linkPreview();

    this->d->m_mutex.unlock();
    this->setState(state);

    emit this->effectsChanged(effects);
    this->d->saveEffects();
    this->d->updateEffectsProperties();
}

void VideoEffects::setPreview(const QString &preview)
{
    auto oldPreview = this->preview();

    if (oldPreview == preview)
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    // Unlink the old CPU preview
    if (this->d->m_preview.element) {
        if (!this->d->m_effects.isEmpty() && this->d->m_chainEffects) {
            auto lastEffect = this->d->m_effects.last();

            if (lastEffect.type == EffectType_Cpu && lastEffect.element)
                lastEffect.element->unlink(this->d->m_preview.element);
        }

        this->d->m_preview.element->unlink(&this->d->m_glPipeline);
        this->d->unlinkPreview();
        this->d->m_preview = {};
    }

    // Unlink the old GPU preview
    if (!this->d->m_glPipeline.preview().isEmpty()) {
        this->d->unlinkGLPreview();
        this->d->m_glPipeline.setPreview({});
    }

    // Set preview
    QString newPreview;

    if (!preview.isEmpty()) {
        if (this->isGpuEffect(preview)) {
            // GPU preview: delegate to the GL pipeline
            this->d->m_glPipeline.setPreview(preview);
            newPreview = this->d->m_glPipeline.preview();
            this->d->linkGLPreview();
        } else {
            // CPU preview
            this->d->m_preview.element = akPluginManager->create<AkElement>(preview);
            this->d->m_preview.info = akPluginManager->pluginInfo(preview);

            if (this->d->m_preview.element) {
                newPreview = this->d->m_preview.info.id();

                // The preview feeds into the GL pipeline (or outputs directly if empty)
                this->d->m_preview.element->link(&this->d->m_glPipeline,
                                                 Qt::DirectConnection);

                // Connect the preview to the display
                this->d->linkPreview();

                // If chaining, redirect the last CPU effect into the preview
                if (this->d->m_chainEffects && !this->d->m_effects.isEmpty()) {
                    auto lastEffect = this->d->m_effects.last();

                    if (lastEffect.type == EffectType_Cpu) {
                        lastEffect.element->unlink(&this->d->m_glPipeline);
                        lastEffect.element->link(this->d->m_preview.element,
                                                 Qt::DirectConnection);
                    }
                }
            }
        }
    }

    this->d->m_mutex.unlock();
    this->setState(state);

    if (oldPreview != newPreview)
        emit this->previewChanged(newPreview);
}

void VideoEffects::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    this->d->m_mutex.lock();

    if (state == AkElement::ElementStatePlaying) {
        this->d->m_glPipeline.setState(state);

        if (this->d->m_preview.element)
            this->d->m_preview.element->setState(state);

        for (auto it = this->d->m_effects.rbegin();
             it != this->d->m_effects.rend();
             ++it) {
            if (it->type == EffectType_Cpu && it->element)
                it->element->setState(state);
        }
    } else {
        for (auto &effect: this->d->m_effects) {
            if (effect.type == EffectType_Cpu && effect.element)
                effect.element->setState(state);
        }

        if (this->d->m_preview.element)
            this->d->m_preview.element->setState(state);

        this->d->m_glPipeline.setState(state);
    }

    this->d->m_state = state;
    this->d->m_mutex.unlock();

    emit this->stateChanged(state);
}

void VideoEffects::setChainEffects(bool chainEffects)
{
    if (this->d->m_chainEffects == chainEffects)
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    if (this->d->m_preview.element && !this->d->m_effects.isEmpty()) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element) {
            if (chainEffects) {
                lastEffect.element->unlink(&this->d->m_glPipeline);
                lastEffect.element->link(this->d->m_preview.element,
                                         Qt::DirectConnection);
            } else {
                lastEffect.element->unlink(this->d->m_preview.element);
                lastEffect.element->link(&this->d->m_glPipeline,
                                         Qt::DirectConnection);
            }
        }
    }

    this->d->m_chainEffects = chainEffects;
    this->d->m_glPipeline.setChainEffects(chainEffects);
    this->d->m_mutex.unlock();
    this->setState(state);

    emit this->chainEffectsChanged(chainEffects);
    this->d->saveChainEffects(chainEffects);
}

void VideoEffects::resetEffects()
{
    this->setEffects({});
}

void VideoEffects::resetPreview()
{
    this->setPreview({});
}

void VideoEffects::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoEffects::resetChainEffects()
{
    this->setChainEffects(false);
}

void VideoEffects::sendPacket(const AkPacket &packet)
{
    auto _packet = packet;
    _packet.setIndex(0);
    emit this->oStream(_packet);
}

void VideoEffects::applyPreview()
{
    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();
    bool applied = false;
    auto effectsId = this->effects();

    // Case 1: GPU preview - delegate entirely to the GL pipeline
    if (!this->d->m_glPipeline.preview().isEmpty()) {
        this->d->unlinkGLPreview();
        this->d->m_glPipeline.applyPreview();
        applied = true;

        // Sync the GPU effect id list from the GL pipeline
        this->d->m_gpuEffectIds = this->d->m_glPipeline.effects();

        // Rebuild m_effects to include the newly promoted GPU effect
        // (keep existing CPU effects, replace GPU portion)
        QVector<VideoEffect> newEffects;

        for (auto &effect: this->d->m_effects)
            if (effect.type == EffectType_Cpu)
                newEffects << effect;

        int gpuIdx = 0;

        for (auto &effectId: this->d->m_gpuEffectIds) {
            if (auto glElement = this->d->m_glPipeline.elementAt(gpuIdx))
                newEffects << VideoEffect(glElement,
                                          akPluginManager->pluginInfo(effectId));

            ++gpuIdx;
        }

        this->d->m_effects = newEffects;
    }
    // Case 2: CPU preview
    else if (this->d->m_preview.element) {
        // Disconnect preview from display
        this->d->unlinkPreview();

        if (!this->d->m_chainEffects) {
            // Non-chain mode: tear down all existing effects and replace
            // with just the preview.

            // Disconnect every CPU element from its successor / from the pipeline
            for (int i = 0; i < this->d->m_effects.size(); ++i) {
                auto &cur = this->d->m_effects[i];

                if (cur.type != EffectType_Cpu || !cur.element)
                    continue;

                if (i + 1 < this->d->m_effects.size()) {
                    auto &next = this->d->m_effects[i + 1];

                    if (next.type == EffectType_Cpu && next.element)
                        cur.element->unlink(next.element);
                    else
                        cur.element->unlink(&this->d->m_glPipeline);
                } else {
                    cur.element->unlink(&this->d->m_glPipeline);
                }
            }

            this->d->m_effects.clear();
            this->d->m_cpuPipeline.clear();
            this->d->m_cpuEffectIds.clear();
            this->d->m_gpuEffectIds.clear();

            // Sync the now-empty GPU effect list to the GL pipeline
            this->d->m_glPipeline.setEffects({});
        }

        this->d->m_cpuPipeline << this->d->m_preview.element;
        this->d->m_cpuEffectIds << this->d->m_preview.info.id();
        this->d->m_effects << this->d->m_preview;
        this->d->m_preview = {};
        applied = true;
    }

    this->d->m_mutex.unlock();
    this->setState(state);

    if (applied)
        emit this->previewChanged({});

    auto curEffectsIds = this->effects();

    if (effectsId != curEffectsIds) {
        emit this->effectsChanged(curEffectsIds);
        this->d->saveEffects();
    }
}

void VideoEffects::moveEffect(int from, int to)
{
    if (from == to
        || from < 0
        || from >= this->d->m_effects.size()
        || to < 0
        || to > this->d->m_effects.size())
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    // Disconnect preview from last effect
    if (this->d->m_chainEffects && this->d->m_preview.element
        && !this->d->m_effects.isEmpty()) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element)
            lastEffect.element->unlink(this->d->m_preview.element);
    }

    auto effectAt = [this](int idx) -> AkElementPtr {
        if (idx < 0 || idx >= this->d->m_effects.size())
            return {};
        auto &e = this->d->m_effects[idx];

        return (e.type == EffectType_Cpu)? e.element: AkElementPtr{};
    };

    // Disconnect the effect being moved from its current neighbours
    auto effect = effectAt(from);
    auto prevEffect = effectAt(from - 1);
    auto nextEffect = effectAt(from + 1);

    if (prevEffect) {
        prevEffect->unlink(effect);

        if (nextEffect)
            prevEffect->link(nextEffect, Qt::DirectConnection);
        else
            prevEffect->link(&this->d->m_glPipeline, Qt::DirectConnection);
    }

    if (effect) {
        if (nextEffect)
            effect->unlink(nextEffect);
        else
            effect->unlink(&this->d->m_glPipeline);
    }

    // Move the effect in the list first so indices are stable for reconnection
    this->d->m_effects.move(from, to);

    // Reconnect effect at its new position
    int newPos = to > from ? to - 1 : to;
    auto newPrevEffect = effectAt(newPos - 1);
    auto newNextEffect = effectAt(newPos + 1);
    auto movedEl = effectAt(newPos);

    if (newPrevEffect) {
        if (newNextEffect)
            newPrevEffect->unlink(newNextEffect);
        else
            newPrevEffect->unlink(&this->d->m_glPipeline);

        newPrevEffect->link(movedEl, Qt::DirectConnection);
    }

    if (movedEl) {
        if (newNextEffect)
            movedEl->link(newNextEffect, Qt::DirectConnection);
        else
            movedEl->link(&this->d->m_glPipeline, Qt::DirectConnection);
    }

    // Re-connect preview to new last effect
    if (this->d->m_chainEffects && this->d->m_preview.element
        && !this->d->m_effects.isEmpty()) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element) {
            lastEffect.element->unlink(&this->d->m_glPipeline);
            lastEffect.element->link(this->d->m_preview.element,
                                     Qt::DirectConnection);
        }
    }

    this->d->m_mutex.unlock();

    this->setState(state);
    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeEffect(int index)
{
    if (index < 0 || index >= this->d->m_effects.size())
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    // Disconnect preview from last effect
    if (this->d->m_chainEffects && this->d->m_preview.element
        && !this->d->m_effects.isEmpty()) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element)
            lastEffect.element->unlink(this->d->m_preview.element);
    }

    auto effect = this->d->m_effects.value(index);
    auto next   = this->d->m_effects.value(index + 1);
    auto prev   = this->d->m_effects.value(index - 1);

    // Unlink effect -> next (or pipeline)
    if (effect.type == EffectType_Cpu && effect.element) {
        if (next.type == EffectType_Cpu && next.element)
            effect.element->unlink(next.element);
        else
            effect.element->unlink(&this->d->m_glPipeline);
    }

    // Bridge prev -> next
    if (prev.type == EffectType_Cpu && prev.element) {
        prev.element->unlink(effect.element);

        if (next.type == EffectType_Cpu && next.element)
            prev.element->link(next.element, Qt::DirectConnection);
        else
            prev.element->link(&this->d->m_glPipeline, Qt::DirectConnection);
    }

    // Remove from auxiliary lists
    if (effect.type == EffectType_Cpu) {
        this->d->m_cpuPipeline.removeAll(effect.element);
        this->d->m_cpuEffectIds.removeAll(effect.info.id());
    } else {
        this->d->m_gpuEffectIds.removeAll(effect.info.id());
    }

    this->d->m_effects.removeAt(index);
    this->d->m_glPipeline.setEffects(this->d->m_gpuEffectIds);

    // Re-connect preview to new last effect
    if (this->d->m_chainEffects && this->d->m_preview.element
        && !this->d->m_effects.isEmpty()) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element) {
            lastEffect.element->unlink(&this->d->m_glPipeline);
            lastEffect.element->link(this->d->m_preview.element,
                                     Qt::DirectConnection);
        }
    }

    this->d->m_mutex.unlock();
    this->setState(state);
    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeAllEffects()
{
    if (this->d->m_effects.isEmpty())
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    // Disconnect preview from last effect
    if (this->d->m_chainEffects && this->d->m_preview.element) {
        auto lastEffect = this->d->m_effects.last();

        if (lastEffect.type == EffectType_Cpu && lastEffect.element)
            lastEffect.element->unlink(this->d->m_preview.element);
    }

    // Disconnect all CPU effects from each other and from the pipeline
    for (int i = 0; i < this->d->m_effects.size(); ++i) {
        auto &cur = this->d->m_effects[i];

        if (cur.type != EffectType_Cpu || !cur.element)
            continue;

        if (i + 1 < this->d->m_effects.size()) {
            auto &next = this->d->m_effects[i + 1];

            if (next.type == EffectType_Cpu && next.element)
                cur.element->unlink(next.element);
        } else {
            cur.element->unlink(&this->d->m_glPipeline);
        }
    }

    this->d->m_glPipeline.removeAllEffects();
    this->d->m_effects.clear();
    this->d->m_cpuPipeline.clear();
    this->d->m_cpuEffectIds.clear();
    this->d->m_gpuEffectIds.clear();
    this->d->m_mutex.unlock();

    this->setState(state);
    emit this->effectsChanged({});
    this->d->saveEffects();
}

void VideoEffects::updateAvailableEffects()
{
    // Get software effects
    auto swEffects = akPluginManager->listPlugins({},
                                                  {"VideoFilter"},
                                                  AkPluginManager::FilterEnabled);

    // Get GPU effects
    auto glEffects = akPluginManager->listPlugins({},
                                                  {"VideoFilterGL"},
                                                  AkPluginManager::FilterEnabled);

    // Combine both
    auto allEffects = swEffects + glEffects;

    // Sort alphabetically by description; fall back to plugin ID on ties.
    std::sort(allEffects.begin(),
              allEffects.end(),
              [this] (const QString &pluginId1, const QString &pluginId2) {
        auto desc1 = this->effectDescription(pluginId1);
        auto desc2 = this->effectDescription(pluginId2);

        if (desc1 == desc2)
            return pluginId1 < pluginId2;

        return desc1 < desc2;
    });

    allEffects.removeDuplicates();

    if (this->d->m_availableEffects != allEffects) {
        this->d->m_availableEffects = allEffects;
        emit this->availableEffectsChanged(allEffects);
    }
}

void VideoEffects::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->d->m_engine == engine)
        return;

    this->d->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("videoEffects", this);
}

AkPacket VideoEffects::iStream(const AkPacket &packet)
{
    if (packet.type() != AkPacket::PacketVideo)
        return {};

    this->d->m_mutex.lock();

    if (this->d->m_state == AkElement::ElementStatePlaying) {
        if (this->d->m_effects.isEmpty()) {
            if (this->d->m_glPipeline.isEmpty())
                this->sendPacket(packet);
            else
                this->d->m_glPipeline.iStream(packet);
        } else {
            auto &first = this->d->m_effects.first();

            if (first.type == EffectType_Cpu && first.element)
                first.element->iStream(packet);
            else if (first.type == EffectType_Gpu)
                this->d->m_glPipeline.iStream(packet);
        }

        if (this->d->m_preview.element
            && (this->d->m_effects.isEmpty() || !this->d->m_chainEffects)) {
            this->d->m_preview.element->iStream(packet);
        }
    }

    this->d->m_mutex.unlock();

    return {};
}

VideoEffectsPrivate::VideoEffectsPrivate(VideoEffects *self):
    self(self)
{
    this->m_glPipeline.setPreserveNullPlugins(true);
    QObject::connect(&this->m_glPipeline,
                     SIGNAL(oStream(AkPacket)),
                     self,
                     SLOT(sendPacket(AkPacket)),
                     Qt::DirectConnection);
}

void VideoEffectsPrivate::updateChainEffects()
{
    QSettings config;
    config.beginGroup("VideoEffects");
    self->setChainEffects(config.value("chainEffects").toBool());
    config.endGroup();
}

void VideoEffectsPrivate::updateEffects()
{
    QSettings config;
    config.beginGroup("VideoEffects");

    int size = config.beginReadArray("effects");
    QStringList effects;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        effects << config.value("effect").toString();
    }

    config.endArray();
    config.endGroup();

    self->setEffects(effects);
}

void VideoEffectsPrivate::updateEffectsProperties()
{
    QSettings config;

    for (auto &effect: this->m_effects) {
        config.beginGroup("VideoEffects_" + effect.info.id());

        for (auto &key: config.allKeys())
            effect.element->setProperty(key.toStdString().c_str(),
                                        config.value(key));

        config.endGroup();
    }
}

void VideoEffectsPrivate::saveChainEffects(bool chainEffects)
{
    QSettings config;
    config.beginGroup("VideoEffects");
    config.setValue("chainEffects", chainEffects);
    config.endGroup();
}

void VideoEffectsPrivate::saveEffects()
{
    QSettings config;
    config.beginGroup("VideoEffects");
    config.beginWriteArray("effects");

    int i = 0;

    for (auto &effect: this->m_effects) {
        config.setArrayIndex(i);
        config.setValue("effect", effect.info.id());
        i++;
    }

    config.endArray();
    config.endGroup();
}

void VideoEffectsPrivate::saveEffectsProperties()
{
    QSettings config;

    for (auto &effect: this->m_effects) {
        config.beginGroup("VideoEffects_" + effect.info.id());

        if (effect.element)
            for (int property = 0;
                property < effect.element->metaObject()->propertyCount();
                property++) {
                auto metaProperty =
                        effect.element->metaObject()->property(property);

                if (metaProperty.isWritable()) {
                    auto propertyName = metaProperty.name();
                    config.setValue(propertyName,
                                    effect.element->property(propertyName));
                }
            }

        config.endGroup();
    }
}

void VideoEffectsPrivate::linkPreview()
{
    if (!this->m_engine || !this->m_preview.element)
        return;

    for (auto &obj: this->m_engine->rootObjects()) {
        auto effectPreview = obj->findChild<VideoDisplay *>("effectPreview");

        if (effectPreview) {
            this->m_preview.element->link(effectPreview, Qt::DirectConnection);

            break;
        }
    }
}

void VideoEffectsPrivate::unlinkPreview()
{
    if (!this->m_engine || !this->m_preview.element)
        return;

    for (auto &obj: this->m_engine->rootObjects()) {
        auto effectPreview = obj->findChild<VideoDisplay *>("effectPreview");

        if (effectPreview) {
            this->m_preview.element->unlink(effectPreview);

            break;
        }
    }
}

void VideoEffectsPrivate::linkGLPreview()
{
    if (!this->m_engine)
        return;

    for (auto &obj: this->m_engine->rootObjects()) {
        auto effectPreview = obj->findChild<VideoDisplay *>("effectPreview");

        if (effectPreview) {
            this->m_glPipeline.link(effectPreview, Qt::DirectConnection);

            break;
        }
    }
}

void VideoEffectsPrivate::unlinkGLPreview()
{
    if (!this->m_engine)
        return;

    for (auto &obj: this->m_engine->rootObjects()) {
        auto effectPreview = obj->findChild<VideoDisplay *>("effectPreview");

        if (effectPreview) {
            this->m_glPipeline.unlink(effectPreview);

            break;
        }
    }
}

VideoEffect::VideoEffect()
{

}

VideoEffect::VideoEffect(const AkElementPtr &element,
                         const AkPluginInfo &info):
    type(EffectType_Cpu),
    element(element),
    info(info)
{

}

VideoEffect::VideoEffect(const AkVideoEffectPtr &glElement,
                         const AkPluginInfo &info):
    type(EffectType_Gpu),
    glElement(glElement),
    info(info)
{

}

VideoEffect &VideoEffect::operator =(const VideoEffect &other)
{
    if (this != &other) {
        this->type = other.type;
        this->element = other.element;
        this->glElement = other.glElement;
        this->info = other.info;
    }

    return *this;
}

#include "moc_videoeffects.cpp"
