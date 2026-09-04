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

#ifndef AKGLCOMPOSITOR_H
#define AKGLCOMPOSITOR_H

#include <QOpenGLFunctions>
#include <QVariant>
#include <qrgb.h>

#include "iak/akelement.h"
#include "iak/akvideoeffect.h"

class AkGLCompositorPrivate;
class AkPacket;
class AkPluginInfo;
class AkVideoCaps;
class QRectF;
class QQuickWindow;

class AKCOMMONS_EXPORT AkGLCompositor:
        public AkElement,
        protected QOpenGLFunctions
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
    // Compositor output pipeline effects.
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
    Q_PROPERTY(bool chainEffects
               READ chainEffects
               WRITE setChainEffects
               RESET resetChainEffects
               NOTIFY chainEffectsChanged)
    Q_PROPERTY(bool preserveNullPlugins
               READ preserveNullPlugins
               WRITE setPreserveNullPlugins
               RESET resetPreserveNullPlugins
               NOTIFY preserveNullPluginsChanged)
    Q_PROPERTY(bool isEmpty
               READ isEmpty
               NOTIFY isEmptyChanged)

    public:
        explicit AkGLCompositor(QObject *parent=nullptr);
        AkGLCompositor(const AkGLCompositor &other) = delete;
        AkGLCompositor &operator =(const AkGLCompositor &other) = delete;
        ~AkGLCompositor();

        Q_INVOKABLE AkVideoCaps outputCaps() const;
        Q_INVOKABLE QRgb canvasColor() const;
        Q_INVOKABLE size_t outputBufferSize() const;

        // Global data for all available effects in all pipelines.
        Q_INVOKABLE QStringList availableEffects() const;
        Q_INVOKABLE AkPluginInfo effectInfo(const QString &effectId) const;
        Q_INVOKABLE QString effectDescription(const QString &effectId) const;

        // Output pipeline effects.
        Q_INVOKABLE QStringList effects() const;
        Q_INVOKABLE QString preview() const;
        Q_INVOKABLE AkVideoEffectPtr elementAt(int index) const;
        Q_INVOKABLE AkVideoEffectPtr previewElement() const;
        Q_INVOKABLE AkPluginInfo effectInfo(int index) const;
        Q_INVOKABLE bool effectEnabled(int index) const;
        Q_INVOKABLE bool chainEffects() const;
        Q_INVOKABLE bool preserveNullPlugins() const;
        Q_INVOKABLE bool isEmpty() const;

        // Sources management.
        Q_INVOKABLE qint64 addSource();
        Q_INVOKABLE qint64 addSource(qint64 id);
        Q_INVOKABLE void removeSource(qint64 id);
        Q_INVOKABLE QVariantList sourceIds() const;

        // Source layout in the canvas. rect is normalized to [0, 1] and
        // relative to  outputCaps().
        Q_INVOKABLE QRectF sourceRect(qint64 id) const;
        Q_INVOKABLE int sourceZOrder(qint64 id) const;
        Q_INVOKABLE qreal sourceOpacity(qint64 id) const;
        Q_INVOKABLE qreal sourceRotation(qint64 id) const;
        Q_INVOKABLE Qt::AspectRatioMode sourceAspectRatioMode(qint64 id) const;

        // Source effect
        Q_INVOKABLE QStringList sourceEffects(qint64 id) const;
        Q_INVOKABLE QString sourcePreview(qint64 id) const;
        Q_INVOKABLE AkVideoEffectPtr sourceElementAt(qint64 id, int index) const;
        Q_INVOKABLE AkVideoEffectPtr sourcePreviewElement(qint64 id) const;
        Q_INVOKABLE AkPluginInfo sourceEffectInfo(qint64 id, int index) const;
        Q_INVOKABLE bool sourceEffectEnabled(qint64 id, int index) const;
        Q_INVOKABLE bool sourceChainEffects(qint64 id) const;
        Q_INVOKABLE bool sourcePreserveNullPlugins(qint64 id) const;
        Q_INVOKABLE bool sourceIsEmpty(qint64 id) const;

    private:
        AkGLCompositorPrivate *d;

    Q_SIGNALS:
        void outputCapsChanged(const AkVideoCaps &outputCaps);
        void canvasColorChanged(QRgb canvasColor);
        void outputBufferSizeChanged(size_t outputBufferSize);
        void availableEffectsChanged(const QStringList &availableEffects);
        void frameCaptured(const QImage &frame);
        void ready();

        // Output pipeline signals
        void effectsChanged(const QStringList &effects);
        void previewChanged(const QString &preview);
        void chainEffectsChanged(bool chainEffects);
        void preserveNullPluginsChanged(bool preserveNullPlugins);
        void isEmptyChanged(bool isEmpty);
        void effectEnabledChanged(int index, bool enabled);

        void sourceAdded(qint64 id);
        void sourceRemoved(qint64 id);
        void sourceRectChanged(qint64 id, const QRectF &rect);
        void sourceZOrderChanged(qint64 id, int zOrder);
        void sourceOpacityChanged(qint64 id, qreal opacity);
        void sourceRotationChanged(qint64 id, qreal rotation);
        void sourceAspectRatioModeChanged(qint64 id, Qt::AspectRatioMode mode);

        // Source pipeline signals
        void sourceEffectsChanged(qint64 id, const QStringList &effects);
        void sourcePreviewChanged(qint64 id, const QString &preview);
        void sourceChainEffectsChanged(qint64 id, bool chainEffects);
        void sourcePreserveNullPluginsChanged(qint64 id, bool preserveNullPlugins);
        void sourceIsEmptyChanged(qint64 id, bool isEmpty);
        void sourceEffectEnabledChanged(qint64 id, int index, bool enabled);

        // Direct read of the output texture
        void outputTextureReady(GLuint texture, const QSize &size);

    public Q_SLOTS:
        void setOutputCaps(const AkVideoCaps &outputCaps);
        void setCanvasColor(QRgb canvasColor);
        void setOutputBufferSize(size_t outputBufferSize);
        bool setState(AkElement::ElementState state) override;
        void resetOutputCaps();
        void resetCanvasColor();
        void resetOutputBufferSize();
        void captureFrame();

        // Output pipeline controls.
        void setEffects(const QStringList &effects);
        void setPreview(const QString &preview);
        void setChainEffects(bool chainEffects);
        void setPreserveNullPlugins(bool preserveNullPlugins);
        void setEffectEnabled(int index, bool enabled);
        void resetEffects();
        void resetPreview();
        void resetChainEffects();
        void resetPreserveNullPlugins();
        void moveEffect(int from, int to);
        void removeEffect(int index);
        void removeAllEffects();
        void applyPreview();

        // Source layout.
        void setSourceRect(qint64 id, const QRectF &rect);
        void setSourceZOrder(qint64 id, int zOrder);
        void setSourceOpacity(qint64 id, qreal opacity);
        void setSourceRotation(qint64 id, qreal rotation);
        void setSourceAspectRatioMode(qint64 id, Qt::AspectRatioMode mode);

        // Sources pipeline controls.
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

        // Enable/disable the CPU readback and oStream().
        void addPacketReader();
        void removePacketReader();
        void attachToWindow(QQuickWindow *window);
        void detachFromWindow();

        AkPacket iVideoStream(const AkVideoPacket &videoPacket) override;

        void updateAvailableEffects();
        static void registerTypes();

    friend class AkGLCompositorPrivate;
};

Q_DECLARE_METATYPE(AkGLCompositor)

#endif // AKGLCOMPOSITOR_H
