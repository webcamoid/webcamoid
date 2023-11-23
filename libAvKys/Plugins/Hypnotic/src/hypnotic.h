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

#ifndef HYPNOTIC_H
#define HYPNOTIC_H

#include <akplugin.h>

class HypnoticPrivate;

class Hypnotic:
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
    Q_PROPERTY(OpticMode mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(int speedInc
               READ speedInc
               WRITE setSpeedInc
               RESET resetSpeedInc
               NOTIFY speedIncChanged)
    Q_PROPERTY(int threshold
               READ threshold
               WRITE setThreshold
               RESET resetThreshold
               NOTIFY thresholdChanged)

    public:
        enum OpticMode
        {
            OpticModeSpiral1,
            OpticModeSpiral2,
            OpticModeParabola,
            OpticModeHorizontalStripe
        };
        Q_ENUM(OpticMode)

        explicit Hypnotic(QObject *parent=nullptr);
        ~Hypnotic();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE OpticMode mode() const;
        Q_INVOKABLE int speedInc() const;
        Q_INVOKABLE int threshold() const;

    private:
        HypnoticPrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(OpticMode mode);
        void speedIncChanged(int speedInc);
        void thresholdChanged(int threshold);

    public slots:
        void setMode(OpticMode mode);
        void setSpeedInc(int speedInc);
        void setThreshold(int threshold);
        void resetMode();
        void resetSpeedInc();
        void resetThreshold();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, Hypnotic::OpticMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, Hypnotic::OpticMode mode);

Q_DECLARE_METATYPE(Hypnotic::OpticMode)

#endif // HYPNOTIC_H
