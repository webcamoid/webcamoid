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
#include <QVariant>
#include <akaudiocaps.h>
#include <akvideocaps.h>

#include "videolayercompat.h"

class VideoLayerCompatPrivate
{
    public:
        VideoLayerCompat *self;
        VideoLayer *m_backend {nullptr};
        qint64 m_activeId {-1};   // -1 = none or ambiguous (0 or 2+ active sources)
        bool m_settingVideoInput {false};   // reentrancy guard, see setVideoInput()

        explicit VideoLayerCompatPrivate(VideoLayerCompat *self, VideoLayer *backend);
        qint64 computeActiveId() const;
        qint64 idForDevice(const QString &device) const;
};

VideoLayerCompat::VideoLayerCompat(VideoLayer *backend, QObject *parent):
    QObject(parent)
{
    this->d = new VideoLayerCompatPrivate(this, backend);

    // Structural changes (source added/removed/enabled) can change WHICH
    // source counts as "the" active one -- recompute and, if it changed,
    // re-emit every derived signal at once.
    QObject::connect(backend,
                     &VideoLayer::sourceAdded,
                     this,
                     &VideoLayerCompat::refreshActiveSource,
                     Qt::DirectConnection);
    QObject::connect(backend,
                     &VideoLayer::sourceRemoved,
                     this,
                     &VideoLayerCompat::refreshActiveSource,
                     Qt::DirectConnection);
    QObject::connect(backend,
                     &VideoLayer::sourceEnabledChanged,
                     this,
                     &VideoLayerCompat::refreshActiveSource,
                     Qt::DirectConnection);

    // Passthrough signals -- same signature on both sides, nothing to
    // translate.
    QObject::connect(backend, &VideoLayer::camerasChanged,
                     this, &VideoLayerCompat::camerasChanged);
    QObject::connect(backend, &VideoLayer::screensChanged,
                     this, &VideoLayerCompat::screensChanged);
    QObject::connect(backend, &VideoLayer::windowsChanged,
                     this, &VideoLayerCompat::windowsChanged);
    QObject::connect(backend, &VideoLayer::canCaptureWindowsChanged,
                     this, &VideoLayerCompat::canCaptureWindowsChanged);
    QObject::connect(backend, &VideoLayer::stateChanged,
                     this, &VideoLayerCompat::stateChanged);
    QObject::connect(backend, &VideoLayer::playOnStartChanged,
                     this, &VideoLayerCompat::playOnStartChanged);
    QObject::connect(backend, &VideoLayer::outputsAsInputsChanged,
                     this, &VideoLayerCompat::outputsAsInputsChanged);

    QObject::connect(backend, &VideoLayer::camerasChanged,
                     this, [this]() { emit this->inputsChanged(this->inputs()); });
    QObject::connect(backend, &VideoLayer::screensChanged,
                     this, [this]() { emit this->inputsChanged(this->inputs()); });
    QObject::connect(backend, &VideoLayer::windowsChanged,
                     this, [this]() { emit this->inputsChanged(this->inputs()); });

    // Per-source signals -- only relevant to us when they're about
    // whichever source currently counts as "the" active one.
    QObject::connect(backend, &VideoLayer::sourceAudioCapsChanged,
                     this, &VideoLayerCompat::handleSourceAudioCapsChanged);
    QObject::connect(backend, &VideoLayer::sourceVideoCapsChanged,
                     this, &VideoLayerCompat::handleSourceVideoCapsChanged);
    QObject::connect(backend, &VideoLayer::sourceErrorChanged,
                     this, &VideoLayerCompat::handleSourceErrorChanged);
    QObject::connect(backend, &VideoLayer::isTorchSupportedChanged,
                     this, &VideoLayerCompat::handleIsTorchSupportedChanged);
    QObject::connect(backend, &VideoLayer::torchModeChanged,
                     this, &VideoLayerCompat::handleTorchModeChanged);
    QObject::connect(backend, &VideoLayer::cameraPermissionStatusChanged,
                     this, &VideoLayerCompat::handleCameraPermissionStatusChanged);

    this->d->m_activeId = this->d->computeActiveId();
}

