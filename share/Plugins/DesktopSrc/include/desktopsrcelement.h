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

#ifndef DESKTOPSRCELEMENT_H
#define DESKTOPSRCELEMENT_H

#include <QtGui>
#include <gst/gst.h>

#include "element.h"

class DesktopSrcElement: public Element
{
    Q_OBJECT

    Q_PROPERTY(bool showPointer READ showPointer
                                WRITE setShowPointer
                                RESET resetShowPointer)

    public:
        explicit DesktopSrcElement();
        ~DesktopSrcElement();

        Q_INVOKABLE bool showPointer();

        Q_INVOKABLE ElementState state();

    private:
        bool m_showPointer;
        ElementState m_state;

        QMutex m_mutex;
        int m_callBack;
        GstElement *m_pipeline;
        QImage m_oFrame;

        static void newBuffer(GstElement *appsink, gpointer self);

    public slots:
        void setShowPointer(bool showPointer);
        void resetShowPointer();

        void iStream(const void *data, int datalen, QString dataType);
        void setState(ElementState state);
        void resetState();
};

#endif // DESKTOPSRCELEMENT_H
