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

#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QQmlEngine>

#include "akpalettegroup.h"
#include "aktheme.h"

#define COLORS_FILE_EXT "colors.conf"

struct Palette
{
    QColor highlightedText;
    QColor highlight;
    QColor text;
    QColor placeholderText;
    QColor base;
    QColor alternateBase;
    QColor windowText;
    QColor window;
    QColor buttonText;
    QColor light;
    QColor midlight;
    QColor button;
    QColor mid;
    QColor dark;
    QColor shadow;
    QColor toolTipText;
    QColor toolTipBase;
    QColor link;
    QColor linkVisited;
};

struct Palettes
{
    Palette active;
    Palette disabled;
    Palette inactive;
};

class AkPaletteGroupGlobalPrivate: public QObject
{
    Q_OBJECT

    public:
        Palettes m_palettes;
        QMap<QPalette::ColorGroup, Palette *> m_groupToPaletteMap;

        explicit AkPaletteGroupGlobalPrivate(QObject *parent=nullptr);

    signals:
        void paletteSyncRequested();
        void paletteCopyRequested(AkPaletteGroup paletteGroup);

    public slots:
        void loadDefaults();
};

Q_GLOBAL_STATIC(AkPaletteGroupGlobalPrivate, akPaletteGroupGlobalPrivate)

class AkPaletteGroupPrivate
{
    public:
        AkPaletteGroup *self;
        QPalette::ColorGroup m_colorGroup {QPalette::Active};
        QColor m_highlightedText;
        QColor m_highlight;
        QColor m_text;
        QColor m_placeholderText;
        QColor m_base;
        QColor m_alternateBase;
        QColor m_windowText;
        QColor m_window;
        QColor m_buttonText;
        QColor m_light;
        QColor m_midlight;
        QColor m_button;
        QColor m_mid;
        QColor m_dark;
        QColor m_shadow;
        QColor m_toolTipText;
        QColor m_toolTipBase;
        QColor m_link;
        QColor m_linkVisited;
        bool m_fixed {false};
        bool m_isDark {false};

        explicit AkPaletteGroupPrivate(AkPaletteGroup *self);
        static QString configFileForPalette(const QString &paletteName);
        static QPalette readPalette(const QString &paletteName);
        static QPalette readPaletteFromFileName(const QString &fileName);
        void loadDefaults();
        void updatePalette();
        void copyPalette(const AkPaletteGroup &paletteGroup);

        inline static QRgb colorFromString(const QString &str, bool *ok=nullptr)
        {
            QRegularExpression re(R"(^[0-9]{0,3},\s*[0-9]{0,3},\s*[0-9]{0,3}$)");
            auto _str = str.trimmed();

            if (!re.match(_str).hasMatch()) {
                if (ok)
                    *ok = false;

                return qRgb(0, 0, 0);
            }

            if (ok)
                *ok = true;

            auto parts = _str.split(',');

            return qRgb(qBound(0, parts[0].trimmed().toInt(), 255),
                        qBound(0, parts[1].trimmed().toInt(), 255),
                        qBound(0, parts[2].trimmed().toInt(), 255));
        }

        inline static QString colorToString(QRgb color)
        {
            return QString("%1,%2,%3").arg(qRed(color)).arg(qGreen(color)).arg(qBlue(color));
        }
};

AkPaletteGroup::AkPaletteGroup(QObject *parent):
    QObject(parent)
{
    this->d = new AkPaletteGroupPrivate(this);
    this->d->loadDefaults();
    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteSyncRequested,
                     this,
                     &AkPaletteGroup::updatePalette);
    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteCopyRequested,
                     this,
                     &AkPaletteGroup::copyPalette);
}

AkPaletteGroup::AkPaletteGroup(QPalette::ColorGroup colorGroup):
    QObject()
{
    this->d = new AkPaletteGroupPrivate(this);
    this->d->m_colorGroup = colorGroup;
    this->d->loadDefaults();
    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteSyncRequested,
                     this,
                     &AkPaletteGroup::updatePalette);
    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteCopyRequested,
                     this,
                     &AkPaletteGroup::copyPalette);
}

