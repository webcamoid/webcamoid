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

#ifndef WAVE_H
#define WAVE_H

#include <akplugin.h>

class WavePrivate;

class Wave:
     public QObject,
     public IAkPlugin,
     public IAkVideoFilter,
     public IAkUIQml
{
    Q_OBJECT
    Q_INTERFACES(IAkPlugin)
    Q_PLUGIN_METADATA(IID "org.avkys.plugin" FILE "pspec.json")
    Q_PROPERTY(QString description
               READ description
               CONSTANT)
    Q_PROPERTY(AkElementType type
               READ type
               CONSTANT)
    Q_PROPERTY(AkElementCategory category
               READ category
               CONSTANT)
    Q_PROPERTY(qreal amplitudeX
               READ amplitudeX
               WRITE setAmplitudeX
               RESET resetAmplitudeX
               NOTIFY amplitudeXChanged)
    Q_PROPERTY(qreal amplitudeY
               READ amplitudeY
               WRITE setAmplitudeY
               RESET resetAmplitudeY
               NOTIFY amplitudeYChanged)
    Q_PROPERTY(qreal frequencyX
               READ frequencyX
               WRITE setFrequencyX
               RESET resetFrequencyX
               NOTIFY frequencyXChanged)
    Q_PROPERTY(qreal frequencyY
               READ frequencyY
               WRITE setFrequencyY
               RESET resetFrequencyY
               NOTIFY frequencyYChanged)
    Q_PROPERTY(qreal phaseX
               READ phaseX
               WRITE setPhaseX
               RESET resetPhaseX
               NOTIFY phaseXChanged)
    Q_PROPERTY(qreal phaseY
               READ phaseY
               WRITE setPhaseY
               RESET resetPhaseY
               NOTIFY phaseYChanged)

    public:
        explicit Wave(QObject *parent=nullptr);
        ~Wave();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE qreal amplitudeX() const;
        Q_INVOKABLE qreal amplitudeY() const;
        Q_INVOKABLE qreal frequencyX() const;
        Q_INVOKABLE qreal frequencyY() const;
        Q_INVOKABLE qreal phaseX() const;
        Q_INVOKABLE qreal phaseY() const;

    private:
        WavePrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void amplitudeXChanged(qreal amplitudeX);
        void amplitudeYChanged(qreal amplitudeY);
        void frequencyXChanged(qreal frequencyX);
        void frequencyYChanged(qreal frequencyY);
        void phaseXChanged(qreal phaseX);
        void phaseYChanged(qreal phaseY);

    public slots:
        void setAmplitudeX(qreal amplitudeX);
        void setAmplitudeY(qreal amplitudeY);
        void setFrequencyX(qreal frequencyX);
        void setFrequencyY(qreal frequencyY);
        void setPhaseX(qreal phaseX);
        void setPhaseY(qreal phaseY);
        void resetAmplitudeX();
        void resetAmplitudeY();
        void resetFrequencyX();
        void resetFrequencyY();
        void resetPhaseX();
        void resetPhaseY();
};

#endif // WAVE_H
