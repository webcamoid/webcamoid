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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <KConfigDialog>

#include "ui_mainwindow.h"

#include "commons.h"
#include "appenvironment.h"
#include "effects.h"
#include "featuresinfo.h"
#include "generalconfig.h"
#include "streamsconfig.h"
#include "v4l2tools.h"
#include "videorecordconfig.h"
#include "webcamconfig.h"

class COMMONSSHARED_EXPORT MainWindow: public QWidget, public Ui::MainWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QObject *parent=NULL);

    private:
        AppEnvironment *appEnvironment;
        Effects *cfgEffects;
        FeaturesInfo *cfgFeaturesInfo;
        GeneralConfig *cfgGeneralConfig;
        QImage webcamFrame;
        StreamsConfig *cfgStreams;
        V4L2Tools *tools;
        VideoRecordConfig *cfgVideoFormats;
        WebcamConfig *cfgWebcamDialog;

        void addWebcamConfigDialog(KConfigDialog *configDialog);
        void addEffectsConfigDialog(KConfigDialog *configDialog);
        void addVideoFormatsConfigDialog(KConfigDialog *configDialog);
        void addStreamsConfigDialog(KConfigDialog *configDialog);
        void addGeneralConfigsDialog(KConfigDialog *configDialog);
        void addFeaturesInfoDialog(KConfigDialog *configDialog);
        void showConfigDialog(KConfigDialog *configDialog=NULL);
        QString saveFile(bool video=false);

    protected:
        void resizeEvent(QResizeEvent *event);
        void enterEvent(QEvent *event);
        void leaveEvent(QEvent *event);
        void closeEvent(QCloseEvent *event);

    public slots:
        void showFrame(const QImage &webcamFrame);

    private slots:
        void updateWebcams();
        void playingStateChanged(bool playing);
        void recordingStateChanged(bool recording);
        void saveConfigs();
        void showGstError();
        void stopEffectsPreview();

        void on_btnTakePhoto_clicked();
        void on_btnVideoRecord_clicked();
        void on_cbxSetWebcam_currentIndexChanged(int index);
        void on_btnStartStop_clicked();
        void on_btnConfigure_clicked();
        void on_btnAbout_clicked();
};

#endif // MAINWINDOW_H
