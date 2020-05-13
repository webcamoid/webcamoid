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

#include <QGuiApplication>
#include <QQmlEngine>

#include "akpalettegroup.h"

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

        AkPaletteGroupPrivate(AkPaletteGroup *self);
};

AkPaletteGroup::AkPaletteGroup(QObject *parent):
    QObject(parent)
{
    this->d = new AkPaletteGroupPrivate(this);
    auto palette = QGuiApplication::palette();
    this->d->m_highlightedText = palette.highlightedText().color();
    this->d->m_highlight = palette.highlight().color();
    this->d->m_text = palette.text().color();
    this->d->m_placeholderText = palette.placeholderText().color();
    this->d->m_base = palette.base().color();
    this->d->m_windowText = palette.windowText().color();
    this->d->m_window = palette.window().color();
    this->d->m_buttonText = palette.buttonText().color();
    this->d->m_light = palette.window().color().lightnessF() < 0.5?
                           palette.dark().color():
                           palette.light().color();
    this->d->m_midlight = palette.window().color().lightnessF() < 0.5?
                              palette.mid().color():
                              palette.midlight().color();
    this->d->m_button = palette.button().color();
    this->d->m_mid = palette.window().color().lightnessF() < 0.5?
                         palette.midlight().color():
                         palette.mid().color();
    this->d->m_dark = palette.window().color().lightnessF() < 0.5?
                          palette.light().color():
                          palette.dark().color();
    this->d->m_shadow = palette.shadow().color();
    this->d->m_toolTipText = palette.toolTipText().color();
    this->d->m_toolTipBase = palette.toolTipBase().color();
    this->d->m_link = palette.link().color();
    this->d->m_linkVisited = palette.linkVisited().color();

    QObject::connect(qGuiApp,
                     &QGuiApplication::paletteChanged,
                     this,
                     &AkPaletteGroup::updatePalette);
}

AkPaletteGroup::AkPaletteGroup(QPalette::ColorGroup colorGroup)
{
    this->d = new AkPaletteGroupPrivate(this);
    this->d->m_colorGroup = colorGroup;
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->d->m_highlightedText = palette.highlightedText().color();
    this->d->m_highlight = palette.highlight().color();
    this->d->m_text = palette.text().color();
    this->d->m_placeholderText = palette.placeholderText().color();
    this->d->m_base = palette.base().color();
    this->d->m_windowText = palette.windowText().color();
    this->d->m_window = palette.window().color();
    this->d->m_buttonText = palette.buttonText().color();
    this->d->m_light = palette.window().color().lightnessF() < 0.5?
                           palette.dark().color():
                           palette.light().color();
    this->d->m_midlight = palette.window().color().lightnessF() < 0.5?
                              palette.mid().color():
                              palette.midlight().color();
    this->d->m_button = palette.button().color();
    this->d->m_mid = palette.window().color().lightnessF() < 0.5?
                         palette.midlight().color():
                         palette.mid().color();
    this->d->m_dark = palette.window().color().lightnessF() < 0.5?
                          palette.light().color():
                          palette.dark().color();
    this->d->m_shadow = palette.shadow().color();
    this->d->m_toolTipText = palette.toolTipText().color();
    this->d->m_toolTipBase = palette.toolTipBase().color();
    this->d->m_link = palette.link().color();
    this->d->m_linkVisited = palette.linkVisited().color();

    QObject::connect(qGuiApp,
                     &QGuiApplication::paletteChanged,
                     this,
                     &AkPaletteGroup::updatePalette);
}

AkPaletteGroup::AkPaletteGroup(const AkPaletteGroup &other):
    QObject()
{
    this->d = new AkPaletteGroupPrivate(this);
    this->d->m_colorGroup = other.d->m_colorGroup;
    this->d->m_highlightedText = other.d->m_highlightedText;
    this->d->m_highlight = other.d->m_highlight;
    this->d->m_text = other.d->m_text;
    this->d->m_placeholderText = other.d->m_placeholderText;
    this->d->m_base = other.d->m_base;
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

    QObject::connect(qGuiApp,
                     &QGuiApplication::paletteChanged,
                     this,
                     &AkPaletteGroup::updatePalette);
}

AkPaletteGroup::~AkPaletteGroup()
{
    delete this->d;
}