AkPaletteGroup::AkPaletteGroup(const AkPaletteGroup &other):
    QObject()
{
    this->d = new AkPaletteGroupPrivate(this);
    this->d->m_fixed = other.d->m_fixed;
    this->d->m_isDark = other.d->m_isDark;
    this->d->m_colorGroup = other.d->m_colorGroup;
    this->d->m_highlightedText = other.d->m_highlightedText;
    this->d->m_highlight = other.d->m_highlight;
    this->d->m_text = other.d->m_text;
    this->d->m_placeholderText = other.d->m_placeholderText;
    this->d->m_base = other.d->m_base;
    this->d->m_alternateBase = other.d->m_alternateBase;
    this->d->m_windowText = other.d->m_windowText;
    this->d->m_window = other.d->m_window;
    this->d->m_buttonText = other.d->m_buttonText;
    this->d->m_light = other.d->m_light;
    this->d->m_midlight = other.d->m_midlight;
    this->d->m_button = other.d->m_button;
    this->d->m_mid = other.d->m_mid;
    this->d->m_dark = other.d->m_dark;
    this->d->m_shadow = other.d->m_shadow;
    this->d->m_toolTipText = other.d->m_toolTipText;
    this->d->m_toolTipBase = other.d->m_toolTipBase;
    this->d->m_link = other.d->m_link;
    this->d->m_linkVisited = other.d->m_linkVisited;

    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteSyncRequested,
                     this,
                     &AkPaletteGroup::updatePalette);
    QObject::connect(akPaletteGroupGlobalPrivate,
                     &AkPaletteGroupGlobalPrivate::paletteCopyRequested,
                     this,
                     &AkPaletteGroup::copyPalette);
}

AkPaletteGroup::~AkPaletteGroup()
{
    QObject::disconnect(akPaletteGroupGlobalPrivate,
                        &AkPaletteGroupGlobalPrivate::paletteSyncRequested,
                        this,
                        &AkPaletteGroup::updatePalette);
    QObject::disconnect(akPaletteGroupGlobalPrivate,
                        &AkPaletteGroupGlobalPrivate::paletteCopyRequested,
                        this,
                        &AkPaletteGroup::copyPalette);

    delete this->d;
}

AkPaletteGroup &AkPaletteGroup::operator =(const AkPaletteGroup &other)
{
    if (this != &other) {
        this->d->m_fixed = other.d->m_fixed;
        this->d->m_isDark = other.d->m_isDark;
        this->d->m_colorGroup = other.d->m_colorGroup;
        this->d->m_highlightedText = other.d->m_highlightedText;
        this->d->m_highlight = other.d->m_highlight;
        this->d->m_text = other.d->m_text;
        this->d->m_placeholderText = other.d->m_placeholderText;
        this->d->m_base = other.d->m_base;
        this->d->m_alternateBase = other.d->m_alternateBase;
        this->d->m_windowText = other.d->m_windowText;
        this->d->m_window = other.d->m_window;
        this->d->m_buttonText = other.d->m_buttonText;
        this->d->m_light = other.d->m_light;
        this->d->m_midlight = other.d->m_midlight;
        this->d->m_button = other.d->m_button;
        this->d->m_mid = other.d->m_mid;
        this->d->m_dark = other.d->m_dark;
        this->d->m_shadow = other.d->m_shadow;
        this->d->m_toolTipText = other.d->m_toolTipText;
        this->d->m_toolTipBase = other.d->m_toolTipBase;
        this->d->m_link = other.d->m_link;
        this->d->m_linkVisited = other.d->m_linkVisited;
    }

    return *this;
}

bool AkPaletteGroup::operator ==(const AkPaletteGroup &other) const
{
    return this->d->m_fixed == other.d->m_fixed
           && this->d->m_isDark == other.d->m_isDark
           && this->d->m_colorGroup == other.d->m_colorGroup
           && this->d->m_highlightedText == other.d->m_highlightedText
           && this->d->m_highlight == other.d->m_highlight
           && this->d->m_text == other.d->m_text
           && this->d->m_placeholderText == other.d->m_placeholderText
           && this->d->m_base == other.d->m_base
           && this->d->m_alternateBase == other.d->m_alternateBase
           && this->d->m_windowText == other.d->m_windowText
           && this->d->m_window == other.d->m_window
           && this->d->m_buttonText == other.d->m_buttonText
           && this->d->m_light == other.d->m_light
           && this->d->m_midlight == other.d->m_midlight
           && this->d->m_button == other.d->m_button
           && this->d->m_mid == other.d->m_mid
           && this->d->m_dark == other.d->m_dark
           && this->d->m_shadow == other.d->m_shadow
           && this->d->m_toolTipText == other.d->m_toolTipText
           && this->d->m_toolTipBase == other.d->m_toolTipBase
           && this->d->m_link == other.d->m_link
           && this->d->m_linkVisited == other.d->m_linkVisited;
}

