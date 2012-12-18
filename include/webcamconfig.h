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

#ifndef WEBCAMCONFIG_H
#define WEBCAMCONFIG_H

#include <QtGui>

#include "commons.h"
#include "appenvironment.h"
#include "mediatools.h"

namespace Ui
{
    class WebcamConfig;
}

class COMMONSSHARED_EXPORT WebcamConfig: public QWidget
{
    Q_OBJECT

    public:
        explicit WebcamConfig(MediaTools *mediaTools=NULL, QWidget *parent=NULL);
        ~WebcamConfig();

    private:
        Ui::WebcamConfig *ui;

        AppEnvironment *m_appEnvironment;
        MediaTools *m_mediaTools;
        QVariantList m_captureDevices;
        QVariantMap m_videoFormats;
        bool m_resetting;

        void resetControls(QString deviceName);

    private slots:
        void pushButtonClicked();
        void sliderMoved(int value);
        void spinboxValueChanged(int i);
        void checkboxToggled(bool checked);
        void comboboxCurrentIndexChanged(int index);
};

#endif // WEBCAMCONFIG_H
