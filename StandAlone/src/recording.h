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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef RECORDING_H
#define RECORDING_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlApplicationEngine>
#include <QImage>
#include <akelement.h>

class Recording;

typedef QSharedPointer<Recording> RecordingPtr;

class Recording: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableFormats
               READ availableFormats
               NOTIFY availableFormatsChanged)
    Q_PROPERTY(QString format
               READ format
               WRITE setFormat
               RESET resetFormat
               NOTIFY formatChanged)
    Q_PROPERTY(AkCaps audioCaps
               READ audioCaps
               WRITE setAudioCaps
               RESET resetAudioCaps
               NOTIFY audioCapsChanged)
    Q_PROPERTY(AkCaps videoCaps
               READ videoCaps
               WRITE setVideoCaps
               RESET resetVideoCaps
               NOTIFY videoCapsChanged)
    Q_PROPERTY(bool recordAudio
               READ recordAudio
               WRITE setRecordAudio
               RESET resetRecordAudio
               NOTIFY recordAudioChanged)
    Q_PROPERTY(QString videoFileName
               READ videoFileName
               WRITE setVideoFileName
               RESET resetVideoFileName
               NOTIFY videoFileNameChanged)
    Q_PROPERTY(AkElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        explicit Recording(QQmlApplicationEngine *engine=NULL,
                           QObject *parent=NULL);
        ~Recording();

        Q_INVOKABLE QStringList availableFormats() const;
        Q_INVOKABLE QString format() const;
        Q_INVOKABLE AkCaps audioCaps() const;
        Q_INVOKABLE AkCaps videoCaps() const;
        Q_INVOKABLE bool recordAudio() const;
        Q_INVOKABLE QString videoFileName() const;
        Q_INVOKABLE AkElement::ElementState state() const;
        Q_INVOKABLE QString formatDescription(const QString &formatId) const;
        Q_INVOKABLE QStringList formatSuffix(const QString &formatId) const;
        Q_INVOKABLE bool embedControls(const QString &where,
                                       const QString &format="",
                                       const QString &name="");
        Q_INVOKABLE void removeInterface(const QString &where);

    private:
        QQmlApplicationEngine *m_engine;
        QStringList m_availableFormats;
        QString m_format;
        AkCaps m_audioCaps;
        AkCaps m_videoCaps;
        bool m_recordAudio;
        QString m_videoFileName;
        AkElement::ElementState m_state;
        AkElementPtr m_pipeline;
        AkElementPtr m_videoGen;
        AkElementPtr m_record;
        QMutex m_mutex;
        AkPacket m_curPacket;
        QImage m_photo;

        QStringList recordingFormats() const;

    signals:
        void availableFormatsChanged(const QStringList &availableFormats);
        void formatChanged(const QString &format);
        void audioCapsChanged(const AkCaps &audioCaps);
        void videoCapsChanged(const AkCaps &videoCaps);
        void recordAudioChanged(bool recordAudio);
        void videoFileNameChanged(const QString &videoFileName);
        void stateChanged(AkElement::ElementState state);

    public slots:
        void setFormat(const QString &format);
        void setAudioCaps(const AkCaps &audioCaps);
        void setVideoCaps(const AkCaps &videoCaps);
        void setRecordAudio(bool recordAudio);
        void setVideoFileName(const QString &videoFileName);
        void setState(AkElement::ElementState state);
        void resetFormat();
        void resetAudioCaps();
        void resetVideoCaps();
        void resetRecordAudio();
        void resetVideoFileName();
        void resetState();
        void takePhoto();
        void savePhoto(const QString &fileName);
        AkPacket iStream(const AkPacket &packet);
        void setQmlEngine(QQmlApplicationEngine *engine=NULL);

    private slots:
        void supportedFormatsUpdated(const QStringList &availableFormats);
        void userControlsUpdated(const QVariantMap &userControls);
        void capsUpdated();
        void updateFormat(const QString &codecLib);
        void loadProperties();
        void saveFormat(const QString &format);
        void saveRecordAudio(bool recordAudio);
        void saveMultiSinkCodecLib(const QString &codecLib);
        void saveProperties();
};

#endif // RECORDING_H