bool AkPaletteGroup::fixed() const
{
    return this->d->m_fixed;
}

bool AkPaletteGroup::isDark() const
{
    return this->d->m_isDark;
}

QColor AkPaletteGroup::highlightedText() const
{
    return this->d->m_highlightedText;
}

QColor AkPaletteGroup::highlight() const
{
    return this->d->m_highlight;
}

QColor AkPaletteGroup::text() const
{
    return this->d->m_text;
}

QColor AkPaletteGroup::placeholderText() const
{
    return this->d->m_placeholderText;
}

QColor AkPaletteGroup::base() const
{
    return this->d->m_base;
}

QColor AkPaletteGroup::alternateBase() const
{
    return this->d->m_alternateBase;
}

QColor AkPaletteGroup::windowText() const
{
    return this->d->m_windowText;
}

QColor AkPaletteGroup::window() const
{
    return this->d->m_window;
}

QColor AkPaletteGroup::buttonText() const
{
    return this->d->m_buttonText;
}

QColor AkPaletteGroup::light() const
{
    return this->d->m_light;
}

QColor AkPaletteGroup::midlight() const
{
    return this->d->m_midlight;
}

QColor AkPaletteGroup::button() const
{
    return this->d->m_button;
}

QColor AkPaletteGroup::mid() const
{
    return this->d->m_mid;
}

QColor AkPaletteGroup::dark() const
{
    return this->d->m_dark;
}

QColor AkPaletteGroup::shadow() const
{
    return this->d->m_shadow;
}

QColor AkPaletteGroup::toolTipText() const
{
    return this->d->m_toolTipText;
}

QColor AkPaletteGroup::toolTipBase() const
{
    return this->d->m_toolTipBase;
}

QColor AkPaletteGroup::link() const
{
    return this->d->m_link;
}

QColor AkPaletteGroup::linkVisited() const
{
    return this->d->m_linkVisited;
}

