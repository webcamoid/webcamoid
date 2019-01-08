/* Webcamoid, webcam capture application.
 * Copyright (C) 2018  Gonzalo Exequiel Pedone
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

#ifndef FRACTION_H
#define FRACTION_H

#include <cstdint>
#include <string>

namespace AkVCam
{
    class Fraction;
    class FractionPrivate;
    typedef std::pair<Fraction, Fraction> FractionRange;

    class Fraction
    {
        public:
            Fraction();
            Fraction(int64_t num, int64_t den);
            Fraction(const std::string &str);
            Fraction(const std::wstring &str);
            Fraction(const Fraction &other);
            virtual ~Fraction();
            Fraction &operator =(const Fraction &other);
            bool operator ==(const Fraction &other) const;
            bool operator <(const Fraction &other) const;

            int64_t num() const;
            int64_t &num();
            int64_t den() const;
            int64_t &den();
            double value() const;
            std::string toString() const;
            std::wstring toWString() const;

        private:
            FractionPrivate *d;
    };
}

#endif // FRACTION_H
