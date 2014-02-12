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

#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <QtCore>

#include "abstractstream.h"

class AudioStream: public AbstractStream
{
    Q_OBJECT
    Q_PROPERTY(bool align READ align WRITE setAlign RESET resetAlign)

    public:
        explicit AudioStream(QObject *parent=NULL);
        AudioStream(const FormatContextPtr &formatContext, uint index);

        Q_INVOKABLE bool align() const;
        Q_INVOKABLE QbCaps caps() const;
        Q_INVOKABLE QList<QbPacket> readPackets(AVPacket *packet);

    private:
        bool m_align;
        bool m_fst;
        int64_t m_pts;
        int64_t m_duration;

    public slots:
        void setAlign(bool align);
        void resetAlign();
};

#endif // AUDIOSTREAM_H
