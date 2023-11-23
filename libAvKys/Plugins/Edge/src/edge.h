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

#ifndef EDGE_H
#define EDGE_H

#include <akplugin.h>

class EdgePrivate;

class Edge:
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
    Q_PROPERTY(bool canny
               READ canny
               WRITE setCanny
               RESET resetCanny
               NOTIFY cannyChanged)
    Q_PROPERTY(int thLow
               READ thLow
               WRITE setThLow
               RESET resetThLow
               NOTIFY thLowChanged)
    Q_PROPERTY(int thHi
               READ thHi
               WRITE setThHi
               RESET resetThHi
               NOTIFY thHiChanged)
    Q_PROPERTY(bool equalize
               READ equalize
               WRITE setEqualize
               RESET resetEqualize
               NOTIFY equalizeChanged)
    Q_PROPERTY(bool invert
               READ invert
               WRITE setInvert
               RESET resetInvert
               NOTIFY invertChanged)

    public:
        explicit Edge(QObject *parent=nullptr);
        ~Edge();

        Q_INVOKABLE QString description() const override;
        Q_INVOKABLE AkElementType type() const override;
        Q_INVOKABLE AkElementCategory category() const override;
        Q_INVOKABLE void *queryInterface(const QString &interfaceId) override;
        Q_INVOKABLE IAkElement *create(const QString &id={}) override;
        Q_INVOKABLE int registerElements(const QStringList &args={}) override;
        Q_INVOKABLE bool canny() const;
        Q_INVOKABLE int thLow() const;
        Q_INVOKABLE int thHi() const;
        Q_INVOKABLE bool equalize() const;
        Q_INVOKABLE bool invert() const;

    private:
        EdgePrivate *d;

    protected:
        void deleteThis(void *userData) const override;
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void cannyChanged(bool canny);
        void thLowChanged(int thLow);
        void thHiChanged(int thHi);
        void equalizeChanged(bool equalize);
        void invertChanged(bool invert);

    public slots:
        void setCanny(bool canny);
        void setThLow(int thLow);
        void setThHi(int thHi);
        void setEqualize(bool equalize);
        void setInvert(bool invert);
        void resetCanny();
        void resetThLow();
        void resetThHi();
        void resetEqualize();
        void resetInvert();
};

#endif // EDGE_H