bool AkPaletteGroup::canWrite(const QString &paletteName)
{
    if (paletteName == "System")
        return false;

    auto dataPaths =
            QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    if (dataPaths.size() < 2)
        return true;

    auto nonWritablePaths = dataPaths.mid(1) + QStringList {":/Webcamoid/share"};
    std::reverse(nonWritablePaths.begin(), nonWritablePaths.end());

    for (auto &path: nonWritablePaths) {
        auto themesPath = QString("%1/themes").arg(path);

        for (auto &theme: QDir(themesPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            auto colorsPath = QString("%1/%2/colors").arg(themesPath).arg(theme);

            for (auto &themeFile: QDir(colorsPath).entryList({QString("*.%1").arg(COLORS_FILE_EXT)}, QDir::Files, QDir::Name)) {
                auto colorFile = QString("%1/%2").arg(colorsPath).arg(themeFile);
                QSettings config(colorFile, QSettings::IniFormat);

                config.beginGroup("Theme");
                auto themeName = config.value("name").toString();
                config.endGroup();

                if (themeName == paletteName)
                    return false;
            }
        }
    }

    return true;
}

void AkPaletteGroup::setFixed(bool fixed)
{
    if (this->d->m_fixed == fixed)
        return;

    this->d->m_fixed = fixed;
    emit this->fixedChanged(this->d->m_fixed);
}

void AkPaletteGroup::setHighlightedText(const QColor &highlightedText)
{
    if (this->d->m_highlightedText == highlightedText)
        return;

    this->d->m_highlightedText = highlightedText;
    emit this->highlightedTextChanged(this->d->m_highlightedText);
}

void AkPaletteGroup::setHighlight(const QColor &highlight)
{
    if (this->d->m_highlight == highlight)
        return;

    this->d->m_highlight = highlight;
    emit this->highlightChanged(this->d->m_highlight);
}

void AkPaletteGroup::setText(const QColor &text)
{
    if (this->d->m_text == text)
        return;

    this->d->m_text = text;
    emit this->textChanged(this->d->m_text);
}

void AkPaletteGroup::setPlaceholderText(const QColor &placeholderText)
{
    if (this->d->m_placeholderText == placeholderText)
        return;

    this->d->m_placeholderText = placeholderText;
    emit this->placeholderTextChanged(this->d->m_placeholderText);
}

void AkPaletteGroup::setBase(const QColor &base)
{
    if (this->d->m_base == base)
        return;

    this->d->m_base = base;
    emit this->baseChanged(this->d->m_base);
}

void AkPaletteGroup::setAlternateBase(const QColor &alternateBase)
{
    if (this->d->m_alternateBase == alternateBase)
        return;

    this->d->m_alternateBase = alternateBase;
    emit this->alternateBaseChanged(this->d->m_alternateBase);
}

void AkPaletteGroup::setWindowText(const QColor &windowText)
{
    if (this->d->m_windowText == windowText)
        return;

    this->d->m_windowText = windowText;
    auto isDark = this->d->m_window.lightness() < this->d->m_windowText.lightness();

    if (this->d->m_isDark != isDark) {
        this->d->m_isDark = isDark;
        emit this->isDarkChanged(this->d->m_isDark);
    }

    emit this->windowTextChanged(this->d->m_windowText);
}

void AkPaletteGroup::setWindow(const QColor &window)
{
    if (this->d->m_window == window)
        return;

    this->d->m_window = window;
    auto isDark = this->d->m_window.lightness() < this->d->m_windowText.lightness();

    if (this->d->m_isDark != isDark) {
        this->d->m_isDark = isDark;
        emit this->isDarkChanged(this->d->m_isDark);
    }
    emit this->windowChanged(this->d->m_window);
}

void AkPaletteGroup::setButtonText(const QColor &buttonText)
{
    if (this->d->m_buttonText == buttonText)
        return;

    this->d->m_buttonText = buttonText;
    emit this->buttonTextChanged(this->d->m_buttonText);
}

void AkPaletteGroup::setLight(const QColor &light)
{
    if (this->d->m_light == light)
        return;

    this->d->m_light = light;
    emit this->lightChanged(this->d->m_light);
}

void AkPaletteGroup::setMidlight(const QColor &midlight)
{
    if (this->d->m_midlight == midlight)
        return;

    this->d->m_midlight = midlight;
    emit this->midlightChanged(this->d->m_midlight);
}

void AkPaletteGroup::setButton(const QColor &button)
{
    if (this->d->m_button == button)
        return;

    this->d->m_button = button;
    emit this->buttonChanged(this->d->m_button);
}

void AkPaletteGroup::setMid(const QColor &mid)
{
    if (this->d->m_mid == mid)
        return;

    this->d->m_mid = mid;
    emit this->midChanged(this->d->m_mid);
}

void AkPaletteGroup::setDark(const QColor &dark)
{
    if (this->d->m_dark == dark)
        return;

    this->d->m_dark = dark;
    emit this->darkChanged(this->d->m_dark);
}

void AkPaletteGroup::setShadow(const QColor &shadow)
{
    if (this->d->m_shadow == shadow)
        return;

    this->d->m_shadow = shadow;
    emit this->shadowChanged(this->d->m_shadow);
}

void AkPaletteGroup::setToolTipText(const QColor &toolTipText)
{
    if (this->d->m_toolTipText == toolTipText)
        return;

    this->d->m_toolTipText = toolTipText;
    emit this->toolTipTextChanged(this->d->m_toolTipText);
}

void AkPaletteGroup::setToolTipBase(const QColor &toolTipBase)
{
    if (this->d->m_toolTipBase == toolTipBase)
        return;

    this->d->m_toolTipBase = toolTipBase;
    emit this->toolTipBaseChanged(this->d->m_toolTipBase);
}

void AkPaletteGroup::setLink(const QColor &link)
{
    if (this->d->m_link == link)
        return;

    this->d->m_link = link;
    emit this->linkChanged(this->d->m_link);
}

void AkPaletteGroup::setLinkVisited(const QColor &linkVisited)
{
    if (this->d->m_linkVisited == linkVisited)
        return;

    this->d->m_linkVisited = linkVisited;
    emit this->linkVisitedChanged(this->d->m_linkVisited);
}

void AkPaletteGroup::resetFixed()
{
    this->setFixed(false);
}

void AkPaletteGroup::resetHighlightedText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setHighlightedText(palRef->highlightedText);
}

