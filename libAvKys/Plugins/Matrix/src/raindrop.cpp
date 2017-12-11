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

#include <cstdlib>
#include <QPainter>
#include <QFont>

#include "raindrop.h"

class RainDropPrivate
{
    public:
        QSize m_textArea;
        QString m_line;
        int m_length;
        QString m_charTable;
        QFont m_font;
        QSize m_fontSize;
        QRgb m_cursorColor;
        QRgb m_startColor;
        QRgb m_endColor;
        QPointF m_pos;
        QPoint m_prevPos;
        qreal m_speed;
        QImage m_sprite;

        inline int randInt(int a, int b);
        inline qreal randReal(qreal a, qreal b);
        inline int gradientColor(int i, int from, int to, int length);
        inline QRgb gradientRgb(int i, QRgb from, QRgb to, int length);
        inline QRgb gradient(int i, QRgb from, QRgb mid, QRgb to, int length);
        inline QImage drawChar(const QChar &chr,
                               const QFont &font, const QSize &fontSize,
                               QRgb foreground, QRgb background) const;
};

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
                   bool randomStart)
{
    this->d = new RainDropPrivate;

    for (int i = 0; i < textArea.height(); i++)
        this->d->m_line.append(charTable[qrand() % charTable.size()]);

    this->d->m_textArea = textArea;
    int y = randomStart? qrand() % textArea.height(): 0;
    this->d->m_pos = QPointF(qrand() % textArea.width(), y);
    this->d->m_font = font;
    this->d->m_fontSize = fontSize;
    this->d->m_cursorColor = cursorColor;
    this->d->m_startColor = startColor;
    this->d->m_endColor = endColor;
    this->d->m_length = this->d->randInt(minLength, maxLength);

    if (this->d->m_length < 1)
        this->d->m_length = 1;

    this->d->m_speed = this->d->randReal(minSpeed, maxSpeed);

    if (this->d->m_speed < 0.1)
        this->d->m_speed = 0.1;
}

RainDrop::RainDrop(const RainDrop &other)
{
    this->d = new RainDropPrivate;
    this->d->m_textArea = other.d->m_textArea;
    this->d->m_line = other.d->m_line;
    this->d->m_length = other.d->m_length;
    this->d->m_charTable = other.d->m_charTable;
    this->d->m_font = other.d->m_font;
    this->d->m_fontSize = other.d->m_fontSize;
    this->d->m_cursorColor = other.d->m_cursorColor;
    this->d->m_startColor = other.d->m_startColor;
    this->d->m_endColor = other.d->m_endColor;
    this->d->m_pos = other.d->m_pos;
    this->d->m_prevPos = other.d->m_prevPos;
    this->d->m_speed = other.d->m_speed;
    this->d->m_sprite = other.d->m_sprite;
}

RainDrop::~RainDrop()
{
    delete this->d;
}

RainDrop &RainDrop::operator =(const RainDrop &other)
{
    if (this != &other) {
        this->d->m_textArea = other.d->m_textArea;
        this->d->m_line = other.d->m_line;
        this->d->m_length = other.d->m_length;
        this->d->m_charTable = other.d->m_charTable;
        this->d->m_font = other.d->m_font;
        this->d->m_fontSize = other.d->m_fontSize;
        this->d->m_cursorColor = other.d->m_cursorColor;
        this->d->m_startColor = other.d->m_startColor;
        this->d->m_endColor = other.d->m_endColor;
        this->d->m_pos = other.d->m_pos;
        this->d->m_prevPos = other.d->m_prevPos;
        this->d->m_speed = other.d->m_speed;
        this->d->m_sprite = other.d->m_sprite;
    }

    return *this;
}

RainDrop RainDrop::operator ++(int)
{
    RainDrop rainDrop = *this;
    this->d->m_pos = QPointF(this->d->m_pos.x(),
                             this->d->m_pos.y() + this->d->m_speed);

    return rainDrop;
}

bool RainDrop::isVisible() const
{
    return int(this->d->m_pos.y() + 1 - this->d->m_length) < this->d->m_line.size();
}

