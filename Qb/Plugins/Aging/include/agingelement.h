/* Webcamoid, webcam capture application.
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

#ifndef AGINGELEMENT_H
#define AGINGELEMENT_H

#include <QtGui>
#include <qb.h>

#include "scratch.h"

class AgingElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int nScratches READ nScratches WRITE setNScratches RESET resetNScratches)
    Q_PROPERTY(int scratchLines READ scratchLines WRITE setScratchLines RESET resetScratchLines)
    Q_PROPERTY(int agingMode READ agingMode WRITE setAgingMode RESET resetAgingMode)

    public:
        explicit AgingElement();

        Q_INVOKABLE int nScratches() const;
        Q_INVOKABLE int scratchLines() const;
        Q_INVOKABLE int agingMode() const;
        bool event(QEvent *event);

    private:
        int m_scratchLines;
        int m_agingMode;

        QbElementPtr m_convert;
        QVector<Scratch> m_scratches;
        QList<int> m_dx;
        QList<int> m_dy;

        QImage colorAging(const QImage &src);
        void scratching(QImage &dest);
        void pits(QImage &dest);
        void dusts(QImage &dest);

    public slots:
        void setNScratches(int nScratches);
        void setScratchLines(int scratchLines);
        void setAgingMode(int agingMode);
        void resetNScratches();
        void resetScratchLines();
        void resetAgingMode();

        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // AGINGELEMENT_H
