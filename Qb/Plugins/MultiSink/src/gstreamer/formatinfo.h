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

#ifndef FORMATINFO_H
#define FORMATINFO_H

#include <QObject>

class FormatInfo: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name
               READ name
               WRITE setName
               RESET resetName
               NOTIFY nameChanged)
    Q_PROPERTY(QString longName
               READ longName
               WRITE setLongName
               RESET resetLongName
               NOTIFY longNameChanged)
    Q_PROPERTY(QStringList extensions
               READ extensions
               WRITE setExtensions
               RESET resetExtensions
               NOTIFY extensionsChanged)
    Q_PROPERTY(QString defaultAudioCodec
               READ defaultAudioCodec
               WRITE setDefaultAudioCodec
               RESET resetDefaultAudioCodec
               NOTIFY defaultAudioCodecChanged)
    Q_PROPERTY(QStringList audioCodec
               READ audioCodec
               WRITE setAudioCodec
               RESET resetAudioCodec
               NOTIFY audioCodecChanged)
    Q_PROPERTY(QString defaultVideoCodec
               READ defaultVideoCodec
               WRITE setDefaultVideoCodec
               RESET resetDefaultVideoCodec
               NOTIFY defaultVideoCodecChanged)
    Q_PROPERTY(QStringList videoCodec
               READ videoCodec
               WRITE setVideoCodec
               RESET resetVideoCodec
               NOTIFY videoCodecChanged)

    public:
        explicit FormatInfo(QObject *parent=NULL);
        FormatInfo(const FormatInfo &other);
        FormatInfo &operator =(const FormatInfo &other);

        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString &name();
        Q_INVOKABLE QString longName() const;
        Q_INVOKABLE QString &longName();
        Q_INVOKABLE QStringList extensions() const;
        Q_INVOKABLE QStringList &extensions();
        Q_INVOKABLE QString defaultAudioCodec() const;
        Q_INVOKABLE QString &defaultAudioCodec();
        Q_INVOKABLE QStringList audioCodec() const;
        Q_INVOKABLE QStringList &audioCodec();
        Q_INVOKABLE QString defaultVideoCodec() const;
        Q_INVOKABLE QString &defaultVideoCodec();
        Q_INVOKABLE QStringList videoCodec() const;
        Q_INVOKABLE QStringList &videoCodec();

    private:
        QString m_name;
        QString m_longName;
        QStringList m_extensions;
        QString m_defaultAudioCodec;
        QStringList m_audioCodec;
        QString m_defaultVideoCodec;
        QStringList m_videoCodec;

    signals:
        void nameChanged(const QString &name);
        void longNameChanged(const QString &longName);
        void extensionsChanged(const QStringList &extensions);
        void defaultAudioCodecChanged(const QString &defaultAudioCodec);
        void audioCodecChanged(const QStringList &audioCodec);
        void defaultVideoCodecChanged(const QString &defaultVideoCodec);
        void videoCodecChanged(const QStringList &videoCodec);

    public slots:
        void setName(const QString &name);
        void setLongName(const QString &longName);
        void setExtensions(const QStringList &extensions);
        void setDefaultAudioCodec(const QString &defaultAudioCodec);
        void setAudioCodec(const QStringList &audioCodec);
        void setDefaultVideoCodec(const QString &defaultVideoCodec);
        void setVideoCodec(const QStringList &videoCodec);
        void resetName();
        void resetLongName();
        void resetExtensions();
        void resetDefaultAudioCodec();
        void resetAudioCodec();
        void resetDefaultVideoCodec();
        void resetVideoCodec();
};

#endif // FORMATINFO_H
