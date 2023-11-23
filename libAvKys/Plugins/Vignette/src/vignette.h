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

#ifndef VIGNETTE_H
#define VIGNETTE_H

#include <akplugin.h>

class VignettePrivate;

class Vignette:
    public QObject,
    public IAkPlugin,
    public IAkVideoFilter,
    public IAkUIQml
{
    Q_OBJECT
    Q_INTERFACES(IAkPlugin)
    Q_PLUGIN_METADATA(IID "org.avkys.plugin" FILE "pspec.json")

    public:
    Q_PROPERTY(QString description
               READ description
               CONSTANT)
    Q_PROPERTY(AkElementType type
               READ type
               CONSTANT)
    Q_PROPERTY(AkElementCategory category
               READ category
               CONSTANT)
    Q_PROPERTY(QRgb color
               READ color
               WRITE setColor
               RESET resetColor
               NOTIFY colorChanged)
    Q_PROPERTY(qreal aspect
               READ aspect
               WRITE setAspect
               RESET resetAspect
               NOTIFY aspectChanged)
    Q_PROPERTY(qreal scale
               READ scale
               WRITE setScale
               RESET resetScale
               NOTIFY scaleChanged)
    Q_PROPERTY(qreal softness
               READ softness
               WRITE setSoftness
               RESET resetSoftness
               NOTIFY softnessChanged)

    public:
        explicit Vignette(QObject *parent=nullptr);
        ~Vignette();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE QRgb color() const;
        Q_INVOKABLE qreal aspect() const;
        Q_INVOKABLE qreal scale() const;
        Q_INVOKABLE qreal softness() const;

    private:
        VignettePrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void colorChanged(QRgb color);
        void aspectChanged(qreal aspect);
        void scaleChanged(qreal scale);
        void softnessChanged(qreal softness);

    public slots:
        void setColor(QRgb color);
        void setAspect(qreal aspect);
        void setScale(qreal scale);
        void setSoftness(qreal softness);
        void resetColor();
        void resetAspect();
        void resetScale();
        void resetSoftness();
};

#endif // VIGNETTE_H