VideoLayerCompat::~VideoLayerCompat()
{
    delete this->d;
}

QStringList VideoLayerCompat::videoSourceFileFilters() const
{
    return this->d->m_backend->videoSourceFileFilters();
}

QString VideoLayerCompat::videoInput() const
{
    if (this->d->m_activeId < 0)
        return {};

    return this->d->m_backend->sourceDevice(this->d->m_activeId);
}

QStringList VideoLayerCompat::inputs() const
{
    QStringList inputs;

    for (auto &source: this->d->m_backend->sourceIds())
        inputs << this->d->m_backend->sourceDevice(source.toLongLong());

    return inputs;
}

AkAudioCaps VideoLayerCompat::inputAudioCaps() const
{
    if (this->d->m_activeId < 0)
        return {};

    return this->d->m_backend->sourceAudioCaps(this->d->m_activeId);
}

AkVideoCaps VideoLayerCompat::inputVideoCaps() const
{
    if (this->d->m_activeId < 0)
        return {};

    return this->d->m_backend->sourceVideoCaps(this->d->m_activeId);
}

QStringList VideoLayerCompat::cameras() const
{
    return this->d->m_backend->cameras();
}

QStringList VideoLayerCompat::screens() const
{
    return this->d->m_backend->screens();
}

QStringList VideoLayerCompat::windows() const
{
    return this->d->m_backend->windows();
}

bool VideoLayerCompat::canCaptureWindows() const
{
    return this->d->m_backend->canCaptureWindows();
}

QStringList VideoLayerCompat::supportedFileFormats() const
{
    return this->d->m_backend->supportedFileFormats();
}

AkElement::ElementState VideoLayerCompat::state() const
{
    return this->d->m_backend->state();
}

bool VideoLayerCompat::isTorchSupported() const
{
    if (this->d->m_activeId < 0)
        return false;

    return this->d->m_backend->isTorchSupported(this->d->m_activeId);
}

VideoLayer::TorchMode VideoLayerCompat::torchMode() const
{
    if (this->d->m_activeId < 0)
        return VideoLayer::Torch_Off;

    return this->d->m_backend->torchMode(this->d->m_activeId);
}

VideoLayer::PermissionStatus VideoLayerCompat::cameraPermissionStatus() const
{
    if (this->d->m_activeId < 0)
        return VideoLayer::PermissionStatus_Undetermined;

    return this->d->m_backend->cameraPermissionStatus(this->d->m_activeId);
}

bool VideoLayerCompat::playOnStart() const
{
    return this->d->m_backend->playOnStart();
}

bool VideoLayerCompat::outputsAsInputs() const
{
    return this->d->m_backend->outputsAsInputs();
}

VideoLayer::InputType VideoLayerCompat::deviceType(const QString &device) const
{
    return this->d->m_backend->deviceType(device);
}

QStringList VideoLayerCompat::devicesByType(VideoLayer::InputType type) const
{
    return this->d->m_backend->devicesByType(type);
}

QString VideoLayerCompat::description(const QString &device) const
{
    return this->d->m_backend->description(device);
}

QString VideoLayerCompat::inputError() const
{
    if (this->d->m_activeId < 0)
        return {};

    return this->d->m_backend->sourceError(this->d->m_activeId);
}

bool VideoLayerCompat::embedInputControls(const QString &where,
                                          const QString &device,
                                          const QString &name) const
{
    auto id = this->d->idForDevice(device);

    if (id < 0)
        return false;

    return this->d->m_backend->embedControls(where, id, name);
}

void VideoLayerCompat::removeInterface(const QString &where) const
{
    this->d->m_backend->removeInterface(where);
}

