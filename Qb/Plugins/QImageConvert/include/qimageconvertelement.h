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

#ifndef QIMAGECONVERTELEMENT_H
#define QIMAGECONVERTELEMENT_H

#include <QtGui>

#include <qb.h>

class QImageConvertElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString format READ format WRITE setFormat RESET resetFormat)

    public:
        explicit QImageConvertElement();

        Q_INVOKABLE QString format();

    private:
        QString m_format;
        QImage::Format m_qFormat;

        QbElementPtr m_capsConvert;

        QMap<QString, QString> m_imageToFormat;
        QMap<QString, QImage::Format> m_imageToQt;

    signals:
        void oStream(QSharedPointer<const QImage> frame);

    public slots:
        void setFormat(QString format);
        void resetFormat();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // QIMAGECONVERTELEMENT_H
