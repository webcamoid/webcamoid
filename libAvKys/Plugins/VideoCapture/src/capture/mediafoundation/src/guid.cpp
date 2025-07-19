/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include <QRegularExpression>

#include "guid.h"

struct GuidData
{
    quint32 data1 {0};
    quint16 data2 {0};
    quint16 data3 {0};
    quint8  data4[8];
};

class GuidPrivate
{
    public:
        GuidData m_data;
};

Guid::Guid(QObject *parent):
    QObject(parent)
{
    this->d = new GuidPrivate;
    memset(this->d->m_data.data4, 0, 8 * sizeof(quint8));
}

Guid::Guid(const QByteArray &data):
    QObject()
{
    this->d = new GuidPrivate;
    memset(this->d->m_data.data4, 0, 8 * sizeof(quint8));

    if (!data.isEmpty())
        memcpy(&this->d->m_data.data1,
               data.constData(),
               qMin<qsizetype>(data.size(), sizeof(GuidData)));
}

Guid::Guid(const char *data, size_t len):
    QObject()
{
    this->d = new GuidPrivate;
    memset(this->d->m_data.data4, 0, 8 * sizeof(quint8));

    if (len < 0)
        memcpy(&this->d->m_data.data1, data, sizeof(GuidData));

    if (len > 0)
        memcpy(&this->d->m_data.data1, data, qMin(len, sizeof(GuidData)));
}

Guid::Guid(const Guid &other):
    QObject()
{
    this->d = new GuidPrivate;
    memcpy(&this->d->m_data.data1,
           &other.d->m_data.data1,
           16 * sizeof(quint8));
}

Guid::~Guid()
{
    delete this->d;
}

Guid &Guid::operator =(const Guid &other)
{
    if (this != &other) {
        memcpy(&this->d->m_data.data1,
               &other.d->m_data.data1,
               16 * sizeof(quint8));
    }

    return *this;
}

bool Guid::operator ==(const Guid &other) const
{
    return memcmp(&this->d->m_data.data1,
                  &other.d->m_data.data1,
                  16 * sizeof(quint8)) == 0;
}

bool Guid::operator <(const Guid &other) const
{
    return memcmp(&this->d->m_data.data1,
                  &other.d->m_data.data1,
                  16 * sizeof(quint8)) < 0;
}

Guid::operator bool() const
{
    auto d = reinterpret_cast<const quint8 *>(&this->d->m_data.data1);

    for (int i = 0; i < 16; i++)
        if (d[i])
            return true;

    return false;
}

Guid::operator QString() const
{
    return this->toString();
}

QString Guid::toString() const
{
    QString guid;
    QTextStream ts(&guid);
    ts << "{";
    ts << QString("%1-").arg(this->d->m_data.data1, 8, 16);
    ts << QString("%1-").arg(this->d->m_data.data2, 4, 16);
    ts << QString("%1-").arg(this->d->m_data.data3, 4, 16);

    for (int i = 0; i < 8; i++) {
        if (i == 2)
            ts << "-";

        ts << QString("%1").arg(this->d->m_data.data4[i], 2, 16);
    }

    ts << "}";

    return guid.replace(' ', '0');
}

Guid Guid::fromString(const QString &str)
{
    QRegularExpression re("^{[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}}$");

    if (!re.match(str).hasMatch())
        return {};

    auto cstr = str;
    cstr = cstr.remove('{').remove('-').remove('}');
    Guid guid;

    guid.d->m_data.data1 = cstr.mid(0, 8).toUInt(nullptr, 16);
    guid.d->m_data.data2 = cstr.mid(8, 4).toUShort(nullptr, 16);
    guid.d->m_data.data3 = cstr.mid(12, 4).toUShort(nullptr, 16);

    for (int i = 0; i < 8; i++)
        guid.d->m_data.data4[i] =
                cstr.mid(2 * (i + 8), 2).toUShort(nullptr, 16);

    return guid;
}

quint8 *Guid::data() const
{
    return reinterpret_cast<quint8 *>(&this->d->m_data.data1);
}

const quint8 *Guid::constData() const
{
    return reinterpret_cast<quint8 *>(&this->d->m_data.data1);
}

QDebug operator <<(QDebug debug, const Guid &guid)
{
    debug.nospace() << guid.toString();

    return debug.space();
}

#include "moc_guid.cpp"
