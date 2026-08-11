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
#include <QRectF>
#include <ak.h>
#include <akfrac.h>
#include <akglcompositor.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akvideopacket.h>

#include "videoeffects.h"
#include "videodisplay.h"

class VideoEffectsPrivate
{
    public:
        VideoEffects *self;
        QQmlApplicationEngine *m_engine {nullptr};
        AkGLCompositor m_glCompositor;
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
    this->d->m_glCompositor.addPacketReader();
}

VideoEffects::~VideoEffects()
{
    this->setState(AkElement::ElementStateNull);
    this->d->m_glCompositor.removePacketReader();
    this->d->saveEffectsProperties();
    delete this->d;
}

QStringList VideoEffects::availableEffects() const
{
    return this->d->m_glCompositor.availableEffects();
}

QStringList VideoEffects::effects() const
{
    return this->d->m_glCompositor.effects();
}

QString VideoEffects::preview() const
{
    return this->d->m_glCompositor.preview();
}

AkPluginInfo VideoEffects::effectInfo(const QString &effectId) const
{
    return this->d->m_glCompositor.effectInfo(effectId);
}

QString VideoEffects::effectDescription(const QString &effectId) const
{
    return this->d->m_glCompositor.effectDescription(effectId);
}

AkElement::ElementState VideoEffects::state() const
{
    return this->d->m_state;
}

bool VideoEffects::chainEffects() const
{
    return this->d->m_glCompositor.chainEffects();
}

bool VideoEffects::embedControls(const QString &where,
                                 int effectIndex,
                                 const QString &name) const
{
    auto element = this->d->m_glCompositor.elementAt(effectIndex);

    if (!element)
        return false;

    auto pluginId = this->d->m_glCompositor.effectInfo(effectIndex).id();
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
    auto glPreview = this->d->m_glCompositor.previewElement();

    if (!glPreview)
        return false;

    auto info = this->d->m_glCompositor.effectInfo(
                    this->d->m_glCompositor.preview());
    auto interface = glPreview->controlInterface(this->d->m_engine, info.id());

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

qint64 VideoEffects::addSource()
{
    return this->addSource(Ak::id());
}

qint64 VideoEffects::addSource(qint64 id)
{
    if (id < 0)
        return id;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.addSource(id);
        this->d->m_glCompositor.setSourcePreserveNullPlugins(id, true);
        this->d->m_glCompositor.setSourceRect(id, QRectF(0.0, 0.0, 1.0, 1.0));
    }

    return id;
}

void VideoEffects::removeSource(qint64 id)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.removeSource(id);
}

QVariantList VideoEffects::sourceIds() const
{
    return this->d->m_glCompositor.sourceIds();
}

QRectF VideoEffects::sourceRect(qint64 id) const
{
    return this->d->m_glCompositor.sourceRect(id);
}

int VideoEffects::sourceZOrder(qint64 id) const
{
    return this->d->m_glCompositor.sourceZOrder(id);
}

qreal VideoEffects::sourceOpacity(qint64 id) const
{
    return this->d->m_glCompositor.sourceOpacity(id);
}

Qt::AspectRatioMode VideoEffects::sourceAspectRatioMode(qint64 id) const
{
    return this->d->m_glCompositor.sourceAspectRatioMode(id);
}

QStringList VideoEffects::sourceEffects(qint64 id) const
{
    return this->d->m_glCompositor.sourceEffects(id);
}

QString VideoEffects::sourcePreview(qint64 id) const
{
    return this->d->m_glCompositor.sourcePreview(id);
}

AkVideoEffectPtr VideoEffects::sourceElementAt(qint64 id, int index) const
{
    return this->d->m_glCompositor.sourceElementAt(id, index);
}

AkVideoEffectPtr VideoEffects::sourcePreviewElement(qint64 id) const
{
    return this->d->m_glCompositor.sourcePreviewElement(id);
}

AkPluginInfo VideoEffects::sourceEffectInfo(qint64 id, int index) const
{
    return this->d->m_glCompositor.sourceEffectInfo(id, index);
}

bool VideoEffects::sourceChainEffects(qint64 id) const
{
    return this->d->m_glCompositor.sourceChainEffects(id);
}

bool VideoEffects::sourcePreserveNullPlugins(qint64 id) const
{
    return this->d->m_glCompositor.sourcePreserveNullPlugins(id);
}

