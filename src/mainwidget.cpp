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

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KConfigSkeleton>
#include <KGlobalSettings>
#include <KLocalizedString>
#include <KNotification>

#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parentWidget, QObject *parentObject): QWidget(parentWidget)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    if (parentWidget || parentObject)
        this->setStyleSheet("QWidget#MainWidget{background-color: "
                            "rgba(0, 0, 0, 0);}");

    this->setWindowTitle(QString("%1 %2").
                         arg(QCoreApplication::applicationName()).
                         arg(QCoreApplication::applicationVersion()));

    this->m_mediaTools = new MediaTools(true, this);

    this->resize(this->m_mediaTools->windowSize());

    QObject::connect(this->m_mediaTools,
                     SIGNAL(devicesModified()),
                     this,
                     SLOT(updateWebcams()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(deviceChanged(QString)),
                     this,
                     SLOT(deviceChanged(QString)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(recordingChanged(bool)),
                     this,
                     SLOT(recordingChanged(bool)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(gstError()),
                     this,
                     SLOT(showGstError()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(frameReady(const QImage &)),
                     this,
                     SLOT(showFrame(const QImage &)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(frameSizeChanged(QSize)),
                     this,
                     SLOT(updateContents(QSize)));

    this->wdgControls->hide();

    foreach (QVariant webcam, this->m_mediaTools->captureDevices())
        this->cbxSetWebcam->addItem(webcam.toList().at(1).toString());
}

void MainWidget::addWebcamConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgWebcamDialog = new WebcamConfig(this->m_mediaTools, this);

    configDialog->addPage(this->m_cfgWebcamDialog,
                          this->tr("Webcam Settings"),
                          "camera-web",
                          this->tr("Configure the parameters of the webcam."),
                          false);
}

void MainWidget::addEffectsConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgEffects = new Effects(this->m_mediaTools, this);

    QObject::connect(this->m_mediaTools,
                     SIGNAL(previewFrameReady(const QImage &, QString)),
                     this->m_cfgEffects,
                     SLOT(setEffectPreview(const QImage &, QString)));

    configDialog->addPage(this->m_cfgEffects,
                          this->tr("Configure Webcam Effects"),
                          "tools-wizard",
                          this->tr("Add funny effects to the webcam"),
                          false);

    QObject::connect(configDialog,
                     SIGNAL(finished()),
                     this,
                     SLOT(stopEffectsPreview()));
}

void MainWidget::addVideoFormatsConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgVideoFormats = new VideoRecordConfig(this->m_mediaTools, this);

    configDialog->
           addPage(this->m_cfgVideoFormats,
                   this->tr("Configure Video Recording Formats"),
                   "video-x-generic",
                   this->tr("Add or remove video formats for recording."),
                   false);
}

void MainWidget::addStreamsConfigDialog(KConfigDialog *configDialog)
{
    this->m_cfgStreams = new StreamsConfig(this->m_mediaTools, this);

    configDialog->
           addPage(this->m_cfgStreams,
                   this->tr("Configure Custom Streams"),
                   "network-workgroup",
                   this->tr("Add or remove local or network live streams."),
                   false);
}

void MainWidget::addGeneralConfigsDialog(KConfigDialog *configDialog)
{
    this->m_cfgGeneralConfig = new GeneralConfig(this->m_mediaTools, this);

    configDialog->
           addPage(this->m_cfgGeneralConfig,
                   this->tr("General Options"),
                   "configure",
                   this->tr("Setup the basic capture options."),
                   false);
}

void MainWidget::addFeaturesInfoDialog(KConfigDialog *configDialog)
{
    this->m_cfgFeaturesInfo = new FeaturesInfo(this->m_mediaTools, this);

    configDialog->
           addPage(this->m_cfgFeaturesInfo,
                   this->tr("Features"),
                   "dialog-information",
                   this->tr("This table will show you what packages you need."),
                   false);
}

void MainWidget::showConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
    {
        KConfigSkeleton *config = new KConfigSkeleton("", this);

        configDialog = \
                new KConfigDialog(this,
                                  this->tr("%1 Settings").arg(QCoreApplication::
                                                applicationName()),
                                  config);

        configDialog->setWindowTitle(this->tr("%1 Settings").arg(QCoreApplication::
                                            applicationName()));
    }

    this->addWebcamConfigDialog(configDialog);
    this->addEffectsConfigDialog(configDialog);
    this->addVideoFormatsConfigDialog(configDialog);
    this->addStreamsConfigDialog(configDialog);
    this->addGeneralConfigsDialog(configDialog);
    this->addFeaturesInfoDialog(configDialog);

    QObject::connect(configDialog,
                     SIGNAL(okClicked()),
                     this,
                     SLOT(saveConfigs()));

    QObject::connect(configDialog,
                     SIGNAL(cancelClicked()),
                     this,
                     SLOT(saveConfigs()));

    configDialog->exec();
}

