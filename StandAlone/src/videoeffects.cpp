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
#include <akglpipeline.h>
#include <akpacket.h>
#include <akplugininfo.h>

#include "videoeffects.h"
#include "videodisplay.h"

class VideoEffectsPrivate
{
    public:
        VideoEffects *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkGLPipeline m_glPipeline;
        QMutex m_mutex;
        AkElement::ElementState m_state {AkElement::ElementStateNull};

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
    this->d->m_glPipeline.addPacketReader();
}

VideoEffects::~VideoEffects()
{
    this->setState(AkElement::ElementStateNull);
    this->d->m_glPipeline.removePacketReader();
    this->d->saveEffectsProperties();
    delete this->d;
}

QStringList VideoEffects::availableEffects() const
{
    return this->d->m_glPipeline.availableEffects();
}

QStringList VideoEffects::effects() const
{
    return this->d->m_glPipeline.effects();
}

QString VideoEffects::preview() const
{
    return this->d->m_glPipeline.preview();
}

AkPluginInfo VideoEffects::effectInfo(const QString &effectId) const
{
    return this->d->m_glPipeline.effectInfo(effectId);
}

QString VideoEffects::effectDescription(const QString &effectId) const
{
    return this->d->m_glPipeline.effectDescription(effectId);
}

AkElement::ElementState VideoEffects::state() const
{
    return this->d->m_state;
}

bool VideoEffects::chainEffects() const
{
    return this->d->m_glPipeline.chainEffects();
}

bool VideoEffects::embedControls(const QString &where,
                                 int effectIndex,
                                 const QString &name) const
{
    auto element = this->d->m_glPipeline.elementAt(effectIndex);

    if (!element)
        return false;

    auto pluginId = this->d->m_glPipeline.effectInfo(effectIndex).id();
    auto interface = element->controlInterface(this->d->m_engine,
                                               pluginId);

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

    if (!glPreview)
        return false;

    auto info = this->d->m_glPipeline.effectInfo(this->d->m_glPipeline.preview());
    interface = glPreview->controlInterface(this->d->m_engine, info.id());

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

    this->d->m_mutex.lock();
    this->d->m_glPipeline.setEffects(effects);
    this->d->m_mutex.unlock();

    emit this->effectsChanged(effects);
    this->d->saveEffects();
    this->d->updateEffectsProperties();
}

void VideoEffects::setPreview(const QString &preview)
{
    if (this->d->m_glPipeline.preview() == preview)
        return;

    this->d->m_mutex.lock();
    auto state = this->d->m_state;
    this->d->m_glPipeline.setState(AkElement::ElementStateNull);
    this->d->unlinkPreview();
    this->d->m_glPipeline.setPreview(preview);

    if (!preview.isEmpty())
        this->d->linkPreview();

    this->d->m_glPipeline.setState(state);
    this->d->m_mutex.unlock();

    emit this->previewChanged(preview);
}

void VideoEffects::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    this->d->m_mutex.lock();
    this->d->m_glPipeline.setState(state);
    this->d->m_state = state;
    this->d->m_mutex.unlock();

    emit this->stateChanged(state);
}

void VideoEffects::setChainEffects(bool chainEffects)
{
    this->d->m_mutex.lock();
    this->d->m_glPipeline.setChainEffects(chainEffects);
    this->d->m_mutex.unlock();

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
    this->d->m_mutex.lock();
    bool applied = false;
    auto effectsId = this->d->m_glPipeline.effects();

    if (!this->d->m_glPipeline.preview().isEmpty()) {
        this->d->unlinkPreview();
        this->d->m_glPipeline.applyPreview();
        applied = true;
    }

    this->d->m_mutex.unlock();

    if (applied)
        emit this->previewChanged({});

    auto curEffectsIds = this->d->m_glPipeline.effects();

    if (effectsId != curEffectsIds) {
        emit this->effectsChanged(curEffectsIds);
        this->d->saveEffects();
    }
}

void VideoEffects::moveEffect(int from, int to)
{
    auto totalEffects = this->d->m_glPipeline.effects().size();

    if (from == to
        || from < 0
        || from >= totalEffects
        || to < 0
        || to > totalEffects)
        return;

    this->d->m_mutex.lock();
    this->d->m_glPipeline.moveEffect(from, to);
    this->d->m_mutex.unlock();

    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeEffect(int index)
{
    if (index < 0 || index >= this->d->m_glPipeline.effects().size())
        return;

    this->d->m_mutex.lock();
    this->d->m_glPipeline.removeEffect(index);
    this->d->m_mutex.unlock();

    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeAllEffects()
{
    if (this->d->m_glPipeline.isEmpty())
        return;

    this->d->m_mutex.lock();
    this->d->m_glPipeline.removeAllEffects();
    this->d->m_mutex.unlock();

    emit this->effectsChanged({});
    this->d->saveEffects();
}

void VideoEffects::updateAvailableEffects()
{
    this->d->m_glPipeline.updateAvailableEffects();
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
    this->d->m_glPipeline.iStream(packet);
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
    QObject::connect(&this->m_glPipeline,
                     &AkGLPipeline::availableEffectsChanged,
                     self,
                     &VideoEffects::availableEffectsChanged);
    QObject::connect(&this->m_glPipeline,
                     &AkGLPipeline::chainEffectsChanged,
                     self,
                     &VideoEffects::chainEffectsChanged);
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
    auto effects = this->m_glPipeline.effects();

    for (int i = 0; i < effects.size(); ++i) {
        config.beginGroup("VideoEffects_" + effects[i]);
        auto element = this->m_glPipeline.elementAt(i);

        for (auto &key: config.allKeys())
            element->setProperty(key.toStdString().c_str(),
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

    for (auto &effect: this->m_glPipeline.effects()) {
        config.setArrayIndex(i);
        config.setValue("effect", effect);
        i++;
    }

    config.endArray();
    config.endGroup();
}

void VideoEffectsPrivate::saveEffectsProperties()
{
    QSettings config;

    auto effects = this->m_glPipeline.effects();

    for (int i = 0; i < effects.size(); ++i) {
        config.beginGroup("VideoEffects_" + effects[i]);
        auto element = this->m_glPipeline.elementAt(i);

        if (element)
            for (int property = 0;
                property < element->metaObject()->propertyCount();
                property++) {
                auto metaProperty =
                        element->metaObject()->property(property);

                if (metaProperty.isWritable()) {
                    auto propertyName = metaProperty.name();
                    config.setValue(propertyName,
                                    element->property(propertyName));
                }
            }

        config.endGroup();
    }
}

void VideoEffectsPrivate::linkPreview()
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

void VideoEffectsPrivate::unlinkPreview()
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

#include "moc_videoeffects.cpp"
