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

#ifndef URISRCELEMENT_H
#define URISRCELEMENT_H

#include <QtGui>

// https://www.libav.org/doxygen/master/index.html
extern "C"
{
    #include <libavdevice/avdevice.h>
}

#include "videostream.h"
#include "qbelement.h"

class UriSrcElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString uri READ uri WRITE setUri RESET resetUri)
    Q_PROPERTY(bool loop READ loop WRITE setLoop RESET resetLoop)

    public:
        explicit UriSrcElement();
        ~UriSrcElement();

        Q_INVOKABLE QString uri();
        Q_INVOKABLE bool loop();

        Q_INVOKABLE ElementState state();
        Q_INVOKABLE QList<QbElement *> srcs();
        Q_INVOKABLE QList<QbElement *> sinks();

    private:
        QString m_uri;
        bool m_loop;

        QImage m_oFrame;

        AVFormatContext *m_inputContext;
        QTimer m_timer;
        QMap<int, AbstractStream *> m_streams;

        bool initCapture();
        void uninitCapture();

    public slots:
        void setUri(QString uri);
        void setLoop(bool loop);
        void resetUri();
        void resetLoop();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);
        void setSrcs(QList<QbElement *> srcs);
        void setSinks(QList<QbElement *> sinks);
        void resetState();
        void resetSrcs();
        void resetSinks();

    private slots:
        void readPackets();
};

#endif // URISRCELEMENT_H
