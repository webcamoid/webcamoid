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

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QQmlEngine>

#include "akpalette.h"
#include "akpalettegroup.h"

class AkPalettePrivate
{
    public:
        AkPalette *self;
        QString m_name;
        AkPaletteGroup m_active {QPalette::Active};
        AkPaletteGroup m_disabled {QPalette::Disabled};

        explicit AkPalettePrivate(AkPalette *self);
};

AkPalette::AkPalette(QObject *parent):
    QObject(parent)
{
    this->d = new AkPalettePrivate(this);
}

AkPalette::AkPalette(const QString &paletteName)
{
    this->d = new AkPalettePrivate(this);
    this->d->m_name = paletteName;
    this->load(paletteName);
}

AkPalette::AkPalette(const AkPalette &other):
    QObject()
{
    this->d = new AkPalettePrivate(this);
    this->d->m_name = other.d->m_name;
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
        this->d->m_name = other.d->m_name;
        this->d->m_active = other.d->m_active;
        this->d->m_disabled = other.d->m_disabled;
    }

    return *this;
}

bool AkPalette::operator ==(const AkPalette &other) const
{
    return this->d->m_name == other.d->m_name
           && this->d->m_active == other.d->m_active
           && this->d->m_disabled == other.d->m_disabled;
}

QObject *AkPalette::create()
{
    return new AkPalette();
}

QObject *AkPalette::create(const QString &paletteName)
{
    return new AkPalette(paletteName);
}

QObject *AkPalette::create(const AkPalette &palette)
{
    return new AkPalette(palette);
}

QVariant AkPalette::toVariant() const
{
    return QVariant::fromValue(*this);
}

QString AkPalette::name() const
{
    return this->d->m_name;
}

AkPaletteGroup *AkPalette::active() const
{
    return &this->d->m_active;
}

AkPaletteGroup *AkPalette::disabled() const
{
    return &this->d->m_disabled;
}

QStringList AkPalette::availablePalettes()
{
    QStringList palettes;
    auto dataPaths = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)
                     + QStringList {":/Webcamoid/share"};
    std::reverse(dataPaths.begin(), dataPaths.end());
    auto sytemPalette = "System";

    for (auto &path: dataPaths) {
        auto themesPath = QString("%1/theme").arg(path);

        for (auto &theme: QDir(themesPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            auto colorsPath = QString("%1/%2/colors").arg(themesPath).arg(theme);

            for (auto &themeFile: QDir(colorsPath).entryList({"*.conf"}, QDir::Files, QDir::Name)) {
                QSettings config(QString("%1/%2").arg(colorsPath).arg(themeFile), QSettings::IniFormat);

                config.beginGroup("Theme");
                auto themeName = config.value("name").toString();
                config.endGroup();

                if (!palettes.contains(themeName) && themeName != sytemPalette)
                    palettes << themeName;
            }
        }
    }

    std::sort(palettes.begin(), palettes.end());

    return QStringList {sytemPalette} + palettes;
}

bool AkPalette::canWrite(const QString &paletteName)
{
    return AkPaletteGroup::canWrite(paletteName);
}

void AkPalette::setName(const QString &name)
{
    if (this->d->m_name == name)
        return;

    this->d->m_name = name;
    emit this->nameChanged(name);
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

void AkPalette::resetName()
{
    this->setName("System");
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

bool AkPalette::load(const QString &paletteName)
{
    return this->d->m_active.load(paletteName)
           && this->d->m_disabled.load(paletteName);
}

QString AkPalette::loadFromFileName(const QString &fileName)
{
    this->d->m_name = this->d->m_active.loadFromFileName(fileName);
    auto name = this->d->m_disabled.loadFromFileName(fileName);

    if (this->d->m_name.isEmpty())
        this->d->m_name = name;

    return this->d->m_name;
}

bool AkPalette::save(const QString &paletteName)
{
    return this->d->m_active.save(paletteName)
           && this->d->m_disabled.save(paletteName);
}

bool AkPalette::remove(const QString &paletteName)
{
    if (paletteName == "System")
        return false;

    auto dataPaths =
            QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    if (dataPaths.isEmpty())
        return false;

    auto themesPath = QString("%1/theme").arg(dataPaths[0]);

    for (auto &theme: QDir(themesPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        auto colorsPath = QString("%1/%2/colors").arg(themesPath).arg(theme);

        for (auto &themeFile: QDir(colorsPath).entryList({"*.conf"}, QDir::Files, QDir::Name)) {
            auto colorFile = QString("%1/%2").arg(colorsPath).arg(themeFile);
            QSettings config(colorFile, QSettings::IniFormat);

            config.beginGroup("Theme");
            auto themeName = config.value("name").toString();
            config.endGroup();

            if (themeName == paletteName)
                return QDir().remove(colorFile);
        }
    }

    return false;
}

void AkPalette::sync()
{
    AkPaletteGroup::sync();
}

void AkPalette::apply(const QString &paletteName)
{
    if (!paletteName.isEmpty()) {
        QSettings config;

        config.beginGroup("ThemeConfigs");
        config.setValue("paletteName", paletteName);
        config.endGroup();

        this->d->m_active.load(paletteName);
        this->d->m_disabled.load(paletteName);
    }

    this->d->m_active.apply();
    this->d->m_disabled.apply();
}

void AkPalette::registerTypes()
{
    qRegisterMetaType<AkPalette>("AkPalette");
    qmlRegisterType<AkPalette>("Ak", 1, 0, "AkPalette");
    qmlRegisterSingletonType<AkPalette>("Ak", 1, 0, "AkPalette",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkPalette();
    });
}

AkPalettePrivate::AkPalettePrivate(AkPalette *self):
    self(self)
{
    QSettings config;

    config.beginGroup("ThemeConfigs");
    this->m_name = config.value("paletteName", "System").toString();
    config.endGroup();
}

#include "moc_akpalette.cpp"
