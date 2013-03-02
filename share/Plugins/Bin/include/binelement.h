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

#ifndef BINELEMENT_H
#define BINELEMENT_H

#include "qb.h"
#include "lexer_auto.h"
#include "parser_auto.h"

class BinElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(RoutingMode)

    Q_PROPERTY(QString description READ description
                                   WRITE setDescription
                                   RESET resetDescription)

    Q_PROPERTY(RoutingMode routingMode READ routingMode
                                       WRITE setRoutingMode
                                       RESET resetRoutingMode)

    public:
        /// Actions to do if some element doesn't exist
        enum RoutingMode
        {
            RoutingModeNoCheck, // Build the pipeline as is.
            RoutingModeFail,    // If an element doesn't exist, create a void
                                // graph.
            RoutingModeRemove,  // If an element doesn't exist, create a graph
                                // without the element and it's connections.
            RoutingModeForce    // If an element doesn't exist try to connect
                                // all elements connected to the lost element.
        };

        explicit BinElement();
        ~BinElement();

        Q_INVOKABLE QString description() const;
        Q_INVOKABLE RoutingMode routingMode() const;

        Q_INVOKABLE QbElementPtr element(QString elementName);
        Q_INVOKABLE bool add(QbElementPtr element);
        Q_INVOKABLE bool remove(QString elementName);

    private:
        QString m_description;
        RoutingMode m_routingMode;

    public slots:
        void setDescription(QString description);
        void setRoutingMode(RoutingMode routingMode);
        void resetDescription();
        void resetRoutingMode();

        void iStream(const QbPacket &packet);
};

#endif // BINELEMENT_H