void AkPaletteGroup::resetHighlight()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setHighlight(palRef->highlight);
}

void AkPaletteGroup::resetText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setText(palRef->text);
}

void AkPaletteGroup::resetPlaceholderText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setPlaceholderText(palRef->placeholderText);
}

void AkPaletteGroup::resetBase()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setBase(palRef->base);
}

void AkPaletteGroup::resetAlternateBase()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setAlternateBase(palRef->alternateBase);
}

void AkPaletteGroup::resetWindowText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setWindowText(palRef->windowText);
}

void AkPaletteGroup::resetWindow()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setWindow(palRef->window);
}

void AkPaletteGroup::resetButtonText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setButtonText(palRef->buttonText);
}

void AkPaletteGroup::resetLight()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setLight(palRef->light);
}

void AkPaletteGroup::resetMidlight()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setMidlight(palRef->midlight);
}

void AkPaletteGroup::resetButton()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setButton(palRef->button);
}

void AkPaletteGroup::resetMid()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setMid(palRef->mid);
}

void AkPaletteGroup::resetDark()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setDark(palRef->dark);
}

void AkPaletteGroup::resetShadow()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setShadow(palRef->shadow);
}

void AkPaletteGroup::resetToolTipText()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setToolTipText(palRef->toolTipText);
}

void AkPaletteGroup::resetToolTipBase()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setToolTipBase(palRef->toolTipBase);
}

void AkPaletteGroup::resetLink()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setLink(palRef->link);
}

void AkPaletteGroup::resetLinkVisited()
{
    auto palRef = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->d->m_colorGroup);
    this->setLinkVisited(palRef->linkVisited);
}

bool AkPaletteGroup::load(const QString &paletteName)
{
    if (this->d->m_fixed)
        return false;

    auto palette = AkPaletteGroupPrivate::readPalette(paletteName);
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setHighlightedText(palette.highlightedText().color());
    this->setHighlight(palette.highlight().color());
    this->setText(palette.text().color());
    this->setPlaceholderText(palette.placeholderText().color());
    this->setBase(palette.base().color());
    this->setAlternateBase(palette.alternateBase().color());
    this->setWindowText(palette.windowText().color());
    this->setWindow(palette.window().color());
    this->setButtonText(palette.buttonText().color());
    this->setLight(palette.light().color());
    this->setMidlight(palette.midlight().color());
    this->setButton(palette.button().color());
    this->setMid(palette.mid().color());
    this->setDark(palette.dark().color());
    this->setShadow(palette.shadow().color());
    this->setToolTipText(palette.toolTipText().color());
    this->setToolTipBase(palette.toolTipBase().color());
    this->setLink(palette.link().color());
    this->setLinkVisited(palette.linkVisited().color());

    return true;
}

QString AkPaletteGroup::loadFromFileName(const QString &fileName)
{
    if (this->d->m_fixed)
        return {};

    auto palette = AkPaletteGroupPrivate::readPaletteFromFileName(fileName);
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setHighlightedText(palette.highlightedText().color());
    this->setHighlight(palette.highlight().color());
    this->setText(palette.text().color());
    this->setPlaceholderText(palette.placeholderText().color());
    this->setBase(palette.base().color());
    this->setAlternateBase(palette.alternateBase().color());
    this->setWindowText(palette.windowText().color());
    this->setWindow(palette.window().color());
    this->setButtonText(palette.buttonText().color());
    this->setLight(palette.light().color());
    this->setMidlight(palette.midlight().color());
    this->setButton(palette.button().color());
    this->setMid(palette.mid().color());
    this->setDark(palette.dark().color());
    this->setShadow(palette.shadow().color());
    this->setToolTipText(palette.toolTipText().color());
    this->setToolTipBase(palette.toolTipBase().color());
    this->setLink(palette.link().color());
    this->setLinkVisited(palette.linkVisited().color());

    if (fileName.isEmpty() || !QFileInfo::exists(fileName))
        return {};

    QSettings config(fileName, QSettings::IniFormat);

    config.beginGroup("Theme");
    auto paletteName = config.value("name").toString();
    config.endGroup();

    return paletteName;
}

