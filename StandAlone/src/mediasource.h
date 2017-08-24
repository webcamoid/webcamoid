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

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <akelement.h>

class MediaSource;

typedef QSharedPointer<MediaSource> MediaSourcePtr;

class MediaSource: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString stream
               READ stream
               WRITE setStream
               RESET resetStream
               NOTIFY streamChanged)
    Q_PROPERTY(QStringList streams
               READ streams
               NOTIFY streamsChanged)
    Q_PROPERTY(QStringList cameras
               READ cameras
               NOTIFY camerasChanged)
    Q_PROPERTY(QStringList syphonServers
               READ syphonServers
               NOTIFY syphonServersChanged)
    Q_PROPERTY(QStringList desktops
               READ desktops
               NOTIFY desktopsChanged)
    Q_PROPERTY(QVariantMap uris
               READ uris
               WRITE setUris
               RESET resetUris
               NOTIFY urisChanged)
    Q_PROPERTY(AkCaps audioCaps
               READ audioCaps
               NOTIFY audioCapsChanged)
    Q_PROPERTY(AkCaps videoCaps
               READ videoCaps
               NOTIFY videoCapsChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)
    Q_PROPERTY(bool playOnStart
               READ playOnStart
               WRITE setPlayOnStart
               RESET resetPlayOnStart
               NOTIFY playOnStartChanged)

    public:
        explicit MediaSource(QQmlApplicationEngine *engine=NULL, QObject *parent=NULL);
        ~MediaSource();

        Q_INVOKABLE QString stream() const;
        Q_INVOKABLE QStringList streams() const;
        Q_INVOKABLE QStringList cameras() const;
        Q_INVOKABLE QStringList syphonServers() const;
        Q_INVOKABLE QStringList desktops() const;
        Q_INVOKABLE QVariantMap uris() const;
        Q_INVOKABLE AkCaps audioCaps() const;
        Q_INVOKABLE AkCaps videoCaps() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE bool playOnStart() const;
        Q_INVOKABLE QString description(const QString &stream) const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       const QString &stream,
                                       const QString &name="") const;
        Q_INVOKABLE void removeInterface(const QString &where) const;

    private:
        QQmlApplicationEngine *m_engine;
        QString m_stream;
        QStringList m_streams;
        QStringList m_cameras;
        QStringList m_desktops;
        QStringList m_syphonServers;
        QVariantMap m_uris;
        QMap<QString, QString> m_descriptions;
        AkCaps m_audioCaps;
        AkCaps m_videoCaps;
        AkElement::ElementState m_inputState;
        bool m_playOnStart;
        AkElementPtr m_pipeline;
        AkElementPtr m_cameraCapture;
        AkElementPtr m_desktopCapture;
        AkElementPtr m_uriCapture;
        AkElementPtr m_syphonCapture;

        AkElementPtr sourceElement(const QString &stream) const;

    signals:
        void streamChanged(const QString &stream);
        void streamsChanged(const QStringList &streams);
        void camerasChanged(const QStringList &cameras);
        void syphonServersChanged(const QStringList &servers);
        void desktopsChanged(const QStringList &desktops);
        void urisChanged(const QVariantMap &uris);
        void audioCapsChanged(const AkCaps &audioCaps);
        void videoCapsChanged(const AkCaps &videoCaps);
        void stateChanged(AkElement::ElementState state);
        void playOnStartChanged(bool playOnStart);
        void oStream(const AkPacket &packet);
        void error(const QString &message);

    public slots:
        void setStream(const QString &stream);
        void setUris(const QVariantMap &uris);
        void setState(AkElement::ElementState state);
        void setPlayOnStart(bool playOnStart);
        void resetStream();
        void resetUris();
        void resetState();
        void resetPlayOnStart();
        void setQmlEngine(QQmlApplicationEngine *engine=NULL);

    private slots:
        void streamUpdated(const QString &stream);
        void updateStreams();
        bool setStreams(const QStringList &streams);
        bool setCameras(const QStringList &cameras);
        bool setSyphonServers(const QStringList &servers);
        bool setDesktops(const QStringList &desktops);
        void setAudioCaps(const AkCaps &audioCaps);
        void setVideoCaps(const AkCaps &videoCaps);
        void loadProperties();
        void saveStream(const QString &stream);
        void saveUris(const QVariantMap &uris);
        void savePlayOnStart(bool playOnStart);
        void saveVideoCaptureCodecLib(const QString &codecLib);
        void saveVideoCaptureCaptureLib(const QString &captureLib);
        void saveDesktopCaptureCaptureLib(const QString &captureLib);
        void saveMultiSrcCodecLib(const QString &codecLib);
        void saveProperties();
};

#endif // MEDIASOURCE_H
