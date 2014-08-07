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

#include "ui_generalconfig.h"

#include "generalconfig.h"

GeneralConfig::GeneralConfig(MediaTools *mediaTools, QWidget *parent):
    QWidget(parent),
    ui(new Ui::GeneralConfig)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(this);

    switch (this->m_mediaTools->recordAudioFrom()) {
    case MediaTools::RecordFromNone:
        this->ui->radNone->setChecked(true);
        break;

    case MediaTools::RecordFromSource:
        this->ui->radSource->setChecked(true);
        break;

    case MediaTools::RecordFromMic:
        this->ui->radMic->setChecked(true);
        break;

    default:
        break;
    }

    this->ui->chkPlaySource->setCheckState(this->m_mediaTools->playAudioFromSource()? Qt::Checked: Qt::Unchecked);
}

GeneralConfig::~GeneralConfig()
{
}

void GeneralConfig::on_chkPlaySource_stateChanged(int state)
{
    this->m_mediaTools->setPlayAudioFromSource((state == Qt::Checked)? true: false);
}

void GeneralConfig::on_radMic_toggled(bool checked)
{
    if (checked)
        this->m_mediaTools->setRecordAudioFrom(MediaTools::RecordFromMic);
}

void GeneralConfig::on_radNone_toggled(bool checked)
{
    if (checked)
        this->m_mediaTools->setRecordAudioFrom(MediaTools::RecordFromNone);
}

void GeneralConfig::on_radSource_toggled(bool checked)
{
    if (checked)
        this->m_mediaTools->setRecordAudioFrom(MediaTools::RecordFromSource);
}