#define SYM_TO_STR(sym) #sym
#define WRITE_VALUE(prop) config.setValue(SYM_TO_STR(prop), AkPaletteGroupPrivate::colorToString(this->d->m_##prop.rgb()))

bool AkPaletteGroup::saveToFileName(const QString &fileName,
                                    const QString &paletteName)
{
    QSettings config(fileName, QSettings::IniFormat);

    config.beginGroup("Theme");
    config.setValue("name", paletteName);
    config.endGroup();

    QMap<QPalette::ColorGroup, QString> cgToStr {
        {QPalette::Active  , "Active"  },
        {QPalette::Disabled, "Disabled"},
        {QPalette::Inactive, "Inactive"},
    };

    config.beginGroup(cgToStr.value(this->d->m_colorGroup));

    WRITE_VALUE(highlightedText);
    WRITE_VALUE(highlight);
    WRITE_VALUE(text);
    WRITE_VALUE(placeholderText);
    WRITE_VALUE(base);
    WRITE_VALUE(alternateBase);
    WRITE_VALUE(windowText);
    WRITE_VALUE(window);
    WRITE_VALUE(buttonText);
    WRITE_VALUE(light);
    WRITE_VALUE(midlight);
    WRITE_VALUE(button);
    WRITE_VALUE(mid);
    WRITE_VALUE(dark);
    WRITE_VALUE(shadow);
    WRITE_VALUE(toolTipText);
    WRITE_VALUE(toolTipBase);
    WRITE_VALUE(link);
    WRITE_VALUE(linkVisited);

    config.endGroup();

    return true;
}

#undef WRITE_VALUE

bool AkPaletteGroup::save(const QString &paletteName)
{
    auto dataPaths =
            QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);

    if (dataPaths.isEmpty())
        return false;

    if (!canWrite(paletteName))
        return false;

    auto themesPath = QString("%1/themes").arg(dataPaths[0]);
    auto colorsPath =
            QString("%1/%2/colors").arg(themesPath).arg(paletteName);

    if (!QDir().mkpath(colorsPath))
        return false;

    return this->saveToFileName(QString("%1/%2." COLORS_FILE_EXT)
                                .arg(colorsPath).arg(paletteName),
                                paletteName);
}

void AkPaletteGroup::sync()
{
    emit akPaletteGroupGlobalPrivate->paletteSyncRequested();
}

void AkPaletteGroup::apply()
{
    emit akPaletteGroupGlobalPrivate->paletteCopyRequested(*this);
}

void AkPaletteGroup::registerTypes()
{
    qRegisterMetaType<AkPaletteGroup>("AkPaletteGroup");
    qmlRegisterType<AkPaletteGroup>("Ak", 1, 0, "AkPaletteGroup");
}

void AkPaletteGroup::updatePalette()
{
    this->d->updatePalette();
}

void AkPaletteGroup::copyPalette(const AkPaletteGroup &paletteGroup)
{
    this->d->copyPalette(paletteGroup);
}

AkPaletteGroupPrivate::AkPaletteGroupPrivate(AkPaletteGroup *self):
    self(self)
{

}

QString AkPaletteGroupPrivate::configFileForPalette(const QString &paletteName)
{
    if (paletteName == "System")
        return {};

    auto dataPaths =
            QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)
            + QStringList {":/Webcamoid/share"};
    std::reverse(dataPaths.begin(), dataPaths.end());

    for (auto &path: dataPaths) {
        auto themesPath = QString("%1/themes").arg(path);

        for (auto &theme: QDir(themesPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            auto colorsPath = QString("%1/%2/colors").arg(themesPath).arg(theme);

            for (auto &themeFile: QDir(colorsPath).entryList({"*." COLORS_FILE_EXT}, QDir::Files, QDir::Name)) {
                auto configFile = QString("%1/%2").arg(colorsPath).arg(themeFile);
                QSettings config(configFile, QSettings::IniFormat);

                config.beginGroup("Theme");
                auto themeName = config.value("name").toString();
                config.endGroup();

                if (themeName == paletteName)
                    return configFile;
            }
        }
    }

    return {};
}

