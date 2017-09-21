/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include <QSettings>
#include <QQuickItem>
#include <QQmlProperty>

#include "videoeffects.h"

VideoEffects::VideoEffects(QQmlApplicationEngine *engine, QObject *parent):
    QObject(parent),
    m_engine(nullptr)
{
    this->m_state = AkElement::ElementStateNull;
    this->m_advancedMode = false;
    this->setQmlEngine(engine);
    this->m_videoMux = AkElement::create("Multiplex");

    if (this->m_videoMux) {
        this->m_videoMux->setProperty("caps", "video/x-raw");
        this->m_videoMux->setProperty("outputIndex", 0);

        QObject::connect(this->m_videoMux.data(),
                         SIGNAL(oStream(const AkPacket &)),
                         this,
                         SIGNAL(oStream(const AkPacket &)));
    }

    this->m_availableEffects = AkElement::listPlugins("VideoFilter");

    std::sort(this->m_availableEffects.begin(), this->m_availableEffects.end(),
              [this] (const QString &pluginId1, const QString &pluginId2) {
        auto desc1 = this->effectDescription(pluginId1);
        auto desc2 = this->effectDescription(pluginId2);

        return desc1 < desc2;
    });

    QObject::connect(this,
                     &VideoEffects::effectsChanged,
                     this,
                     &VideoEffects::saveEffects);
    QObject::connect(this,
                     &VideoEffects::advancedModeChanged,
                     this,
                     &VideoEffects::advancedModeUpdated);
    QObject::connect(this,
                     &VideoEffects::advancedModeChanged,
                     this,
                     &VideoEffects::saveAdvancedMode);

    this->loadProperties();
}

VideoEffects::~VideoEffects()
{
    this->saveProperties();
    this->setState(AkElement::ElementStateNull);
}

QStringList VideoEffects::availableEffects() const
{
    auto effects = this->m_availableEffects;

    if (this->m_advancedMode)
        for (const AkElementPtr &effect: this->m_effects) {
            int i = effects.indexOf(effect->pluginId());

            if (i < 0 || effect->property("preview").toBool())
                continue;

            effects.removeAt(i);
        }

    return effects;
}

QStringList VideoEffects::effects() const
{
    return this->m_effectsId;
}

QVariantMap VideoEffects::effectInfo(const QString &effectId) const
{
    return AkElement::pluginInfo(effectId);
}

QString VideoEffects::effectDescription(const QString &effectId) const
{
    if (effectId.isEmpty())
        return QString();

    return AkElement::pluginInfo(effectId)["MetaData"].toMap()
                                          ["description"].toString();
}

AkElement::ElementState VideoEffects::state() const
{
    return this->m_state;
}

bool VideoEffects::advancedMode() const
{
    return this->m_advancedMode;
}

bool VideoEffects::embedControls(const QString &where,
                                 int effectIndex,
                                 const QString &name) const
{
    auto effect = this->m_effects.value(effectIndex);

    if (!effect)
        return false;

    auto interface = effect->controlInterface(this->m_engine,
                                              effect->pluginId());

    if (!interface)
        return false;

    if (!name.isEmpty())
        interface->setObjectName(name);

    for (auto obj: this->m_engine->rootObjects()) {
        // First, find where to embed the UI.
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        // Create an item with the plugin context.
        auto interfaceItem = qobject_cast<QQuickItem *>(interface);

        // Finally, embed the plugin item UI in the desired place.
        interfaceItem->setParentItem(item);
        interfaceItem->setParent(item);

        QQmlProperty::write(interfaceItem,
                            "anchors.fill",
                            qVariantFromValue(item));

        return true;
    }

    return false;
}

void VideoEffects::removeInterface(const QString &where) const
{
    if (!this->m_engine)
        return;

    for (const QObject *obj: this->m_engine->rootObjects()) {
        auto item = obj->findChild<QQuickItem *>(where);

        if (!item)
            continue;

        QList<decltype(item)> childItems = item->childItems();

        for (auto child: childItems) {
            child->setParentItem(NULL);
            child->setParent(NULL);

            delete child;
        }
    }
}

void VideoEffects::setEffects(const QStringList &effects, bool emitSignal)
{
    if (this->m_effectsId == effects)
        return;

    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();

    if (!this->m_effects.isEmpty()) {
        if (this->m_videoMux)
            this->m_effects.last()->unlink(this->m_videoMux);

        this->m_effects.clear();
        this->m_effectsId.clear();
    }

    QStringList curEffects;
    AkElementPtr prevEffect;

    for (const QString &effectId: effects)
        if (auto effect = AkElement::create(effectId)) {
            this->m_effects << effect;
            this->m_effectsId << effectId;
            curEffects << effectId;

            if (prevEffect)
                prevEffect->link(effect, Qt::DirectConnection);
            else
                prevEffect = effect;
        }

    if (!this->m_effects.isEmpty() && this->m_videoMux)
        this->m_effects.last()->link(this->m_videoMux, Qt::DirectConnection);

    this->m_mutex.unlock();
    this->setState(state);

    if (emitSignal)
        emit this->effectsChanged(curEffects);
}

