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
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef MEDIAWRITER_H
#define MEDIAWRITER_H

#include "akpacket.h"

class MediaWriter: public QObject
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
    Q_PROPERTY(qint64 maxPacketQueueSize
               READ maxPacketQueueSize
               WRITE setMaxPacketQueueSize
               RESET resetMaxPacketQueueSize
               NOTIFY maxPacketQueueSizeChanged)

    public:
        explicit MediaWriter(QObject *parent=NULL);
        virtual ~MediaWriter();

        Q_INVOKABLE virtual QString location() const;
        Q_INVOKABLE virtual QString outputFormat() const;
        Q_INVOKABLE virtual QVariantMap formatOptions() const;
        Q_INVOKABLE virtual QVariantList streams() const;
        Q_INVOKABLE virtual qint64 maxPacketQueueSize() const;

        Q_INVOKABLE virtual QStringList supportedFormats();
        Q_INVOKABLE virtual QStringList fileExtensions(const QString &format);
        Q_INVOKABLE virtual QString formatDescription(const QString &format);
        Q_INVOKABLE virtual QStringList supportedCodecs(const QString &format);
        Q_INVOKABLE virtual QStringList supportedCodecs(const QString &format,
                                                        const QString &type);
        Q_INVOKABLE virtual QString defaultCodec(const QString &format,
                                                 const QString &type);
        Q_INVOKABLE virtual QString codecDescription(const QString &codec);
        Q_INVOKABLE virtual QString codecType(const QString &codec);
        Q_INVOKABLE virtual QVariantMap defaultCodecParams(const QString &codec);
        Q_INVOKABLE virtual QVariantMap addStream(int streamIndex,
                                                  const AkCaps &streamCaps);
        Q_INVOKABLE virtual QVariantMap addStream(int streamIndex,
                                                  const AkCaps &streamCaps,
                                                  const QVariantMap &codecParams);
        Q_INVOKABLE virtual QVariantMap updateStream(int index);
        Q_INVOKABLE virtual QVariantMap updateStream(int index,
                                                     const QVariantMap &codecParams);

    signals:
        void locationChanged(const QString &location);
        void outputFormatChanged(const QString &outputFormat);
        void formatOptionsChanged(const QVariantMap &formatOptions);
        void streamsChanged(const QVariantList &streams);
        void maxPacketQueueSizeChanged(qint64 maxPacketQueueSize);
        void streamUpdated(int index);

    public slots:
        virtual void setLocation(const QString &location);
        virtual void setOutputFormat(const QString &outputFormat);
        virtual void setFormatOptions(const QVariantMap &formatOptions);
        virtual void setMaxPacketQueueSize(qint64 maxPacketQueueSize);
        virtual void resetLocation();
        virtual void resetOutputFormat();
        virtual void resetFormatOptions();
        virtual void resetMaxPacketQueueSize();
        virtual void enqueuePacket(const AkPacket &packet);
        virtual void clearStreams();
        virtual bool init();
        virtual void uninit();
};

#endif // MEDIAWRITER_H
