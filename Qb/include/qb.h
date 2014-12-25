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

#ifndef QB_H
#define QB_H

#include <QObject>
#include <QQmlApplicationEngine>

#include "qbplugin.h"
#include "qbelement.h"

class Qb: public QObject
{
    Q_OBJECT

    public:
        explicit Qb(QObject *parent=NULL);
        Qb(const Qb &other);

        static void init();
        static qint64 id();
        static bool qmlRegister(QQmlApplicationEngine *engine);

        Q_INVOKABLE QObject *newFrac() const;
        Q_INVOKABLE QObject *newFrac(qint64 num, qint64 den) const;
        Q_INVOKABLE QObject *newFrac(const QString &fracString) const;

        Q_INVOKABLE QObject *copy(const QbFrac &frac) const;
};

Q_DECLARE_METATYPE(Qb)

#endif // QB_H
