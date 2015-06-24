/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "raindrop.h"

RainDrop::RainDrop(const QSize &textArea,
                   const QString &charTable,
                   const QFont &font,
                   const QSize &fontSize,
                   QRgb cursorColor,
                   QRgb startColor,
                   QRgb endColor,
                   int minLength,
                   int maxLength,
                   qreal minSpeed,
                   qreal maxSpeed,
                   bool randomStart, QObject *parent):
    QObject(parent)
{
    for (int i = 0; i < textArea.height(); i++)
        this->m_line.append(charTable[qrand() % charTable.size()]);

    this->m_textArea = textArea;
    int y = randomStart? qrand() % textArea.height(): 0;
    this->m_pos = QPointF(qrand() % textArea.width(), y);
    this->m_font = font;
    this->m_fontSize = fontSize;
    this->m_cursorColor = cursorColor;
    this->m_startColor = startColor;
    this->m_endColor = endColor;
    this->m_length = this->randInt(minLength, maxLength);

    if (this->m_length < 1)
        this->m_length = 1;

    this->m_speed = this->randReal(minSpeed, maxSpeed);

    if (this->m_speed < 0.1)
        this->m_speed = 0.1;
}

RainDrop::RainDrop(const RainDrop &other):
    QObject(other.parent()),
    m_textArea(other.m_textArea),
    m_line(other.m_line),
    m_length(other.m_length),
    m_charTable(other.m_charTable),
    m_font(other.m_font),
    m_fontSize(other.m_fontSize),
    m_cursorColor(other.m_cursorColor),
    m_startColor(other.m_startColor),
    m_endColor(other.m_endColor),
    m_pos(other.m_pos),
    m_prevPos(other.m_prevPos),
    m_speed(other.m_speed),
    m_sprite(other.m_sprite)
{
}

RainDrop &RainDrop::operator =(const RainDrop &other)
{
    if (this != &other) {
        this->m_textArea = other.m_textArea;
        this->m_line = other.m_line;
        this->m_length = other.m_length;
        this->m_charTable = other.m_charTable;
        this->m_font = other.m_font;
        this->m_fontSize = other.m_fontSize;
        this->m_cursorColor = other.m_cursorColor;
        this->m_startColor = other.m_startColor;
        this->m_endColor = other.m_endColor;
        this->m_pos = other.m_pos;
        this->m_prevPos = other.m_prevPos;
        this->m_speed = other.m_speed;
        this->m_sprite = other.m_sprite;
    }

    return *this;
}

RainDrop RainDrop::operator ++(int)
{
    RainDrop rainDrop = *this;
    this->m_pos = QPointF(this->m_pos.x(), this->m_pos.y() + this->m_speed);

    return rainDrop;
}

bool RainDrop::isVisible() const
{
    return (int) this->m_pos.y() + 1 - this->m_length < this->m_line.size();
}

QImage RainDrop::render(QRgb tailColor, bool showCursor)
{
    if (!this->isVisible())
        return QImage();

    if (this->pos() == this->m_prevPos) {
        if (!showCursor)
            return this->m_sprite;

        QPainter painter;

        painter.begin(&this->m_sprite);
        QChar c = this->m_line[qrand() % this->m_line.size()];

        QImage sprite = this->drawChar(c,
                                       this->m_font,
                                       this->m_fontSize,
                                       this->m_endColor,
                                       this->m_cursorColor);

        painter.drawImage(0,
                          (this->m_length - 1) * this->m_fontSize.height(),
                          sprite);

        painter.end();

        return this->m_sprite;
    }

    this->m_prevPos = this->pos();

    QImage drop(this->m_fontSize.width(),
                this->m_length * this->m_fontSize.height(),
                QImage::Format_RGB32);

    QPainter painter;

    painter.begin(&drop);
    QChar chr;
    QRgb foreground;
    QRgb background;

    for (int i = 0; i < this->m_length; i++) {
        int c = i + this->m_pos.y() + 1 - this->m_length;

        if (c >= 0 && c < this->m_line.size()) {
            if (i == this->m_length - 1) {
                chr = this->m_line[qrand() % this->m_line.size()];

                if (showCursor) {
                    foreground = this->m_endColor;
                    background = this->m_cursorColor;
                }
                else {
                    foreground = this->m_cursorColor;
                    background = this->m_endColor;
                }
            }
            else {
                chr = this->m_line[c];
                foreground = this->gradient(i,
                                            tailColor,
                                            this->m_startColor,
                                            this->m_cursorColor,
                                            this->m_length);
                background = this->m_endColor;
            }

            QImage sprite = this->drawChar(chr,
                                           this->m_font,
                                           this->m_fontSize,
                                           foreground,
                                           background);

            painter.drawImage(0, i * this->m_fontSize.height(), sprite);
        }
    }

    painter.end();

    this->m_sprite = drop;

    return drop;
}

QPoint RainDrop::pos() const
{
    int x = this->m_pos.x() * this->m_fontSize.width();
    int y = ((int) this->m_pos.y() + 1 - this->m_length) * this->m_fontSize.height();

    return QPoint(x, y);
}

QPoint RainDrop::tail() const
{
    int y = (int) this->m_pos.y() - this->m_length;

    return QPoint(this->m_pos.x(), y);
}

QImage RainDrop::drawChar(const QChar &chr,
                          const QFont &font, const QSize &fontSize,
                          QRgb foreground, QRgb background) const
{
    QImage fontImg(fontSize, QImage::Format_RGB32);
    fontImg.fill(background);

    QPainter painter;

    painter.begin(&fontImg);
    painter.setPen(foreground);
    painter.setFont(font);
    painter.drawText(fontImg.rect(), chr, Qt::AlignHCenter | Qt::AlignVCenter);
    painter.end();

    return fontImg;
}
