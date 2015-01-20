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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MATRIXELEMENT_H
#define MATRIXELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

#include "character.h"
#include "raindrop.h"

class MatrixElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int nDrops
               READ nDrops
               WRITE setNDrops
               RESET resetNDrops
               NOTIFY nDropsChanged)
    Q_PROPERTY(QString charTable
               READ charTable
               WRITE setCharTable
               RESET resetCharTable
               NOTIFY charTableChanged)
    Q_PROPERTY(QFont font
               READ font
               WRITE setFont
               RESET resetFont
               NOTIFY fontChanged)
    Q_PROPERTY(QRgb cursorColor
               READ cursorColor
               WRITE setCursorColor
               RESET resetCursorColor
               NOTIFY cursorColorChanged)
    Q_PROPERTY(QRgb foregroundColor
               READ foregroundColor
               WRITE setForegroundColor
               RESET resetForegroundColor
               NOTIFY foregroundColorChanged)
    Q_PROPERTY(QRgb backgroundColor
               READ backgroundColor
               WRITE setBackgroundColor
               RESET resetBackgroundColor
               NOTIFY backgroundColorChanged)
    Q_PROPERTY(int minDropLength
               READ minDropLength
               WRITE setMinDropLength
               RESET resetMinDropLength
               NOTIFY minDropLengthChanged)
    Q_PROPERTY(int maxDropLength
               READ maxDropLength
               WRITE setMaxDropLength
               RESET resetMaxDropLength
               NOTIFY maxDropLengthChanged)
    Q_PROPERTY(qreal minSpeed
               READ minSpeed
               WRITE setMinSpeed
               RESET resetMinSpeed
               NOTIFY minSpeedChanged)
    Q_PROPERTY(qreal maxSpeed
               READ maxSpeed
               WRITE setMaxSpeed
               RESET resetMaxSpeed
               NOTIFY maxSpeedChanged)
    Q_PROPERTY(bool showCursor
               READ showCursor
               WRITE setShowCursor
               RESET resetShowCursor
               NOTIFY showCursorChanged)

    public:
        explicit MatrixElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int nDrops() const;
        Q_INVOKABLE QString charTable() const;
        Q_INVOKABLE QFont font() const;
        Q_INVOKABLE QRgb cursorColor() const;
        Q_INVOKABLE QRgb foregroundColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE int minDropLength() const;
        Q_INVOKABLE int maxDropLength() const;
        Q_INVOKABLE qreal minSpeed() const;
        Q_INVOKABLE qreal maxSpeed() const;
        Q_INVOKABLE bool showCursor() const;

    private:
        int m_nDrops;
        QString m_charTable;
        QFont m_font;
        QRgb m_cursorColor;
        QRgb m_foregroundColor;
        QRgb m_backgroundColor;
        int m_minDropLength;
        int m_maxDropLength;
        qreal m_minSpeed;
        qreal m_maxSpeed;
        bool m_showCursor;

        QbCaps m_caps;
        QbElementPtr m_convert;
        QList<Character> m_characters;
        QSize m_fontSize;
        QList<RainDrop> m_rain;

        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QImage drawChar(const QChar &chr, const QFont &font,
                        const QSize &fontSize,
                        QRgb foreground, QRgb background) const;
        int imageWeight(const QImage &image) const;
        static bool chrLessThan(const Character &chr1, const Character &chr2);
        void createCharTable(const QString &charTable, const QFont &font,
                             QRgb cursor,
                             QRgb foreground,
                             QRgb background);
        QImage renderRain(const QSize &frameSize, const QImage &textImage);

    signals:
        void nDropsChanged();
        void charTableChanged();
        void fontChanged();
        void cursorColorChanged();
        void foregroundColorChanged();
        void backgroundColorChanged();
        void minDropLengthChanged();
        void maxDropLengthChanged();
        void minSpeedChanged();
        void maxSpeedChanged();
        void showCursorChanged();

    public slots:
        void setNDrops(int nDrops);
        void setCharTable(const QString &charTable);
        void setFont(const QFont &font);
        void setCursorColor(QRgb cursorColor);
        void setForegroundColor(QRgb foregroundColor);
        void setBackgroundColor(QRgb backgroundColor);
        void setMinDropLength(int minDropLength);
        void setMaxDropLength(int maxDropLength);
        void setMinSpeed(qreal minSpeed);
        void setMaxSpeed(qreal maxSpeed);
        void setShowCursor(bool showCursor);
        void resetNDrops();
        void resetCharTable();
        void resetFont();
        void resetCursorColor();
        void resetForegroundColor();
        void resetBackgroundColor();
        void resetMinDropLength();
        void resetMaxDropLength();
        void resetMinSpeed();
        void resetMaxSpeed();
        void resetShowCursor();

        QbPacket iStream(const QbPacket &packet);
};

#endif // MATRIXELEMENT_H
