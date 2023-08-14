/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef SUBTITLESTREAM_H
#define SUBTITLESTREAM_H

#include "abstractstream.h"

class SubtitleStream: public AbstractStream
{
    Q_OBJECT

    public:
        SubtitleStream(const AVFormatContext *formatContext=nullptr,
                       uint index=0,
                       qint64 id=-1,
                       Clock *globalClock=nullptr,
                       bool sync=true,
                       bool noModify=false,
                       QObject *parent=nullptr);

        Q_INVOKABLE AkCaps caps() const override;
        Q_INVOKABLE bool decodeData() override;

    protected:
        void processPacket(AVPacket *packet) override;
        void processData(AVSubtitle *subtitle) override;
};

#endif // SUBTITLESTREAM_H
