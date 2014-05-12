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

#include "ui_about.h"

#include "about.h"

About::About(QWidget *parent):
    QDialog(parent),
    ui(new Ui::About)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);
    this->ui->lblQtVersion->setText(QT_VERSION_STR);

    QPushButton *btnClose = this->ui->bbxClose->button(QDialogButtonBox::Close);
    connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));
}

About::~About()
{

}

void About::setIcon(const QIcon &icon)
{
    this->m_icon = icon;
    this->ui->lblIcon->setPixmap(icon.pixmap(48, 48));
}

void About::setAppName(const QString &appName)
{
    this->ui->lblAppName->setText(appName);
    this->setWindowTitle(this->tr("About %1").arg(appName));
}

void About::setVersion(const QString &version)
{
    this->ui->lblVersion->setText(version);
}

void About::setShortDescription(const QString &shortDescription)
{
    this->ui->lblShortDescription->setText(shortDescription);
}

void About::setDescription(const QString &description)
{
    this->ui->lblDescription->setText(description);
}

void About::setCopyrightNotice(const QString &copyrightNotice)
{
    this->ui->lblCopyright->setText(copyrightNotice);
}

void About::setWebsiteLink(const QString &websiteLink)
{
    this->m_websiteLink = websiteLink;
}

void About::setWebsiteLicense(const QString &websiteLicense)
{
    this->m_websiteLicense = websiteLicense;
}

void About::on_btnLicense_clicked()
{
    QDesktopServices::openUrl(this->m_websiteLicense);
}

void About::on_btnWebsite_clicked()
{
    QDesktopServices::openUrl(this->m_websiteLink);
}
