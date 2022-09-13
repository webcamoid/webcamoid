/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QDataStream>
#include <QDebug>
#include <QMetaEnum>
#include <QQmlEngine>
#include <QRect>

#include "aksubtitlecaps.h"
#include "akcaps.h"

class AkSubtitleCapsPrivate
{
    public:
        AkSubtitleCaps::SubtitleFormat m_format {AkSubtitleCaps::SubtitleFormat_none};
        QRect m_rect;
};

AkSubtitleCaps::AkSubtitleCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkSubtitleCapsPrivate();
}

AkSubtitleCaps::AkSubtitleCaps(SubtitleFormat format):
    QObject()
{
    this->d = new AkSubtitleCapsPrivate();
    this->d->m_format = format;
}

AkSubtitleCaps::AkSubtitleCaps(SubtitleFormat format,
                               const QRect &rect):
    QObject()
{
    this->d = new AkSubtitleCapsPrivate();
    this->d->m_format = format;
    this->d->m_rect = rect;
}

AkSubtitleCaps::AkSubtitleCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkSubtitleCapsPrivate();

    if (other.type() == AkCaps::CapsSubtitle) {
        auto data = reinterpret_cast<AkSubtitleCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_rect = data->d->m_rect;
    }
}

AkSubtitleCaps::AkSubtitleCaps(const AkSubtitleCaps &other):
    QObject()
{
    this->d = new AkSubtitleCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_rect = other.d->m_rect;
}

AkSubtitleCaps::~AkSubtitleCaps()
{
    delete this->d;
}

AkSubtitleCaps &AkSubtitleCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsSubtitle) {
        auto data = reinterpret_cast<AkSubtitleCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_rect = data->d->m_rect;
    } else {
        this->d->m_format = SubtitleFormat_none;
        this->d->m_rect = {};
    }

    return *this;
}

AkSubtitleCaps &AkSubtitleCaps::operator =(const AkSubtitleCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_rect = other.d->m_rect;
    }

    return *this;
}

bool AkSubtitleCaps::operator ==(const AkSubtitleCaps &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_rect == other.d->m_rect;
}

bool AkSubtitleCaps::operator !=(const AkSubtitleCaps &other) const
{
    return !(*this == other);
}

QObject *AkSubtitleCaps::create()
{
    return new AkSubtitleCaps();
}

QObject *AkSubtitleCaps::create(const AkSubtitleCaps &caps)
{
    return new AkSubtitleCaps(caps);
}

QObject *AkSubtitleCaps::create(SubtitleFormat format)
{
    return new AkSubtitleCaps(format);
}

QObject *AkSubtitleCaps::create(AkSubtitleCaps::SubtitleFormat format,
                                const QRect &rect)
{
    return new AkSubtitleCaps(format, rect);
}

QVariant AkSubtitleCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkSubtitleCaps::operator bool() const
{
    return this->d->m_format != SubtitleFormat_none
           && !this->d->m_rect.isEmpty();
}

AkSubtitleCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsSubtitle);
    caps.setPrivateData(new AkSubtitleCaps(*this),
                        [] (void *data) -> void * {
                            return new AkSubtitleCaps(*reinterpret_cast<AkSubtitleCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkSubtitleCaps *>(data);
                        });

    return caps;
}

AkSubtitleCaps::SubtitleFormat AkSubtitleCaps::format() const
{
    return this->d->m_format;
}

QRect AkSubtitleCaps::rect() const
{
    return this->d->m_rect;
}

QString AkSubtitleCaps::formatToString(SubtitleFormat subtitleFormat)
{
    AkSubtitleCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SubtitleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(subtitleFormat));
    format.remove("SubtitleFormat_");

    return format;
}

AkSubtitleCaps::SubtitleFormat AkSubtitleCaps::formatFromString(const QString &subtitleFormat)
{
    AkSubtitleCaps caps;
    QString format = "SubtitleFormat_" + subtitleFormat;
    int formatIndex = caps.metaObject()->indexOfEnumerator("SubtitleFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    int formatInt = formatEnum.keyToValue(format.toStdString().c_str());

    return static_cast<SubtitleFormat>(formatInt);
}

void AkSubtitleCaps::setFormat(SubtitleFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkSubtitleCaps::setRect(const QRect &rect)
{
    if (this->d->m_rect == rect)
        return;

    this->d->m_rect = rect;
    emit this->rectChanged(rect);
}

void AkSubtitleCaps::resetFormat()
{
    this->setFormat(SubtitleFormat_none);
}

void AkSubtitleCaps::resetRect()
{
    this->setRect({});
}

void AkSubtitleCaps::registerTypes()
{
    qRegisterMetaType<AkSubtitleCaps>("AkSubtitleCaps");
    qRegisterMetaTypeStreamOperators<AkSubtitleCaps>("AkSubtitleCaps");
    qRegisterMetaType<SubtitleFormat>("SubtitleFormat");
    QMetaType::registerDebugStreamOperator<SubtitleFormat>();
    qmlRegisterSingletonType<AkSubtitleCaps>("Ak", 1, 0, "AkSubtitleCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkSubtitleCaps();
    });
}

QDebug operator <<(QDebug debug, const AkSubtitleCaps &caps)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "AkSubtitleCaps("
                    << "format="
                    << caps.format()
                    << ",rect="
                    << caps.rect()
                    << ")";

    return debug;
}

QDebug operator <<(QDebug debug, AkSubtitleCaps::SubtitleFormat format)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << AkSubtitleCaps::formatToString(format).toStdString().c_str();

    return debug;
}

QDataStream &operator >>(QDataStream &istream, AkSubtitleCaps &caps)
{
    AkSubtitleCaps::SubtitleFormat format = AkSubtitleCaps::SubtitleFormat_none;
    istream >> format;
    caps.setFormat(format);
    QRect rect;
    istream >> rect;
    caps.setRect(rect);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkSubtitleCaps &caps)
{
    ostream << caps.format();
    ostream << caps.rect();

    return ostream;
}

#include "moc_aksubtitlecaps.cpp"
