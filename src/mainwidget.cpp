/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#include "ui_mainwidget.h"

#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parentWidget, QObject *parentObject):
    QWidget(parentWidget),
    ui(new Ui::MainWidget)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);

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
                     SIGNAL(error(QString)),
                     this,
                     SLOT(showError(QString)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(frameReady(const QImage &)),
                     this,
                     SLOT(showFrame(const QImage &)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(frameSizeChanged(QSize)),
                     this,
                     SLOT(updateContents(QSize)));

    this->ui->wdgControls->hide();

    foreach (QStringList webcam, this->m_mediaTools->captureDevices())
        this->ui->cbxSetWebcam->addItem(webcam.at(1));
}

MainWidget::~MainWidget()
{
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

    configDialog->addPage(this->m_cfgEffects,
                          this->tr("Configure Webcam Effects"),
                          "tools-wizard",
                          this->tr("Add funny effects to the webcam"),
                          false);
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
        QList<QStringList> videoRecordFormats = this->m_mediaTools->videoRecordFormats();

        QStringList filtersList;
        bool fst = true;

        foreach (QStringList format, videoRecordFormats)
            foreach (QString s, format.at(0).split(",", QString::SkipEmptyParts))
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

    this->ui->wdgControls->show();
}

void MainWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    if (!this->rect().contains(this->mapFromGlobal(QCursor::pos())))
        this->ui->wdgControls->hide();
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    this->m_mediaTools->setWindowSize(this->size());
}

void MainWidget::showFrame(const QImage &webcamFrame)
{
    if (!this->m_mediaTools->device().isEmpty())
    {
        this->m_webcamFrame = webcamFrame;
        this->ui->lblFrame->setPixmap(QPixmap::fromImage(this->m_webcamFrame));
    }
}

void MainWidget::updateWebcams()
{
    QString oldDevice = this->m_mediaTools->device();
    bool timerIsActive = !this->m_mediaTools->device().isEmpty();
    this->m_mediaTools->resetDevice();
    this->ui->cbxSetWebcam->clear();
    QList<QStringList> webcams = this->m_mediaTools->captureDevices();
    QStringList devices;

    foreach (QStringList webcam, webcams)
    {
        devices << webcam.at(0);
        this->ui->cbxSetWebcam->addItem(webcam.at(1));
    }

    if (devices.contains(oldDevice) && timerIsActive)
        this->m_mediaTools->setDevice(oldDevice);
}

void MainWidget::deviceChanged(QString device)
{
    if (device.isEmpty())
    {
        this->ui->btnTakePhoto->setEnabled(false);
        this->ui->btnVideoRecord->setEnabled(false);
        this->ui->btnStartStop->setIcon(QIcon::fromTheme("media-playback-start"));
        this->m_webcamFrame = QImage();
        this->ui->lblFrame->setPixmap(QPixmap::fromImage(this->m_webcamFrame));
    }
    else
    {
        this->ui->btnTakePhoto->setEnabled(true);
        this->ui->btnVideoRecord->setEnabled(true);
        this->ui->btnStartStop->setIcon(QIcon::fromTheme("media-playback-stop"));
    }
}

void MainWidget::recordingChanged(bool recording)
{
    if (recording)
        this->ui->btnVideoRecord->setIcon(QIcon::fromTheme("media-playback-stop"));
    else
        this->ui->btnVideoRecord->setIcon(QIcon::fromTheme("video-x-generic"));
}

void MainWidget::saveConfigs()
{
    this->m_mediaTools->saveConfigs();
}

void MainWidget::showError(QString message)
{
    KNotification::event(KNotification::Error,
                         this->tr("An error has occurred"),
                         message,
                         QPixmap(),
                         NULL,
                         KNotification::Persistent);
}

void MainWidget::updateContents(QSize pixmapSize)
{
    QSize size = this->size();

    QSize curPixmapSize = pixmapSize.isValid()? pixmapSize: this->ui->lblFrame->pixmap()? this->ui->lblFrame->pixmap()->size(): QSize();

    curPixmapSize.scale(size, Qt::KeepAspectRatio);
    int x = (size.width() - curPixmapSize.width()) >> 1;
    int y = (size.height() - curPixmapSize.height()) >> 1;

    this->ui->lblFrame->setGeometry(x, y, curPixmapSize.width(), curPixmapSize.height());

    QRect geometry(0,
                   size.height() - this->ui->wdgControls->height(),
                   size.width(),
                   this->ui->wdgControls->height());

    this->ui->wdgControls->setGeometry(geometry);
}

void MainWidget::on_btnTakePhoto_clicked()
{
    QImage image(this->m_webcamFrame);

    this->m_mediaTools->mutexLock();

    QString fileName = this->saveFile();

    if (!fileName.isEmpty())
        image.save(fileName);

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
        this->m_mediaTools->setDevice(this->m_mediaTools->captureDevices().at(index).at(0));
}

void MainWidget::on_btnStartStop_clicked()
{
    if (!this->m_mediaTools->device().isEmpty())
        this->m_mediaTools->resetDevice();
    else
        this->m_mediaTools->setDevice(this->m_mediaTools->
                    captureDevices().at(this->ui->cbxSetWebcam->currentIndex()).at(0));
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
                   ki18n(QCoreApplication::applicationName().toStdString().c_str()),
                   QCoreApplication::applicationVersion().toUtf8().data(),
                   ki18n(this->tr("webcam capture plasmoid.").toStdString().c_str()),
                   KAboutData::License_GPL_V3,
                   ki18n(COMMONS_COPYRIGHT_NOTICE),
                   ki18n(this->tr("A simple webcam plasmoid and "
                                  "stand alone app for picture and "
                                  "video capture.").toStdString().c_str()),
                   COMMONS_PROJECT_URL,
                   COMMONS_PROJECT_BUG_URL);

    aboutData->setProgramIconName("camera-web");

    KAboutApplicationDialog aboutDialog(aboutData, this);
    aboutDialog.exec();
}
