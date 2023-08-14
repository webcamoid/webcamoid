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

#include <QMutex>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "multisinkelement.h"
#include "mediawriter.h"

using MediaWriterPtr = QSharedPointer<MediaWriter>;

class MultiSinkElementPrivate
{
    public:
        MultiSinkElement *self;
        QString m_location;
        MediaWriterPtr m_mediaWriter;
        QString m_mediaWriterImpl;
        QList<int> m_inputStreams;
        QReadWriteLock m_mutex;

        // Formats and codecs info cache.
        QStringList m_supportedFormats;
        QMap<QString, QStringList> m_fileExtensions;
        QMap<QString, QString> m_formatDescription;
        QStringList m_supportedCodecs;
        QMap<QString, QString> m_codecDescription;
        QMap<QString, AkCaps::CapsType> m_codecType;
        QMap<QString, QVariantMap> m_defaultCodecParams;

        explicit MultiSinkElementPrivate(MultiSinkElement *self);
        void linksChanged(const AkPluginLinks &links);
};

MultiSinkElement::MultiSinkElement():
    AkElement()
{
    this->d = new MultiSinkElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });

    if (this->d->m_mediaWriter) {
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::locationChanged,
                         this,
                         &MultiSinkElement::locationChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::defaultFormatChanged,
                         this,
                         &MultiSinkElement::defaultFormatChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::outputFormatChanged,
                         this,
                         &MultiSinkElement::outputFormatChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::formatOptionsChanged,
                         this,
                         &MultiSinkElement::formatOptionsChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::codecOptionsChanged,
                         this,
                         &MultiSinkElement::codecOptionsChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::streamsChanged,
                         this,
                         &MultiSinkElement::streamsChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::formatsBlackListChanged,
                         this,
                         &MultiSinkElement::formatsBlackListChanged);
        QObject::connect(this->d->m_mediaWriter.data(),
                         &MediaWriter::codecsBlackListChanged,
                         this,
                         &MultiSinkElement::formatsBlackListChanged);
        QObject::connect(this,
                         &MultiSinkElement::locationChanged,
                         this->d->m_mediaWriter.data(),
                         &MediaWriter::setLocation);
        QObject::connect(this,
                         &MultiSinkElement::formatOptionsChanged,
                         this->d->m_mediaWriter.data(),
                         &MediaWriter::setFormatOptions);

        for (auto &format: this->d->m_mediaWriter->supportedFormats()) {
            this->d->m_supportedFormats << format;
            this->d->m_fileExtensions[format] =
                    this->d->m_mediaWriter->fileExtensions(format);
            this->d->m_formatDescription[format] =
                    this->d->m_mediaWriter->formatDescription(format);

            for (auto &codec: this->d->m_mediaWriter->supportedCodecs(format))
                if (!this->d->m_supportedCodecs.contains(codec)) {
                    this->d->m_supportedCodecs << codec;
                    this->d->m_codecDescription[codec] =
                            this->d->m_mediaWriter->codecDescription(codec);
                    this->d->m_codecType[codec] =
                            this->d->m_mediaWriter->codecType(codec);
                    this->d->m_defaultCodecParams[codec] =
                            this->d->m_mediaWriter->defaultCodecParams(codec);
                }
        }
    }
}

MultiSinkElement::~MultiSinkElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QString MultiSinkElement::location() const
{
    return this->d->m_location;
}

QString MultiSinkElement::defaultFormat() const
{
    this->d->m_mutex.lockForRead();
    QString defaultFormat;

    if (this->d->m_mediaWriter)
        defaultFormat = this->d->m_mediaWriter->defaultFormat();

    this->d->m_mutex.unlock();

    return defaultFormat;
}

QStringList MultiSinkElement::supportedFormats() const
{
    return this->d->m_supportedFormats;
}

QString MultiSinkElement::outputFormat() const
{
    this->d->m_mutex.lockForRead();
    QString outputFormat;

    if (this->d->m_mediaWriter)
        outputFormat = this->d->m_mediaWriter->outputFormat();

    this->d->m_mutex.unlock();

    return outputFormat;
}

QVariantList MultiSinkElement::streams()
{
    this->d->m_mutex.lockForRead();
    QVariantList streams;

    if (this->d->m_mediaWriter)
        streams = this->d->m_mediaWriter->streams();

    this->d->m_mutex.unlock();

    return streams;
}

QStringList MultiSinkElement::formatsBlackList() const
{
    this->d->m_mutex.lockForRead();
    QStringList formatsBlackList;

    if (this->d->m_mediaWriter)
        formatsBlackList = this->d->m_mediaWriter->formatsBlackList();

    this->d->m_mutex.unlock();

    return formatsBlackList;
}

