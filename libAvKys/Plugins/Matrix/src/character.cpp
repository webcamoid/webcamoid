/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
 * Web-Site: http://webcamoid.github.io/
 */

#include <QPainter>
#include <akfrac.h>
#include <akvideopacket.h>

#include "character.h"

class CharacterPrivate
{
    public:
        QChar m_chr;
        AkVideoPacket m_image;
        int m_weight {0};

        AkVideoPacket drawChar(const QChar &chr,
                               const QFont &font,
                               const QSize &fontSize) const;
        int imageWeight(const AkVideoPacket &image) const;
};

Character::Character()
{
    this->d = new CharacterPrivate;
}

Character::Character(const QChar &chr, const QFont &font, const QSize &fontSize)
{
    this->d = new CharacterPrivate;
    this->d->m_chr = chr;
    this->d->m_image = this->d->drawChar(chr, font, fontSize);
    this->d->m_weight = this->d->imageWeight(this->d->m_image);
}

Character::Character(const Character &other)
{
    this->d = new CharacterPrivate;
    this->d->m_chr = other.d->m_chr;
    this->d->m_image = other.d->m_image;
    this->d->m_weight = other.d->m_weight;
}

Character::~Character()
{
    delete this->d;
}

Character &Character::operator =(const Character &other)
{
    if (this != &other) {
        this->d->m_chr = other.d->m_chr;
        this->d->m_image = other.d->m_image;
        this->d->m_weight = other.d->m_weight;
    }

    return *this;
}

QChar Character::chr() const
{
    return this->d->m_chr;
}

const AkVideoPacket &Character::image() const
{
    return this->d->m_image;
}

int Character::weight() const
{
    return this->d->m_weight;
}

AkVideoPacket CharacterPrivate::drawChar(const QChar &chr,
                                         const QFont &font,
                                         const QSize &fontSize) const
{
    QImage fontImg(fontSize, QImage::Format_Grayscale8);
    fontImg.fill(qRgb(0, 0, 0));

    QPainter painter;
    painter.begin(&fontImg);
    painter.setPen(qRgb(255, 255, 255));
    painter.setFont(font);
    painter.drawText(fontImg.rect(), chr, Qt::AlignHCenter | Qt::AlignVCenter);
    painter.end();

    AkVideoPacket charSprite({AkVideoCaps::Format_gray8,
                              fontSize.width(),
                              fontSize.height(),
                              {}});
    auto lineSize = qMin<size_t>(fontImg.bytesPerLine(),
                                 charSprite.lineSize(0));

    for (int y = 0; y < fontSize.height(); y++)
        memcpy(charSprite.line(0, y),
               fontImg.constScanLine(y),
               lineSize);

    return charSprite;
}

int CharacterPrivate::imageWeight(const AkVideoPacket &image) const
{
    int weight = 0;

    for (int y = 0; y < image.caps().height(); y++) {
        auto imageLine = image.constLine(0, y);

        for (int x = 0; x < image.caps().width(); x++)
            weight += imageLine[x];
    }

    weight /= image.caps().width() * image.caps().height();

    return weight;
}
