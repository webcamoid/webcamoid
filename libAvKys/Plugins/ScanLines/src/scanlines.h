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

#ifndef SCANLINES_H
#define SCANLINES_H

#include <akplugin.h>

class ScanLinesPrivate;

class ScanLines:
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
    Q_PROPERTY(int showSize
               READ showSize
               WRITE setShowSize
               RESET resetShowSize
               NOTIFY showSizeChanged)
    Q_PROPERTY(int hideSize
               READ hideSize
               WRITE setHideSize
               RESET resetHideSize
               NOTIFY hideSizeChanged)
    Q_PROPERTY(QRgb hideColor
               READ hideColor
               WRITE setHideColor
               RESET resetHideColor
               NOTIFY hideColorChanged)

    public:
        explicit ScanLines(QObject *parent=nullptr);
        ~ScanLines();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE int showSize() const;
        Q_INVOKABLE int hideSize() const;
        Q_INVOKABLE QRgb hideColor() const;

    private:
        ScanLinesPrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void showSizeChanged(int showSize);
        void hideSizeChanged(int hideSize);
        void hideColorChanged(QRgb hideColor);

    public slots:
        void setShowSize(int showSize);
        void setHideSize(int hideSize);
        void setHideColor(QRgb hideColor);
        void resetShowSize();
        void resetHideSize();
        void resetHideColor();
};

#endif // SCANLINES_H
