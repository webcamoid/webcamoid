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

#ifndef AKGLPIPELINE_H
#define AKGLPIPELINE_H

#include "iak/akvideoeffect.h"

class AkGLPipelinePrivate;
class AkPluginInfo;
class QOpenGLBuffer;
class QOpenGLFramebufferObject;

class AKCOMMONS_EXPORT AkGLPipeline: public QObject
{
    Q_OBJECT
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
        explicit AkGLPipeline(QObject *parent=nullptr);
        AkGLPipeline(const AkGLPipeline &other) = delete;
        AkGLPipeline &operator =(const AkGLPipeline &other) = delete;
        ~AkGLPipeline() override;

        Q_INVOKABLE QStringList effects() const;
        Q_INVOKABLE QString preview() const;
        Q_INVOKABLE bool chainEffects() const;
        Q_INVOKABLE bool preserveNullPlugins() const;
        Q_INVOKABLE bool effectEnabled(int index) const;
        Q_INVOKABLE bool isEmpty() const;
        Q_INVOKABLE AkVideoEffectPtr elementAt(int index) const;
        Q_INVOKABLE AkVideoEffectPtr previewElement() const;
        Q_INVOKABLE AkPluginInfo effectInfo(int index) const;
        Q_INVOKABLE void init(QOpenGLFunctions *glFunctions,
                              QOpenGLBuffer *vbo,
                              QOpenGLBuffer *ibo);
        Q_INVOKABLE void uninit();
        Q_INVOKABLE void process(QOpenGLFramebufferObject *inputFbo,
                                 QOpenGLFramebufferObject *&outputFbo,
                                 qint64 streamId,
                                 qreal pts);

    private:
        AkGLPipelinePrivate *d;

    Q_SIGNALS:
        void effectsChanged(const QStringList &effects);
        void previewChanged(const QString &preview);
        void chainEffectsChanged(bool chainEffects);
        void preserveNullPluginsChanged(bool preserveNullPlugins);
        void effectEnabledChanged(int index, bool enabled);
        void isEmptyChanged(bool isEmpty);

    public Q_SLOTS:
        void setEffects(const QStringList &effectIds);
        void setPreview(const QString &effectId);
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

        static void registerTypes();
};

Q_DECLARE_METATYPE(AkGLPipeline)

#endif // AKGLPIPELINE_H
