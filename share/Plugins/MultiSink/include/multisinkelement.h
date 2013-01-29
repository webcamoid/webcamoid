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

#ifndef MULTISINKELEMENT_H
#define MULTISINKELEMENT_H

#include <QtGui>

extern "C"
{
    #include <libavutil/mathematics.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#include "qbpipeline.h"
#include "optionparser.h"

class MultiSinkElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString location READ location
                                WRITE setLocation
                                RESET resetLocation)

    Q_PROPERTY(QString options READ options
                               WRITE setOptions
                               RESET resetOptions)

    Q_PROPERTY(QSize frameSize READ frameSize
                               WRITE setFrameSize
                               RESET resetFrameSize)

    public:
        explicit MultiSinkElement();
        ~MultiSinkElement();

        Q_INVOKABLE QString location();
        Q_INVOKABLE QString options();
        Q_INVOKABLE QSize frameSize();

    private:
        QString m_location;
        QString m_options;
        QSize m_frameSize;

        AVPicture m_oPicture;
        AVFormatContext *m_outputContext;
        OptionParser m_optionParser;
        QVariantMap m_optionsMap;
        AVStream *m_audioStream;
        AVStream *m_videoStream;
        QbPipeline m_pipeline;
        QbElement *m_vFilter;
        QbCaps m_curInputCaps;
        int m_pictureAlloc;

        QMap<QString, PixelFormat> m_mimeToFF;

        bool init();
        void uninit();
        QList<PixelFormat> pixelFormats(AVCodec *videoCodec);
        QList<AVSampleFormat> sampleFormats(AVCodec *audioCodec);
        QList<int> sampleRates(AVCodec *audioCodec);
        AVStream *addStream(AVCodec **codec, QString codecName="");
        void adjustToInputFrameSize(QSize frameSize);
        void cleanAll();

    public slots:
        void setLocation(QString location);
        void setOptions(QString options);
        void setFrameSize(QSize frameSize);
        void resetLocation();
        void resetOptions();
        void resetFrameSize();
        void processVFrame(const QbPacket &packet);

        void iStream(const QbPacket &packet);
        void setState(ElementState state);
};

#endif // MULTISINKELEMENT_H
