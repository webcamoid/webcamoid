/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef MEDIASINK_H
#define MEDIASINK_H

#include <QtConcurrent>

#include "outputparams.h"

class MediaSink: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
    Q_PROPERTY(QString outputFormat
               READ outputFormat
               WRITE setOutputFormat
               RESET resetOutputFormat
               NOTIFY outputFormatChanged)
    Q_PROPERTY(QVariantMap formatOptions
               READ formatOptions
               WRITE setFormatOptions
               RESET resetFormatOptions
               NOTIFY formatOptionsChanged)
    Q_PROPERTY(QVariantList streams
               READ streams
               NOTIFY streamsChanged)

    public:
        explicit MediaSink(QObject *parent=NULL);
        ~MediaSink();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantMap formatOptions() const;
        Q_INVOKABLE QVariantList streams() const;

        Q_INVOKABLE QStringList supportedFormats();
        Q_INVOKABLE QStringList fileExtensions(const QString &format);
        Q_INVOKABLE QString formatDescription(const QString &format);
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                const QString &type="");
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         const QString &type);
        Q_INVOKABLE QString codecDescription(const QString &codec);
        Q_INVOKABLE QString codecType(const QString &codec);
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams=QVariantMap());

    private:
        QString m_location;
        QString m_outputFormat;
        QVariantMap m_formatOptions;
        bool m_run;

        QList<QVariantMap> m_streamConfigs;
        QList<OutputParams> m_streamParams;

        QThreadPool m_threadPool;
        GstElement *m_pipeline;
        GMainLoop *m_mainLoop;
        guint m_busWatchId;

        QString guessFormat(const QString &fileName);
        QStringList readCaps(const QString &element);
        void waitState(GstState state);
        static gboolean busCallback(GstBus *bus,
                                    GstMessage *message,
                                    gpointer userData);
        void setElementOptions(GstElement *element, const QVariantMap &options);
        AkVideoCaps nearestH263Caps(const AkVideoCaps &caps) const;

    signals:
        void locationChanged(const QString &location);
        void outputFormatChanged(const QString &outputFormat);
        void formatOptionsChanged(const QVariantMap &formatOptions);
        void streamsChanged(const QVariantList &streams);
        void streamUpdated(int index);

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void resetLocation();
        void resetOutputFormat();
        void resetFormatOptions();
        void writeAudioPacket(const AkAudioPacket &packet);
        void writeVideoPacket(const AkVideoPacket &packet);
        void writeSubtitlePacket(const AkPacket &packet);
        void clearStreams();
        bool init();
        void uninit();

    private slots:
        void updateStreams();
};

#endif // MEDIASINK_H
