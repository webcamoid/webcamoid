/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MEDIATOOLS_H
#define MEDIATOOLS_H

#include <QtGui>
#include <qb.h>

#include "commons.h"
#include "appenvironment.h"

class COMMONSSHARED_EXPORT MediaTools: public QObject
{
    Q_OBJECT
    Q_ENUMS(StreamType)

    Q_PROPERTY(QString device READ device
                              WRITE setDevice
                              RESET resetDevice
                              NOTIFY deviceChanged)

    Q_PROPERTY(QVariantList videoFormat READ videoFormat
                                        WRITE setVideoFormat
                                        RESET resetVideoFormat)

    Q_PROPERTY(bool effectsPreview READ effectsPreview
                                   WRITE setEffectsPreview
                                   RESET resetEffectsPreview)

    Q_PROPERTY(bool recordAudio READ recordAudio
                                WRITE setRecordAudio
                                RESET resetRecordAudio)

    Q_PROPERTY(bool recording READ recording
                              WRITE setRecording
                              RESET resetRecording)

    Q_PROPERTY(QList<QStringList> videoRecordFormats READ videoRecordFormats
                                                     WRITE setVideoRecordFormats
                                                     RESET resetVideoRecordFormats)

    Q_PROPERTY(QVariantList streams READ streams
                                    WRITE setStreams
                                    RESET resetStreams)

    Q_PROPERTY(QSize windowSize READ windowSize
                                WRITE setWindowSize
                                RESET resetWindowSize)

    public:
        enum StreamType
        {
            StreamTypeUnknown,
            StreamTypeWebcam,
            StreamTypeURI,
            StreamTypeDesktop
        };

        explicit MediaTools(bool watchDevices=false, QObject *parent=NULL);
        ~MediaTools();

        Q_INVOKABLE QString device();
        Q_INVOKABLE QVariantList videoFormat(QString device="");
        Q_INVOKABLE bool effectsPreview();
        Q_INVOKABLE bool recordAudio();
        Q_INVOKABLE bool recording();
        Q_INVOKABLE QList<QStringList> videoRecordFormats();
        Q_INVOKABLE QVariantList streams();
        Q_INVOKABLE QSize windowSize();

        Q_INVOKABLE QString fcc2s(uint val=0);
        Q_INVOKABLE QVariantList videoFormats(QString device="/dev/video0");
        Q_INVOKABLE QVariantList captureDevices();
        Q_INVOKABLE QVariantList listControls(QString dev_name="/dev/video0");
        Q_INVOKABLE bool setControls(QString dev_name="/dev/video0", QMap<QString, uint> controls=QMap<QString, uint>());
        Q_INVOKABLE QMap<QString, QString> availableEffects();
        Q_INVOKABLE QStringList currentEffects();
        Q_INVOKABLE QString bestRecordFormatOptions(QString fileName="");

    private:
        QString m_device;
        QVariantList m_videoFormat;
        bool m_showEffectsPreview;
        bool m_recordAudio;
        bool m_recording;
        QList<QStringList> m_videoRecordFormats;
        QVariantList m_streams;
        QSize m_windowSize;

        AppEnvironment *m_appEnvironment;
        QFileSystemWatcher *m_fsWatcher;
        QbElementPtr m_pipeline;
        QbElementPtr m_source;
        QbElementPtr m_effects;
        QbElementPtr m_effectsPreview;
        QbElementPtr m_audioSwitch;
        QbElementPtr m_audioOutput;
        QbElementPtr m_mic;
        QbElementPtr m_record;
        QStringList m_effectsList;
        QVariantList m_webcams;
        QMap<QString, int> m_callBacks;
        QSize m_curFrameSize;
        QMutex m_mutex;

        QVariantList queryControl(int dev_fd, struct v4l2_queryctrl *queryctrl);
        QMap<QString, uint> findControls(int dev_fd);
        QString hashFromName(QString name="");
        QString nameFromHash(QString hash="");
        StreamType deviceType(QString device="/dev/video0");

    signals:
        void devicesModified();
        void frameSizeChanged(QSize size);
        void deviceChanged(QString device);
        void recordingChanged(bool recording);
        void frameReady(const QImage &frame);
        void previewFrameReady(const QImage &frame, QString effectName);
        void gstError();

    public slots:
        void mutexLock();
        void mutexUnlock();
        void setDevice(QString device);
        void setVideoFormat(QVariantList videoFormat, QString device="");
        void setEffectsPreview(bool effectsPreview);
        void setRecordAudio(bool recordAudio);
        void setRecording(bool recording, QString fileName="");
        void setVideoRecordFormats(QList<QStringList> videoRecordFormats);
        void setStreams(QVariantList streams);
        void setWindowSize(QSize windowSize);
        void resetDevice();
        void resetVideoFormat(QString device="");
        void resetEffectsPreview();
        void resetRecordAudio();
        void resetRecording();
        void resetVideoRecordFormats();
        void resetStreams();
        void resetWindowSize();

        void reset(QString device="/dev/video0");
        void loadConfigs();
        void saveConfigs();
        void setEffects(QStringList effects=QStringList());
        void clearVideoRecordFormats();
        void clearCustomStreams();
        void setCustomStream(QString dev_name="",
                             QString description="",
                             bool hasAudio=false,
                             bool playAudio=false);
        void enableAudioRecording(bool enable);
        void setVideoRecordFormat(QString suffix="", QString options="");

    private slots:
        void iStream(const QbPacket &packet);

        void aboutToQuit();
        void onDirectoryChanged(const QString &path);
};

#endif // MEDIATOOLS_H
