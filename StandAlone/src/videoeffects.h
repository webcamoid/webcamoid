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

#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QMutex>
#include <QQmlApplicationEngine>
#include <akelement.h>

class VideoEffects;

typedef QSharedPointer<VideoEffects> VideoEffectsPtr;

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
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(bool advancedMode
               READ advancedMode
               WRITE setAdvancedMode
               RESET resetAdvancedMode
               NOTIFY advancedModeChanged)

    public:
        explicit VideoEffects(QQmlApplicationEngine *engine=nullptr,
                              QObject *parent=nullptr);
        ~VideoEffects();

        Q_INVOKABLE QStringList availableEffects() const;
        Q_INVOKABLE QStringList effects() const;
        Q_INVOKABLE QVariantMap effectInfo(const QString &effectId) const;
        Q_INVOKABLE QString effectDescription(const QString &effectId) const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool advancedMode() const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       int effectIndex,
                                       const QString &name="") const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        QQmlApplicationEngine *m_engine;
        QStringList m_availableEffects;
        AkElement::ElementState m_state;
        bool m_advancedMode;
        QList<AkElementPtr> m_effects;
        QStringList m_effectsId;
        AkElementPtr m_videoMux;
        QMutex m_mutex;

    signals:
        void availableEffectsChanged(const QStringList &availableEffects);
        void effectsChanged(const QStringList &effects);
        void oStream(const AkPacket &packet);
        void stateChanged(AkElement::ElementState state);
        void advancedModeChanged(bool advancedMode);

    public slots:
        void setEffects(const QStringList &effects, bool emitSignal=true);
        void setState(AkElement::ElementState state);
        void setAdvancedMode(bool advancedMode);
        void resetEffects();
        void resetState();
        void resetAdvancedMode();
        AkElementPtr appendEffect(const QString &effectId, bool preview=false);
        void showPreview(const QString &effectId);
        void setAsPreview(int index, bool preview=false);
        void moveEffect(int from, int to);
        void removeEffect(int index);
        void removeAllPreviews();
        void updateEffects();
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=nullptr);

    private slots:
        void advancedModeUpdated(bool advancedMode);
        void loadProperties();
        void saveEffects(const QStringList &effects);
        void saveAdvancedMode(bool advancedMode);
        void saveProperties();
};

#endif // VIDEOEFFECTS_H
