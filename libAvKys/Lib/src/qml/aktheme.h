/* Webcamoid, camera capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef AKTHEME_H
#define AKTHEME_H

#include <QObject>
#include <QtQml>

#include "../akcommons.h"

class AkThemePrivate;
class AkFontSettings;
class AkPalette;
class QColor;

class AKCOMMONS_EXPORT AkTheme: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal controlScale
               READ controlScale
               WRITE setControlScale
               RESET resetControlScale
               NOTIFY controlScaleChanged)
    Q_PROPERTY(AkPalette *palette
               READ palette
               WRITE setPalette
               RESET resetPalette
               NOTIFY paletteChanged)
    Q_PROPERTY(AkFontSettings *fontSettings
               READ fontSettings
               WRITE setFontSettings
               RESET resetFontSettings
               NOTIFY fontSettingsChanged)

    public:
        explicit AkTheme(QObject *parent=nullptr);
        ~AkTheme();

        static AkTheme *qmlAttachedProperties(QObject *object);

        Q_INVOKABLE AkPalette *palette() const;
        Q_INVOKABLE AkFontSettings *fontSettings() const;
        Q_INVOKABLE qreal controlScale() const;
        Q_INVOKABLE static QColor contrast(const QColor &color,
                                           qreal value=0.5);
        Q_INVOKABLE static QColor complementary(const QColor &color);
        Q_INVOKABLE static QColor constShade(const QColor &color,
                                             qreal value,
                                             qreal alpha=1);
        Q_INVOKABLE static QColor shade(const QColor &color,
                                        qreal value,
                                        qreal alpha=1);
        Q_INVOKABLE static qreal distance(const QColor &color1,
                                          const QColor &color2);

    private:
        AkThemePrivate *d;

    signals:
        void controlScaleChanged(qreal controlScale);
        void paletteChanged(const AkPalette *palette);
        void fontSettingsChanged(const AkFontSettings *fontSettings);

    public slots:
        void setControlScale(qreal controlScale);
        void setPalette(const AkPalette *palette);
        void setFontSettings(const AkFontSettings *fontSettings);
        void resetControlScale();
        void resetPalette();
        void resetFontSettings();
        static void registerTypes();
};

QML_DECLARE_TYPEINFO(AkTheme, QML_HAS_ATTACHED_PROPERTIES)

#endif // AKTHEME_H
