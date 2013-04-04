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

#ifndef WARHOLELEMENT_H
#define WARHOLELEMENT_H

#include <QtGui>
#include <qb.h>

class WarholElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(int nFrames READ nFrames WRITE setNFrames RESET resetNFrames)

    public:
        explicit WarholElement();
        ~WarholElement();

        Q_INVOKABLE int nFrames() const;

    private:
        int m_nFrames;

        QbElementPtr m_convert;
        QImage m_oFrame;
        QList<quint32> m_colorTable;

    public slots:
        void setNFrames(int nFrames);
        void resetNFrames();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // WARHOLELEMENT_H
