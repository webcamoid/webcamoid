/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef MEDIAWRITERGSTREAMER_H
#define MEDIAWRITERGSTREAMER_H

#include <QtConcurrent>

#include "mediawriter.h"
#include "outputparams.h"

class MediaWriterGStreamer: public MediaWriter
{
    Q_OBJECT

    public:
        explicit MediaWriterGStreamer(QObject *parent=NULL);
        ~MediaWriterGStreamer();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantList streams() const;

        Q_INVOKABLE QStringList supportedFormats();
        Q_INVOKABLE QStringList fileExtensions(const QString &format);
        Q_INVOKABLE QString formatDescription(const QString &format);
        Q_INVOKABLE QVariantList formatOptions();
        Q_INVOKABLE QStringList supportedCodecs(const QString &format);
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                const QString &type);
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         const QString &type);
        Q_INVOKABLE QString codecDescription(const QString &codec);
        Q_INVOKABLE QString codecType(const QString &codec);
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec);
        Q_INVOKABLE QVariantList codecOptions(const QString &codec);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams);
        Q_INVOKABLE QVariantMap updateStream(int index);
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams);

    private:
        QString m_location;
        QString m_outputFormat;
        QMap<QString, QVariantMap> m_formatOptions;
        bool m_isRecording;

        QList<QVariantMap> m_streamConfigs;
        QList<OutputParams> m_streamParams;
        QThreadPool m_threadPool;
        GstElement *m_pipeline;
        GMainLoop *m_mainLoop;
        guint m_busWatchId;

        QString guessFormat(const QString &fileName);
        QStringList readCaps(const QString &element);
        QVariantList parseOptions(const GstElement *element) const;
        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        void setElementOptions(GstElement *element, const QVariantMap &options);
        AkVideoCaps nearestDVCaps(const AkVideoCaps &caps) const;
        AkVideoCaps nearestH263Caps(const AkVideoCaps &caps) const;
        AkAudioCaps nearestFLVAudioCaps(const AkAudioCaps &caps,
                                        const QString &codec) const;

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void resetLocation();
        void resetOutputFormat();
        void resetFormatOptions();
        void enqueuePacket(const AkPacket &packet);
        void clearStreams();
        bool init();
        void uninit();

    private slots:
        void writeAudioPacket(const AkAudioPacket &packet);
        void writeVideoPacket(const AkVideoPacket &packet);
        void writeSubtitlePacket(const AkPacket &packet);
        void updateStreams();
};

#endif // MEDIAWRITERGSTREAMER_H
