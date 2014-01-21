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

#include "qbthreadstock.h"

QbThreadStock::QbThreadStock(QObject *parent):
    QObject(parent)
{
}

QbThreadStock::~QbThreadStock()
{
    QStringList threads = this->m_threads.keys();

    foreach (QString threadName, threads)
        this->deleteInstance(threadName);
}

QStringList QbThreadStock::threadsList() const
{
    return this->m_threads.keys();
}

QbThreadPtr QbThreadStock::requestInstance(const QString &threadName)
{
    if (threadName.isEmpty())
        return QbThreadPtr();

    if (this->m_threads.contains(threadName))
        return this->m_threads[threadName];

    QbThreadPtr thread(new QbThread());

    thread->m_threadList = this;
    thread->setObjectName(threadName);
    thread->start();

    this->m_threads[threadName] = thread;

    return thread;
}

void QbThreadStock::deleteInstance(const QString &threadName)
{
    if (!this->m_threads.contains(threadName))
        return;

    this->m_threads[threadName]->m_threadList = NULL;
    this->m_threads[threadName]->quit();
    this->m_threads[threadName]->wait();
    this->m_threads.remove(threadName);
}

void QbThreadStock::setThread(const QString &threadName)
{
    this->m_currentThread = (threadName.isNull() || threadName == "MAIN")?
                                QbThreadPtr():
                                this->requestInstance(threadName);

    this->m_currentThreadName = threadName;
}

QbThreadPtr QbThreadStock::currentThread() const
{
    return this->m_currentThread;
}

QString QbThreadStock::currentThreadName() const
{
    return this->m_currentThreadName;
}
