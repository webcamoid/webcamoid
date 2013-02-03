/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "qbfrac.h"

QbFrac::QbFrac(QObject *parent): QObject(parent)
{
    this->m_num = 0;
    this->m_den = 0;
    this->m_isValid = false;
}

QbFrac::QbFrac(QString fracString)
{
    this->m_num = 0;
    this->m_den = 0;

    this->m_isValid = QRegExp("\\s*\\d+\\s*/"
                              "\\s*\\d+\\s*").exactMatch(fracString);

    if (!this->m_isValid)
        return;

    QStringList fracChunks = fracString.split(QRegExp("\\s*/\\s*"),
                                              QString::SkipEmptyParts);

    this->m_num = fracChunks[0].trimmed().toInt();
    this->m_den = fracChunks[0].trimmed().toInt();

    if (!this->m_den)
        this->m_isValid = false;
}

QbFrac::QbFrac(const QbFrac &other):
    QObject(NULL),
    m_num(other.m_num),
    m_den(other.m_den),
    m_isValid(other.m_isValid)
{
}

QbFrac &QbFrac::operator =(const QbFrac &other)
{
    if (this != &other)
    {
        this->m_num = other.m_num;
        this->m_den = other.m_den;
        this->m_isValid = other.m_isValid;
    }

    return *this;
}

bool QbFrac::operator ==(const QbFrac &other) const
{
    if (this->toString() == other.toString())
        return true;

    return false;
}

bool QbFrac::operator !=(const QbFrac &other) const
{
    return !(*this == other);
}

int QbFrac::num() const
{
    return this->m_num;
}

int QbFrac::den() const
{
    return this->m_den;
}

bool QbFrac::isValid() const
{
    return this->m_isValid;
}

QString QbFrac::toString() const
{
    return QString("%1/%2").arg(this->m_num)
                           .arg(this->m_den);
}

void QbFrac::setNum(int num)
{
    this->m_num = num;
}

void QbFrac::setDen(int den)
{
    this->m_den = den;

    if (!this->m_den)
        this->m_isValid = false;
}

void QbFrac::resetNum()
{
    this->setNum(0);
}

void QbFrac::resetDen()
{
    this->setDen(0);
}