AkPaletteGroup &AkPaletteGroup::operator =(const AkPaletteGroup &other)
{
    if (this != &other) {
        this->d->m_colorGroup = other.d->m_colorGroup;
        this->d->m_highlightedText = other.d->m_highlightedText;
        this->d->m_highlight = other.d->m_highlight;
        this->d->m_text = other.d->m_text;
        this->d->m_placeholderText = other.d->m_placeholderText;
        this->d->m_base = other.d->m_base;
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
    return this->d->m_colorGroup == other.d->m_colorGroup
           && this->d->m_highlightedText == other.d->m_highlightedText
           && this->d->m_highlight == other.d->m_highlight
           && this->d->m_text == other.d->m_text
           && this->d->m_placeholderText == other.d->m_placeholderText
           && this->d->m_base == other.d->m_base
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
    emit placeholderTextChanged(this->d->m_placeholderText);
}

void AkPaletteGroup::setBase(const QColor &base)
{
    if (this->d->m_base == base)
        return;

    this->d->m_base = base;
    emit baseChanged(this->d->m_base);
}

void AkPaletteGroup::setWindowText(const QColor &windowText)
{
    if (this->d->m_windowText == windowText)
        return;

    this->d->m_windowText = windowText;
    emit this->windowTextChanged(this->d->m_windowText);
}

void AkPaletteGroup::setWindow(const QColor &window)
{
    if (this->d->m_window == window)
        return;

    this->d->m_window = window;
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

void AkPaletteGroup::resetHighlightedText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setHighlightedText(palette.highlightedText().color());
}

void AkPaletteGroup::resetHighlight()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setHighlight(palette.highlight().color());
}

void AkPaletteGroup::resetText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setText(palette.text().color());
}

void AkPaletteGroup::resetPlaceholderText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setPlaceholderText(palette.placeholderText().color());
}

void AkPaletteGroup::resetBase()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setBase(palette.base().color());
}

void AkPaletteGroup::resetWindowText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setWindowText(palette.windowText().color());
}

void AkPaletteGroup::resetWindow()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setWindow(palette.window().color());
}

void AkPaletteGroup::resetButtonText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setButtonText(palette.buttonText().color());
}

void AkPaletteGroup::resetLight()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setLight(palette.light().color());
}

void AkPaletteGroup::resetMidlight()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setMidlight(palette.midlight().color());
}

void AkPaletteGroup::resetButton()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setButton(palette.button().color());
}

void AkPaletteGroup::resetMid()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setMid(palette.mid().color());
}

void AkPaletteGroup::resetDark()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setDark(palette.dark().color());
}

void AkPaletteGroup::resetShadow()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setShadow(palette.shadow().color());
}

void AkPaletteGroup::resetToolTipText()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setToolTipText(palette.toolTipText().color());
}

void AkPaletteGroup::resetToolTipBase()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setToolTipBase(palette.toolTipBase().color());
}

void AkPaletteGroup::resetLink()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setLink(palette.link().color());
}

void AkPaletteGroup::resetLinkVisited()
{
    auto palette = QGuiApplication::palette();
    palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setLinkVisited(palette.linkVisited().color());
}

void AkPaletteGroup::registerTypes()
{
    qRegisterMetaType<AkPaletteGroup>("AkPaletteGroup");
    qmlRegisterType<AkPaletteGroup>("Ak", 1, 0, "AkPaletteGroup");
}

void AkPaletteGroup::updatePalette(const QPalette &palette)
{
    auto _palette = palette;
    _palette.setCurrentColorGroup(this->d->m_colorGroup);
    this->setHighlightedText(_palette.highlightedText().color());
    this->setHighlight(_palette.highlight().color());
    this->setText(_palette.text().color());
    this->setPlaceholderText(_palette.placeholderText().color());
    this->setBase(_palette.base().color());
    this->setWindowText(_palette.windowText().color());
    this->setWindow(_palette.window().color());
    this->setButtonText(_palette.buttonText().color());
    this->setLight(_palette.window().color().lightnessF() < 0.5?
                       _palette.dark().color():
                       _palette.light().color());
    this->setMidlight(_palette.window().color().lightnessF() < 0.5?
                          _palette.mid().color():
                          _palette.midlight().color());
    this->setButton(_palette.button().color());
    this->setMid(_palette.window().color().lightnessF() < 0.5?
                     _palette.midlight().color():
                     _palette.mid().color());
    this->setDark(_palette.window().color().lightnessF() < 0.5?
                      _palette.light().color():
                      _palette.dark().color());
    this->setShadow(_palette.shadow().color());
    this->setToolTipText(_palette.toolTipText().color());
    this->setToolTipBase(_palette.toolTipBase().color());
    this->setLink(_palette.link().color());
    this->setLinkVisited(_palette.linkVisited().color());
}

AkPaletteGroupPrivate::AkPaletteGroupPrivate(AkPaletteGroup *self):
    self(self)
{

}

#include "moc_akpalettegroup.cpp"
