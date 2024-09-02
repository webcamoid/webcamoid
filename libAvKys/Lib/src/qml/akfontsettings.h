/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef AKFONTSETTINGS_H
#define AKFONTSETTINGS_H

#include <QObject>

#include "../akcommons.h"

class AkFontSettingsPrivate;
class QFont;

class AKCOMMONS_EXPORT AkFontSettings: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QFont h1
               READ h1
               WRITE setH1
               RESET resetH1
               NOTIFY h1Changed)
    Q_PROPERTY(QFont h2
               READ h2
               WRITE setH2
               RESET resetH2
               NOTIFY h2Changed)
    Q_PROPERTY(QFont h3
               READ h3
               WRITE setH3
               RESET resetH3
               NOTIFY h3Changed)
    Q_PROPERTY(QFont h4
               READ h4
               WRITE setH4
               RESET resetH4
               NOTIFY h4Changed)
    Q_PROPERTY(QFont h5
               READ h5
               WRITE setH5
               RESET resetH5
               NOTIFY h5Changed)
    Q_PROPERTY(QFont h6
               READ h6
               WRITE setH6
               RESET resetH6
               NOTIFY h6Changed)
    Q_PROPERTY(QFont subtitle1
               READ subtitle1
               WRITE setSubtitle1
               RESET resetSubtitle1
               NOTIFY subtitle1Changed)
    Q_PROPERTY(QFont subtitle2
               READ subtitle2
               WRITE setSubtitle2
               RESET resetSubtitle2
               NOTIFY subtitle2Changed)
    Q_PROPERTY(QFont body1
               READ body1
               WRITE setBody1
               RESET resetBody1
               NOTIFY body1Changed)
    Q_PROPERTY(QFont body2
               READ body2
               WRITE setBody2
               RESET resetBody2
               NOTIFY body2Changed)
    Q_PROPERTY(QFont button
               READ button
               WRITE setButton
               RESET resetButton
               NOTIFY buttonChanged)
    Q_PROPERTY(QFont caption
               READ caption
               WRITE setCaption
               RESET resetCaption
               NOTIFY captionChanged)
    Q_PROPERTY(QFont overline
               READ overline
               WRITE setOverline
               RESET resetOverline
               NOTIFY overlineChanged)

    public:
        explicit AkFontSettings(QObject *parent=nullptr);
        AkFontSettings(const QFont &font);
        AkFontSettings(const AkFontSettings &other);
        ~AkFontSettings();
        AkFontSettings &operator =(const AkFontSettings &other);
        bool operator ==(const AkFontSettings &other) const;

        Q_INVOKABLE QFont h1() const;
        Q_INVOKABLE QFont h2() const;
        Q_INVOKABLE QFont h3() const;
        Q_INVOKABLE QFont h4() const;
        Q_INVOKABLE QFont h5() const;
        Q_INVOKABLE QFont h6() const;
        Q_INVOKABLE QFont subtitle1() const;
        Q_INVOKABLE QFont subtitle2() const;
        Q_INVOKABLE QFont body1() const;
        Q_INVOKABLE QFont body2() const;
        Q_INVOKABLE QFont button() const;
        Q_INVOKABLE QFont caption() const;
        Q_INVOKABLE QFont overline() const;

    private:
        AkFontSettingsPrivate *d;

    signals:
        void h1Changed(const QFont &h1);
        void h2Changed(const QFont &h2);
        void h3Changed(const QFont &h3);
        void h4Changed(const QFont &h4);
        void h5Changed(const QFont &h5);
        void h6Changed(const QFont &h6);
        void subtitle1Changed(const QFont &subtitle1);
        void subtitle2Changed(const QFont &subtitle2);
        void body1Changed(const QFont &body1);
        void body2Changed(const QFont &body2);
        void buttonChanged(const QFont &button);
        void captionChanged(const QFont &caption);
        void overlineChanged(const QFont &overline);

    public slots:
        void setH1(const QFont &h1);
        void setH2(const QFont &h2);
        void setH3(const QFont &h3);
        void setH4(const QFont &h4);
        void setH5(const QFont &h5);
        void setH6(const QFont &h6);
        void setSubtitle1(const QFont &subtitle1);
        void setSubtitle2(const QFont &subtitle2);
        void setBody1(const QFont &body1);
        void setBody2(const QFont &body2);
        void setButton(const QFont &button);
        void setCaption(const QFont &caption);
        void setOverline(const QFont &overline);
        void resetH1();
        void resetH2();
        void resetH3();
        void resetH4();
        void resetH5();
        void resetH6();
        void resetSubtitle1();
        void resetSubtitle2();
        void resetBody1();
        void resetBody2();
        void resetButton();
        void resetCaption();
        void resetOverline();
        static void registerTypes();

    private slots:
        void updateFonts(const QFont &font);
};

Q_DECLARE_METATYPE(AkFontSettings)

#endif // AKFONTSETTINGS_H