QString MainWidget::saveFile(bool video)
{
    QString curTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString defaultSuffix;
    QString filters;
    QString defaultFileName;

    if (video)
    {
        QString videosPath = KGlobalSettings::videosPath();
        QVariantList videoRecordFormats = this->m_mediaTools->videoRecordFormats();

        QStringList filtersList;
        bool fst = true;

        foreach (QVariant format, videoRecordFormats)
            foreach (QString s, format.toList().at(0).toString().split(",", QString::SkipEmptyParts))
            {
                s = s.trimmed();
                filtersList << QString("%1 file (*.%2)").arg(s.toUpper()).arg(s.toLower());

                if (fst)
                {
                    defaultSuffix = s.toLower();
                    fst = false;
                }
            }

        filters = filtersList.join(";;");

        defaultFileName = QDir::toNativeSeparators(QString("%1/Video %2.%3").
                                                   arg(videosPath).
                                                   arg(curTime).
                                                   arg(defaultSuffix));
    }
    else
    {
        QString picturesPath = KGlobalSettings::picturesPath();

        filters = "PNG file (*.png);;" \
                  "JPEG file (*.jpg);;" \
                  "BMP file (*.bmp);;" \
                  "GIF file (*.gif)";

        defaultSuffix = "png";

        defaultFileName = QDir::toNativeSeparators(QString("%1/Picture %2.png").
                                                   arg(picturesPath).
                                                   arg(curTime));
    }

    if (defaultSuffix.isEmpty())
        return "";

    QFileDialog saveFileDialog(NULL,
                               this->tr("Save File As..."),
                               defaultFileName,
                               filters);

    saveFileDialog.setModal(true);
    saveFileDialog.setDefaultSuffix(defaultSuffix);
    saveFileDialog.setFileMode(QFileDialog::AnyFile);
    saveFileDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveFileDialog.exec();

    QStringList selectedFiles = saveFileDialog.selectedFiles();

    return selectedFiles.isEmpty()? "": selectedFiles.at(0);
}

void MainWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    this->m_mediaTools->setWindowSize(event->size());

    this->updateContents();
}

void MainWidget::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);

    this->wdgControls->show();
}

void MainWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    if (!this->rect().contains(this->mapFromGlobal(QCursor::pos())))
        this->wdgControls->hide();
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    this->m_mediaTools->setWindowSize(this->size());
}

void MainWidget::showFrame(const QImage &webcamFrame)
{
    if (!this->m_mediaTools->device().isEmpty())
        this->lblFrame->setPixmap(QPixmap::fromImage(webcamFrame));
}

void MainWidget::updateWebcams()
{
    QString oldDevice = this->m_mediaTools->device();
    bool timerIsActive = !this->m_mediaTools->device().isEmpty();
    this->m_mediaTools->resetDevice();
    this->cbxSetWebcam->clear();
    QVariantList webcams = this->m_mediaTools->captureDevices();
    QStringList devices;

    foreach (QVariant webcam, webcams)
    {
        devices << webcam.toStringList().at(0);
        this->cbxSetWebcam->addItem(webcam.toStringList().at(1));
    }

    if (devices.contains(oldDevice) && timerIsActive)
        this->m_mediaTools->setDevice(oldDevice);
}

void MainWidget::deviceChanged(QString device)
{
    if (device.isEmpty())
    {
        this->btnTakePhoto->setEnabled(false);
        this->btnVideoRecord->setEnabled(false);
        this->btnStartStop->setIcon(QIcon::fromTheme("media-playback-start"));
        this->m_webcamFrame = QImage();
        this->lblFrame->setPixmap(QPixmap::fromImage(this->m_webcamFrame));
    }
    else
    {
        this->btnTakePhoto->setEnabled(true);
        this->btnVideoRecord->setEnabled(true);
        this->btnStartStop->setIcon(QIcon::fromTheme("media-playback-stop"));
    }
}

