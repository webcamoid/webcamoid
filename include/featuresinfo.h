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

#ifndef FEATURESINFO_H
#define FEATURESINFO_H

#include <QtGui>

#include "ui_featuresinfo.h"

#include "commons.h"
#include "appenvironment.h"
#include "mediatools.h"

class COMMONSSHARED_EXPORT FeaturesInfo: public QWidget, public Ui::FeaturesInfo
{
    Q_OBJECT

    public:
        explicit FeaturesInfo(MediaTools *mediaTools=NULL, QWidget *parent=NULL);

    private:
        AppEnvironment *m_appEnvironment;
        MediaTools *m_mediaTools;

    private slots:
        void on_btnRecheck_clicked();
};

#endif // FEATURESINFO_H
