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

#ifndef RAINDROP_H
#define RAINDROP_H

#include <QPainter>

class RainDrop: public QObject
{
    Q_OBJECT

    public:
        explicit RainDrop(const QSize &textArea,
                          const QString &charTable,
                          const QFont &font,
                          const QSize &fontSize, QRgb cursorColor,
                          QRgb startColor,
                          QRgb endColor,
                          int minLength,
                          int maxLength,
                          qreal minSpeed,
                          qreal maxSpeed,
                          bool randomStart, QObject *parent = NULL);
        RainDrop(const RainDrop &other);
        RainDrop &operator =(const RainDrop &other);
        RainDrop operator ++(int);
        Q_INVOKABLE bool isVisible() const;
        Q_INVOKABLE QImage render(QRgb tailColor, bool showCursor);
        Q_INVOKABLE QPoint pos() const;
        Q_INVOKABLE QPoint tail() const;

    private:
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

        inline int randInt(int a, int b)
        {
            if (a > b) {
                int c = a;
                a = b;
                b = c;
            }

            return qrand() % (b + 1 - a) + a;
        }

        inline qreal randReal(qreal a, qreal b)
        {
            if (a > b) {
                qreal c = a;
                a = b;
                b = c;
            }

            return qrand() * (b - a) / RAND_MAX + a;
        }

        inline int gradientColor(int i, int from, int to, int length)
        {
            if (length < 2)
                return from;

            return (i * (to - from)) / (length - 1) + from;
        }

        inline QRgb gradientRgb(int i, QRgb from, QRgb to, int length)
        {
            int r = this->gradientColor(i, qRed(from), qRed(to), length);
            int g = this->gradientColor(i, qGreen(from), qGreen(to), length);
            int b = this->gradientColor(i, qBlue(from), qBlue(to), length);

            return qRgb(r, g, b);
        }

        inline QRgb gradient(int i, QRgb from, QRgb mid, QRgb to, int length)
        {
            int l1 = length >> 1;

            if (i < l1)
                return this->gradientRgb(i, from, mid, l1);

            return this->gradientRgb(i - l1, mid, to, length - l1);
        }

        QImage drawChar(const QChar &chr,
                        const QFont &font, const QSize &fontSize,
                        QRgb foreground, QRgb background) const;
};

#endif // RAINDROP_H
