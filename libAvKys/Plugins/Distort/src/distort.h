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

#ifndef DISTORT_H
#define DISTORT_H

#include <akplugin.h>

class DistortPrivate;

class Distort:
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
    Q_PROPERTY(qreal amplitude
               READ amplitude
               WRITE setAmplitude
               RESET resetAmplitude
               NOTIFY amplitudeChanged)
    Q_PROPERTY(qreal frequency
               READ frequency
               WRITE setFrequency
               RESET resetFrequency
               NOTIFY frequencyChanged)
    Q_PROPERTY(int gridSizeLog
               READ gridSizeLog
               WRITE setGridSizeLog
               RESET resetGridSizeLog
               NOTIFY gridSizeLogChanged)

    public:
        explicit Distort(QObject *parent=nullptr);
        ~Distort();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE qreal amplitude() const;
        Q_INVOKABLE qreal frequency() const;
        Q_INVOKABLE int gridSizeLog() const;

    private:
        DistortPrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void amplitudeChanged(qreal amplitude);
        void frequencyChanged(qreal frequency);
        void gridSizeLogChanged(int gridSizeLog);

    public slots:
        void setAmplitude(qreal amplitude);
        void setFrequency(qreal frequency);
        void setGridSizeLog(int gridSizeLog);
        void resetAmplitude();
        void resetFrequency();
        void resetGridSizeLog();
};

#endif // DISTORT_H
