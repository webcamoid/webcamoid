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

#ifndef MULTISINKELEMENT_H
#define MULTISINKELEMENT_H

#include <QVariantMap>
#include <akcaps.h>
#include <akelement.h>

class MultiSinkElementPrivate;
class AkCaps;

class MultiSinkElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
    Q_PROPERTY(QString defaultFormat
               READ defaultFormat
               NOTIFY defaultFormatChanged)
    Q_PROPERTY(QStringList supportedFormats
               READ supportedFormats
               NOTIFY supportedFormatsChanged)
    Q_PROPERTY(QString outputFormat
               READ outputFormat
               WRITE setOutputFormat
               RESET resetOutputFormat
               NOTIFY outputFormatChanged)
    Q_PROPERTY(QVariantList streams
               READ streams
               NOTIFY streamsChanged)
    Q_PROPERTY(QStringList formatsBlackList
               READ formatsBlackList
               WRITE setFormatsBlackList
               RESET resetFormatsBlackList
               NOTIFY formatsBlackListChanged)
    Q_PROPERTY(QStringList codecsBlackList
               READ codecsBlackList
               WRITE setCodecsBlackList
               RESET resetCodecsBlackList
               NOTIFY codecsBlackListChanged)

    public:
        MultiSinkElement();
        ~MultiSinkElement();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QString defaultFormat() const;
        Q_INVOKABLE QStringList supportedFormats() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantList streams();
        Q_INVOKABLE QStringList formatsBlackList() const;
        Q_INVOKABLE QStringList codecsBlackList() const;
        Q_INVOKABLE QStringList fileExtensions(const QString &format) const;
        Q_INVOKABLE QString formatDescription(const QString &format) const;
        Q_INVOKABLE QVariantList formatOptions() const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                AkCaps::CapsType type=AkCaps::CapsUnknown);
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         AkCaps::CapsType type);
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE AkCaps::CapsType codecType(const QString &codec) const;
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec) const;
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams=QVariantMap());
        Q_INVOKABLE QVariantList codecOptions(int index);

    private:
        MultiSinkElementPrivate *d;

    signals:
        void locationChanged(const QString &location);
        void defaultFormatChanged(const QString &defaultFormat);
        void supportedFormatsChanged(const QStringList &supportedFormats);
        void outputFormatChanged(const QString &outputFormat);
        void formatOptionsChanged(const QVariantMap &formatOptions);
        void codecOptionsChanged(const QString &key,
                                 const QVariantMap &codecOptions);
        void streamsChanged(const QVariantList &streams);
        void formatsBlackListChanged(const QStringList &formatsBlackList);
        void codecsBlackListChanged(const QStringList &codecsBlackList);

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void setCodecOptions(int index, const QVariantMap &codecOptions);
        void setFormatsBlackList(const QStringList &formatsBlackList);
        void setCodecsBlackList(const QStringList &codecsBlackList);
        void resetLocation();
        void resetOutputFormat();
        void resetFormatOptions();
        void resetCodecOptions(int index);
        void resetFormatsBlackList();
        void resetCodecsBlackList();
        void clearStreams();

        AkPacket iStream(const AkPacket &packet) override;
        bool setState(AkElement::ElementState state) override;
};

#endif // MULTISINKELEMENT_H
