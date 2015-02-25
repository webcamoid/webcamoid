/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef VIDEODISPLAY_H
#define VIDEODISPLAY_H

#include <QQuickItem>
#include <QSGSimpleTextureNode>

#include "videoframe.h"

class VideoDisplay: public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(VideoDisplay)

    Q_PROPERTY(bool fillDisplay
               READ fillDisplay
               WRITE setFillDisplay
               RESET resetFillDisplay
               NOTIFY fillDisplayChanged)

    public:
        VideoDisplay(QQuickItem *parent = NULL);
        ~VideoDisplay();

        Q_INVOKABLE bool fillDisplay() const;

    private:
        bool m_fillDisplay;
        VideoFrame m_videoFrame;

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode,
                                 UpdatePaintNodeData *updatePaintNodeData);

    signals:
        void fillDisplayChanged();

    public slots:
        void setFrame(const QbPacket &packet);
        void setFillDisplay(bool fillDisplay);
        void resetFillDisplay();
};

#endif // VIDEODISPLAY_H