QPalette AkPaletteGroupPrivate::readPalette(const QString &paletteName)
{
    return readPaletteFromFileName(configFileForPalette(paletteName));
}

QPalette AkPaletteGroupPrivate::readPaletteFromFileName(const QString &fileName)
{
    auto palette = QGuiApplication::palette();

    QMap<QPalette::ColorGroup, QString> cgToStr {
        {QPalette::Active  , "Active"  },
        {QPalette::Disabled, "Disabled"},
        {QPalette::Inactive, "Inactive"},
    };

    for (auto it = cgToStr.constBegin();
         it != cgToStr.constEnd();
         ++it) {
        auto colorGroup = it.key();
        palette.setCurrentColorGroup(colorGroup);

        auto highlightDiff =
                AkTheme::distance(palette.highlight().color(),
                                  palette.highlightedText().color());

        if (highlightDiff < 0.5) {
            auto highlightedText =
                    AkTheme::contrast(palette.highlight().color());
            palette.setColor(colorGroup,
                             QPalette::HighlightedText,
                             highlightedText);
        }

        auto light = palette.window().color().lightnessF() < 0.5?
                         palette.dark().color():
                         palette.light().color();
        auto midlight = palette.window().color().lightnessF() < 0.5?
                            palette.mid().color():
                            palette.midlight().color();
        auto mid = palette.window().color().lightnessF() < 0.5?
                       palette.midlight().color():
                       palette.mid().color();
        auto dark = palette.window().color().lightnessF() < 0.5?
                        palette.light().color():
                        palette.dark().color();

        palette.setColor(colorGroup, QPalette::Light, light);
        palette.setColor(colorGroup, QPalette::Midlight, midlight);
        palette.setColor(colorGroup, QPalette::Mid, mid);
        palette.setColor(colorGroup, QPalette::Dark, dark);
    }

    if (fileName.isEmpty())
        return palette;

    QSettings configs(fileName, QSettings::IniFormat);

    for (auto it = cgToStr.constBegin();
         it != cgToStr.constEnd();
         ++it) {
        auto colorGroup = it.key();
        configs.beginGroup(it.value());

        bool ok = false;
        auto color = AkPaletteGroupPrivate::colorFromString(configs.value("highlightedText").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::HighlightedText, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("highlight").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Highlight, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("text").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Text, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("placeholderText").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::PlaceholderText, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("base").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Base, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("alternateBase").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::AlternateBase, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("windowText").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::WindowText, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("window").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Window, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("buttonText").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::ButtonText, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("button").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Button, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("light").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Light, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("midlight").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Midlight, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("mid").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Mid, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("dark").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Dark, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("shadow").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Shadow, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("toolTipText").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::ToolTipText, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("toolTipBase").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::ToolTipBase, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("link").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::Link, color);

        color = AkPaletteGroupPrivate::colorFromString(configs.value("linkVisited").toString(), &ok);

        if (ok)
            palette.setColor(colorGroup, QPalette::LinkVisited, color);

        configs.endGroup();
    }

    return palette;
}

void AkPaletteGroupPrivate::loadDefaults()
{
    auto palette = akPaletteGroupGlobalPrivate->m_groupToPaletteMap.value(this->m_colorGroup);
    this->m_highlightedText = palette->highlightedText;
    this->m_highlight = palette->highlight;
    this->m_text = palette->text;
    this->m_placeholderText = palette->placeholderText;
    this->m_base = palette->base;
    this->m_alternateBase = palette->alternateBase;
    this->m_windowText = palette->windowText;
    this->m_window = palette->window;
    this->m_buttonText = palette->buttonText;
    this->m_light = palette->light;
    this->m_midlight = palette->midlight;
    this->m_button = palette->button;
    this->m_mid = palette->mid;
    this->m_dark = palette->dark;
    this->m_shadow = palette->shadow;
    this->m_toolTipText = palette->toolTipText;
    this->m_toolTipBase = palette->toolTipBase;
    this->m_link = palette->link;
    this->m_linkVisited = palette->linkVisited;
    this->m_isDark = this->m_window.lightness() < this->m_windowText.lightness();
}

