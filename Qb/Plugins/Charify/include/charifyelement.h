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

#ifndef CHARIFYELEMENT_H
#define CHARIFYELEMENT_H

#include <QGraphicsScene>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QQmlComponent>
#include <QQmlContext>

#include <qb.h>
#include <qbutils.h>

#include "character.h"

class CharifyElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(ColorMode)
    Q_PROPERTY(QString mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
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
    Q_PROPERTY(bool reversed
               READ reversed
               WRITE setReversed
               RESET resetReversed
               NOTIFY reversedChanged)

    public:
        enum ColorMode
        {
            ColorModeNatural,
            ColorModeFixed
        };

        explicit CharifyElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString mode() const;
        Q_INVOKABLE QString charTable() const;
        Q_INVOKABLE QFont font() const;
        Q_INVOKABLE QRgb foregroundColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE bool reversed() const;

    private:
        ColorMode m_mode;
        QString m_charTable;
        QFont m_font;
        QRgb m_foregroundColor;
        QRgb m_backgroundColor;
        bool m_reversed;

        QbElementPtr m_convert;
        QMap<ColorMode, QString> m_colorModeToStr;
        QList<Character> m_characters;
        QSize m_fontSize;

        QSize fontSize(const QString &chrTable, const QFont &font) const;
        QImage drawChar(const QChar &chr, const QFont &font,
                        const QSize &fontSize,
                        QRgb foreground, QRgb background) const;
        int imageWeight(const QImage &image, bool reversed) const;
        static bool chrLessThan(const Character &chr1, const Character &chr2);
        void createCharTable(ColorMode mode, const QString &charTable,
                             const QFont &font, QRgb foreground, QRgb background, bool reversed);

    signals:
        void modeChanged();
        void charTableChanged();
        void fontChanged();
        void foregroundColorChanged();
        void backgroundColorChanged();
        void reversedChanged();

    public slots:
        void setMode(const QString &mode);
        void setCharTable(const QString &charTable);
        void setFont(const QFont &font);
        void setForegroundColor(QRgb foregroundColor);
        void setBackgroundColor(QRgb backgroundColor);
        void setReversed(bool reversed);
        void resetMode();
        void resetCharTable();
        void resetFont();
        void resetForegroundColor();
        void resetBackgroundColor();
        void resetReversed();

        QbPacket iStream(const QbPacket &packet);
};

#endif // CHARIFYELEMENT_H
