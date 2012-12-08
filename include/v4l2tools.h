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

#ifndef V4L2TOOLS_H
#define V4L2TOOLS_H

#include <QtGui>
#include <QGst/Bin>
#include <QGst/Pipeline>

#include "commons.h"
#include "appenvironment.h"

class COMMONSSHARED_EXPORT V4L2Tools: public QWidget
{
    Q_OBJECT

    public:
        enum StreamType
        {
            StreamTypeUnknown,
            StreamTypeWebcam,
            StreamTypeURI,
            StreamTypeDesktop
        };

        explicit V4L2Tools(bool watchDevices=false, QObject *parent=NULL);
        ~V4L2Tools();

        Q_INVOKABLE QString fcc2s(uint val=0);
        Q_INVOKABLE QVariantList videoFormats(QString dev_name="/dev/video0");
        Q_INVOKABLE QVariantList currentVideoFormat(QString dev_name="/dev/video0");
        Q_INVOKABLE bool setVideoFormat(QString dev_name="/dev/video0", QVariantList videoFormat=QVariantList());
        Q_INVOKABLE QVariantList captureDevices();
        Q_INVOKABLE QVariantList listControls(QString dev_name="/dev/video0");
        Q_INVOKABLE bool setControls(QString dev_name="/dev/video0", QMap<QString, uint> controls=QMap<QString, uint>());
        Q_INVOKABLE QVariantMap featuresMatrix();
        Q_INVOKABLE QMap<QString, QString> availableEffects();
        Q_INVOKABLE QStringList currentEffects();
        Q_INVOKABLE QStringList bestVideoRecordFormat(QString fileName="");

    private:
        AppEnvironment *appEnvironment;
        bool playing;
        bool recordAudio;
        bool recording;
        QFileSystemWatcher *fsWatcher;
        QGst::BinPtr captureDevice;
        QGst::BinPtr effectsBin;
        QGst::BinPtr effectsPreviewBin;
        QGst::BinPtr mainBin;
        QGst::PipelinePtr mainPipeline;
        QMutex mutex;
        QString curDevName;
        QString curOutVidFmt;
        QStringList effects;
        QVariantList streams;
        QVariantList videoRecordFormats;
        QVariantList webcams;

        QVariantList queryControl(int dev_fd, struct v4l2_queryctrl *queryctrl);
        QMap<QString, uint> findControls(int dev_fd);
        QString hashFromName(QString name="");
        QString nameFromHash(QString hash="");
        StreamType deviceType(QString dev_name="/dev/video0");

    signals:
        void devicesModified();
        void playingStateChanged(bool playing);
        void recordingStateChanged(bool recording);
        void gstError();
        void frameReady(const QImage &frame);
        void previewFrameReady(const QImage &frame, QString effectName);

    public slots:
        void loadConfigs();
        void saveConfigs();
        void setEffects(QStringList effects=QStringList());
        void startEffectsPreview();
        void stopEffectsPreview();
        void startDevice(QString dev_name="/dev/video0", QVariantList forcedFormat=QVariantList());
        void stopCurrentDevice();
        void startVideoRecord(QString fileName="");
        void stopVideoRecord();
        void clearVideoRecordFormats();
        void clearCustomStreams();
        void setCustomStream(QString dev_name="", QString description="");
        void enableAudioRecording(bool enable);
        void setVideoRecordFormat(QString suffix="", QString videoEncoder="",
                                  QString audioEncoder="", QString muxer="");

    private slots:
        void aboutToQuit();
        void busMessage(const QGst::MessagePtr &message);
        void reset(QString dev_name="/dev/video0");
        void readFrame(QGst::ElementPtr sink);
};

#endif // V4L2TOOLS_H
