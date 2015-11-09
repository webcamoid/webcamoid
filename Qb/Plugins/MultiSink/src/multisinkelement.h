/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#ifndef MULTISINKELEMENT_H
#define MULTISINKELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>

#ifndef USE_GSTREAMER
#include "ffmpeg/mediasink.h"
#else
#include "gstreamer/mediasink.h"
#endif

class MultiSinkElement: public QbElement
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
    Q_PROPERTY(QVariantList streams
               READ streams
               NOTIFY streamsChanged)

    public:
        explicit MultiSinkElement();
        ~MultiSinkElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString outputFormat() const;
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
                                          const QbCaps &streamCaps,
                                          const QVariantMap &codecParams=QVariantMap());

    protected:
        void stateChange(QbElement::ElementState from,
                         QbElement::ElementState to);

    private:
        MediaSink m_mediaSink;
        QMutex m_mutex;

    signals:
        void locationChanged(const QString &location);
        void outputFormatChanged(const QString &outputFormat);
        void streamsChanged(const QVariantList &streams);

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void resetLocation();
        void resetOutputFormat();
        void clearStreams();

        QbPacket iStream(const QbPacket &packet);
};

#endif // MULTISINKELEMENT_H
