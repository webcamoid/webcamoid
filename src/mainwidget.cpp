/* Webcamoid, webcam capture application.
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

#include "ui_mainwidget.h"
#include "about.h"
#include "mainwidget.h"

MainWidget::MainWidget(QWidget *parentWidget, QObject *parentObject):
    QWidget(parentWidget),
    ui(new Ui::MainWidget)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);

    this->m_imageDispay = new ImageDisplay(this);

    QWidget *parent = qobject_cast<QWidget *>(this->ui->wdgControls->parent());
    this->ui->wdgControls->setParent(NULL);
    this->ui->wdgControls->setParent(parent);

    if (parentWidget || parentObject)
        this->setStyleSheet("QWidget#MainWidget{background-color: "
                            "rgba(0, 0, 0, 0);}");

    this->setWindowTitle(QString("%1 %2").
                         arg(QCoreApplication::applicationName()).
                         arg(QCoreApplication::applicationVersion()));

    this->m_mediaTools = new MediaTools(this);

    this->resize(this->m_mediaTools->windowSize());

    QObject::connect(this->m_mediaTools,
                     SIGNAL(devicesModified()),
                     this,
                     SLOT(updateWebcams()));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(deviceChanged(const QString &)),
                     this,
                     SLOT(deviceChanged(const QString &)));

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

    foreach (QStringList webcam, this->m_mediaTools->captureDevices())
        this->ui->cbxSetWebcam->addItem(webcam.at(1));
}

MainWidget::~MainWidget()
{
}

void MainWidget::addWebcamConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgWebcamDialog = new CameraConfig(this->m_mediaTools);

    configDialog->addPage(this->m_cfgWebcamDialog,
                          this->tr("Webcam Settings"),
                          QIcon::fromTheme("camera-web"),
                          this->tr("Configure the parameters of the webcam."));
}

void MainWidget::addEffectsConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->m_cfgEffects = new Effects(this->m_mediaTools);

    configDialog->addPage(this->m_cfgEffects,
                          this->tr("Configure Webcam Effects"),
                          QIcon::fromTheme("tools-wizard"),
                          this->tr("Add funny effects to the webcam"));
}

void MainWidget::addVideoFormatsConfigDialog(ConfigDialog *configDialog)
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

void MainWidget::addStreamsConfigDialog(ConfigDialog *configDialog)
{
    this->m_cfgStreams = new StreamsConfig(this->m_mediaTools);

    configDialog->
           addPage(this->m_cfgStreams,
                   this->tr("Configure Custom Streams"),
                   QIcon::fromTheme("network-workgroup"),
                   this->tr("Add or remove local or network live streams."));
}

void MainWidget::addGeneralConfigsDialog(ConfigDialog *configDialog)
{
    this->m_cfgGeneralConfig = new GeneralConfig(this->m_mediaTools);

    configDialog->
           addPage(this->m_cfgGeneralConfig,
                   this->tr("General Options"),
                   QIcon::fromTheme("configure"),
                   this->tr("Setup the basic capture options."));
}

void MainWidget::showConfigDialog(ConfigDialog *configDialog)
{
    if (!configDialog) {
        configDialog = new ConfigDialog(this);

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

    if (video) {
        QString videosPath(".");
#if QT_VERSION >= 0x050000
        QStringList videosPaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);

        if (!videosPaths.isEmpty())
            videosPath = videosPaths[0];
#else
        videosPath = QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
#endif // QT_VERSION >= 0x050000

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
#if QT_VERSION >= 0x050000
        QStringList picturesPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);

        if (!picturesPaths.isEmpty())
            picturesPath = picturesPaths[0];
#else
        picturesPath = QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
#endif // QT_VERSION >= 0x050000

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

void MainWidget::showFrame(const QbPacket &webcamFrame)
{
    if (!this->m_mediaTools->device().isEmpty()) {
        this->m_webcamFrame = webcamFrame;

        this->m_imageDispay->setImage(this->m_webcamFrame);
    }
}

void MainWidget::cleanAll()
{
    this->m_mediaTools->cleanAll();
}

void MainWidget::updateWebcams()
{
    QString oldDevice = this->m_mediaTools->device();
    bool timerIsActive = !this->m_mediaTools->device().isEmpty();
    this->m_mediaTools->resetDevice();
    this->ui->cbxSetWebcam->clear();
    QList<QStringList> webcams = this->m_mediaTools->captureDevices();
    QStringList devices;

    foreach (QStringList webcam, webcams) {
        devices << webcam.at(0);
        this->ui->cbxSetWebcam->addItem(webcam.at(1));
    }

    if (devices.contains(oldDevice) && timerIsActive)
        this->m_mediaTools->setDevice(oldDevice);
}

void MainWidget::deviceChanged(const QString &device)
{
    if (device.isEmpty()) {
        this->ui->btnTakePhoto->setEnabled(false);
        this->ui->btnVideoRecord->setEnabled(false);
        this->ui->btnStartStop->setIcon(QIcon::fromTheme("media-playback-start"));
        this->m_webcamFrame = QbPacket();
        this->m_imageDispay->setImage(this->m_webcamFrame);
    }
    else {
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

void MainWidget::showError(const QString &message)
{
    QMessageBox messageBox(QMessageBox::Critical,
                           this->tr("An error has occurred"),
                           message,
                           QMessageBox::Ok,
                           this);

    messageBox.exec();
}

void MainWidget::updateContents()
{
    this->m_imageDispay->setGeometry(0, 0, this->width(), this->height());

    QRect geometry(0,
                   this->height() - this->ui->wdgControls->height(),
                   this->width(),
                   this->ui->wdgControls->height());

    this->ui->wdgControls->setGeometry(geometry);
}

void MainWidget::on_btnTakePhoto_clicked()
{
    QImage image(reinterpret_cast<uchar *>(this->m_webcamFrame.buffer().data()),
                 this->m_webcamFrame.caps().property("width").toInt(),
                 this->m_webcamFrame.caps().property("height").toInt(),
                 QImage::Format_ARGB32);

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
    else {
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
    About aboutDialog(this);

    aboutDialog.setIcon(QIcon::fromTheme("camera-web"));
    aboutDialog.setAppName(QCoreApplication::applicationName());
    aboutDialog.setVersion(QCoreApplication::applicationVersion());
    aboutDialog.setShortDescription(this->tr("webcam capture application."));
    aboutDialog.setDescription(this->tr("A simple webcam application "
                                        "for picture and "
                                        "video capture."));
    aboutDialog.setCopyrightNotice(COMMONS_COPYRIGHT_NOTICE);
    aboutDialog.setWebsiteLink(COMMONS_PROJECT_URL);
    aboutDialog.setWebsiteLicense(COMMONS_PROJECT_LICENSE_URL);

    aboutDialog.exec();
}