bool VideoEffects::sourceIsEmpty(qint64 id) const
{
    return this->d->m_glCompositor.sourceIsEmpty(id);
}

bool VideoEffects::embedControls(const QString &where,
                                 qint64 id,
                                 int effectIndex,
                                 const QString &name) const
{
    auto element = this->d->m_glCompositor.sourceElementAt(id, effectIndex);

    if (!element)
        return false;

    auto pluginId =
            this->d->m_glCompositor.sourceEffectInfo(id, effectIndex).id();
    auto interface = element->controlInterface(this->d->m_engine, pluginId);

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
                                        qint64 id,
                                        const QString &name) const
{
    auto glPreview = this->d->m_glCompositor.sourcePreviewElement(id);

    if (!glPreview)
        return false;

    auto info = this->d->m_glCompositor.effectInfo(
                    this->d->m_glCompositor.sourcePreview(id));
    auto interface = glPreview->controlInterface(this->d->m_engine, info.id());

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

void VideoEffects::setSourceRect(qint64 id, const QRectF &rect)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourceRect(id, rect);
}

void VideoEffects::setSourceZOrder(qint64 id, int zOrder)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourceZOrder(id, zOrder);
}

void VideoEffects::setSourceOpacity(qint64 id, qreal opacity)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourceOpacity(id, opacity);
}

void VideoEffects::setSourceAspectRatioMode(qint64 id,
                                            Qt::AspectRatioMode mode)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourceAspectRatioMode(id, mode);
}

void VideoEffects::setSourceEffects(qint64 id, const QStringList &effects)
{
    if (this->sourceEffects(id) == effects)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setSourceEffects(id, effects);
    }
}

void VideoEffects::setSourcePreview(qint64 id, const QString &preview)
{
    if (this->sourcePreview(id) == preview)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setSourcePreview(id, preview);
    }
}

void VideoEffects::setSourceChainEffects(qint64 id, bool chainEffects)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourceChainEffects(id, chainEffects);
}

void VideoEffects::setSourcePreserveNullPlugins(qint64 id, bool preserveNullPlugins)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.setSourcePreserveNullPlugins(id, preserveNullPlugins);
}

void VideoEffects::resetSourceEffects(qint64 id)
{
    this->setSourceEffects(id, {});
}

void VideoEffects::resetSourcePreview(qint64 id)
{
    this->setSourcePreview(id, {});
}

void VideoEffects::resetSourceChainEffects(qint64 id)
{
    this->setSourceChainEffects(id, false);
}

void VideoEffects::resetSourcePreserveNullPlugins(qint64 id)
{
    this->setSourcePreserveNullPlugins(id, true);
}

void VideoEffects::moveSourceEffect(qint64 id, int from, int to)
{
    auto totalEffects = this->d->m_glCompositor.sourceEffects(id).size();

    if (from == to
        || from < 0
        || from >= totalEffects
        || to < 0
        || to > totalEffects)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.moveSourceEffect(id, from, to);
    }
}

void VideoEffects::removeSourceEffect(qint64 id, int index)
{
    if (index < 0 || index >= this->d->m_glCompositor.sourceEffects(id).size())
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.removeSourceEffect(id, index);
    }
}

void VideoEffects::removeAllSourceEffects(qint64 id)
{
    if (this->d->m_glCompositor.sourceIsEmpty(id))
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.removeAllSourceEffects(id);
    }
}

void VideoEffects::applySourcePreview(qint64 id)
{
    QMutexLocker locker(&this->d->m_mutex);
    this->d->m_glCompositor.applySourcePreview(id);
}

void VideoEffects::setEffects(const QStringList &effects)
{
    if (this->effects() == effects)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setEffects(effects);
    }

    emit this->effectsChanged(effects);
    this->d->saveEffects();
    this->d->updateEffectsProperties();
}

void VideoEffects::setPreview(const QString &preview)
{
    if (this->d->m_glCompositor.preview() == preview)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        auto state = this->d->m_state;
        this->d->m_glCompositor.setState(AkElement::ElementStateNull);
        this->d->unlinkPreview();
        this->d->m_glCompositor.setPreview(preview);

        if (!preview.isEmpty())
            this->d->linkPreview();

        this->d->m_glCompositor.setState(state);
    }

    emit this->previewChanged(preview);
}

void VideoEffects::setState(AkElement::ElementState state)
{
    if (this->d->m_state == state)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setState(state);
        this->d->m_state = state;
    }

    emit this->stateChanged(state);
}

