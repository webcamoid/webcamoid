/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "bitmap.h"

namespace AkVCam
{
    class BitmapPrivate
    {
        public:
            int m_width;
            int m_height;
            uint32_t *m_data;
    };
}

AkVCam::Bitmap::Bitmap(CStreamRead bitmapStream)
{
    this->d = new BitmapPrivate();
    this->d->m_width = 0;
    this->d->m_height = 0;
    this->d->m_data = nullptr;

    if (!bitmapStream.data())
        return;

    if (bitmapStream.read<char>() != 'B'
        || bitmapStream.read<char>() != 'M')
        return;

    bitmapStream.seek(8);
    auto pixelsOffset = bitmapStream.read<uint32_t>();
    bitmapStream.seek(4);
    this->d->m_width = int(bitmapStream.read<uint32_t>());
    this->d->m_height = int(bitmapStream.read<uint32_t>());
    bitmapStream.seek(2);
    auto depth = bitmapStream.read<uint16_t>();
    bitmapStream.seek(int(pixelsOffset), CStreamRead::SeekSet);

    this->d->m_data = new uint32_t[this->d->m_width * this->d->m_height];

    switch (depth) {
        case 24:
            for (int y = 0; y < this->d->m_height; y++) {
                auto line = this->line(this->d->m_height - y - 1);

                for (int x = 0; x < this->d->m_width; x++) {
                    auto b = bitmapStream.read<uint8_t>();
                    auto g = bitmapStream.read<uint8_t>();
                    auto r = bitmapStream.read<uint8_t>();
                    line[x] = Color::rgb(r, g, b, 255);
                }
            }

            break;

        case 32:
            for (int y = 0; y < this->d->m_height; y++) {
                auto line = this->line(this->d->m_height - y - 1);

                for (int x = 0; x < this->d->m_width; x++) {
                    auto b = bitmapStream.read<uint8_t>();
                    auto g = bitmapStream.read<uint8_t>();
                    auto r = bitmapStream.read<uint8_t>();
                    auto a = bitmapStream.read<uint8_t>();
                    line[x] = Color::rgb(r, g, b, a);
                }
            }

            break;

        default:
            break;
    }
}

AkVCam::Bitmap::~Bitmap()
{
    if (this->d->m_data)
        delete this->d->m_data;

    delete this->d;
}

int AkVCam::Bitmap::width() const
{
    return this->d->m_width;
}

int AkVCam::Bitmap::height() const
{
    return this->d->m_height;
}

uint32_t *AkVCam::Bitmap::data() const
{
    return this->d->m_data;
}

uint32_t *AkVCam::Bitmap::line(int y) const
{
    return this->d->m_data + this->d->m_width * y;
}

uint32_t AkVCam::Bitmap::pixel(int x, int y) const
{
    return this->line(y)[x];
}
