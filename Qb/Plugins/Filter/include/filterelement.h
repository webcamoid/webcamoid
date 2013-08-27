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

#ifndef FILTERELEMENT_H
#define FILTERELEMENT_H

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavfilter/avcodec.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/avfiltergraph.h>
    #include <libavfilter/buffersrc.h>
    #include <libavfilter/buffersink.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/channel_layout.h>
}

#include <qbelement.h>

class FilterElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description
                                   WRITE setDescription
                                   RESET resetDescription)

    Q_PROPERTY(QString format READ format
                              WRITE setFormat
                              RESET resetFormat)

    Q_PROPERTY(QbFrac pixelAspect READ pixelAspect
                                  WRITE setPixelAspect
                                  RESET resetPixelAspect)

    public:
        explicit FilterElement();
        ~FilterElement();

        Q_INVOKABLE QString description();
        Q_INVOKABLE QString format();
        Q_INVOKABLE QbFrac pixelAspect();

    protected:
        bool initBuffers();
        void uninitBuffers();

    private:
        QString m_description;
        QString m_format;
        QbFrac m_timeBase;
        QbFrac m_pixelAspect;

        AVPixelFormat m_oFormat;
        QbCaps m_curInputCaps;
        AVFilterContext *m_bufferSinkContext;
        AVFilterContext *m_bufferSrcContext;
        AVFilterGraph *m_filterGraph;

    public slots:
        void setDescription(QString description);
        void setFormat(QString format);
        void setPixelAspect(QbFrac pixelAspect);
        void resetDescription();
        void resetFormat();
        void resetPixelAspect();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);
};

#endif // FILTERELEMENT_H
