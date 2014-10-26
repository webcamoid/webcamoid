/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MEDIATOOLS_H
#define MEDIATOOLS_H

#include <QSize>
#include <QMutex>
#include <qb.h>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>

class MediaTools: public QObject
{
    Q_OBJECT
    Q_ENUMS(RecordFrom)

    Q_PROPERTY(QString curStream
               READ curStream
               WRITE setCurStream
               RESET resetCurStream
               NOTIFY curStreamChanged)

    Q_PROPERTY(QStringList streams
               READ streams
               RESET resetStreams
               NOTIFY streamsChanged)

    Q_PROPERTY(bool playAudioFromSource
               READ playAudioFromSource
               WRITE setPlayAudioFromSource
               RESET resetPlayAudioFromSource)

    Q_PROPERTY(RecordFrom recordAudioFrom
               READ recordAudioFrom
               WRITE setRecordAudioFrom
               RESET resetRecordAudioFrom)

    Q_PROPERTY(bool recording
               READ recording
               WRITE setRecording
               RESET resetRecording)

    Q_PROPERTY(QList<QStringList> videoRecordFormats
               READ videoRecordFormats
               WRITE setVideoRecordFormats
               RESET resetVideoRecordFormats)

    Q_PROPERTY(QSize windowSize
               READ windowSize
               WRITE setWindowSize
               RESET resetWindowSize)

    public:
        enum RecordFrom
        {
            RecordFromNone,
            RecordFromSource,
            RecordFromMic
        };

        explicit MediaTools(QObject *parent=NULL);
        ~MediaTools();

        Q_INVOKABLE QString curStream() const;
        Q_INVOKABLE QSize videoSize(const QString &stream);
        Q_INVOKABLE bool playAudioFromSource() const;
        Q_INVOKABLE RecordFrom recordAudioFrom() const;
        Q_INVOKABLE bool recording() const;
        Q_INVOKABLE QList<QStringList> videoRecordFormats() const;
        Q_INVOKABLE QStringList streams() const;
        Q_INVOKABLE QSize windowSize() const;
        Q_INVOKABLE QString applicationName() const;
        Q_INVOKABLE QString applicationVersion() const;
        Q_INVOKABLE QString qtVersion() const;
        Q_INVOKABLE QString copyrightNotice() const;
        Q_INVOKABLE QString projectUrl() const;
        Q_INVOKABLE QString projectLicenseUrl() const;

        Q_INVOKABLE QString streamDescription(const QString &stream) const;
        Q_INVOKABLE bool canModify(const QString &stream) const;
        Q_INVOKABLE bool isCamera(const QString &stream) const;
        Q_INVOKABLE QVariantList videoSizes(const QString &stream);
        Q_INVOKABLE QVariantList listImageControls(const QString &stream);
        Q_INVOKABLE void setImageControls(const QString &stream, const QVariantMap &controls);
        Q_INVOKABLE QMap<QString, QString> availableEffects() const;
        Q_INVOKABLE QStringList currentEffects() const;
        Q_INVOKABLE QString bestRecordFormatOptions(const QString &fileName="") const;
        Q_INVOKABLE bool isPlaying();
        Q_INVOKABLE QString fileNameFromUri(const QString &uri) const;
        Q_INVOKABLE bool embedCameraControls(const QString &where,
                                             const QString &stream,
                                             const QString &name="") const;
        Q_INVOKABLE void removeCameraControls(const QString &where) const;

    private:
        QString m_curStream;
        QMap<QString, QString> m_streams;
        bool m_playAudioFromSource;
        RecordFrom m_recordAudioFrom;
        bool m_recording;
        QList<QStringList> m_videoRecordFormats;
        QSize m_windowSize;
        QQmlApplicationEngine *m_appEngine;

        QbElementPtr m_pipeline;
        QbElementPtr m_source;
        QbElementPtr m_effects;
        QbElementPtr m_effectsPreview;
        QbElementPtr m_applyPreview;
        QbElementPtr m_audioSwitch;
        QbElementPtr m_audioOutput;
        QbElementPtr m_mic;
        QbElementPtr m_record;
        QbElementPtr m_videoCapture;
        QbElementPtr m_videoSync;
        QbElementPtr m_videoConvert;
        QStringList m_effectsList;
        QMutex m_mutex;

        bool embedInterface(QQmlApplicationEngine *engine,
                            QObject *interface,
                            const QString &where) const;

        void removeInterface(QQmlApplicationEngine *engine,
                             const QString &where) const;


    signals:
        void curStreamChanged();
        void streamsChanged();
        void stateChanged();
        void recordingChanged(bool recording);
        void frameReady(const QbPacket &frame);
        void effectPreviewReady(const QbPacket &frame);
        void applyPreviewReady(const QbPacket &frame);
        void error(const QString &message);
        void interfaceLoaded();

    public slots:
        void mutexLock();
        void mutexUnlock();
        bool start();
        void stop();
        bool startStream();
        void stopStream();
        void setCurStream(const QString &stream);
        void setVideoSize(const QString &stream, const QSize &size);
        void setEffectsPreview(const QString &effect);
        void setPlayAudioFromSource(bool playAudioFromSource);
        void setRecordAudioFrom(RecordFrom recordAudioFrom);
        void setRecording(bool recording, QString fileName="");
        void setVideoRecordFormats(QList<QStringList> videoRecordFormats);
        void setWindowSize(const QSize &windowSize);
        void setAppEngine(QQmlApplicationEngine *engine);
        void resetCurStream();
        void resetVideoSize(const QString &stream);
        void resetEffectsPreview();
        void resetPlayAudioFromSource();
        void resetRecordAudioFrom();
        void resetRecording();
        void resetVideoRecordFormats();
        void resetWindowSize();

        void connectPreview(bool link);
        void reset(const QString &stream);
        void loadConfigs();
        void saveConfigs();
        void setEffects(const QStringList &effects=QStringList());
        void clearVideoRecordFormats();
        void setStream(const QString &stream, const QString &description);
        void removeStream(const QString &stream);
        void resetStreams();
        void setVideoRecordFormat(QString suffix="", QString options="");
        void cleanAll();

    private slots:
        void iStream(const QbPacket &packet);
        void webcamsChanged(const QStringList &webcams);
};

#endif // MEDIATOOLS_H
