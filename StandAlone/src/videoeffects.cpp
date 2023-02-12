/* Webcamoid, webcam capture application.
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
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "videoeffects.h"
#include "videodisplay.h"

class VideoEffect
{
    public:
        AkElementPtr element;
        AkPluginInfo info;

        VideoEffect();
        VideoEffect(const AkElementPtr &element, const AkPluginInfo &info);
        VideoEffect &operator =(const VideoEffect &other);
};

class VideoEffectsPrivate
{
    public:
        VideoEffects *self;
        QQmlApplicationEngine *m_engine {nullptr};
        QStringList m_availableEffects;
        QList<VideoEffect> m_effects;
        VideoEffect m_preview;
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
};

VideoEffects::VideoEffects(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent)
{
    this->d = new VideoEffectsPrivate(this);
    this->setQmlEngine(engine);
    this->updateAvailableEffects();
    this->d->updateChainEffects();
    this->d->updateEffects();
    this->d->updateEffectsProperties();
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
    if (!this->d->m_preview.element)
        return {};

    return this->d->m_preview.info.id();
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

bool VideoEffects::chainEffects() const
{
    return this->d->m_chainEffects;
}

bool VideoEffects::embedControls(const QString &where,
                                 int effectIndex,
                                 const QString &name) const
{
    auto effect = this->d->m_effects.value(effectIndex);

    if (!effect.element)
        return false;

    auto interface = effect.element->controlInterface(this->d->m_engine,
                                                      effect.info.id());

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);

        return true;
    }

    return false;
}

bool VideoEffects::embedPreviewControls(const QString &where, const QString &name) const
{
    auto &effect = this->d->m_preview;

    if (!effect.element)
        return false;

    auto interface = effect.element->controlInterface(this->d->m_engine,
                                                      effect.info.id());

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto &obj: this->d->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        // Finally, embed the plugin item UI in the desired place.
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

        QList<decltype(item)> childItems = item->childItems();

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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();

    // Remove old effects
    if (!this->d->m_effects.isEmpty()) {
        auto lastElement = this->d->m_effects.last();
        QObject::disconnect(lastElement.element.data(),
                            SIGNAL(oStream(AkPacket)),
                            this,
                            SLOT(sendPacket(AkPacket)));

        if (this->d->m_preview.element)
            lastElement.element->unlink(this->d->m_preview.element);

        this->d->m_effects.clear();
    }

    // Populate the effects
    AkElementPtr prevEffect;

    for (auto &effectId: effects)
        if (auto effect = akPluginManager->create<AkElement>(effectId)) {
            this->d->m_effects << VideoEffect(effect,
                                              akPluginManager->pluginInfo(effectId));

            if (prevEffect)
                prevEffect->link(effect, Qt::DirectConnection);
            else
                prevEffect = effect;
        }

    // Link the effects to the outputs
    if (!this->d->m_effects.isEmpty()) {
        auto lastElement = this->d->m_effects.last();
        QObject::connect(lastElement.element.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(sendPacket(AkPacket)),
                         Qt::DirectConnection);

        if (this->d->m_chainEffects && this->d->m_preview.element)
            lastElement.element->link(this->d->m_preview.element, Qt::DirectConnection);
    }

    this->d->m_mutex.unlock();
    this->setState(state);

    emit this->effectsChanged(effects);
    this->d->saveEffects();
    this->d->updateEffectsProperties();
}

void VideoEffects::setPreview(const QString &preview)
{
    QString oldPreview;

    if (this->d->m_preview.element)
        oldPreview = this->d->m_preview.info.id();

    if (oldPreview == preview)
        return;

    auto state = this->d->m_state;

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();

    // Unlink the old preview
    if (!this->d->m_effects.isEmpty() && this->d->m_preview.element) {
        auto lastElement = this->d->m_effects.last();
        lastElement.element->unlink(this->d->m_preview.element);
        this->d->unlinkPreview();
    }

    // Set preview
    QString newPreview;
    this->d->m_preview.element = akPluginManager->create<AkElement>(preview);
    this->d->m_preview.info = akPluginManager->pluginInfo(preview);

    if (this->d->m_preview.element) {
        newPreview = this->d->m_preview.info.id();
        this->d->linkPreview();

        // Link the preview
        if (!this->d->m_effects.isEmpty() && this->d->m_chainEffects) {
            auto lastElement = this->d->m_effects.last();
            lastElement.element->link(this->d->m_preview.element,
                                      Qt::DirectConnection);
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
        if (this->d->m_preview.element)
            this->d->m_preview.element->setState(state);

        for (auto it = this->d->m_effects.rbegin();
             it != this->d->m_effects.rend();
             it++)
            it->element->setState(state);
    } else {
        for (auto &effect: this->d->m_effects)
            effect.element->setState(state);

        if (this->d->m_preview.element)
            this->d->m_preview.element->setState(state);
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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();

    if (this->d->m_preview.element) {
        if (chainEffects) {
            if (!this->d->m_effects.isEmpty()) {
                auto lastElement = this->d->m_effects.last();

                if (this->d->m_preview.element)
                    lastElement.element->link(this->d->m_preview.element,
                                              Qt::DirectConnection);
            }
        } else {
            if (!this->d->m_effects.isEmpty()) {
                auto lastElement = this->d->m_effects.last();

                if (this->d->m_preview.element)
                    lastElement.element->unlink(this->d->m_preview.element);
            }
        }
    }

    this->d->m_mutex.unlock();
    this->setState(state);

    this->d->m_chainEffects = chainEffects;
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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();
    bool applied = false;
    auto effectsId = this->effects();

    if (this->d->m_preview.element) {
        this->d->unlinkPreview();
        QObject::connect(this->d->m_preview.element.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(sendPacket(AkPacket)),
                         Qt::DirectConnection);

        if (this->d->m_chainEffects) {
            if (!this->d->m_effects.isEmpty()) {
                auto lastEffect = this->d->m_effects.last();
                QObject::disconnect(lastEffect.element.data(),
                                    SIGNAL(oStream(AkPacket)),
                                    this,
                                    SLOT(sendPacket(AkPacket)));
                lastEffect.element->link(this->d->m_preview.element,
                                         Qt::DirectConnection);
            }
        } else {
            this->d->m_effects.clear();
        }

        this->d->m_effects << this->d->m_preview;
        this->d->m_preview = {};
        applied = true;
    }

    this->d->m_mutex.unlock();
    this->setState(state);

    if (applied)
        emit this->previewChanged({});

    auto curEffectsIdsv = this->effects();

    if (effectsId != curEffectsIdsv) {
        emit this->effectsChanged(curEffectsIdsv);
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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();

    // Disconnect preview
    if (this->d->m_preview.element) {
        auto lastEffect = this->d->m_effects.last();
        lastEffect.element->unlink(this->d->m_preview.element);
    }

    // Diconnect effect from list.
    auto effect = this->d->m_effects.value(from);
    auto prev = this->d->m_effects.value(from - 1);
    auto next = this->d->m_effects.value(from + 1);

    if (prev.element) {
        prev.element->unlink(effect.element);

        if (next.element)
            prev.element->link(next.element, Qt::DirectConnection);
        else
            QObject::connect(prev.element.data(),
                             SIGNAL(oStream(AkPacket)),
                             this,
                             SLOT(sendPacket(AkPacket)),
                             Qt::DirectConnection);
    }

    if (next.element)
        effect.element->unlink(next.element);
    else
        QObject::disconnect(effect.element.data(),
                            SIGNAL(oStream(AkPacket)),
                            this,
                            SLOT(sendPacket(AkPacket)));

    // Reconnect effect.
    prev = this->d->m_effects.value(to - 1);
    next = this->d->m_effects.value(to);

    if (prev.element) {
        if (next.element)
            prev.element->unlink(next.element);
        else
            QObject::disconnect(prev.element.data(),
                                SIGNAL(oStream(AkPacket)),
                                this,
                                SLOT(sendPacket(AkPacket)));

        prev.element->link(effect.element, Qt::DirectConnection);
    }

    if (next.element)
        effect.element->link(next.element, Qt::DirectConnection);
    else
        QObject::connect(effect.element.data(),
                         SIGNAL(oStream(AkPacket)),
                         this,
                         SLOT(sendPacket(AkPacket)),
                         Qt::DirectConnection);

    // Move the effect in the list.
    this->d->m_effects.move(from, to);

    // Re-connect preview
    if (this->d->m_chainEffects && this->d->m_preview.element) {
        auto lastEffect = this->d->m_effects.last();
        lastEffect.element->link(this->d->m_preview.element,
                                 Qt::DirectConnection);
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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();

    // Disconnect preview
    if (this->d->m_preview.element) {
        auto lastEffect = this->d->m_effects.last();
        lastEffect.element->unlink(this->d->m_preview.element);
    }

    auto effect = this->d->m_effects.value(index);
    auto next = this->d->m_effects.value(index + 1);

    if (next.element)
        effect.element->unlink(next.element);
    else
        QObject::disconnect(effect.element.data(),
                            SIGNAL(oStream(AkPacket)),
                            this,
                            SLOT(sendPacket(AkPacket)));

    auto prev = this->d->m_effects.value(index - 1);

    if (prev.element) {
        prev.element->unlink(effect.element);

        if (next.element)
            prev.element->link(next.element, Qt::DirectConnection);
        else
            QObject::connect(prev.element.data(),
                             SIGNAL(oStream(AkPacket)),
                             this,
                             SLOT(sendPacket(AkPacket)),
                             Qt::DirectConnection);
    }

    this->d->m_effects.removeAt(index);

    // Re-connect preview
    if (this->d->m_chainEffects && this->d->m_preview.element) {
        auto lastEffect = this->d->m_effects.last();
        lastEffect.element->link(this->d->m_preview.element,
                                 Qt::DirectConnection);
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
        this->setState(AkElement::ElementStatePaused);

    this->d->m_mutex.lock();
    auto lastEffect = this->d->m_effects.last();

    // Disconnect preview
    if (this->d->m_preview.element)
        lastEffect.element->unlink(this->d->m_preview.element);

    // Disconnect last effect
    QObject::disconnect(lastEffect.element.data(),
                        SIGNAL(oStream(AkPacket)),
                        this,
                        SLOT(sendPacket(AkPacket)));

    this->d->m_effects.clear();
    this->d->m_mutex.unlock();

    this->setState(state);
    emit this->effectsChanged({});
    this->d->saveEffects();
}

void VideoEffects::updateAvailableEffects()
{
    auto availableEffects =
            akPluginManager->listPlugins({},
                                         {"VideoFilter"},
                                         AkPluginManager::FilterEnabled);
    std::sort(availableEffects.begin(),
              availableEffects.end(),
              [this] (const QString &pluginId1, const QString &pluginId2) {
        auto desc1 = this->effectDescription(pluginId1);
        auto desc2 = this->effectDescription(pluginId2);

        return desc1 < desc2;
    });

    if (this->d->m_availableEffects != availableEffects) {
        this->d->m_availableEffects = availableEffects;
        emit this->availableEffectsChanged(availableEffects);
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
            this->sendPacket(packet);
        } else {
            this->d->m_effects.first().element->iStream(packet);
        }

        if (this->d->m_preview.element
            && (this->d->m_effects.isEmpty() || !this->d->m_chainEffects))
            this->d->m_preview.element->iStream(packet);
    }

    this->d->m_mutex.unlock();

    return {};
}

VideoEffectsPrivate::VideoEffectsPrivate(VideoEffects *self):
    self(self)
{

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

VideoEffect::VideoEffect()
{

}

VideoEffect::VideoEffect(const AkElementPtr &element, const AkPluginInfo &info):
    element(element),
    info(info)
{
}

VideoEffect &VideoEffect::operator =(const VideoEffect &other)
{
    if (this != &other) {
        this->element = other.element;
        this->info = other.info;
    }

    return *this;
}

#include "moc_videoeffects.cpp"
