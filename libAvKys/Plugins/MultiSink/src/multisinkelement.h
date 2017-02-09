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

#ifndef MULTISINKELEMENT_H
#define MULTISINKELEMENT_H

#include <QMutex>
#include <QQmlComponent>
#include <QQmlContext>
#include <ak.h>

#include "mediawriter.h"
#include "multisinkutils.h"

typedef QSharedPointer<MediaWriter> MediaWriterPtr;

class MultiSinkElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(QString location
               READ location
               WRITE setLocation
               RESET resetLocation
               NOTIFY locationChanged)
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
    Q_PROPERTY(QString codecLib
               READ codecLib
               WRITE setCodecLib
               RESET resetCodecLib
               NOTIFY codecLibChanged)
    Q_PROPERTY(bool showFormatOptions
               READ showFormatOptions
               WRITE setShowFormatOptions
               RESET resetShowFormatOptions
               NOTIFY showFormatOptionsChanged)
    Q_PROPERTY(QVariantList userControls
               READ userControls
               WRITE setUserControls
               RESET resetUserControls
               NOTIFY userControlsChanged)
    Q_PROPERTY(QVariantMap userControlsValues
               READ userControlsValues
               WRITE setUserControlsValues
               RESET resetUserControlsValues
               NOTIFY userControlsValuesChanged)

    public:
        explicit MultiSinkElement();
        ~MultiSinkElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE QStringList supportedFormats() const;
        Q_INVOKABLE QString outputFormat() const;
        Q_INVOKABLE QVariantList streams();
        Q_INVOKABLE QString codecLib() const;
        Q_INVOKABLE bool showFormatOptions() const;
        Q_INVOKABLE QVariantList userControls() const;
        Q_INVOKABLE QVariantMap userControlsValues() const;
        Q_INVOKABLE QStringList fileExtensions(const QString &format) const;
        Q_INVOKABLE QString formatDescription(const QString &format) const;
        Q_INVOKABLE QVariantList formatOptions() const;
        Q_INVOKABLE QStringList supportedCodecs(const QString &format,
                                                const QString &type="");
        Q_INVOKABLE QString defaultCodec(const QString &format,
                                         const QString &type);
        Q_INVOKABLE QString codecDescription(const QString &codec) const;
        Q_INVOKABLE QString codecType(const QString &codec) const;
        Q_INVOKABLE QVariantMap defaultCodecParams(const QString &codec) const;
        Q_INVOKABLE QVariantList codecOptions(const QString &codec);
        Q_INVOKABLE QVariantMap addStream(int streamIndex,
                                          const AkCaps &streamCaps,
                                          const QVariantMap &codecParams=QVariantMap());
        Q_INVOKABLE QVariantMap updateStream(int index,
                                             const QVariantMap &codecParams=QVariantMap());

    private:
        QString m_location;
        bool m_showFormatOptions;
        QVariantList m_userControls;
        QVariantMap m_userControlsValues;
        MediaWriterPtr m_mediaWriter;
        QMutex m_mutex;
        QMutex m_mutexLib;
        MultiSinkUtils m_utils;

        // Formats and codecs info cache.
        QStringList m_supportedFormats;
        QMap<QString, QStringList> m_fileExtensions;
        QMap<QString, QString> m_formatDescription;
        QStringList m_supportedCodecs;
        QMap<QString, QString> m_codecDescription;
        QMap<QString, QString> m_codecType;
        QMap<QString, QVariantMap> m_defaultCodecParams;

    signals:
        void locationChanged(const QString &location);
        void supportedFormatsChanged(const QStringList &supportedFormats);
        void outputFormatChanged(const QString &outputFormat);
        void formatOptionsChanged(const QVariantMap &formatOptions);
        void streamsChanged(const QVariantList &streams);
        void codecLibChanged(const QString &codecLib);
        void showFormatOptionsChanged(bool showFormatOptions);
        void userControlsChanged(const QVariantList &userControls);
        void userControlsValuesChanged(const QVariantMap &userControlsValues);
        void streamUpdated(int index);

    public slots:
        void setLocation(const QString &location);
        void setOutputFormat(const QString &outputFormat);
        void setFormatOptions(const QVariantMap &formatOptions);
        void setCodecLib(const QString &codecLib);
        void setShowFormatOptions(bool showFormatOptions);
        void setUserControls(const QVariantList &userControls);
        void setUserControlsValues(const QVariantMap &userControlsValues);
        void resetLocation();
        void resetOutputFormat();
        void resetFormatOptions();
        void resetCodecLib();
        void resetShowFormatOptions();
        void resetUserControls();
        void resetUserControlsValues();
        void clearStreams();

        AkPacket iStream(const AkPacket &packet);
        bool setState(AkElement::ElementState state);

    private slots:
        void codecLibUpdated(const QString &codecLib);
};

#endif // MULTISINKELEMENT_H