void VideoLayerCompat::setInputStream(const QString &stream,
                                      const QString &description)
{
    auto id = this->d->m_backend->addSource(stream);

    if (id < 0)
        return;

    this->d->m_backend->setSourceLabel(id, description);

    // Old behavior: adding a stream/file only registers it as a known
    // device -- it does NOT start playing until separately selected via
    // setVideoInput(). New sources default to enabled, so this has to be
    // undone explicitly (unless it's already the selected one, e.g. this
    // device was re-added while it was already active).
    if (id != this->d->m_activeId)
        this->d->m_backend->setSourceEnabled(id, false);
}

void VideoLayerCompat::removeInputStream(const QString &stream)
{
    auto id = this->d->idForDevice(stream);

    if (id >= 0)
        this->d->m_backend->removeSource(id);
}

bool VideoLayerCompat::addCameraSource(const QString &source)
{
    auto id = this->d->m_backend->addSource(source);

    if (id < 0)
        return false;

    if (id != this->d->m_activeId)
        this->d->m_backend->setSourceEnabled(id, false);

    return true;
}

void VideoLayerCompat::removeCameraSource(const QString &source)
{
    auto id = this->d->idForDevice(source);

    if (id >= 0)
        this->d->m_backend->removeSource(id);
}

bool VideoLayerCompat::addScreenSource(const QString &source)
{
    auto id = this->d->m_backend->addSource(source);

    if (id < 0)
        return false;

    // Same as setInputStream() above -- adding a screen/window only makes
    // it available, it doesn't start capturing it.
    if (id != this->d->m_activeId)
        this->d->m_backend->setSourceEnabled(id, false);

    return true;
}

void VideoLayerCompat::removeScreenSource(const QString &source)
{
    auto id = this->d->idForDevice(source);

    if (id >= 0)
        this->d->m_backend->removeSource(id);
}

void VideoLayerCompat::setVideoInput(const QString &videoInput)
{
    if (this->videoInput() == videoInput)
        return;

    // Reentrancy guard: setSourceEnabled() below synchronously triggers
    // refreshActiveSource(), which emits videoInputChanged(). If anything
    // on the QML side reacts to that by writing videoInput again (a
    // two-way binding, a Connections handler "correcting" a selection,
    // etc.), that write re-enters this exact function -- without this
    // guard, that's unbounded synchronous recursion (stack overflow), not
    // just a redundant update.
    if (this->d->m_settingVideoInput)
        return;

    this->d->m_settingVideoInput = true;

    // Old behavior: only one source can ever be truly active/playing at
    // once, even though several may already exist (added via
    // addScreenSource()/setInputStream() without being "selected" yet).
    // Replicate it with enable/disable instead of remove/add -- removing
    // and re-adding tore down and rebuilt the element every switch, and
    // crucially left whatever was previously active still enabled (and
    // therefore still playing) if the new one hadn't fully replaced it
    // yet, which is what caused two sources' frames to interleave.
    auto id = this->d->idForDevice(videoInput);

    if (id < 0 && !videoInput.isEmpty())
        id = this->d->m_backend->addSource(videoInput);

    for (auto &idVariant: this->d->m_backend->sourceIds()) {
        auto sourceId = idVariant.toLongLong();
        this->d->m_backend->setSourceEnabled(sourceId, sourceId == id);
    }

    this->d->m_settingVideoInput = false;

    // refreshActiveSource() fires from the backend's sourceEnabledChanged
    // signals above, no need to call it here too.
}

void VideoLayerCompat::setState(AkElement::ElementState state)
{
    this->d->m_backend->setState(state);
}

void VideoLayerCompat::setTorchMode(VideoLayer::TorchMode mode)
{
    if (this->d->m_activeId >= 0)
        this->d->m_backend->setTorchMode(this->d->m_activeId, mode);
}

void VideoLayerCompat::setPlayOnStart(bool playOnStart)
{
    this->d->m_backend->setPlayOnStart(playOnStart);
}

void VideoLayerCompat::setOutputsAsInputs(bool outputsAsInputs)
{
    this->d->m_backend->setOutputsAsInputs(outputsAsInputs);
}

