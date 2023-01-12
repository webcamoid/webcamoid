/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef MATRIXELEMENT_H
#define MATRIXELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class MatrixElementPrivate;

class MatrixElement: public AkElement
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
    Q_PROPERTY(QString hintingPreference
               READ hintingPreference
               WRITE setHintingPreference
               RESET resetHintingPreference
               NOTIFY hintingPreferenceChanged)
    Q_PROPERTY(QString styleStrategy
               READ styleStrategy
               WRITE setStyleStrategy
               RESET resetStyleStrategy
               NOTIFY styleStrategyChanged)
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
    Q_PROPERTY(bool smooth
               READ smooth
               WRITE setSmooth
               RESET resetSmooth
               NOTIFY smoothChanged)
    Q_PROPERTY(bool showCursor
               READ showCursor
               WRITE setShowCursor
               RESET resetShowCursor
               NOTIFY showCursorChanged)
    Q_PROPERTY(bool showRain
               READ showRain
               WRITE setShowRain
               RESET resetShowRain
               NOTIFY showRainChanged)

    public:
        MatrixElement();
        ~MatrixElement();

        Q_INVOKABLE int nDrops() const;
        Q_INVOKABLE QString charTable() const;
        Q_INVOKABLE QFont font() const;
        Q_INVOKABLE QString hintingPreference() const;
        Q_INVOKABLE QString styleStrategy() const;
        Q_INVOKABLE QRgb cursorColor() const;
        Q_INVOKABLE QRgb foregroundColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE int minDropLength() const;
        Q_INVOKABLE int maxDropLength() const;
        Q_INVOKABLE qreal minSpeed() const;
        Q_INVOKABLE qreal maxSpeed() const;
        Q_INVOKABLE bool smooth() const;
        Q_INVOKABLE bool showCursor() const;
        Q_INVOKABLE bool showRain() const;

    private:
        MatrixElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void nDropsChanged(int nDrops);
        void charTableChanged(const QString &charTable);
        void fontChanged(const QFont &font);
        void hintingPreferenceChanged(const QString &hintingPreference);
        void styleStrategyChanged(const QString &styleStrategy);
        void cursorColorChanged(QRgb cursorColor);
        void foregroundColorChanged(QRgb foregroundColor);
        void backgroundColorChanged(QRgb backgroundColor);
        void minDropLengthChanged(int minDropLength);
        void maxDropLengthChanged(int maxDropLength);
        void minSpeedChanged(qreal minSpeed);
        void maxSpeedChanged(qreal maxSpeed);
        void smoothChanged(bool smooth);
        void showCursorChanged(bool showCursor);
        void showRainChanged(bool showRain);

    public slots:
        void setNDrops(int nDrops);
        void setCharTable(const QString &charTable);
        void setFont(const QFont &font);
        void setHintingPreference(const QString &hintingPreference);
        void setStyleStrategy(const QString &styleStrategy);
        void setCursorColor(QRgb cursorColor);
        void setForegroundColor(QRgb foregroundColor);
        void setBackgroundColor(QRgb backgroundColor);
        void setMinDropLength(int minDropLength);
        void setMaxDropLength(int maxDropLength);
        void setMinSpeed(qreal minSpeed);
        void setMaxSpeed(qreal maxSpeed);
        void setSmooth(bool smooth);
        void setShowCursor(bool showCursor);
        void setShowRain(bool showRain);
        void resetNDrops();
        void resetCharTable();
        void resetFont();
        void resetHintingPreference();
        void resetStyleStrategy();
        void resetCursorColor();
        void resetForegroundColor();
        void resetBackgroundColor();
        void resetMinDropLength();
        void resetMaxDropLength();
        void resetMinSpeed();
        void resetMaxSpeed();
        void resetSmooth();
        void resetShowCursor();
        void resetShowRain();
};

#endif // MATRIXELEMENT_H