void MainWidget::recordingChanged(bool recording)
{
    if (recording)
        this->btnVideoRecord->setIcon(QIcon::fromTheme("media-playback-stop"));
    else
        this->btnVideoRecord->setIcon(QIcon::fromTheme("video-x-generic"));
}

void MainWidget::saveConfigs()
{
    this->m_mediaTools->saveConfigs();
}

void MainWidget::showGstError()
{
    KNotification::event(KNotification::Error,
                         this->tr("An error has occurred"),
                         this->tr("Please, check the \"Features\" section."),
                         QPixmap(),
                         NULL,
                         KNotification::Persistent);
}

void MainWidget::stopEffectsPreview()
{
    QObject::disconnect(this->m_mediaTools,
                        SIGNAL(previewFrameReady(const QImage &, QString)),
                        this->m_cfgEffects,
                        SLOT(setEffectPreview(const QImage &, QString)));
}

void MainWidget::updateContents(QSize pixmapSize)
{
    QSize size = this->size();

    QSize curPixmapSize = pixmapSize.isValid()? pixmapSize: this->lblFrame->pixmap()? this->lblFrame->pixmap()->size(): QSize();

    curPixmapSize.scale(size, Qt::KeepAspectRatio);
    int x = (size.width() - curPixmapSize.width()) >> 1;
    int y = (size.height() - curPixmapSize.height()) >> 1;

    this->lblFrame->setGeometry(x, y, curPixmapSize.width(), curPixmapSize.height());

    QRect geometry(0,
                   size.height() - this->wdgControls->height(),
                   size.width(),
                   this->wdgControls->height());

    this->wdgControls->setGeometry(geometry);
}

void MainWidget::on_btnTakePhoto_clicked()
{
    this->m_mediaTools->mutexLock();

    QImage image(this->m_webcamFrame);
    QString filename = this->saveFile();

    if (!filename.isEmpty())
        image.save(filename);

    this->m_mediaTools->mutexUnlock();
}

void MainWidget::on_btnVideoRecord_clicked()
{
    if (this->m_mediaTools->recording())
        this->m_mediaTools->resetRecording();
    else
    {
        this->m_mediaTools->mutexLock();
        QString fileName = this->saveFile(true);
        this->m_mediaTools->mutexUnlock();
        this->m_mediaTools->setRecording(true, fileName);
    }
}

void MainWidget::on_cbxSetWebcam_currentIndexChanged(int index)
{
    if (!this->m_mediaTools->device().isEmpty())
        this->m_mediaTools->setDevice(this->m_mediaTools->captureDevices().at(index).toStringList().at(0));
}

void MainWidget::on_btnStartStop_clicked()
{
    if (!this->m_mediaTools->device().isEmpty())
        this->m_mediaTools->resetDevice();
    else
        this->m_mediaTools->setDevice(this->m_mediaTools->
                    captureDevices().at(this->cbxSetWebcam->currentIndex()).toStringList().at(0));
}

void MainWidget::on_btnConfigure_clicked()
{
    this->showConfigDialog();
}

void MainWidget::on_btnAbout_clicked()
{
    KAboutData *aboutData = new \
        KAboutData(QCoreApplication::applicationName().toUtf8().data(),
                   QCoreApplication::applicationName().toUtf8().data(),
                   ki18n(QCoreApplication::applicationName().toUtf8().constData()),
                   QCoreApplication::applicationVersion().toUtf8().data(),
                   ki18n(this->tr("webcam capture plasmoid.").toUtf8().constData()),
                   KAboutData::License_GPL_V3,
                   ki18n("Copyright (C) 2011-2012  "
                         "Gonzalo Exequiel Pedone"),
                   ki18n(this->tr("A simple webcam plasmoid and "
                                  "stand alone app for picture and "
                                  "video capture.").toUtf8().constData()),
                   "http://github.com/hipersayanX/Webcamoid",
                   "submit@bugs.kde.org");

    aboutData->setProgramIconName("camera-web");

    KAboutApplicationDialog *aboutDialog = new KAboutApplicationDialog(aboutData, this);
    aboutDialog->exec();
}