void VideoLayerCompat::resetVideoInput()
{
    this->setVideoInput({});
}

void VideoLayerCompat::resetState()
{
    this->setState(AkElement::ElementStateNull);
}

void VideoLayerCompat::resetTorchMode()
{
    this->setTorchMode(VideoLayer::Torch_Off);
}

void VideoLayerCompat::resetPlayOnStart()
{
    this->setPlayOnStart(true);
}

void VideoLayerCompat::resetOutputsAsInputs()
{
    this->setOutputsAsInputs(false);
}

void VideoLayerCompat::updateInputs()
{
    this->d->m_backend->updateInputs();
}

void VideoLayerCompat::refreshActiveSource()
{
    auto newId = this->d->computeActiveId();

    if (newId == this->d->m_activeId)
        return;

    this->d->m_activeId = newId;

    // Switching which source counts as "the" active one changes every
    // derived value at once.
    emit this->videoInputChanged(this->videoInput());
    emit this->inputAudioCapsChanged(this->inputAudioCaps());
    emit this->inputVideoCapsChanged(this->inputVideoCaps());
    emit this->inputErrorChanged(this->inputError());
    emit this->isTorchSupportedChanged(this->isTorchSupported());
    emit this->torchModeChanged(this->torchMode());
    emit this->cameraPermissionStatusChanged(this->cameraPermissionStatus());
    emit this->inputsChanged(this->inputs());
}

void VideoLayerCompat::handleSourceAudioCapsChanged(qint64 id,
                                                     const AkAudioCaps &caps)
{
    if (id == this->d->m_activeId)
        emit this->inputAudioCapsChanged(caps);
}

void VideoLayerCompat::handleSourceVideoCapsChanged(qint64 id,
                                                     const AkVideoCaps &caps)
{
    if (id == this->d->m_activeId)
        emit this->inputVideoCapsChanged(caps);
}

void VideoLayerCompat::handleSourceErrorChanged(qint64 id, const QString &error)
{
    if (id == this->d->m_activeId)
        emit this->inputErrorChanged(error);
}

void VideoLayerCompat::handleIsTorchSupportedChanged(qint64 id, bool torchSupported)
{
    if (id == this->d->m_activeId)
        emit this->isTorchSupportedChanged(torchSupported);
}

void VideoLayerCompat::handleTorchModeChanged(qint64 id, VideoLayer::TorchMode mode)
{
    if (id == this->d->m_activeId)
        emit this->torchModeChanged(mode);
}

void VideoLayerCompat::handleCameraPermissionStatusChanged(
        qint64 id, VideoLayer::PermissionStatus status)
{
    if (id == this->d->m_activeId)
        emit this->cameraPermissionStatusChanged(status);
}

VideoLayerCompatPrivate::VideoLayerCompatPrivate(VideoLayerCompat *self,
                                                 VideoLayer *backend):
    self(self),
    m_backend(backend)
{
}

qint64 VideoLayerCompatPrivate::computeActiveId() const
{
    // "The" active source is the single ENABLED one -- several may exist
    // at once now (added but not selected), which didn't happen in the
    // old single-source model this class emulates.
    qint64 activeId = -1;
    int enabledCount = 0;

    for (auto &idVariant: this->m_backend->sourceIds()) {
        auto id = idVariant.toLongLong();

        if (this->m_backend->sourceEnabled(id)) {
            activeId = id;
            enabledCount++;
        }
    }

    if (enabledCount != 1)
        return -1;   // none enabled, or ambiguous (2+) -- old API has no answer for this

    return activeId;
}

qint64 VideoLayerCompatPrivate::idForDevice(const QString &device) const
{
    for (auto &idVariant: this->m_backend->sourceIds()) {
        auto id = idVariant.toLongLong();

        if (this->m_backend->sourceDevice(id) == device)
            return id;
    }

    return -1;
}

#include "moc_videolayercompat.cpp"
