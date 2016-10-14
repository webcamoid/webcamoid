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

#include "multisinkelement.h"

class MultiSinkElementPrivate
{
    public:
        QStringList m_supportedFormats;
        QMap<QString, QStringList> m_fileExtensions;
        QMap<QString, QString> m_formatDescription;
        QStringList m_supportedCodecs;
        QMap<QString, QString> m_codecDescription;
        QMap<QString, QString> m_codecType;
        QMap<QString, QVariantMap> m_defaultCodecParams;

        MultiSinkElementPrivate()
        {
            MediaSink mediaSink;

            for (const QString &format: mediaSink.supportedFormats()) {
                this->m_supportedFormats << format;
                this->m_fileExtensions[format] = mediaSink.fileExtensions(format);
                this->m_formatDescription[format] = mediaSink.formatDescription(format);

                for (const QString &codec: mediaSink.supportedCodecs(format))
                    if (!this->m_supportedCodecs.contains(codec)) {
                        this->m_supportedCodecs << codec;
                        this->m_codecDescription[codec] = mediaSink.codecDescription(codec);
                        this->m_codecType[codec] = mediaSink.codecType(codec);
                        this->m_defaultCodecParams[codec] = mediaSink.defaultCodecParams(codec);
                    }
            }
        }

        QStringList supportedFormats() const
        {
            return this->m_supportedFormats;
        }

        QStringList fileExtensions(const QString &format) const
        {
            return this->m_fileExtensions.value(format, QStringList());
        }

        QString formatDescription(const QString &format) const
        {
            return this->m_formatDescription.value(format, QString());
        }

        QString codecDescription(const QString &codec) const
        {
            return this->m_codecDescription.value(codec, QString());
        }

        QString codecType(const QString &codec) const
        {
            return this->m_codecType.value(codec, QString());
        }

        QVariantMap defaultCodecParams(const QString &codec) const
        {
            return this->m_defaultCodecParams.value(codec, QVariantMap());
        }
};

Q_GLOBAL_STATIC(MultiSinkElementPrivate, multiSinkGlobalStuff)

MultiSinkElement::MultiSinkElement(): AkElement()
{
    this->m_showFormatOptions = false;

    QObject::connect(&this->m_mediaSink,
                     &MediaSink::locationChanged,
                     this,
                     &MultiSinkElement::locationChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::outputFormatChanged,
                     this,
                     &MultiSinkElement::outputFormatChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::formatOptionsChanged,
                     this,
                     &MultiSinkElement::formatOptionsChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::streamsChanged,
                     this,
                     &MultiSinkElement::streamsChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::streamUpdated,
                     this,
                     &MultiSinkElement::streamUpdated);
}

MultiSinkElement::~MultiSinkElement()
{
    this->m_mediaSink.uninit();
}

QObject *MultiSinkElement::controlInterface(QQmlEngine *engine,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/MultiSink/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("MultiSink", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QString MultiSinkElement::location() const
{
    return this->m_mediaSink.location();
}

QString MultiSinkElement::outputFormat() const
{
    return this->m_mediaSink.outputFormat();
}

QVariantMap MultiSinkElement::formatOptions() const
{
    return this->m_mediaSink.formatOptions();
}

QVariantList MultiSinkElement::streams() const
{
    return this->m_mediaSink.streams();
}

bool MultiSinkElement::showFormatOptions() const
{
    return this->m_showFormatOptions;
}

QStringList MultiSinkElement::supportedFormats()
{
    return multiSinkGlobalStuff->supportedFormats();
}

QStringList MultiSinkElement::fileExtensions(const QString &format)
{
    return multiSinkGlobalStuff->fileExtensions(format);
}

QString MultiSinkElement::formatDescription(const QString &format)
{
    return multiSinkGlobalStuff->formatDescription(format);
}

QStringList MultiSinkElement::supportedCodecs(const QString &format,
                                              const QString &type)
{
    return this->m_mediaSink.supportedCodecs(format, type);
}

QString MultiSinkElement::defaultCodec(const QString &format,
                                       const QString &type)
{
    return this->m_mediaSink.defaultCodec(format, type);
}

QString MultiSinkElement::codecDescription(const QString &codec)
{
    return multiSinkGlobalStuff->codecDescription(codec);
}

QString MultiSinkElement::codecType(const QString &codec)
{
    return multiSinkGlobalStuff->codecType(codec);
}

QVariantMap MultiSinkElement::defaultCodecParams(const QString &codec)
{
    return multiSinkGlobalStuff->defaultCodecParams(codec);
}

QVariantMap MultiSinkElement::addStream(int streamIndex,
                                        const AkCaps &streamCaps,
                                        const QVariantMap &codecParams)
{
    return this->m_mediaSink.addStream(streamIndex, streamCaps, codecParams);
}

QVariantMap MultiSinkElement::updateStream(int index,
                                           const QVariantMap &codecParams)
{
    return this->m_mediaSink.updateStream(index, codecParams);
}

void MultiSinkElement::stateChange(AkElement::ElementState from,
                                   AkElement::ElementState to)
{
    if (from == AkElement::ElementStateNull
        && to == AkElement::ElementStatePaused) {
        this->m_mediaSink.init();
    } else if (from == AkElement::ElementStatePaused
               && to == AkElement::ElementStateNull) {
        this->m_mediaSink.uninit();
    }
}

void MultiSinkElement::setLocation(const QString &location)
{
    this->m_mediaSink.setLocation(location);
}

void MultiSinkElement::setOutputFormat(const QString &outputFormat)
{
    this->m_mediaSink.setOutputFormat(outputFormat);
}

void MultiSinkElement::setFormatOptions(const QVariantMap &formatOptions)
{
    this->m_mediaSink.setFormatOptions(formatOptions);
}

void MultiSinkElement::setShowFormatOptions(bool showFormatOptions)
{
    if (this->m_showFormatOptions == showFormatOptions)
        return;

    this->m_showFormatOptions = showFormatOptions;
    emit this->showFormatOptionsChanged(showFormatOptions);
}

void MultiSinkElement::resetLocation()
{
    this->m_mediaSink.resetLocation();
}

void MultiSinkElement::resetOutputFormat()
{
    this->m_mediaSink.resetOutputFormat();
}

void MultiSinkElement::resetFormatOptions()
{
    this->m_mediaSink.resetFormatOptions();
}

void MultiSinkElement::resetShowFormatOptions()
{
    this->setShowFormatOptions(false);
}

void MultiSinkElement::clearStreams()
{
    this->m_mediaSink.clearStreams();
}

AkPacket MultiSinkElement::iStream(const AkPacket &packet)
{
    this->m_mediaSink.enqueuePacket(packet);

    return AkPacket();
}
