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

#ifndef AKPALETTE_H
#define AKPALETTE_H

#include <QObject>

#include "../akcommons.h"

class AkPalettePrivate;
class AkPaletteGroup;

class AKCOMMONS_EXPORT AkPalette: public QObject
{
    Q_OBJECT
    Q_PROPERTY(AkPaletteGroup *active
               READ active
               WRITE setActive
               RESET resetActive
               NOTIFY activeChanged)
    Q_PROPERTY(AkPaletteGroup *disabled
               READ disabled
               WRITE setDisabled
               RESET resetDisabled
               NOTIFY disabledChanged)

    public:
        explicit AkPalette(QObject *parent=nullptr);
        AkPalette(const AkPalette &other);
        ~AkPalette();
        AkPalette &operator =(const AkPalette &other);
        bool operator ==(const AkPalette &other) const;

        Q_INVOKABLE AkPaletteGroup *active() const;
        Q_INVOKABLE AkPaletteGroup *disabled() const;

    private:
        AkPalettePrivate *d;

    signals:
        void activeChanged(const AkPaletteGroup *active);
        void disabledChanged(const AkPaletteGroup *disabled);

    public slots:
        void setActive(const AkPaletteGroup *active);
        void setDisabled(const AkPaletteGroup *disabled);
        void resetActive();
        void resetDisabled();
        static void registerTypes();
};

Q_DECLARE_METATYPE(AkPalette)

#endif // AKPALETTE_H