QStringList MultiSinkElement::codecsBlackList() const
{
    this->d->m_mutex.lockForRead();
    QStringList codecsBlackList;

    if (this->d->m_mediaWriter)
        codecsBlackList = this->d->m_mediaWriter->codecsBlackList();

    this->d->m_mutex.unlock();

    return codecsBlackList;
}

QStringList MultiSinkElement::fileExtensions(const QString &format) const
{
    return this->d->m_fileExtensions.value(format);
}

QString MultiSinkElement::formatDescription(const QString &format) const
{
    return this->d->m_formatDescription.value(format);
}

QVariantList MultiSinkElement::formatOptions() const
{
    this->d->m_mutex.lockForRead();
    QVariantList formatOptions;

    if (this->d->m_mediaWriter)
        formatOptions = this->d->m_mediaWriter->formatOptions();

    this->d->m_mutex.unlock();

    return formatOptions;
}

QStringList MultiSinkElement::supportedCodecs(const QString &format,
                                              AkCaps::CapsType type)
{
    this->d->m_mutex.lockForRead();
    QStringList supportedCodecs;

    if (this->d->m_mediaWriter)
        supportedCodecs = this->d->m_mediaWriter->supportedCodecs(format, type);

    this->d->m_mutex.unlock();

    return supportedCodecs;
}

QString MultiSinkElement::defaultCodec(const QString &format,
                                       AkCaps::CapsType type)
{
    this->d->m_mutex.lockForRead();
    QString defaultCodec;

    if (this->d->m_mediaWriter)
        defaultCodec = this->d->m_mediaWriter->defaultCodec(format, type);

    this->d->m_mutex.unlock();

    return defaultCodec;
}

QString MultiSinkElement::codecDescription(const QString &codec) const
{
    return this->d->m_codecDescription.value(codec);
}

AkCaps::CapsType MultiSinkElement::codecType(const QString &codec) const
{
    return this->d->m_codecType.value(codec);
}

QVariantMap MultiSinkElement::defaultCodecParams(const QString &codec) const
{
    return this->d->m_defaultCodecParams.value(codec);
}

QVariantMap MultiSinkElement::addStream(int streamIndex,
                                        const AkCaps &streamCaps,
                                        const QVariantMap &codecParams)
{
    QVariantMap stream;
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        stream = this->d->m_mediaWriter->addStream(streamIndex,
                                                   streamCaps,
                                                   codecParams);

    this->d->m_mutex.unlock();

    if (!stream.isEmpty())
        this->d->m_inputStreams << streamIndex;

    return stream;
}

QVariantMap MultiSinkElement::updateStream(int index,
                                           const QVariantMap &codecParams)
{
    QVariantMap stream;
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        stream = this->d->m_mediaWriter->updateStream(index, codecParams);

    this->d->m_mutex.unlock();

    return stream;
}

QVariantList MultiSinkElement::codecOptions(int index)
{
    QVariantList options;
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        options = this->d->m_mediaWriter->codecOptions(index);

    this->d->m_mutex.unlock();

    return options;
}

void MultiSinkElement::setLocation(const QString &location)
{
    if (this->d->m_location == location)
        return;

    this->d->m_location = location;
    emit this->locationChanged(location);
}

void MultiSinkElement::setOutputFormat(const QString &outputFormat)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->setOutputFormat(outputFormat);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::setFormatOptions(const QVariantMap &formatOptions)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->setFormatOptions(formatOptions);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::setCodecOptions(int index,
                                       const QVariantMap &codecOptions)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->setCodecOptions(index, codecOptions);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::setFormatsBlackList(const QStringList &formatsBlackList)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->setFormatsBlackList(formatsBlackList);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::setCodecsBlackList(const QStringList &codecsBlackList)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->setCodecsBlackList(codecsBlackList);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::resetLocation()
{
    this->setLocation("");
}

void MultiSinkElement::resetOutputFormat()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->resetOutputFormat();

    this->d->m_mutex.unlock();
}

void MultiSinkElement::resetFormatOptions()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->resetFormatOptions();

    this->d->m_mutex.unlock();
}

void MultiSinkElement::resetCodecOptions(int index)
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->resetCodecOptions(index);

    this->d->m_mutex.unlock();
}

void MultiSinkElement::resetFormatsBlackList()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->resetFormatsBlackList();

    this->d->m_mutex.unlock();
}

void MultiSinkElement::resetCodecsBlackList()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->resetCodecsBlackList();

    this->d->m_mutex.unlock();
}