void VideoEffects::setState(AkElement::ElementState state)
{
    if (this->m_state == state)
        return;

    this->m_mutex.lock();

    if (state == AkElement::ElementStatePlaying)
        for (int i = this->m_effects.size() - 1; i >= 0; i--)
            this->m_effects[i]->setState(state);
    else
        for (AkElementPtr &effect: this->m_effects)
            effect->setState(state);

    this->m_state = state;

    this->m_mutex.unlock();

    emit this->stateChanged(state);
}

void VideoEffects::setAdvancedMode(bool advancedMode)
{
    if (this->m_advancedMode == advancedMode)
        return;

    this->m_advancedMode = advancedMode;
    emit this->advancedModeChanged(advancedMode);
}

void VideoEffects::resetEffects()
{
    this->setEffects({});
}

void VideoEffects::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoEffects::resetAdvancedMode()
{
    this->setAdvancedMode(false);
}

AkElementPtr VideoEffects::appendEffect(const QString &effectId, bool preview)
{
    auto effect = AkElement::create(effectId);

    if (!effect)
        return AkElementPtr();

    if (preview)
        effect->setProperty("preview", preview);

    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();

    if (!this->m_effects.isEmpty()) {
        auto prevEffect = this->m_effects.last();

        if (this->m_videoMux)
            prevEffect->unlink(this->m_videoMux);

        prevEffect->link(effect, Qt::DirectConnection);
    }

    if (this->m_videoMux)
        effect->link(this->m_videoMux, Qt::DirectConnection);

    this->m_effects << effect;

    if (!preview)
        this->m_effectsId << effectId;

    this->m_mutex.unlock();

    this->setState(state);

    if (!preview)
        emit this->effectsChanged(this->m_effectsId);

    return effect;
}

void VideoEffects::showPreview(const QString &effectId)
{
    this->removeAllPreviews();
    this->appendEffect(effectId, true);
}

void VideoEffects::setAsPreview(int index, bool preview)
{
    auto effect = this->m_effects.value(index);

    if (!effect)
        return;

    effect->setProperty("preview", preview? true: QVariant());

    if (preview) {
        if (index < this->m_effectsId.size())
            this->m_effectsId.removeAt(index);
    } else {
        if (index >= this->m_effectsId.size())
            this->m_effectsId << effect->pluginId();
    }

    emit this->effectsChanged(this->m_effectsId);
}

void VideoEffects::moveEffect(int from, int to)
{
    if (from == to
        || from < 0
        || from >= this->m_effects.size()
        || to < 0
        || to > this->m_effects.size())
        return;

    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();

    // Diconnect effect from list.
    auto effect = this->m_effects.value(from);
    auto prev = this->m_effects.value(from - 1);
    auto next = this->m_effects.value(from + 1);

    if (!next)
        next = this->m_videoMux;

    if (prev) {
        prev->unlink(effect);
        prev->link(next, Qt::DirectConnection);
    }

    effect->unlink(next);

    // Reconnect effect.
    prev = this->m_effects.value(to - 1);
    next = this->m_effects.value(to);

    if (!next)
        next = this->m_videoMux;

    if (prev) {
        prev->unlink(next);
        prev->link(effect, Qt::DirectConnection);
    }

    effect->link(next, Qt::DirectConnection);

    // Move the effect in the list.
    this->m_effects.move(from, to);
    this->m_effectsId.move(from, to);

    this->m_mutex.unlock();

    this->setState(state);
    emit this->effectsChanged(this->m_effectsId);
}

void VideoEffects::removeEffect(int index)
{
    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();

    auto effect = this->m_effects.value(index);

    if (effect) {
        auto next = this->m_effects.value(index + 1);

        if (!next)
            next = this->m_videoMux;

        effect->unlink(next);

        auto prev = this->m_effects.value(index - 1);

        if (prev) {
            prev->unlink(effect);
            prev->link(next, Qt::DirectConnection);
        }

        this->m_effects.removeAt(index);
        this->m_effectsId.removeAt(index);
    }

    this->m_mutex.unlock();

    this->setState(state);

    if (effect)
        emit this->effectsChanged(this->m_effectsId);
}

void VideoEffects::removeAllPreviews()
{
    bool hasPreview = false;

    for (AkElementPtr &effect: this->m_effects)
        if (effect->property("preview").toBool()) {
            hasPreview = true;

            break;
        }

    if (!hasPreview)
        return;

    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();

    for (int i = 0; i < this->m_effects.size(); i++) {
        auto effect = this->m_effects[i];

        if (effect->property("preview").toBool()) {
            auto prev = this->m_effects.value(i - 1);
            auto next = this->m_effects.value(i + 1);

            if (!next)
                next = this->m_videoMux;

            if (prev) {
                prev->unlink(effect);
                prev->link(next, Qt::DirectConnection);
            }

            effect->unlink(next);
            this->m_effects.removeAt(i);
            i--;
        }
    }

    this->m_mutex.unlock();
    this->setState(state);
}

