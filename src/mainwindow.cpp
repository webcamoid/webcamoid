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

#include "ui_mainwindow.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->ui->setupUi(this);

    this->m_imageDispay = new ImageDisplay(this);

    this->ui->wdgControls->setParent(NULL);
    this->ui->wdgControls->setParent(this);

    this->setWindowTitle(QString("%1 %2").
                         arg(QCoreApplication::applicationName()).
                         arg(QCoreApplication::applicationVersion()));

    this->m_mediaTools = new MediaTools(NULL, this);

    this->resize(QSize(this->m_mediaTools->windowWidth(),
                       this->m_mediaTools->windowHeight()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(streamsChanged()),
                     this,
                     SLOT(updateWebcams()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(stateChanged()),
                     this,
                     SLOT(stateChanged()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(recordingChanged(bool)),
                     this,
                     SLOT(recordingChanged(bool)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(error(const QString &)),
                     this,
                     SLOT(showError(const QString &)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(frameReady(const QbPacket &)),
                     this,
                     SLOT(showFrame(const QbPacket &)));

    this->ui->wdgControls->hide();

    foreach (QString stream, this->m_mediaTools->streams())
        this->ui->cbxSetWebcam->addItem(this->m_mediaTools->streamDescription(stream));
}

void MainWindow::showConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog) {
        configDialog = new ConfigDialog(this);

        configDialog->setWindowTitle(this->tr("%1 Settings").arg(QCoreApplication::
                                            applicationName()));
    }

    this->addWebcamConfigDialog(configDialog);
    this->addVideoFormatsConfigDialog(configDialog);
    this->addStreamsConfigDialog(configDialog);
    this->addGeneralConfigsDialog(configDialog);

    QObject::connect(configDialog,
                     SIGNAL(okClicked()),
                     this->m_mediaTools,
                     SLOT(saveConfigs()));

    QObject::connect(configDialog,
                     SIGNAL(cancelClicked()),
                     this->m_mediaTools,
                     SLOT(saveConfigs()));

    configDialog->exec();
}

QString MainWindow::saveFile(bool video)
{
    QString curTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString defaultSuffix;
    QString filters;
    QString defaultFileName;

    if (video) {
        QString videosPath(".");
        QStringList videosPaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);

        if (!videosPaths.isEmpty())
            videosPath = videosPaths[0];

        QList<QStringList> videoRecordFormats = this->m_mediaTools->videoRecordFormats();

        QStringList filtersList;
        bool fst = true;

        foreach (QStringList format, videoRecordFormats)
            foreach (QString s, format.at(0).split(",", QString::SkipEmptyParts)) {
                s = s.trimmed();
                filtersList << QString("%1 file (*.%2)").arg(s.toUpper()).arg(s.toLower());

                if (fst) {
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
    else {
        QString picturesPath(".");
        QStringList picturesPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

        if (!picturesPaths.isEmpty())
            picturesPath = picturesPaths[0];

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

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);

    if (event->type() == QEvent::LanguageChange)
        this->ui->retranslateUi(this);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    this->m_mediaTools->setWindowWidth(event->size().width());
    this->m_mediaTools->setWindowHeight(event->size().height());
    this->updateContents();
}

void MainWindow::enterEvent(QEvent *event)
{
    QMainWindow::enterEvent(event);

    this->ui->wdgControls->show();
}

void MainWindow::leaveEvent(QEvent *event)
{
    QMainWindow::leaveEvent(event);

    if (!this->rect().contains(this->mapFromGlobal(QCursor::pos())))
        this->ui->wdgControls->hide();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);

    this->m_mediaTools->cleanAll();
}

void MainWindow::addWebcamConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgWebcamDialog = new CameraConfig(this->m_mediaTools);

    configDialog->addPage(this->m_cfgWebcamDialog,
                          this->tr("Webcam Settings"),
                          QIcon::fromTheme("camera-web"),
                          this->tr("Configure the parameters of the webcam."));
}

void MainWindow::addVideoFormatsConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgVideoFormats = new VideoRecordConfig(this->m_mediaTools);

    configDialog->
           addPage(this->m_cfgVideoFormats,
                   this->tr("Configure Video Recording Formats"),
                   QIcon::fromTheme("video-x-generic"),
                   this->tr("Add or remove video formats for recording."));
}

void MainWindow::addStreamsConfigDialog(ConfigDialog *configDialog)
{
    this->m_cfgStreams = new StreamsConfig(this->m_mediaTools);

    configDialog->
           addPage(this->m_cfgStreams,
                   this->tr("Configure Custom Streams"),
                   QIcon::fromTheme("network-workgroup"),
                   this->tr("Add or remove local or network live streams."));
}

void MainWindow::addGeneralConfigsDialog(ConfigDialog *configDialog)
{
    this->m_cfgGeneralConfig = new GeneralConfig(this->m_mediaTools);

    configDialog->
           addPage(this->m_cfgGeneralConfig,
                   this->tr("General Options"),
                   QIcon::fromTheme("configure"),
                   this->tr("Setup the basic capture options."));
}

void MainWindow::showFrame(const QbPacket &webcamFrame)
{
    if (!this->m_mediaTools->curStream().isEmpty()) {
        this->m_webcamFrame = webcamFrame;

        this->m_imageDispay->setImage(this->m_webcamFrame);
    }
}

void MainWindow::updateWebcams()
{
    QString oldDevice = this->m_mediaTools->curStream();
    bool timerIsActive = !this->m_mediaTools->curStream().isEmpty();
    this->m_mediaTools->resetCurStream();
    this->ui->cbxSetWebcam->clear();
    QStringList devices;

    foreach (QString stream, this->m_mediaTools->streams()) {
        devices << stream;
        QString description = this->m_mediaTools->streamDescription(stream);
        this->ui->cbxSetWebcam->addItem(description);
    }

    if (devices.contains(oldDevice) && timerIsActive)
        this->m_mediaTools->setCurStream(oldDevice);
}

void MainWindow::stateChanged()
{
    if (this->m_mediaTools->isPlaying()) {
        this->ui->btnTakePhoto->setEnabled(true);
        this->ui->btnVideoRecord->setEnabled(true);
        this->ui->btnStartStop->setIcon(QIcon::fromTheme("media-playback-stop"));
    }
    else {
        this->ui->btnTakePhoto->setEnabled(false);
        this->ui->btnVideoRecord->setEnabled(false);
        this->ui->btnStartStop->setIcon(QIcon::fromTheme("media-playback-start"));
        this->m_webcamFrame = QbPacket();
        this->m_imageDispay->setImage(this->m_webcamFrame);
    }
}

void MainWindow::recordingChanged(bool recording)
{
    if (recording)
        this->ui->btnVideoRecord->setIcon(QIcon::fromTheme("media-playback-stop"));
    else
        this->ui->btnVideoRecord->setIcon(QIcon::fromTheme("video-x-generic"));
}

void MainWindow::showError(const QString &message)
{
    QMessageBox messageBox(QMessageBox::Critical,
                           this->tr("An error has occurred"),
                           message,
                           QMessageBox::Ok,
                           this);

    messageBox.exec();
}

void MainWindow::updateContents()
{
    this->m_imageDispay->setGeometry(0, 0, this->width(), this->height());

    QRect geometry(0,
                   this->height() - this->ui->wdgControls->height(),
                   this->width(),
                   this->ui->wdgControls->height());

    this->ui->wdgControls->setGeometry(geometry);
}

void MainWindow::on_btnTakePhoto_clicked()
{
    this->m_mediaTools->mutexLock();

    QImage image = QbUtils::packetToImage(this->m_webcamFrame);
    QString fileName = this->saveFile();

    if (!fileName.isEmpty())
        image.save(fileName);

    this->m_mediaTools->mutexUnlock();
}

void MainWindow::on_btnVideoRecord_clicked()
{
    if (this->m_mediaTools->recording())
        this->m_mediaTools->resetRecording();
    else {
        this->m_mediaTools->mutexLock();
        QString fileName = this->saveFile(true);
        this->m_mediaTools->mutexUnlock();
        this->m_mediaTools->setRecording(true, fileName);
    }
}

void MainWindow::on_cbxSetWebcam_currentIndexChanged(int index)
{
    QStringList streams = this->m_mediaTools->streams();

    if (streams.isEmpty())
        return;

    index = qBound(0, index, streams.size() - 1);
    this->m_mediaTools->setCurStream(streams[index]);
}

void MainWindow::on_btnStartStop_clicked()
{
    if (this->m_mediaTools->isPlaying())
        this->m_mediaTools->stop();
    else
        this->m_mediaTools->start();
}

void MainWindow::on_btnConfigure_clicked()
{
    this->showConfigDialog();
}

void MainWindow::on_btnAbout_clicked()
{
    About aboutDialog(this);

    aboutDialog.setIcon(QIcon::fromTheme("camera-web"));
    aboutDialog.setAppName(QCoreApplication::applicationName());
    aboutDialog.setVersion(QCoreApplication::applicationVersion());
    aboutDialog.setShortDescription(this->tr("webcam capture application."));
    aboutDialog.setDescription(this->tr("A simple webcam application for picture and video capture."));
    aboutDialog.setCopyrightNotice(COMMONS_COPYRIGHT_NOTICE);
    aboutDialog.setWebsiteLink(COMMONS_PROJECT_URL);
    aboutDialog.setWebsiteLicense(COMMONS_PROJECT_LICENSE_URL);

    aboutDialog.exec();
}
