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

#include <QQmlEngine>

#include "akpalette.h"
#include "akpalettegroup.h"

class AkPalettePrivate
{
    public:
        AkPalette *self;
        AkPaletteGroup m_active {QPalette::Active};
        AkPaletteGroup m_disabled {QPalette::Disabled};

        AkPalettePrivate(AkPalette *self);
};

AkPalette::AkPalette(QObject *parent):
    QObject(parent)
{
    this->d = new AkPalettePrivate(this);
}

AkPalette::AkPalette(const AkPalette &other):
    QObject()
{
    this->d = new AkPalettePrivate(this);
    this->d->m_active = other.d->m_active;
    this->d->m_disabled = other.d->m_disabled;
}

AkPalette::~AkPalette()
{
    delete this->d;
}

AkPalette &AkPalette::operator =(const AkPalette &other)
{
    if (this != &other) {
        this->d->m_active = other.d->m_active;
        this->d->m_disabled = other.d->m_disabled;
    }

    return *this;
}

bool AkPalette::operator ==(const AkPalette &other) const
{
    return this->d->m_active == other.d->m_active
           && this->d->m_disabled == other.d->m_disabled;
}

AkPaletteGroup *AkPalette::active() const
{
    return &this->d->m_active;
}

AkPaletteGroup *AkPalette::disabled() const
{
    return &this->d->m_disabled;
}

void AkPalette::setActive(const AkPaletteGroup *active)
{
    if (this->d->m_active == *active)
        return;

    this->d->m_active = *active;
    emit this->activeChanged(&this->d->m_active);
}

void AkPalette::setDisabled(const AkPaletteGroup *disabled)
{
    if (this->d->m_disabled == *disabled)
        return;

    this->d->m_disabled = *disabled;
    emit this->disabledChanged(&this->d->m_disabled);
}

void AkPalette::resetActive()
{
    AkPaletteGroup active(QPalette::Active);
    this->setActive(&active);
}

void AkPalette::resetDisabled()
{
    AkPaletteGroup disabled(QPalette::Disabled);
    this->setDisabled(&disabled);
}

void AkPalette::registerTypes()
{
    qRegisterMetaType<AkPalette>("AkPalette");
    qmlRegisterType<AkPalette>("Ak", 1, 0, "AkPalette");
}

AkPalettePrivate::AkPalettePrivate(AkPalette *self):
    self(self)
{

}

#include "moc_akpalette.cpp"
