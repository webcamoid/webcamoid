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

#include "multisinkelement.h"

MultiSinkElement::MultiSinkElement(): QbElement()
{
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::locationChanged,
                     this,
                     &MultiSinkElement::locationChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::outputFormatChanged,
                     this,
                     &MultiSinkElement::outputFormatChanged);
    QObject::connect(&this->m_mediaSink,
                     &MediaSink::streamsChanged,
                     this,
                     &MultiSinkElement::streamsChanged);
}

MultiSinkElement::~MultiSinkElement()
{
    this->m_mutex.lock();
    this->m_mediaSink.uninit();
    this->m_mutex.unlock();
}

QObject *MultiSinkElement::controlInterface(QQmlEngine *engine,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/MultiSink/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("MultiSink", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
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

QVariantList MultiSinkElement::streams() const
{
    return this->m_mediaSink.streams();
}

QStringList MultiSinkElement::supportedFormats()
{
    return this->m_mediaSink.supportedFormats();
}

QStringList MultiSinkElement::fileExtensions(const QString &format)
{
    return this->m_mediaSink.fileExtensions(format);
}

QString MultiSinkElement::formatDescription(const QString &format)
{
    return this->m_mediaSink.formatDescription(format);
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
    return this->m_mediaSink.codecDescription(codec);
}

QString MultiSinkElement::codecType(const QString &codec)
{
    return this->m_mediaSink.codecType(codec);
}

QVariantMap MultiSinkElement::defaultCodecParams(const QString &codec)
{
    return this->m_mediaSink.defaultCodecParams(codec);
}

QVariantMap MultiSinkElement::addStream(int streamIndex,
                                        const QbCaps &streamCaps,
                                        const QVariantMap &codecParams)
{
    return this->m_mediaSink.addStream(streamIndex, streamCaps, codecParams);
}

void MultiSinkElement::stateChange(QbElement::ElementState from,
                                   QbElement::ElementState to)
{
    this->m_mutex.lock();

    if (from == QbElement::ElementStateNull
        && to == QbElement::ElementStatePaused) {
        this->m_mediaSink.init();
    } else if (from == QbElement::ElementStatePaused
               && to == QbElement::ElementStateNull) {
        this->m_mediaSink.uninit();
    }

    this->m_mutex.unlock();
}

void MultiSinkElement::setLocation(const QString &location)
{
    return this->m_mediaSink.setLocation(location);
}

void MultiSinkElement::setOutputFormat(const QString &outputFormat)
{
    return this->m_mediaSink.setOutputFormat(outputFormat);
}

void MultiSinkElement::resetLocation()
{
    return this->m_mediaSink.resetLocation();
}

void MultiSinkElement::resetOutputFormat()
{
    return this->m_mediaSink.resetOutputFormat();
}

void MultiSinkElement::clearStreams()
{
    return this->m_mediaSink.clearStreams();
}

QbPacket MultiSinkElement::iStream(const QbPacket &packet)
{
    this->m_mutex.lock();

    if (packet.caps().mimeType() == "audio/x-raw")
        this->m_mediaSink.writeAudioPacket(packet);
    else if (packet.caps().mimeType() == "video/x-raw")
        this->m_mediaSink.writeVideoPacket(packet);
    else if (packet.caps().mimeType() == "text/x-raw")
        this->m_mediaSink.writeSubtitlePacket(packet);

    this->m_mutex.unlock();

    return QbPacket();
}
