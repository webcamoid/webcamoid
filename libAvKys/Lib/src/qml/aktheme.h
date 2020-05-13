/* Webcamoid, webcam capture application.
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

    public:
        explicit AkTheme(QObject *parent=nullptr);
        ~AkTheme();

        static AkTheme *qmlAttachedProperties(QObject *object);

        Q_INVOKABLE AkPalette *palette() const;
        Q_INVOKABLE qreal controlScale() const;
        Q_INVOKABLE QColor contrast(const QColor &color, qreal value=0.5) const;
        Q_INVOKABLE QColor complementary(const QColor &color) const;
        Q_INVOKABLE QColor constShade(const QColor &color,
                                      qreal value,
                                      qreal alpha=1) const;
        Q_INVOKABLE QColor shade(const QColor &color,
                                 qreal value,
                                 qreal alpha=1) const;

    private:
        AkThemePrivate *d;

    signals:
        void controlScaleChanged(qreal controlScale);
        void paletteChanged(const AkPalette *palette);

    public slots:
        void setControlScale(qreal controlScale);
        void setPalette(const AkPalette *palette);
        void resetControlScale();
        void resetPalette();
        static void registerTypes();
};

QML_DECLARE_TYPEINFO(AkTheme, QML_HAS_ATTACHED_PROPERTIES)

#endif // AKTHEME_H
