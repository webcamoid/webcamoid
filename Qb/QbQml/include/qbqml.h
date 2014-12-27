/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef QBQML_H
#define QBQML_H

#include <QQuickItem>
#include <qb.h>

class QbQml: public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(QbQml)

    public:
        QbQml(QQuickItem *parent=NULL);
        ~QbQml();

        Q_INVOKABLE void init() const;
        Q_INVOKABLE qint64 id() const;

        Q_INVOKABLE QObject *newFrac() const;
        Q_INVOKABLE QObject *newFrac(qint64 num, qint64 den) const;
        Q_INVOKABLE QObject *newFrac(const QString &fracString) const;
        Q_INVOKABLE QObject *newFrac(const QbFrac &frac) const;

        Q_INVOKABLE QVariant toVar(QbFrac *frac) const;
};

#endif // QBQML_H

