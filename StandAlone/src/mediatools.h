/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef MEDIATOOLS_H
#define MEDIATOOLS_H

#include <QSize>
#include <QMutex>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <ak.h>
#include <akutils.h>

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
               RESET resetPlayAudioFromSource
               NOTIFY playAudioFromSourceChanged)
    Q_PROPERTY(QString recordAudioFrom
               READ recordAudioFrom
               WRITE setRecordAudioFrom
               RESET resetRecordAudioFrom
               NOTIFY recordAudioFromChanged)
    Q_PROPERTY(QString curRecordingFormat
               READ curRecordingFormat
               WRITE setCurRecordingFormat
               RESET resetCurRecordingFormat
               NOTIFY curRecordingFormatChanged)
    Q_PROPERTY(bool recording
               READ recording
               WRITE setRecording
               RESET resetRecording
               NOTIFY recordingChanged)
    Q_PROPERTY(QStringList recordingFormats
               READ recordingFormats)
    Q_PROPERTY(int windowWidth
               READ windowWidth
               WRITE setWindowWidth
               RESET resetWindowWidth
               NOTIFY windowWidthChanged)
    Q_PROPERTY(int windowHeight
               READ windowHeight
               WRITE setWindowHeight
               RESET resetWindowHeight
               NOTIFY windowHeightChanged)
    Q_PROPERTY(bool advancedMode
               READ advancedMode
               WRITE setAdvancedMode
               RESET resetAdvancedMode
               NOTIFY advancedModeChanged)
    Q_PROPERTY(QStringList currentEffects
               READ currentEffects
               NOTIFY currentEffectsChanged)
    Q_PROPERTY(bool isPlaying
               READ isPlaying
               NOTIFY isPlayingChanged)

    public:
        enum RecordFrom
        {
            RecordFromNone,
            RecordFromSource,
            RecordFromMic
        };

        explicit MediaTools(QQmlApplicationEngine *engine=NULL, QObject *parent=NULL);
        ~MediaTools();

        Q_INVOKABLE QString curStream() const;
        Q_INVOKABLE bool playAudioFromSource() const;
        Q_INVOKABLE QString recordAudioFrom() const;
        Q_INVOKABLE QString curRecordingFormat() const;
        Q_INVOKABLE bool recording() const;
        Q_INVOKABLE QStringList recordingFormats() const;
        Q_INVOKABLE QString recordingFormatDescription(const QString &formatId) const;
        Q_INVOKABLE QStringList recordingFormatSuffix(const QString &formatId) const;
        Q_INVOKABLE QStringList streams() const;
        Q_INVOKABLE int windowWidth() const;
        Q_INVOKABLE int windowHeight() const;
        Q_INVOKABLE bool advancedMode() const;
        Q_INVOKABLE QString applicationName() const;
        Q_INVOKABLE QString applicationVersion() const;
        Q_INVOKABLE QString qtVersion() const;
        Q_INVOKABLE QString copyrightNotice() const;
        Q_INVOKABLE QString projectUrl() const;
        Q_INVOKABLE QString projectLicenseUrl() const;
        Q_INVOKABLE QString streamDescription(const QString &stream) const;
        Q_INVOKABLE bool canModify(const QString &stream) const;
        Q_INVOKABLE bool isCamera(const QString &stream) const;
        Q_INVOKABLE bool isDesktop(const QString &stream) const;
        Q_INVOKABLE bool isVideo(const QString &stream) const;
        Q_INVOKABLE QStringList availableEffects() const;
        Q_INVOKABLE QVariantMap effectInfo(const QString &effectId) const;
        Q_INVOKABLE QString effectDescription(const QString &effectId) const;
        Q_INVOKABLE QStringList currentEffects() const;
        Q_INVOKABLE AkElementPtr appendEffect(const QString &effectId, bool preview=false);
        Q_INVOKABLE void removeEffect(const QString &effectId);
        Q_INVOKABLE void moveEffect(const QString &effectId, int index);
        Q_INVOKABLE void showPreview(const QString &effectId);
        Q_INVOKABLE void setAsPreview(const QString &effectId, bool preview=false);
        Q_INVOKABLE void removePreview(const QString &effectId="");
        Q_INVOKABLE bool isPlaying();
        Q_INVOKABLE QString fileNameFromUri(const QString &uri) const;
        Q_INVOKABLE bool matches(const QString &pattern, const QStringList &strings) const;
        Q_INVOKABLE QString currentTime() const;
        Q_INVOKABLE QStringList standardLocations(const QString &type) const;
        Q_INVOKABLE QString saveFileDialog(const QString &caption="",
                                           const QString &fileName="",
                                           const QString &directory="",
                                           const QString &suffix="",
                                           const QString &filters="") const;
        Q_INVOKABLE QString readFile(const QString &fileName) const;
        Q_INVOKABLE QString urlToLocalFile(const QUrl &url) const;
        Q_INVOKABLE bool embedEffectControls(const QString &where,
                                             const QString &effectId,
                                             const QString &name="") const;
        Q_INVOKABLE bool embedMediaControls(const QString &where,
                                            const QString &stream="",
                                            const QString &name="") const;
        Q_INVOKABLE bool embedRecordControls(const QString &where,
                                             const QString &format="",
                                             const QString &name="") const;
        Q_INVOKABLE void removeInterface(const QString &where,
                                         QQmlApplicationEngine *engine=NULL) const;

    private:
        QString m_curStream;
        QMap<QString, QString> m_streams;
        bool m_playAudioFromSource;
        RecordFrom m_recordAudioFrom;
        bool m_recording;
        int m_windowWidth;
        int m_windowHeight;
        bool m_advancedMode;
        QQmlApplicationEngine *m_appEngine;

        AkElementPtr m_pipeline;
        AkElementPtr m_source;
        AkElementPtr m_audioSwitch;
        AkElementPtr m_audioOutput;
        AkElementPtr m_mic;
        AkElementPtr m_record;
        AkElementPtr m_videoCapture;
        AkElementPtr m_desktopCapture;
        AkElementPtr m_videoMux;
        AkElementPtr m_videoOutput;
        AkElementPtr m_videoGen;
        AkElementPtr m_virtualCamera;
        QList<AkElementPtr> m_effectsList;
        QMutex m_mutex;
        AkPacket m_curPacket;
        QImage m_photo;

        bool embedInterface(QQmlApplicationEngine *engine,
                            QObject *interface,
                            const QString &where) const;
        static bool sortByDescription(const QString &pluginId1,
                                      const QString &pluginId2);

    signals:
        void curStreamChanged(const QString &curStream);
        void streamsChanged();
        void playAudioFromSourceChanged(bool playAudioFromSource);
        void recordAudioFromChanged(const QString &recordAudioFrom);
        void curRecordingFormatChanged(const QString &curRecordingFormat);
        void recordingChanged(bool recording);
        void windowWidthChanged(int windowWidth);
        void windowHeightChanged(int windowHeight);
        void stateChanged(AkElement::ElementState state);
        void advancedModeChanged(bool advancedMode);
        void currentEffectsChanged();
        void isPlayingChanged();
        void frameReady(const AkPacket &frame);
        void error(const QString &message);
        void interfaceLoaded();

    public slots:
        void mutexLock();
        void mutexUnlock();
        void takePhoto();
        void savePhoto(const QString &fileName);
        bool start();
        void stop();
        bool startRecording(const QString &fileName);
        void stopRecording();
        void setCurStream(const QString &stream);
        void setPlayAudioFromSource(bool playAudioFromSource);
        void setRecordAudioFrom(const QString &recordAudioFrom);
        void setCurRecordingFormat(const QString &curRecordingFormat);
        void setRecording(bool recording);
        void setWindowWidth(int windowWidth);
        void setWindowHeight(int windowHeight);
        void setAdvancedMode(bool advancedMode);
        void resetCurStream();
        void resetPlayAudioFromSource();
        void resetRecordAudioFrom();
        void resetCurRecordingFormat();
        void resetRecording();
        void resetWindowWidth();
        void resetWindowHeight();
        void resetAdvancedMode();
        void resetEffects();
        void loadConfigs();
        void saveConfigs();
        void setStream(const QString &stream, const QString &description);
        void removeStream(const QString &stream);
        void resetStreams();
        void cleanAll();

    private slots:
        void iStream(const AkPacket &packet);
        void webcamsChanged(const QStringList &webcams);
        void updateRecordingParams();
};

#endif // MEDIATOOLS_H
