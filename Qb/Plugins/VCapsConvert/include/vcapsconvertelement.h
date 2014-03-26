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

#ifndef VCAPSCONVERTELEMENT_H
#define VCAPSCONVERTELEMENT_H

#include <qb.h>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
}

#include "convertio.h"

typedef QSharedPointer<SwsContext> SwsContextPtr;

class VCapsConvertElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString caps READ caps WRITE setCaps RESET resetCaps)

    public:
        explicit VCapsConvertElement();

        Q_INVOKABLE QString caps();

    private:
        QbCaps m_caps;
        SwsContextPtr m_scaleContext;
        QList<int> m_check;

        static void deleteSwsContext(SwsContext *context);

    public slots:
        void setCaps(QString caps);
        void resetCaps();

        void iStream(const QbPacket &packet);
};

#endif // VCAPSCONVERTELEMENT_H
