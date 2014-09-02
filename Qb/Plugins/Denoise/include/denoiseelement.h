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

#ifndef DENOISEELEMENT_H
#define DENOISEELEMENT_H

#include <QImage>
#include <opencv2/opencv.hpp>
#include <qb.h>

class DenoiseElement: public QbElement
{
    Q_OBJECT
        Q_PROPERTY(float strength READ strength WRITE setStrength RESET resetStrength)

        Q_PROPERTY(int templateWindowSize READ templateWindowSize
                                          WRITE setTemplateWindowSize
                                          RESET resetTemplateWindowSize)

        Q_PROPERTY(int searchWindowSize READ searchWindowSize
                                        WRITE setSearchWindowSize
                                        RESET resetSearchWindowSize)

    public:
        explicit DenoiseElement();
        Q_INVOKABLE float strength() const;
        Q_INVOKABLE int templateWindowSize() const;
        Q_INVOKABLE int searchWindowSize() const;

    private:
        float m_strength;
        int m_templateWindowSize;
        int m_searchWindowSize;

        QbElementPtr m_convert;

    public slots:
        void setStrength(float strength);
        void setTemplateWindowSize(int templateWindowSize);
        void setSearchWindowSize(int searchWindowSize);
        void resetStrength();
        void resetTemplateWindowSize();
        void resetSearchWindowSize();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // DENOISEELEMENT_H
