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

#ifndef CODECINFO_H
#define CODECINFO_H

#include <QObject>
#include <qbfrac.h>

typedef QList<int> IntList;
typedef QList<QbFrac> FracList;

class CodecInfo: public QObject
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
    Q_PROPERTY(IntList supportedSamplerates
               READ supportedSamplerates
               WRITE setSupportedSamplerates
               RESET resetSupportedSamplerates
               NOTIFY supportedSampleratesChanged)
    Q_PROPERTY(QStringList sampleFormats
               READ sampleFormats
               WRITE setSampleFormats
               RESET resetSampleFormats
               NOTIFY sampleFormatsChanged)
    Q_PROPERTY(QStringList channelLayouts
               READ channelLayouts
               WRITE setChannelLayouts
               RESET resetChannelLayouts
               NOTIFY channelLayoutsChanged)
    Q_PROPERTY(FracList supportedFramerates
               READ supportedFramerates
               WRITE setSupportedFramerates
               RESET resetSupportedFramerates
               NOTIFY supportedFrameratesChanged)
    Q_PROPERTY(QStringList pixelFormats
               READ pixelFormats
               WRITE setPixelFormats
               RESET resetPixelFormats
               NOTIFY pixelFormatsChanged)

    public:
        explicit CodecInfo(QObject *parent=NULL);
        CodecInfo(const CodecInfo &other);
        CodecInfo &operator =(const CodecInfo &other);

        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString &name();
        Q_INVOKABLE QString longName() const;
        Q_INVOKABLE QString &longName();
        Q_INVOKABLE IntList supportedSamplerates() const;
        Q_INVOKABLE IntList &supportedSamplerates();
        Q_INVOKABLE QStringList sampleFormats() const;
        Q_INVOKABLE QStringList &sampleFormats();
        Q_INVOKABLE QStringList channelLayouts() const;
        Q_INVOKABLE QStringList &channelLayouts();
        Q_INVOKABLE FracList supportedFramerates() const;
        Q_INVOKABLE FracList &supportedFramerates();
        Q_INVOKABLE QStringList pixelFormats() const;
        Q_INVOKABLE QStringList &pixelFormats();

    private:
        QString m_name;
        QString m_longName;
        IntList m_supportedSamplerates;
        QStringList m_sampleFormats;
        QStringList m_channelLayouts;
        FracList m_supportedFramerates;
        QStringList m_pixelFormats;

    signals:
        void nameChanged(const QString &name);
        void longNameChanged(const QString &longName);
        void supportedSampleratesChanged(const IntList &supportedSamplerates);
        void sampleFormatsChanged(const QStringList &sampleFormats);
        void channelLayoutsChanged(const QStringList &channelLayouts);
        void supportedFrameratesChanged(const FracList &supportedFramerates);
        void pixelFormatsChanged(const QStringList &pixelFormats);

    public slots:
        void setName(const QString &name);
        void setLongName(const QString &longName);
        void setSupportedSamplerates(const IntList &supportedSamplerates);
        void setSampleFormats(const QStringList &sampleFormats);
        void setChannelLayouts(const QStringList &channelLayouts);
        void setSupportedFramerates(const FracList &supportedFramerates);
        void setPixelFormats(const QStringList &pixelFormats);
        void resetName();
        void resetLongName();
        void resetSupportedSamplerates();
        void resetSampleFormats();
        void resetChannelLayouts();
        void resetSupportedFramerates();
        void resetPixelFormats();
};

#endif // CODECINFO_H
