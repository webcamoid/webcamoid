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

#ifndef SWIRLELEMENT_H
#define SWIRLELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <QMutex>
#include <ak.h>
#include <akmultimediasourceelement.h>

class SyphonIOElementPrivate;

class SyphonIOElement: public AkMultimediaSourceElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop
               NOTIFY loopChanged)
    Q_PROPERTY(QString description
               READ description
               WRITE setDescription
               RESET resetDescription
               NOTIFY descriptionChanged)
    Q_PROPERTY(bool isOutput
               READ isOutput
               WRITE setAsOutput
               RESET resetAsInput
               NOTIFY isOutputChanged)

    public:
        explicit SyphonIOElement();
        ~SyphonIOElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QStringList medias();
        Q_INVOKABLE QString media() const;
        Q_INVOKABLE QList<int> streams() const;
        Q_INVOKABLE bool isOutput() const;
        Q_INVOKABLE int defaultStream(const QString &mimeType);
        Q_INVOKABLE QString description(const QString &media="");
        Q_INVOKABLE AkCaps caps(int stream);

        void updateServers();
        void frameReady(const QImage &frame);

    private:
        QString m_media;
        bool m_isOutput;
        SyphonIOElementPrivate *d;
        QString m_description;
        QMap<QString, QString> m_servers;
        QMutex m_mutex;
        qint64 m_id;
        AkFrac m_fps;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);
        void descriptionChanged(const QString &description);
        void error(const QString &message);
        void isOutputChanged(bool isOutput);

    public slots:
        void setMedia(const QString &media);
        void setDescription(const QString &description);
        void setAsOutput(bool isOutput);
        void resetMedia();
        void resetDescription();
        void resetAsInput();
        bool setState(AkElement::ElementState state);
        AkPacket iStream(const AkPacket &packet);

    private slots:
        void isServerChanged(bool isOutput);
};

#endif // SWIRLELEMENT_H