void MultiSinkElement::clearStreams()
{
    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter)
        this->d->m_mediaWriter->clearStreams();

    this->d->m_mutex.unlock();
    this->d->m_inputStreams.clear();
}

AkPacket MultiSinkElement::iStream(const AkPacket &packet)
{
    if (this->state() != ElementStatePlaying)
        return AkPacket();

    this->d->m_mutex.lockForRead();

    if (this->d->m_mediaWriter && this->d->m_inputStreams.contains(packet.index()))
        this->d->m_mediaWriter->enqueuePacket(packet);

    this->d->m_mutex.unlock();

    return AkPacket();
}

bool MultiSinkElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_mediaWriter)
        return false;

    AkElement::ElementState curState = this->state();

    if (curState == AkElement::ElementStateNull) {
        if (state != AkElement::ElementStateNull
            && (!this->d->m_mediaWriter->init())) {
            return false;
        }
    } else {
        if (state == AkElement::ElementStateNull)
            this->d->m_mediaWriter->uninit();
    }

    return AkElement::setState(state);
}

MultiSinkElementPrivate::MultiSinkElementPrivate(MultiSinkElement *self):
    self(self)
{
    this->m_mediaWriter =
            akPluginManager->create<MediaWriter>("MultimediaSink/MultiSink/Impl/*");
    this->m_mediaWriterImpl =
            akPluginManager->defaultPlugin("MultimediaSink/MultiSink/Impl/*",
                                           {"MultiSinkImpl"}).id();
}

void MultiSinkElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("MultimediaSink/MultiSink/Impl/*")
        || links["MultimediaSink/MultiSink/Impl/*"] == this->m_mediaWriterImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutex.lockForWrite();
    QString location;

    if (this->m_mediaWriter)
        location = this->m_mediaWriter->location();

    this->m_mediaWriter =
            akPluginManager->create<MediaWriter>("MultimediaSink/MultiSink/Impl/*");
    this->m_mutex.unlock();

    this->m_mediaWriterImpl = links["MultimediaSink/MultiSink/Impl/*"];

    if (!this->m_mediaWriter)
        return;

    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::locationChanged,
                     self,
                     &MultiSinkElement::locationChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::defaultFormatChanged,
                     self,
                     &MultiSinkElement::defaultFormatChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::outputFormatChanged,
                     self,
                     &MultiSinkElement::outputFormatChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::formatOptionsChanged,
                     self,
                     &MultiSinkElement::formatOptionsChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::codecOptionsChanged,
                     self,
                     &MultiSinkElement::codecOptionsChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::streamsChanged,
                     self,
                     &MultiSinkElement::streamsChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::formatsBlackListChanged,
                     self,
                     &MultiSinkElement::formatsBlackListChanged);
    QObject::connect(this->m_mediaWriter.data(),
                     &MediaWriter::codecsBlackListChanged,
                     self,
                     &MultiSinkElement::formatsBlackListChanged);
    QObject::connect(self,
                     &MultiSinkElement::locationChanged,
                     this->m_mediaWriter.data(),
                     &MediaWriter::setLocation);
    QObject::connect(self,
                     &MultiSinkElement::formatOptionsChanged,
                     this->m_mediaWriter.data(),
                     &MediaWriter::setFormatOptions);

    this->m_supportedFormats.clear();
    this->m_fileExtensions.clear();
    this->m_formatDescription.clear();
    this->m_supportedCodecs.clear();
    this->m_codecDescription.clear();
    this->m_codecType.clear();
    this->m_defaultCodecParams.clear();

    for (auto &format: this->m_mediaWriter->supportedFormats()) {
        this->m_supportedFormats << format;
        this->m_fileExtensions[format] =
                this->m_mediaWriter->fileExtensions(format);
        this->m_formatDescription[format] =
                this->m_mediaWriter->formatDescription(format);

        for (auto &codec: this->m_mediaWriter->supportedCodecs(format))
            if (!this->m_supportedCodecs.contains(codec)) {
                this->m_supportedCodecs << codec;
                this->m_codecDescription[codec] =
                        this->m_mediaWriter->codecDescription(codec);
                this->m_codecType[codec] =
                        this->m_mediaWriter->codecType(codec);
                this->m_defaultCodecParams[codec] =
                        this->m_mediaWriter->defaultCodecParams(codec);
            }
    }

    this->m_mediaWriter->setLocation(location);
    emit self->supportedFormatsChanged(self->supportedFormats());

    self->setState(state);
}

#include "moc_multisinkelement.cpp"
