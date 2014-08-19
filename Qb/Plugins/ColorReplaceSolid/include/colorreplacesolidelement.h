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

#ifndef COLORREPLACESOLIDELEMENT_H
#define COLORREPLACESOLIDELEMENT_H

#include <QImage>
#include <qrgb.h>

#include <qb.h>

class ColorReplaceSolidElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QVariantList table READ table WRITE setTable RESET resetTable)

    public:
        explicit ColorReplaceSolidElement();

        Q_INVOKABLE QVariantList table() const;

    private:
        QList<QRgb> m_table;

        QbElementPtr m_convert;

    public slots:
        void setTable(const QVariantList &table);
        void resetTable();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // COLORREPLACESOLIDELEMENT_H
