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

#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <akelement.h>

class VideoEffectsPrivate;
class VideoEffects;
class QQmlApplicationEngine;
class AkPluginInfo;

using VideoEffectsPtr = QSharedPointer<VideoEffects>;

class VideoEffects: public QObject
{
    Q_OBJECT
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

        Q_INVOKABLE QStringList availableEffects() const;
        Q_INVOKABLE QStringList effects() const;
        Q_INVOKABLE QString preview() const;
        Q_INVOKABLE AkPluginInfo effectInfo(const QString &effectId) const;
        Q_INVOKABLE QString effectDescription(const QString &effectId) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool chainEffects() const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       int effectIndex,
                                       const QString &name={}) const;
        Q_INVOKABLE bool embedPreviewControls(const QString &where,
                                              const QString &name={}) const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        VideoEffectsPrivate *d;

    signals:
        void availableEffectsChanged(const QStringList &availableEffects);
        void effectsChanged(const QStringList &effects);
        void previewChanged(const QString &preview);
        void oStream(const AkPacket &packet);
        void stateChanged(AkElement::ElementState state);
        void chainEffectsChanged(bool chainEffects);

    public slots:
        void setEffects(const QStringList &effects);
        void setPreview(const QString &preview);
        void setState(AkElement::ElementState state);
        void setChainEffects(bool chainEffects);
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
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);
        AkPacket iStream(const AkPacket &packet);
};

#endif // VIDEOEFFECTS_H