QImage RainDrop::render(QRgb tailColor, bool showCursor)
{
    if (!this->isVisible())
        return QImage();

    if (this->pos() == this->d->m_prevPos) {
        if (!showCursor)
            return this->d->m_sprite;

        QPainter painter;

        painter.begin(&this->d->m_sprite);
        QChar c = this->d->m_line[qrand() % this->d->m_line.size()];

        QImage sprite =
                this->d->drawChar(c,
                                  this->d->m_font,
                                  this->d->m_fontSize,
                                  this->d->m_endColor,
                                  this->d->m_cursorColor);

        painter.drawImage(0,
                          (this->d->m_length - 1) * this->d->m_fontSize.height(),
                          sprite);

        painter.end();

        return this->d->m_sprite;
    }

    this->d->m_prevPos = this->pos();

    QImage drop(this->d->m_fontSize.width(),
                this->d->m_length * this->d->m_fontSize.height(),
                QImage::Format_RGB32);

    QPainter painter;

    painter.begin(&drop);
    QChar chr;
    QRgb foreground;
    QRgb background;

    for (int i = 0; i < this->d->m_length; i++) {
        int c = int(i + this->d->m_pos.y() + 1 - this->d->m_length);

        if (c >= 0 && c < this->d->m_line.size()) {
            if (i == this->d->m_length - 1) {
                chr = this->d->m_line[qrand() % this->d->m_line.size()];

                if (showCursor) {
                    foreground = this->d->m_endColor;
                    background = this->d->m_cursorColor;
                } else {
                    foreground = this->d->m_cursorColor;
                    background = this->d->m_endColor;
                }
            } else {
                chr = this->d->m_line[c];
                foreground =
                        this->d->gradient(i,
                                          tailColor,
                                          this->d->m_startColor,
                                          this->d->m_cursorColor,
                                          this->d->m_length);
                background = this->d->m_endColor;
            }

            QImage sprite =
                    this->d->drawChar(chr,
                                      this->d->m_font,
                                      this->d->m_fontSize,
                                      foreground,
                                      background);

            painter.drawImage(0, i * this->d->m_fontSize.height(), sprite);
        }
    }

    painter.end();

    this->d->m_sprite = drop;

    return drop;
}

QPoint RainDrop::pos() const
{
    int x = int(this->d->m_pos.x() * this->d->m_fontSize.width());
    int y = int(this->d->m_pos.y() + 1 - this->d->m_length)
            * this->d->m_fontSize.height();

    return QPoint(x, y);
}

QPoint RainDrop::tail() const
{
    int y = int(this->d->m_pos.y() - this->d->m_length);

    return QPoint(int(this->d->m_pos.x()), y);
}

QImage RainDropPrivate::drawChar(const QChar &chr,
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

int RainDropPrivate::randInt(int a, int b)
{
    if (a > b) {
        int c = a;
        a = b;
        b = c;
    }

    return qrand() % (b + 1 - a) + a;
}

qreal RainDropPrivate::randReal(qreal a, qreal b)
{
    if (a > b) {
        qreal c = a;
        a = b;
        b = c;
    }

    return qrand() * (b - a) / RAND_MAX + a;
}

int RainDropPrivate::gradientColor(int i, int from, int to, int length)
{
    if (length < 2)
        return from;

    return (i * (to - from)) / (length - 1) + from;
}

QRgb RainDropPrivate::gradientRgb(int i, QRgb from, QRgb to, int length)
{
    int r = this->gradientColor(i, qRed(from), qRed(to), length);
    int g = this->gradientColor(i, qGreen(from), qGreen(to), length);
    int b = this->gradientColor(i, qBlue(from), qBlue(to), length);

    return qRgb(r, g, b);
}

QRgb RainDropPrivate::gradient(int i, QRgb from, QRgb mid, QRgb to, int length)
{
    int l1 = length >> 1;

    if (i < l1)
        return this->gradientRgb(i, from, mid, l1);

    return this->gradientRgb(i - l1, mid, to, length - l1);
}
