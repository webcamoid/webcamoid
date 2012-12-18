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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtGui>
#include <KConfigDialog>

#include "commons.h"
#include "appenvironment.h"
#include "effects.h"
#include "featuresinfo.h"
#include "generalconfig.h"
#include "streamsconfig.h"
#include "mediatools.h"
#include "videorecordconfig.h"
#include "webcamconfig.h"

namespace Ui
{
    class MainWidget;
}

class COMMONSSHARED_EXPORT MainWidget: public QWidget
{
    Q_OBJECT

    public:
        explicit MainWidget(QWidget *parentWidget=NULL, QObject *parentObject=NULL);
        ~MainWidget();

    private:
        Ui::MainWidget *ui;

        AppEnvironment *m_appEnvironment;
        Effects *m_cfgEffects;
        FeaturesInfo *m_cfgFeaturesInfo;
        GeneralConfig *m_cfgGeneralConfig;
        QImage m_webcamFrame;
        StreamsConfig *m_cfgStreams;
        MediaTools *m_mediaTools;
        VideoRecordConfig *m_cfgVideoFormats;
        WebcamConfig *m_cfgWebcamDialog;

        void showConfigDialog(KConfigDialog *configDialog=NULL);
        QString saveFile(bool video=false);

    protected:
        void resizeEvent(QResizeEvent *event);
        void enterEvent(QEvent *event);
        void leaveEvent(QEvent *event);
        void closeEvent(QCloseEvent *event);

    public slots:
        void addWebcamConfigDialog(KConfigDialog *configDialog);
        void addEffectsConfigDialog(KConfigDialog *configDialog);
        void addVideoFormatsConfigDialog(KConfigDialog *configDialog);
        void addStreamsConfigDialog(KConfigDialog *configDialog);
        void addGeneralConfigsDialog(KConfigDialog *configDialog);
        void addFeaturesInfoDialog(KConfigDialog *configDialog);
        void showFrame(const QImage &webcamFrame);

    private slots:
        void updateWebcams();
        void deviceChanged(QString device);
        void recordingChanged(bool recording);
        void saveConfigs();
        void showGstError();
        void updateContents(QSize pixmapSize=QSize());

        void on_btnTakePhoto_clicked();
        void on_btnVideoRecord_clicked();
        void on_cbxSetWebcam_currentIndexChanged(int index);
        void on_btnStartStop_clicked();
        void on_btnConfigure_clicked();
        void on_btnAbout_clicked();
};

#endif // MAINWIDGET_H
