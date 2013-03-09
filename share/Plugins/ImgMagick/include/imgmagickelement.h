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

#ifndef IMGMAGICKELEMENT_H
#define IMGMAGICKELEMENT_H

#include <Magick++.h>

#include "qb.h"

class ImgMagickElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString method READ method WRITE setMethod RESET resetMethod)
    Q_PROPERTY(QVariantList params READ params WRITE setParams RESET resetParams)

    public:
        explicit ImgMagickElement();
        ~ImgMagickElement();

        Q_INVOKABLE QString method() const;
        Q_INVOKABLE QVariantList params() const;

    private:
        QString m_method;
        QVariantList m_params;
        QbElementPtr m_capsConvert;
        Magick::Image m_magickimage;

    public slots:
        void setMethod(QString method);
        void setParams(QVariantList params);
        void resetMethod();
        void resetParams();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // IMGMAGICKELEMENT_H
