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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef BINELEMENT_H
#define BINELEMENT_H

#include "lexer_auto.h"
#include "parser_auto.h"
#include "pipeline.h"

class BinElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description
                                   WRITE setDescription
                                   RESET resetDescription)

    Q_PROPERTY(bool blocking READ blocking
                             WRITE setBlocking
                             RESET resetBlocking)

    Q_PROPERTY(ThreadsMap threads READ threads
                                  WRITE setThreads
                                  RESET resetThreads)

    public:
        explicit BinElement();

        Q_INVOKABLE QString description() const;
        Q_INVOKABLE bool blocking() const;
        Q_INVOKABLE ThreadsMap threads() const;
        Q_INVOKABLE QbElementPtr element(const QString &elementName);
        Q_INVOKABLE void add(QbElementPtr element);
        Q_INVOKABLE void remove(const QString &elementName);

        bool event(QEvent *event);

    private:
        QString m_description;
        bool m_blocking;
        ThreadsMap m_threads;
        QMap<QString, QbElementPtr> m_elements;
        QList<QbElementPtr> m_inputs;
        QList<QbElementPtr> m_outputs;
        Pipeline m_pipelineDescription;

    public slots:
        void setDescription(const QString &description);
        void setBlocking(bool blocking);
        void setThreads(const ThreadsMap &threads);
        void resetDescription();
        void resetBlocking();
        void resetThreads();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void connectOutputs();
        void disconnectOutputs();
};

#endif // BINELEMENT_H
