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

#include "ui_imagedisplay.h"

#include "imagedisplay.h"

ImageDisplay::ImageDisplay(QWidget *parent):
    QWidget(parent),
    ui(new Ui::ImageDisplay)
{
    this->ui->setupUi(this);
}

ImageDisplay::~ImageDisplay()
{
}

QbPacket ImageDisplay::image() const
{
    return this->m_image;
}

void ImageDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QImage image(reinterpret_cast<uchar *>(this->m_image.buffer().data()),
                 this->m_image.caps().property("width").toInt(),
                 this->m_image.caps().property("height").toInt(),
                 QImage::Format_ARGB32);

    QSize size = image.size();
    size.scale(this->size(), Qt::KeepAspectRatio);
    QRect rect(QPoint(), size);
    rect.moveCenter(this->rect().center());

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, false);

    painter.beginNativePainting();

    painter.drawImage(rect,
                      image,
                      image.rect());

    painter.endNativePainting();
}

void ImageDisplay::setImage(const QbPacket &image)
{
    this->m_image = image;

    this->update();
}

void ImageDisplay::resetImage()
{
    this->setImage(QbPacket());
}
