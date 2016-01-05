/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef BINELEMENT_H
#define BINELEMENT_H

#include "pipeline.h"

class BinElement: public AkElement
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description
                                   WRITE setDescription
                                   RESET resetDescription)

    Q_PROPERTY(bool blocking READ blocking
                             WRITE setBlocking
                             RESET resetBlocking)

    public:
        explicit BinElement();

        Q_INVOKABLE QString description() const;
        Q_INVOKABLE bool blocking() const;
        Q_INVOKABLE AkElementPtr element(const QString &elementName);
        Q_INVOKABLE void add(AkElementPtr element);
        Q_INVOKABLE void remove(const QString &elementName);

    private:
        QString m_description;
        bool m_blocking;
        QMap<QString, AkElementPtr> m_elements;
        QList<AkElementPtr> m_inputs;
        QList<AkElementPtr> m_outputs;
        Pipeline m_pipelineDescription;

    public slots:
        void setDescription(const QString &description);
        void setBlocking(bool blocking);
        void resetDescription();
        void resetBlocking();

        AkPacket iStream(const AkPacket &packet);
        void setState(AkElement::ElementState state);

    private slots:
        void connectOutputs();
        void disconnectOutputs();
};

#endif // BINELEMENT_H
