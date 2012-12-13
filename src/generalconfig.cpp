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

#include "generalconfig.h"

GeneralConfig::GeneralConfig(MediaTools *mediaTools, QWidget *parent): QWidget(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(true, this);
    this->chkAudioRecord->setCheckState(this->m_mediaTools->recordAudio()? Qt::Checked: Qt::Unchecked);
}

void GeneralConfig::on_chkAudioRecord_stateChanged(int state)
{
    this->m_mediaTools->enableAudioRecording((state == Qt::Checked)? true: false);
}
