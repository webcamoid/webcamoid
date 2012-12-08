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

#include <KIcon>

#include "generalconfig.h"

GeneralConfig::GeneralConfig(V4L2Tools *tools, QWidget *parent): QWidget(parent)
{
    this->appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->setWindowIcon(KIcon("camera-web"));

    this->tools = (tools)? tools: new V4L2Tools(true, this);
    this->chkAudioRecord->setCheckState((this->tools->recordAudio())? Qt::Checked: Qt::Unchecked);
}

void GeneralConfig::on_chkAudioRecord_stateChanged(int state)
{
    this->tools->enableAudioRecording((state == Qt::Checked)? true: false);
}
