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

#ifndef ACAPSCONVERTELEMENT_H
#define ACAPSCONVERTELEMENT_H

extern "C"
{
    #include <libavutil/opt.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/audioconvert.h>
    #include <libswresample/swresample.h>
}

#include <qbelement.h>

typedef QSharedPointer<SwrContext> SwrContextPtr;

class ACapsConvertElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString caps READ caps WRITE setCaps RESET resetCaps)

    public:
        explicit ACapsConvertElement();
        ~ACapsConvertElement();

        Q_INVOKABLE QString caps();

    private:
        QbCaps m_caps;

        QbCaps m_curInputCaps;
        SwrContextPtr m_resampleContext;

        static void deleteSwrContext(SwrContext *context);

    public slots:
        void setCaps(QString caps);
        void resetCaps();

        void iStream(const QbPacket &packet);
};

#endif // ACAPSCONVERTELEMENT_H
