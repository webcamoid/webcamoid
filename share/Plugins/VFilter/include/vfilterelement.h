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

#ifndef VFILTERELEMENT_H
#define VFILTERELEMENT_H

extern "C"
{
    #include <libavfilter/avcodec.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/avfiltergraph.h>
    #include <libavutil/imgutils.h>
    #include <libavfilter/buffersink.h>
}

#include "qbelement.h"

class VFilterElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description
                                   WRITE setDescription
                                   RESET resetDescription)

    Q_PROPERTY(QString format READ format
                              WRITE setFormat
                              RESET resetFormat)

    Q_PROPERTY(QString timeBase READ timeBase
                                WRITE setTimeBase
                                RESET resetTimeBase)

    Q_PROPERTY(QString pixelAspect READ pixelAspect
                                   WRITE setPixelAspect
                                   RESET resetPixelAspect)

    public:
        explicit VFilterElement();
        ~VFilterElement();

        Q_INVOKABLE QString description();
        Q_INVOKABLE QString format();
        Q_INVOKABLE QString timeBase();
        Q_INVOKABLE QString pixelAspect();

    protected:
        bool initBuffers();
        void uninitBuffers();

    private:
        QString m_description;
        QString m_format;
        int m_timeBaseNum;
        int m_timeBaseDen;
        int m_pixelAspectNum;
        int m_pixelAspectDen;

        PixelFormat m_oFormat;
        QByteArray m_oFrame;
        int m_oFrameSize;
        QbCaps m_curInputCaps;
        AVFilterContext *m_bufferSinkContext;
        AVFilterContext *m_bufferSrcContext;
        AVFilterGraph *m_filterGraph;

    public slots:
        void setDescription(QString description);
        void setFormat(QString format);
        void setTimeBase(QString timeBase);
        void setPixelAspect(QString pixelAspect);
        void resetDescription();
        void resetFormat();
        void resetTimeBase();
        void resetPixelAspect();

        void iStream(const QbPacket &packet);
};

#endif // VFILTERELEMENT_H
