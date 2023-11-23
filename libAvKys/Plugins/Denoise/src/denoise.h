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

#ifndef DENOISE_H
#define DENOISE_H

#include <akplugin.h>

class DenoisePrivate;

class Denoise:
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
    Q_PROPERTY(int radius
               READ radius
               WRITE setRadius
               RESET resetRadius
               NOTIFY radiusChanged)
    Q_PROPERTY(int factor
               READ factor
               WRITE setFactor
               RESET resetFactor
               NOTIFY factorChanged)
    Q_PROPERTY(int mu
               READ mu
               WRITE setMu
               RESET resetMu
               NOTIFY muChanged)
    Q_PROPERTY(qreal sigma
               READ sigma
               WRITE setSigma
               RESET resetSigma
               NOTIFY sigmaChanged)

    public:
        explicit Denoise(QObject *parent=nullptr);
        ~Denoise();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE int radius() const;
        Q_INVOKABLE int factor() const;
        Q_INVOKABLE int mu() const;
        Q_INVOKABLE qreal sigma() const;

    private:
        DenoisePrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void radiusChanged(int radius);
        void factorChanged(int factor);
        void muChanged(int mu);
        void sigmaChanged(qreal sigma);

    public slots:
        void setRadius(int radius);
        void setFactor(int factor);
        void setMu(int mu);
        void setSigma(qreal sigma);
        void resetRadius();
        void resetFactor();
        void resetMu();
        void resetSigma();
};

#endif // DENOISE_H
