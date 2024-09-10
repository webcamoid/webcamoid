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

#include <QFont>
#include <QGuiApplication>
#include <QQmlEngine>

#include "akfontsettings.h"

#ifdef Q_OS_ANDROID
    #define DEFAULT_POINT_SIZE (14 * 100 / 72)
#else
    #define DEFAULT_POINT_SIZE 9
#endif

#define MINIMUM_POINT_SIZE 1

class AkFontSettingsPrivate
{
    public:
        AkFontSettings *self;
        QFont m_h1;
        QFont m_h2;
        QFont m_h3;
        QFont m_h4;
        QFont m_h5;
        QFont m_h6;
        QFont m_subtitle1;
        QFont m_subtitle2;
        QFont m_body1;
        QFont m_body2;
        QFont m_button;
        QFont m_caption;
        QFont m_overline;

        explicit AkFontSettingsPrivate(AkFontSettings *self);
};

AkFontSettings::AkFontSettings(QObject *parent):
    QObject(parent)
{
    this->d = new AkFontSettingsPrivate(this);
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(96 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Light);
    this->d->m_h1 = font;
    font.setPointSize(qMax(60 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Light);
    this->d->m_h2 = font;
    font.setPointSize(qMax(48 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_h3 = font;
    font.setPointSize(qMax(34 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_h4 = font;
    font.setPointSize(qMax(24 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_h5 = font;
    font.setPointSize(qMax(20 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Medium);
    this->d->m_h6 = font;
    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_subtitle1 = font;
    font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Medium);
    this->d->m_subtitle2 = font;
    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_body1 = font;
    font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_body2 = font;
    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Bold);
    this->d->m_button = font;
    font.setPointSize(qMax(12 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_caption = font;
    font.setPointSize(qMax(10 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->d->m_overline = font;
}

AkFontSettings::AkFontSettings(const QFont &font):
    QObject()
{
    this->d = new AkFontSettingsPrivate(this);
    auto _font = font;
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    _font.setPointSize(qMax(96 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Light);
    this->d->m_h1 = _font;
    _font.setPointSize(qMax(60 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Light);
    this->d->m_h2 = _font;
    _font.setPointSize(qMax(48 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_h3 = _font;
    _font.setPointSize(qMax(34 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_h4 = _font;
    _font.setPointSize(qMax(24 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_h5 = _font;
    _font.setPointSize(qMax(20 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Medium);
    this->d->m_h6 = _font;
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_subtitle1 = _font;
    _font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Medium);
    this->d->m_subtitle2 = _font;
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_body1 = _font;
    _font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_body2 = _font;
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Bold);
    this->d->m_button = _font;
    _font.setPointSize(qMax(12 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_caption = _font;
    _font.setPointSize(qMax(10 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->d->m_overline = _font;
}

AkFontSettings::AkFontSettings(const AkFontSettings &other):
    QObject()
{
    this->d = new AkFontSettingsPrivate(this);
    this->d->m_h1 = other.d->m_h1;
    this->d->m_h2 = other.d->m_h2;
    this->d->m_h3 = other.d->m_h3;
    this->d->m_h4 = other.d->m_h4;
    this->d->m_h5 = other.d->m_h5;
    this->d->m_h6 = other.d->m_h6;
    this->d->m_subtitle1 = other.d->m_subtitle1;
    this->d->m_subtitle2 = other.d->m_subtitle2;
    this->d->m_body1 = other.d->m_body1;
    this->d->m_body2 = other.d->m_body2;
    this->d->m_button = other.d->m_button;
    this->d->m_caption = other.d->m_caption;
    this->d->m_overline = other.d->m_overline;
}

AkFontSettings::~AkFontSettings()
{
    delete this->d;
}

AkFontSettings &AkFontSettings::operator =(const AkFontSettings &other)
{
    if (this != &other) {
        this->d->m_h1 = other.d->m_h1;
        this->d->m_h2 = other.d->m_h2;
        this->d->m_h3 = other.d->m_h3;
        this->d->m_h4 = other.d->m_h4;
        this->d->m_h5 = other.d->m_h5;
        this->d->m_h6 = other.d->m_h6;
        this->d->m_subtitle1 = other.d->m_subtitle1;
        this->d->m_subtitle2 = other.d->m_subtitle2;
        this->d->m_body1 = other.d->m_body1;
        this->d->m_body2 = other.d->m_body2;
        this->d->m_button = other.d->m_button;
        this->d->m_caption = other.d->m_caption;
        this->d->m_overline = other.d->m_overline;
    }

    return *this;
}

bool AkFontSettings::operator ==(const AkFontSettings &other) const
{
    return this->d->m_h1 == other.d->m_h1
           && this->d->m_h2 == other.d->m_h2
           && this->d->m_h3 == other.d->m_h3
           && this->d->m_h4 == other.d->m_h4
           && this->d->m_h5 == other.d->m_h5
           && this->d->m_h6 == other.d->m_h6
           && this->d->m_subtitle1 == other.d->m_subtitle1
           && this->d->m_subtitle2 == other.d->m_subtitle2
           && this->d->m_body1 == other.d->m_body1
           && this->d->m_body2 == other.d->m_body2
           && this->d->m_button == other.d->m_button
           && this->d->m_caption == other.d->m_caption
           && this->d->m_overline == other.d->m_overline;
}

QFont AkFontSettings::h1() const
{
    return this->d->m_h1;
}

QFont AkFontSettings::h2() const
{
    return this->d->m_h2;
}

QFont AkFontSettings::h3() const
{
    return this->d->m_h3;
}

QFont AkFontSettings::h4() const
{
    return this->d->m_h4;
}

QFont AkFontSettings::h5() const
{
    return this->d->m_h5;
}

QFont AkFontSettings::h6() const
{
    return this->d->m_h6;
}

QFont AkFontSettings::subtitle1() const
{
    return this->d->m_subtitle1;
}

QFont AkFontSettings::subtitle2() const
{
    return this->d->m_subtitle2;
}

QFont AkFontSettings::body1() const
{
    return this->d->m_body1;
}

QFont AkFontSettings::body2() const
{
    return this->d->m_body2;
}

QFont AkFontSettings::button() const
{
    return this->d->m_button;
}

QFont AkFontSettings::caption() const
{
    return this->d->m_caption;
}

QFont AkFontSettings::overline() const
{
    return this->d->m_overline;
}

void AkFontSettings::setH1(const QFont &h1)
{
    if (this->d->m_h1 == h1)
        return;

    this->d->m_h1 = h1;
    emit this->h1Changed(this->d->m_h1);
}

void AkFontSettings::setH2(const QFont &h2)
{
    if (this->d->m_h2 == h2)
        return;

    this->d->m_h2 = h2;
    emit this->h2Changed(this->d->m_h2);
}

void AkFontSettings::setH3(const QFont &h3)
{
    if (this->d->m_h3 == h3)
        return;

    this->d->m_h3 = h3;
    emit this->h3Changed(this->d->m_h3);
}

void AkFontSettings::setH4(const QFont &h4)
{
    if (this->d->m_h4 == h4)
        return;

    this->d->m_h4 = h4;
    emit this->h4Changed(this->d->m_h4);
}

void AkFontSettings::setH5(const QFont &h5)
{
    if (this->d->m_h5 == h5)
        return;

    this->d->m_h5 = h5;
    emit this->h5Changed(this->d->m_h5);
}

void AkFontSettings::setH6(const QFont &h6)
{
    if (this->d->m_h6 == h6)
        return;

    this->d->m_h6 = h6;
    emit this->h6Changed(this->d->m_h6);
}

void AkFontSettings::setSubtitle1(const QFont &subtitle1)
{
    if (this->d->m_subtitle1 == subtitle1)
        return;

    this->d->m_subtitle1 = subtitle1;
    emit this->subtitle1Changed(this->d->m_subtitle1);
}

void AkFontSettings::setSubtitle2(const QFont &subtitle2)
{
    if (this->d->m_subtitle2 == subtitle2)
        return;

    this->d->m_subtitle2 = subtitle2;
    emit this->subtitle2Changed(this->d->m_subtitle2);
}

void AkFontSettings::setBody1(const QFont &body1)
{
    if (this->d->m_body1 == body1)
        return;

    this->d->m_body1 = body1;
    emit this->body1Changed(this->d->m_body1);
}

void AkFontSettings::setBody2(const QFont &body2)
{
    if (this->d->m_body2 == body2)
        return;

    this->d->m_body2 = body2;
    emit this->body2Changed(this->d->m_body2);
}

void AkFontSettings::setButton(const QFont &button)
{
    if (this->d->m_button == button)
        return;

    this->d->m_button = button;
    emit this->buttonChanged(this->d->m_button);
}

void AkFontSettings::setCaption(const QFont &caption)
{
    if (this->d->m_caption == caption)
        return;

    this->d->m_caption = caption;
    emit this->captionChanged(this->d->m_caption);
}

void AkFontSettings::setOverline(const QFont &overline)
{
    if (this->d->m_overline == overline)
        return;

    this->d->m_overline = overline;
    emit this->overlineChanged(this->d->m_overline);
}

void AkFontSettings::resetH1()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(96 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Light);
    this->setH1(font);
}

void AkFontSettings::resetH2()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(60 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Light);
    this->setH2(font);
}

void AkFontSettings::resetH3()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(48 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setH3(font);
}

void AkFontSettings::resetH4()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(34 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setH4(font);
}

void AkFontSettings::resetH5()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(24 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setH5(font);
}

void AkFontSettings::resetH6()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(20 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Medium);
    this->setH6(font);
}

void AkFontSettings::resetSubtitle1()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setSubtitle1(font);
}

void AkFontSettings::resetSubtitle2()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Medium);
    this->setSubtitle2(font);
}

void AkFontSettings::resetBody1()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setBody1(font);
}

void AkFontSettings::resetBody2()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setBody2(font);
}

void AkFontSettings::resetButton()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Bold);
    this->setButton(font);
}

void AkFontSettings::resetCaption()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(12 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setCaption(font);
}

void AkFontSettings::resetOverline()
{
    auto font = QGuiApplication::font();
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    font.setPointSize(qMax(10 * pointSize / 16, MINIMUM_POINT_SIZE));
    font.setWeight(QFont::Normal);
    this->setOverline(font);
}

void AkFontSettings::registerTypes()
{
    qRegisterMetaType<AkFontSettings>("AkFontSettings");
    qmlRegisterType<AkFontSettings>("Ak", 1, 0, "AkFontSettings");
}

void AkFontSettings::updateFonts(const QFont &font)
{
    auto _font = font;
    auto pointSize = font.pointSize();

    if (pointSize < 1)
        pointSize = DEFAULT_POINT_SIZE;

    _font.setPointSize(qMax(96 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Light);
    this->setH1(_font);
    _font.setPointSize(qMax(60 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Light);
    this->setH2(_font);
    _font.setPointSize(qMax(48 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setH3(_font);
    _font.setPointSize(qMax(34 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setH4(_font);
    _font.setPointSize(qMax(24 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setH5(_font);
    _font.setPointSize(qMax(20 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Medium);
    this->setH6(_font);
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setSubtitle1(_font);
    _font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Medium);
    this->setSubtitle2(_font);
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setBody1(_font);
    _font.setPointSize(qMax(14 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setBody2(_font);
    _font.setPointSize(qMax(16 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Medium);
    this->setButton(_font);
    _font.setPointSize(qMax(12 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Bold);
    this->setCaption(_font);
    _font.setPointSize(qMax(10 * pointSize / 16, MINIMUM_POINT_SIZE));
    _font.setWeight(QFont::Normal);
    this->setOverline(_font);
}

AkFontSettingsPrivate::AkFontSettingsPrivate(AkFontSettings *self):
    self(self)
{

}

#include "moc_akfontsettings.cpp"
