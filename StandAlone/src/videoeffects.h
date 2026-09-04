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

#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QOpenGLFunctions>
#include <qrgb.h>
#include <iak/akelement.h>
#include <iak/akvideoeffect.h>

class VideoEffectsPrivate;
class VideoEffects;
class QQmlApplicationEngine;
class QQuickWindow;
class QRectF;
class QSize;
class AkPluginInfo;
class AkVideoCaps;

using VideoEffectsPtr = QSharedPointer<VideoEffects>;

class VideoEffects: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkVideoCaps outputCaps
               READ outputCaps
               WRITE setOutputCaps
               RESET resetOutputCaps
               NOTIFY outputCapsChanged)
    Q_PROPERTY(QRgb canvasColor
               READ canvasColor
               WRITE setCanvasColor
               RESET resetCanvasColor
               NOTIFY canvasColorChanged)
    Q_PROPERTY(size_t outputBufferSize
               READ outputBufferSize
               WRITE setOutputBufferSize
               RESET resetOutputBufferSize
               NOTIFY outputBufferSizeChanged)
    Q_PROPERTY(QStringList availableEffects
               READ availableEffects
               NOTIFY availableEffectsChanged)
    Q_PROPERTY(QStringList effects
               READ effects
               WRITE setEffects
               RESET resetEffects
               NOTIFY effectsChanged)
    Q_PROPERTY(QString preview
               READ preview
               WRITE setPreview
               RESET resetPreview
               NOTIFY previewChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(bool chainEffects
               READ chainEffects
               WRITE setChainEffects
               RESET resetChainEffects
               NOTIFY chainEffectsChanged)

    public:
        VideoEffects(QQmlApplicationEngine *engine=nullptr,
                     QObject *parent=nullptr);
        ~VideoEffects();

        // Global output pipeline
        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE QRgb canvasColor() const;
        Q_INVOKABLE size_t outputBufferSize() const;
        Q_INVOKABLE QStringList availableEffects() const;
        Q_INVOKABLE QStringList effects() const;
        Q_INVOKABLE QString preview() const;
        Q_INVOKABLE AkPluginInfo effectInfo(const QString &effectId) const;
        Q_INVOKABLE QString effectDescription(const QString &effectId) const;
        Q_INVOKABLE bool effectEnabled(int index) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool chainEffects() const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       int effectIndex,
                                       const QString &name={}) const;
        Q_INVOKABLE bool embedPreviewControls(const QString &where,
                                              const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

        // Source management
        Q_INVOKABLE qint64 addSource();
        Q_INVOKABLE qint64 addSource(qint64 id, const QString &device={});
        Q_INVOKABLE void removeSource(qint64 id);
        Q_INVOKABLE QVariantList sourceIds() const;

        // Source layout
        Q_INVOKABLE QRectF sourceRect(qint64 id) const;
        Q_INVOKABLE int sourceZOrder(qint64 id) const;
        Q_INVOKABLE qreal sourceOpacity(qint64 id) const;
        Q_INVOKABLE qreal sourceRotation(qint64 id) const;
        Q_INVOKABLE Qt::AspectRatioMode sourceAspectRatioMode(qint64 id) const;

        // Source effect pipeline
        Q_INVOKABLE QStringList sourceEffects(qint64 id) const;
        Q_INVOKABLE QString sourcePreview(qint64 id) const;
        Q_INVOKABLE AkVideoEffectPtr sourceElementAt(qint64 id, int index) const;
        Q_INVOKABLE AkVideoEffectPtr sourcePreviewElement(qint64 id) const;
        Q_INVOKABLE AkPluginInfo sourceEffectInfo(qint64 id, int index) const;
        Q_INVOKABLE bool sourceEffectEnabled(qint64 id, int index) const;
        Q_INVOKABLE bool sourceChainEffects(qint64 id) const;
        Q_INVOKABLE bool sourcePreserveNullPlugins(qint64 id) const;
        Q_INVOKABLE bool sourceIsEmpty(qint64 id) const;

        // Source UI embedding
        Q_INVOKABLE bool embedControls(const QString &where,
                                       qint64 id,
                                       int effectIndex,
                                       const QString &name={}) const;
        Q_INVOKABLE bool embedPreviewControls(const QString &where,
                                              qint64 id,
                                              const QString &name={}) const;

    private:
        VideoEffectsPrivate *d;

    signals:
        void frameCaptured(const QImage &frame);
        void ready();

        // Global output pipeline
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void canvasColorChanged(QRgb canvasColor);
        void outputBufferSizeChanged(size_t outputBufferSize);
        void availableEffectsChanged(const QStringList &availableEffects);
        void effectsChanged(const QStringList &effects);
        void previewChanged(const QString &preview);
        void oStream(const AkPacket &packet);
        void stateChanged(AkElement::ElementState state);
        void chainEffectsChanged(bool chainEffects);
        void effectEnabledChanged(int index, bool enabled);

        // Per-source signals
        void sourceAdded(qint64 id);
        void sourceRemoved(qint64 id);
        void sourceRectChanged(qint64 id, const QRectF &rect);
        void sourceZOrderChanged(qint64 id, int zOrder);
        void sourceOpacityChanged(qint64 id, qreal opacity);
        void sourceRotationChanged(qint64 id, qreal rotation);
        void sourceAspectRatioModeChanged(qint64 id, Qt::AspectRatioMode mode);
        void sourceEffectsChanged(qint64 id, const QStringList &effects);
        void sourcePreviewChanged(qint64 id, const QString &preview);
        void sourceChainEffectsChanged(qint64 id, bool chainEffects);
        void sourcePreserveNullPluginsChanged(qint64 id, bool preserveNullPlugins);
        void sourceIsEmptyChanged(qint64 id, bool isEmpty);
        void sourceEffectEnabledChanged(qint64 id, int index, bool enabled);
        void outputTextureReady(GLuint texture, const QSize &size);

    public slots:
        // Global output pipeline
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setCanvasColor(QRgb canvasColor);
        void setOutputBufferSize(size_t outputBufferSize);
        void setEffects(const QStringList &effects);
        void setPreview(const QString &preview);
        void setState(AkElement::ElementState state);
        void setChainEffects(bool chainEffects);
        void setEffectEnabled(int index, bool enabled);
        void resetOutputCaps();
        void resetCanvasColor();
        void resetOutputBufferSize();
        void resetEffects();
        void resetPreview();
        void resetState();
        void resetChainEffects();
        void sendPacket(const AkPacket &packet);
        void applyPreview();
        void moveEffect(int from, int to);
        void removeEffect(int index);
        void removeAllEffects();
        void updateAvailableEffects();
        void linkLayoutEditor();
        void unlinkLayoutEditor();
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        AkPacket iStream(const AkPacket &packet);
        void addPacketReader();
        void removePacketReader();
        void attachToWindow(QQuickWindow *window);
        void detachFromWindow();
        void captureFrame();

        // Source layout
        void setSourceRect(qint64 id, const QRectF &rect);
        void setSourceZOrder(qint64 id, int zOrder);
        void setSourceOpacity(qint64 id, qreal opacity);
        void setSourceRotation(qint64 id, qreal rotation);
        void setSourceAspectRatioMode(qint64 id, Qt::AspectRatioMode mode);

        // Source effect pipeline
        void setSourceEffects(qint64 id, const QStringList &effects);
        void setSourcePreview(qint64 id, const QString &preview);
        void setSourceChainEffects(qint64 id, bool chainEffects);
        void setSourcePreserveNullPlugins(qint64 id, bool preserveNullPlugins);
        void setSourceEffectEnabled(qint64 id, int index, bool enabled);
        void resetSourceEffects(qint64 id);
        void resetSourcePreview(qint64 id);
        void resetSourceChainEffects(qint64 id);
        void resetSourcePreserveNullPlugins(qint64 id);
        void moveSourceEffect(qint64 id, int from, int to);
        void removeSourceEffect(qint64 id, int index);
        void removeAllSourceEffects(qint64 id);
        void applySourcePreview(qint64 id);
};

#endif // VIDEOEFFECTS_H
