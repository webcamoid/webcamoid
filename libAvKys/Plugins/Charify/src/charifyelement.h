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

#ifndef CHARIFYELEMENT_H
#define CHARIFYELEMENT_H

#include <qrgb.h>
#include <akelement.h>

class CharifyElementPrivate;

class CharifyElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(ColorMode mode
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
    Q_PROPERTY(bool smooth
               READ smooth
               WRITE setSmooth
               RESET resetSmooth
               NOTIFY smoothChanged)
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
        Q_ENUM(ColorMode)

        CharifyElement();
        ~CharifyElement();

        Q_INVOKABLE ColorMode mode() const;
        Q_INVOKABLE QString charTable() const;
        Q_INVOKABLE QFont font() const;
        Q_INVOKABLE QString hintingPreference() const;
        Q_INVOKABLE QString styleStrategy() const;
        Q_INVOKABLE QRgb foregroundColor() const;
        Q_INVOKABLE QRgb backgroundColor() const;
        Q_INVOKABLE bool smooth() const;
        Q_INVOKABLE bool reversed() const;

    private:
        CharifyElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;
        AkPacket iVideoStream(const AkVideoPacket &packet) override;

    signals:
        void modeChanged(ColorMode mode);
        void charTableChanged(const QString &charTable);
        void fontChanged(const QFont &font);
        void hintingPreferenceChanged(const QString &hintingPreference);
        void styleStrategyChanged(const QString &styleStrategy);
        void foregroundColorChanged(QRgb foregroundColor);
        void backgroundColorChanged(QRgb backgroundColor);
        void smoothChanged(bool smooth);
        void reversedChanged(bool reversed);

    public slots:
        void setMode(CharifyElement::ColorMode mode);
        void setCharTable(const QString &charTable);
        void setFont(const QFont &font);
        void setHintingPreference(const QString &hintingPreference);
        void setStyleStrategy(const QString &styleStrategy);
        void setForegroundColor(QRgb foregroundColor);
        void setBackgroundColor(QRgb backgroundColor);
        void setSmooth(bool smooth);
        void setReversed(bool reversed);
        void resetMode();
        void resetCharTable();
        void resetFont();
        void resetHintingPreference();
        void resetStyleStrategy();
        void resetForegroundColor();
        void resetBackgroundColor();
        void resetSmooth();
        void resetReversed();
};

Q_DECL_EXPORT QDataStream &operator >>(QDataStream &istream, CharifyElement::ColorMode &mode);
Q_DECL_EXPORT QDataStream &operator <<(QDataStream &ostream, CharifyElement::ColorMode mode);

Q_DECLARE_METATYPE(CharifyElement::ColorMode)

#endif // CHARIFYELEMENT_H
