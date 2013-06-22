/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#ifndef QBFRAC_H
#define QBFRAC_H

#include <QtCore>

class QbFrac: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int num READ num WRITE setNum RESET resetNum)
    Q_PROPERTY(int den READ den WRITE setDen RESET resetDen)
    Q_PROPERTY(double value READ value)
    Q_PROPERTY(bool isValid READ isValid)

    public:
        explicit QbFrac(QObject *parent=NULL);
        QbFrac(qint64 num, qint64 den);
        QbFrac(QString fracString);
        QbFrac(const QbFrac &other);
        virtual ~QbFrac();
        QbFrac &operator =(const QbFrac &other);
        bool operator ==(const QbFrac &other) const;
        bool operator !=(const QbFrac &other) const;
        QbFrac operator *(const QbFrac &other) const;

        Q_INVOKABLE qint64 num() const;
        Q_INVOKABLE qint64 den() const;
        Q_INVOKABLE double value() const;
        Q_INVOKABLE bool isValid() const;
        Q_INVOKABLE QString toString() const;
        Q_INVOKABLE QbFrac invert() const;

    private:
        int m_num;
        int m_den;
        bool m_isValid;

        qint64 gcd() const;

        friend QDebug operator <<(QDebug debug, const QbFrac &frac);

    public slots:
        void reduce();
        void setNum(qint64 num);
        void setDen(qint64 den);
        void resetNum();
        void resetDen();
};

QDebug operator <<(QDebug debug, const QbFrac &frac);
QbFrac operator *(int number, const QbFrac &frac);
QbFrac operator /(int number, const QbFrac &frac);

Q_DECLARE_METATYPE(QbFrac)

#endif // QBFRAC_H