void VideoEffects::setChainEffects(bool chainEffects)
{
    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setChainEffects(chainEffects);
    }

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
    bool applied = false;
    QStringList effectsId;

    {
        QMutexLocker locker(&this->d->m_mutex);
        effectsId = this->d->m_glCompositor.effects();

        if (!this->d->m_glCompositor.preview().isEmpty()) {
            this->d->unlinkPreview();
            this->d->m_glCompositor.applyPreview();
            applied = true;
        }
    }

    if (applied)
        emit this->previewChanged({});

    auto curEffectsIds = this->d->m_glCompositor.effects();

    if (effectsId != curEffectsIds) {
        emit this->effectsChanged(curEffectsIds);
        this->d->saveEffects();
    }
}

void VideoEffects::moveEffect(int from, int to)
{
    auto totalEffects = this->d->m_glCompositor.effects().size();

    if (from == to
        || from < 0
        || from >= totalEffects
        || to < 0
        || to > totalEffects)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.moveEffect(from, to);
    }

    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeEffect(int index)
{
    if (index < 0 || index >= this->d->m_glCompositor.effects().size())
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.removeEffect(index);
    }

    emit this->effectsChanged(this->effects());
    this->d->saveEffects();
}

void VideoEffects::removeAllEffects()
{
    if (this->d->m_glCompositor.isEmpty())
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.removeAllEffects();
    }

    emit this->effectsChanged({});
    this->d->saveEffects();
}

void VideoEffects::updateAvailableEffects()
{
    this->d->m_glCompositor.updateAvailableEffects();
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

    AkVideoPacket videoPacket(packet);

    if (!videoPacket)
        return {};

    auto inputCaps = videoPacket.caps();
    AkVideoCaps outputCaps(AkVideoCaps::Format_rgba,
                           inputCaps.width(),
                           inputCaps.height(),
                           inputCaps.fps());

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_glCompositor.setOutputCaps(outputCaps);
        this->d->m_glCompositor.iStream(videoPacket);
    }

    return {};
}

VideoEffectsPrivate::VideoEffectsPrivate(VideoEffects *self):
    self(self)
{
    this->m_glCompositor.setPreserveNullPlugins(true);

    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::oStream,
                     self,
                     &VideoEffects::sendPacket,
                     Qt::DirectConnection);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::availableEffectsChanged,
                     self,
                     &VideoEffects::availableEffectsChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::chainEffectsChanged,
                     self,
                     &VideoEffects::chainEffectsChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::ready,
                     self,
                     &VideoEffects::ready);

    // Sources management.
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceAdded,
                     self,
                     &VideoEffects::sourceAdded,
                     Qt::DirectConnection);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceRemoved,
                     self,
                     &VideoEffects::sourceRemoved,
                     Qt::DirectConnection);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceRectChanged,
                     self,
                     &VideoEffects::sourceRectChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceZOrderChanged,
                     self,
                     &VideoEffects::sourceZOrderChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceOpacityChanged,
                     self,
                     &VideoEffects::sourceOpacityChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceAspectRatioModeChanged,
                     self,
                     &VideoEffects::sourceAspectRatioModeChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceEffectsChanged,
                     self,
                     &VideoEffects::sourceEffectsChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourcePreviewChanged,
                     self,
                     &VideoEffects::sourcePreviewChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceChainEffectsChanged,
                     self,
                     &VideoEffects::sourceChainEffectsChanged);
    QObject::connect(&this->m_glCompositor,
                     &AkGLCompositor::sourceIsEmptyChanged,
                     self,
                     &VideoEffects::sourceIsEmptyChanged);
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
    auto effects = this->m_glCompositor.effects();

    for (int i = 0; i < effects.size(); ++i) {
        config.beginGroup("VideoEffects_" + effects[i]);
        auto element = this->m_glCompositor.elementAt(i);

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

    for (auto &effect: this->m_glCompositor.effects()) {
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

    auto effects = this->m_glCompositor.effects();

    for (int i = 0; i < effects.size(); ++i) {
        config.beginGroup("VideoEffects_" + effects[i]);
        auto element = this->m_glCompositor.elementAt(i);

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
            this->m_glCompositor.link(effectPreview, Qt::DirectConnection);

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
            this->m_glCompositor.unlink(effectPreview);

            break;
        }
    }
}

#include "moc_videoeffects.cpp"
