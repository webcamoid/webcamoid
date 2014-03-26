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

#ifndef AUDIOINPUTELEMENT_H
#define AUDIOINPUTELEMENT_H

#include <qb.h>

class AudioInputElement: public QbElement
{
    Q_OBJECT
        Q_PROPERTY(QString audioSystem READ audioSystem
                                       WRITE setAudioSystem
                                       RESET resetAudioSystem)

        Q_PROPERTY(QStringList availableAudioSystem READ availableAudioSystem)
        Q_PROPERTY(QVariantMap streamCaps READ streamCaps)

    public:
        explicit AudioInputElement();

        Q_INVOKABLE QString audioSystem();
        Q_INVOKABLE QStringList availableAudioSystem();
        Q_INVOKABLE QVariantMap streamCaps();
        bool event(QEvent *event);

    private:
        QString m_audioSystem;

        QbElementPtr m_input;

    public slots:
        void setAudioSystem(QString audioSystem);
        void resetAudioSystem();
};

#endif // AUDIOINPUTELEMENT_H
