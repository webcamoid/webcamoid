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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef GENERALCONFIG_H
#define GENERALCONFIG_H

#include <QWidget>

#include "mediatools.h"

namespace Ui
{
    class GeneralConfig;
}

class COMMONSSHARED_EXPORT GeneralConfig: public QWidget
{
    Q_OBJECT

    public:
        explicit GeneralConfig(MediaTools *mediaTools=NULL, QWidget *parent=NULL);
        ~GeneralConfig();

    private:
        QSharedPointer<Ui::GeneralConfig> ui;

        AppEnvironment *m_appEnvironment;
        MediaTools *m_mediaTools;

    public slots:
        void on_chkPlaySource_stateChanged(int state);
        void on_radMic_toggled(bool checked);
        void on_radNone_toggled(bool checked);
        void on_radSource_toggled(bool checked);
};

#endif // GENERALCONFIG_H
