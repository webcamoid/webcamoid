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

#ifndef ELEMENT_H
#define ELEMENT_H

#include <QtCore>

/// Plugin template.
class Element: public QObject
{
    Q_OBJECT
    Q_ENUMS(ElementState)
    Q_PROPERTY(ElementState state READ state WRITE setState RESET resetState)

    public:
        enum ElementState
        {
            Null,
            Ready,
            Paused,
            Playing
        };

        Q_INVOKABLE virtual ElementState state() = 0;

    signals:
        void oStream(const void *data, int datalen, QString dataType);

    public slots:
        virtual void iStream(const void *data, int datalen, QString dataType) = 0;
        virtual void setState(ElementState state) = 0;
        virtual void resetState() = 0;
};

#endif // ELEMENT_H
