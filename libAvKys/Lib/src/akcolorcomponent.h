/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#ifndef AKCOLORCOMPONENT_H
#define AKCOLORCOMPONENT_H

#include <QObject>

#include "akcommons.h"

class AkColorComponent;
class AkColorComponentPrivate;

using AkColorComponentList = QVector<AkColorComponent>;

class AKCOMMONS_EXPORT AkColorComponent: public QObject
{
    Q_OBJECT
    Q_PROPERTY(ComponentType type
               READ type
               WRITE setType
               RESET resetType
               NOTIFY typeChanged)
    Q_PROPERTY(size_t step
               READ step
               WRITE setStep
               RESET resetStep
               NOTIFY stepChanged)
    Q_PROPERTY(size_t offset
               READ offset
               WRITE setOffset
               RESET resetOffset
               NOTIFY offsetChanged)
    Q_PROPERTY(size_t shift
               READ shift
               WRITE setShift
               RESET resetShift
               NOTIFY shiftChanged)
    Q_PROPERTY(size_t rlength
               READ rlength
               WRITE setRlength
               RESET resetRlength
               NOTIFY rlengthChanged)
    Q_PROPERTY(size_t length
               READ length
               WRITE setLength
               RESET resetLength
               NOTIFY lengthChanged)
    Q_PROPERTY(size_t widthDiv
               READ widthDiv
               WRITE setWidthDiv
               RESET resetWidthDiv
               NOTIFY widthDivChanged)
    Q_PROPERTY(size_t heightDiv
               READ heightDiv
               WRITE setHeightDiv
               RESET resetHeightDiv
               NOTIFY heightDivChanged)

    public:
        enum ComponentType
        {
            CT_R,
            CT_G,
            CT_B,
            CT_Y,
            CT_U,
            CT_V,
            CT_A
        };
        Q_ENUM(ComponentType)

        AkColorComponent(QObject *parent=nullptr);
        AkColorComponent(ComponentType type,
                         size_t step,
                         size_t offset,
                         size_t shift,
                         size_t rlength,
                         size_t length,
                         size_t widthDiv,
                         size_t heightDiv);
        AkColorComponent(const AkColorComponent &other);
        ~AkColorComponent();
        AkColorComponent &operator =(const AkColorComponent &other);
        bool operator ==(const AkColorComponent &other) const;
        bool operator !=(const AkColorComponent &other) const;

        Q_INVOKABLE static QObject *create();
        Q_INVOKABLE static QObject *create(const AkColorComponent &colorComponent);
        Q_INVOKABLE static QObject *create(AkColorComponent::ComponentType type,
                                           size_t step,
                                           size_t offset,
                                           size_t shift,
                                           size_t rlength,
                                           size_t length,
                                           size_t widthDiv,
                                           size_t heightDiv);
        Q_INVOKABLE QVariant toVariant() const;

        Q_INVOKABLE AkColorComponent::ComponentType type() const;
        Q_INVOKABLE size_t step() const;
        Q_INVOKABLE size_t offset() const;
        Q_INVOKABLE size_t shift() const;
        Q_INVOKABLE size_t rlength() const;
        Q_INVOKABLE size_t length() const;
        Q_INVOKABLE size_t widthDiv() const;
        Q_INVOKABLE size_t heightDiv() const;

        template <typename T>
        inline T max() const
        {
            return T(1) << this->length();
        }

    private:
        AkColorComponentPrivate *d;

    Q_SIGNALS:
        void typeChanged(AkColorComponent::ComponentType type);
        void stepChanged(size_t step);
        void offsetChanged(size_t offset);
        void shiftChanged(size_t shift);
        void rlengthChanged(size_t rlength);
        void lengthChanged(size_t length);
        void widthDivChanged(size_t widthDiv);
        void heightDivChanged(size_t heightDiv);

    public Q_SLOTS:
        void setType(AkColorComponent::ComponentType type);
        void setStep(size_t step);
        void setOffset(size_t offset);
        void setShift(size_t shift);
        void setRlength(size_t rlength);
        void setLength(size_t length);
        void setWidthDiv(size_t widthDiv);
        void setHeightDiv(size_t heightDiv);
        void resetType();
        void resetStep();
        void resetOffset();
        void resetShift();
        void resetRlength();
        void resetLength();
        void resetWidthDiv();
        void resetHeightDiv();
        static void registerTypes();
};

AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, const AkColorComponent &component);
AKCOMMONS_EXPORT QDebug operator <<(QDebug debug, AkColorComponent::ComponentType type);
AKCOMMONS_EXPORT QDataStream &operator >>(QDataStream &istream, AkColorComponent &component);
AKCOMMONS_EXPORT QDataStream &operator <<(QDataStream &ostream, const AkColorComponent &component);

Q_DECLARE_METATYPE(AkColorComponent)
Q_DECLARE_METATYPE(AkColorComponentList)
Q_DECLARE_METATYPE(AkColorComponent::ComponentType)

#endif // AKCOLORCOMPONENT_H
