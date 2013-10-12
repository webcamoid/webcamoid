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

#ifndef QBTHREADSTOCK_H
#define QBTHREADSTOCK_H

#include "qbthread.h"

class QbThreadStock: public QObject
{
    Q_OBJECT

    public:
        explicit QbThreadStock(QObject *parent=NULL);
        ~QbThreadStock();

        Q_INVOKABLE QbThreadPtr requestInstance(const QString &threadName);
        Q_INVOKABLE void deleteInstance(const QString &threadName);
        Q_INVOKABLE QbThreadPtr findThread(const QThread *thread) const;
        Q_INVOKABLE void setThread(const QString &threadName);
        Q_INVOKABLE QbThreadPtr currentThread() const;

    private:
        QMap<QString, QbThreadPtr> m_threads;
        QbThreadPtr m_currentThread;
        QString m_currentThreadName;
};

#endif // QBTHREADSTOCK_H