void AkPaletteGroupPrivate::updatePalette()
{
    QSettings config;

    config.beginGroup("ThemeConfigs");
    self->load(config.value("paletteName").toString());
    config.endGroup();
}

void AkPaletteGroupPrivate::copyPalette(const AkPaletteGroup &paletteGroup)
{
    if (this->m_fixed || this->m_colorGroup != paletteGroup.d->m_colorGroup)
        return;

    this->m_colorGroup = paletteGroup.d->m_colorGroup;
    self->setHighlightedText(paletteGroup.highlightedText());
    self->setHighlight(paletteGroup.highlight());
    self->setText(paletteGroup.text());
    self->setPlaceholderText(paletteGroup.placeholderText());
    self->setBase(paletteGroup.base());
    self->setAlternateBase(paletteGroup.alternateBase());
    self->setWindowText(paletteGroup.windowText());
    self->setWindow(paletteGroup.window());
    self->setButtonText(paletteGroup.buttonText());
    self->setLight(paletteGroup.light());
    self->setMidlight(paletteGroup.midlight());
    self->setButton(paletteGroup.button());
    self->setMid(paletteGroup.mid());
    self->setDark(paletteGroup.dark());
    self->setShadow(paletteGroup.shadow());
    self->setToolTipText(paletteGroup.toolTipText());
    self->setToolTipBase(paletteGroup.toolTipBase());
    self->setLink(paletteGroup.link());
    self->setLinkVisited(paletteGroup.linkVisited());
}

AkPaletteGroupGlobalPrivate::AkPaletteGroupGlobalPrivate(QObject *parent):
    QObject(parent)
{
    this->m_groupToPaletteMap = {
        {QPalette::Active  , &this->m_palettes.active  },
        {QPalette::Disabled, &this->m_palettes.disabled},
        {QPalette::Inactive, &this->m_palettes.inactive},
    };

    this->loadDefaults();

    QObject::connect(this,
                     &AkPaletteGroupGlobalPrivate::paletteSyncRequested,
                     this,
                     &AkPaletteGroupGlobalPrivate::loadDefaults);
    QObject::connect(this,
                     &AkPaletteGroupGlobalPrivate::paletteCopyRequested,
                     this,
                     &AkPaletteGroupGlobalPrivate::loadDefaults);
}

void AkPaletteGroupGlobalPrivate::loadDefaults()
{
    QSettings config;

    config.beginGroup("ThemeConfigs");
    auto palette = AkPaletteGroupPrivate::readPalette(config.value("paletteName").toString());
    config.endGroup();

    QVector<QPalette::ColorGroup> colorGroups {
        QPalette::Active  ,
        QPalette::Disabled,
        QPalette::Inactive,
    };

    for (auto &group: colorGroups) {
        palette.setCurrentColorGroup(group);
        auto palRef = this->m_groupToPaletteMap.value(group);
        palRef->highlightedText = palette.highlightedText().color();
        palRef->highlight = palette.highlight().color();
        palRef->text = palette.text().color();
        palRef->placeholderText = palette.placeholderText().color();
        palRef->base = palette.base().color();
        palRef->alternateBase = palette.alternateBase().color();
        palRef->windowText = palette.windowText().color();
        palRef->window = palette.window().color();
        palRef->buttonText = palette.buttonText().color();
        palRef->light = palette.light().color();
        palRef->midlight = palette.midlight().color();
        palRef->button = palette.button().color();
        palRef->mid = palette.mid().color();
        palRef->dark = palette.dark().color();
        palRef->shadow = palette.shadow().color();
        palRef->toolTipText = palette.toolTipText().color();
        palRef->toolTipBase = palette.toolTipBase().color();
        palRef->link = palette.link().color();
        palRef->linkVisited = palette.linkVisited().color();
    }
}

#include "akpalettegroup.moc"
#include "moc_akpalettegroup.cpp"