void VideoEffects::updateEffects()
{
    QStringList availableEffects = AkElement::listPlugins("VideoFilter");

    if (this->m_availableEffects != availableEffects) {
        this->m_availableEffects = availableEffects;
        emit this->availableEffectsChanged(availableEffects);
        QStringList effects;

        for (const QString &effectId: this->m_effectsId)
            if (availableEffects.contains(effectId))
                effects << effectId;

        this->setEffects(effects);
    }
}

AkPacket VideoEffects::iStream(const AkPacket &packet)
{
    this->m_mutex.lock();

    if (this->m_state == AkElement::ElementStatePlaying) {
        if (this->m_effects.isEmpty()) {
            if (this->m_videoMux)
                (*this->m_videoMux)(packet);
        } else
            (*this->m_effects.first())(packet);
    }

    this->m_mutex.unlock();

    return AkPacket();
}

void VideoEffects::setQmlEngine(QQmlApplicationEngine *engine)
{
    if (this->m_engine == engine)
        return;

    this->m_engine = engine;

    if (engine)
        engine->rootContext()->setContextProperty("VideoEffects", this);
}

void VideoEffects::advancedModeUpdated(bool advancedMode)
{
    if (advancedMode || this->m_effects.size() < 1)
        return;

    auto effect = this->m_effects.last();
    effect->setProperty("preview", QVariant());

    auto state = this->state();

    if (state != AkElement::ElementStateNull)
        this->setState(AkElement::ElementStatePaused);

    this->m_mutex.lock();
    this->m_effects = {effect};
    this->m_effectsId = QStringList {effect->pluginId()};
    this->m_mutex.unlock();

    this->setState(state);
    emit this->effectsChanged(this->m_effectsId);
}

void VideoEffects::loadProperties()
{
    QSettings config;

    config.beginGroup("VideoEffects");
    this->setAdvancedMode(config.value("advancedMode").toBool());

    int size = config.beginReadArray("effects");
    QStringList effects;

    for (int i = 0; i < size; i++) {
        config.setArrayIndex(i);
        effects << config.value("effect").toString();
    }

    config.endArray();
    config.endGroup();

    this->setEffects(effects, false);

    for (AkElementPtr &effect: this->m_effects) {
        config.beginGroup("VideoEffects_" + effect->pluginId());

        for (const QString &key: config.allKeys())
            effect->setProperty(key.toStdString().c_str(), config.value(key));

        config.endGroup();
    }
}

void VideoEffects::saveEffects(const QStringList &effects)
{
    Q_UNUSED(effects)

    QSettings config;

    config.beginGroup("VideoEffects");
    config.beginWriteArray("effects");

    int i = 0;

    for (const AkElementPtr &effect: this->m_effects)
        if (!effect->property("preview").toBool()) {
            config.setArrayIndex(i);
            config.setValue("effect", effect->pluginId());
            i++;
        }

    config.endArray();
    config.endGroup();

    for (const AkElementPtr &effect: this->m_effects) {
        config.beginGroup("VideoEffects_" + effect->pluginId());

        for (int property = 0;
             property < effect->metaObject()->propertyCount();
             property++) {
            auto metaProperty = effect->metaObject()->property(property);

            if (metaProperty.isWritable()) {
                auto propertyName = metaProperty.name();
                config.setValue(propertyName, effect->property(propertyName));
            }
        }

        config.endGroup();
    }
}

void VideoEffects::saveAdvancedMode(bool advancedMode)
{
    QSettings config;

    config.beginGroup("VideoEffects");
    config.setValue("advancedMode", advancedMode);
    config.endGroup();
}

void VideoEffects::saveProperties()
{
    QSettings config;

    config.beginGroup("VideoEffects");
    config.setValue("advancedMode", this->advancedMode());

    config.beginWriteArray("effects");

    int i = 0;

    for (const AkElementPtr &effect: this->m_effects)
        if (!effect->property("preview").toBool()) {
            config.setArrayIndex(i);
            config.setValue("effect", effect->pluginId());
            i++;
        }

    config.endArray();
    config.endGroup();

    for (const AkElementPtr &effect: this->m_effects) {
        config.beginGroup("VideoEffects_" + effect->pluginId());

        for (int property = 0;
             property < effect->metaObject()->propertyCount();
             property++) {
            auto metaProperty = effect->metaObject()->property(property);

            if (metaProperty.isWritable()) {
                auto propertyName = metaProperty.name();
                config.setValue(propertyName, effect->property(propertyName));
            }
        }

        config.endGroup();
    }
}
