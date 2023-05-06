/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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

#include <QDataStream>
#include <QDebug>
#include <QGuiApplication>
#include <QQuickItem>
#include <QScreen>
#include <QWindow>

#include "akunit.h"

using UnitsMap = QMap<QString, AkUnit::Unit>;

class AkUnitPrivate
{
    public:
        AkUnit *self;
        qreal m_value {0.0};
        AkUnit::Unit m_unit {AkUnit::px};
        qreal m_pixels {0.0};
        QSize m_parentSize;
        qreal m_scaleFactor {1.0};

        // Screen info
        QSize m_screenSize;
        qreal m_dotsPerInch {0.0};
        qreal m_dotsPerInchX {0.0};
        qreal m_dotsPerInchY {0.0};
        bool m_hasParent {false};

        explicit AkUnitPrivate(AkUnit *self);
        void updateScaleFactor();
        qreal pixels(qreal value, AkUnit::Unit unit) const;
        qreal fromPixels(qreal value, AkUnit::Unit unit) const;
        static const UnitsMap &unitsMap();
        void updateDpi(const QScreen *screen);
        void updateScreenInfo(bool updatePixels);
        void updatePixels();
        static QString matchClassName(const QObject *obj,
                                      const QStringList &classes);
};

AkUnit::AkUnit(qreal value, Unit unit):
    QObject()
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = unit;
    this->d->m_parentSize = this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, unit);
}

AkUnit::AkUnit(qreal value, const QString &unit):
    QObject()
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = AkUnitPrivate::unitsMap().value(unit, AkUnit::px);
    this->d->m_parentSize = this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, this->d->m_unit);
}

AkUnit::AkUnit(qreal value, Unit unit, QWindow *parent):
    QObject(parent)
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = unit;
    this->d->m_hasParent = parent != nullptr;
    this->d->m_parentSize = parent? parent->size(): this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, unit);

    if (parent) {
        QObject::connect(parent,
                         &QWindow::widthChanged,
                         this,
                         [this] (int width) {
            this->d->m_parentSize.setWidth(width);
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QWindow::heightChanged,
                         this,
                         [this] (int height) {
            this->d->m_parentSize.setHeight(height);
            this->d->updatePixels();
        });
    }
}

AkUnit::AkUnit(qreal value, const QString &unit, QWindow *parent):
    QObject(parent)
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = AkUnitPrivate::unitsMap().value(unit, AkUnit::px);
    this->d->m_hasParent = parent != nullptr;
    this->d->m_parentSize = parent? parent->size(): this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, this->d->m_unit);

    if (parent) {
        QObject::connect(parent,
                         &QWindow::widthChanged,
                         this,
                         [this] (int width) {
            this->d->m_parentSize.setWidth(width);
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QWindow::heightChanged,
                         this,
                         [this] (int height) {
            this->d->m_parentSize.setHeight(height);
            this->d->updatePixels();
        });
    }
}

AkUnit::AkUnit(qreal value, Unit unit, QQuickItem *parent):
    QObject(parent)
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = unit;
    this->d->m_hasParent = parent != nullptr;
    this->d->m_parentSize = parent?
                                QSize(qRound(parent->width()),
                                      qRound(parent->height())):
                                this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, unit);

    if (parent) {
        QObject::connect(parent,
                         &QQuickItem::widthChanged,
                         this,
                         [this, parent] () {
            this->d->m_parentSize.setWidth(qRound(parent->width()));
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QQuickItem::heightChanged,
                         this,
                         [this, parent] () {
            this->d->m_parentSize.setHeight(qRound(parent->height()));
            this->d->updatePixels();
        });
    }
}

AkUnit::AkUnit(qreal value, const QString &unit, QQuickItem *parent):
    QObject(parent)
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = AkUnitPrivate::unitsMap().value(unit, AkUnit::px);
    this->d->m_hasParent = parent != nullptr;
    this->d->m_parentSize = parent?
                                QSize(qRound(parent->width()),
                                      qRound(parent->height())):
                                this->d->m_screenSize;
    this->d->m_pixels = this->d->m_scaleFactor * this->d->pixels(value, this->d->m_unit);

    if (parent) {
        QObject::connect(parent,
                         &QQuickItem::widthChanged,
                         this,
                         [this, parent] () {
            this->d->m_parentSize.setWidth(qRound(parent->width()));
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QQuickItem::heightChanged,
                         this,
                         [this, parent] () {
            this->d->m_parentSize.setHeight(qRound(parent->height()));
            this->d->updatePixels();
        });
    }
}

AkUnit::AkUnit(const AkUnit &other):
    QObject()
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = other.d->m_value;
    this->d->m_unit = other.d->m_unit;
    this->d->m_hasParent = other.d->m_hasParent;
    this->d->m_parentSize = other.d->m_screenSize;
    this->d->m_pixels = this->d->m_pixels;
}

AkUnit::~AkUnit()
{
    delete this->d;
}

AkUnit &AkUnit::operator =(const AkUnit &other)
{
    if (this != &other) {
        this->d->m_value = other.d->m_value;
        this->d->m_unit = other.d->m_unit;
        this->d->m_hasParent = other.d->m_hasParent;
        this->d->m_pixels = other.d->m_pixels;
        this->d->m_parentSize = other.d->m_parentSize;
        this->d->m_screenSize = other.d->m_screenSize;
        this->d->m_dotsPerInch = other.d->m_dotsPerInch;
        this->d->m_dotsPerInchX = other.d->m_dotsPerInchX;
        this->d->m_dotsPerInchY = other.d->m_dotsPerInchY;
    }

    return *this;
}

bool AkUnit::operator ==(const AkUnit &other) const
{
    return qFuzzyCompare(this->d->m_pixels, other.d->m_pixels);
}

bool AkUnit::operator !=(const AkUnit &other) const
{
    return !qFuzzyCompare(this->d->m_pixels, other.d->m_pixels);
}

QObject *AkUnit::create(qreal value, AkUnit::Unit unit)
{
    return new AkUnit(value, unit);
}

QObject *AkUnit::create(qreal value, const QString &unit)
{
    return new AkUnit(value, unit);
}

QObject *AkUnit::create(qreal value, AkUnit::Unit unit, QObject *parent)
{
    auto className =
            AkUnitPrivate::matchClassName(parent,
                                          {"QWindow",
                                           "QQuickItem"});

    if (className == "QWindow")
        return new AkUnit(value, unit, qobject_cast<QWindow *>(parent));
    else if (className == "QQuickItem")
        return new AkUnit(value, unit, qobject_cast<QQuickItem *>(parent));

    return new AkUnit(value, unit);
}

QObject *AkUnit::create(qreal value, const QString &unit, QObject *parent)
{
    auto className =
            AkUnitPrivate::matchClassName(parent,
                                          {"QWindow",
                                           "QQuickItem"});

    if (className == "QWindow")
        return new AkUnit(value, unit, qobject_cast<QWindow *>(parent));
    else if (className == "QQuickItem")
        return new AkUnit(value, unit, qobject_cast<QQuickItem *>(parent));

    return new AkUnit(value, unit);
}

QVariant AkUnit::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkUnit::operator int() const
{
    return qRound(this->d->m_pixels);
}

AkUnit::operator QString() const
{
    return QString("%1 %2")
            .arg(this->d->m_value)
            .arg(AkUnitPrivate::unitsMap().key(this->d->m_unit, "px"));
}

qreal AkUnit::value() const
{
    return this->d->m_value;
}

AkUnit::Unit AkUnit::unit() const
{
    return this->d->m_unit;
}

int AkUnit::pixels() const
{
    return qRound(this->d->m_pixels);
}

AkUnit AkUnit::convert(Unit unit) const
{
    return AkUnit(this->d->fromPixels(this->d->m_pixels, unit), unit);
}

AkUnit AkUnit::convert(const QString &unit) const
{
    return this->convert(AkUnitPrivate::unitsMap().value(unit, AkUnit::px));
}

void AkUnit::setValue(qreal value)
{
    if (qFuzzyCompare(this->d->m_value, value))
        return;

    this->d->m_value = value;
    auto pixels = this->d->m_scaleFactor * this->d->pixels(this->d->m_value, this->d->m_unit);
    bool pixelsChanged = !qFuzzyCompare(this->d->m_pixels, pixels);

    if (pixelsChanged)
        this->d->m_pixels = pixels;

    emit this->valueChanged(value);

    if (pixelsChanged)
        emit this->pixelsChanged(qRound(this->d->m_pixels));
}

void AkUnit::setUnit(AkUnit::Unit unit)
{
    if (this->d->m_unit == unit)
        return;

    this->d->m_unit = unit;
    auto pixels = this->d->m_scaleFactor * this->d->pixels(this->d->m_value, this->d->m_unit);
    bool pixelsChanged = !qFuzzyCompare(this->d->m_pixels, pixels);

    if (pixelsChanged)
        this->d->m_pixels = pixels;

    emit this->unitChanged(unit);

    if (pixelsChanged)
        emit this->pixelsChanged(qRound(this->d->m_pixels));
}

void AkUnit::resetValue()
{
    this->setValue(0.0);
}

void AkUnit::resetUnit()
{
    this->setUnit(px);
}

void AkUnit::registerTypes()
{
    qRegisterMetaType<AkUnit>("AkUnit");
    qRegisterMetaTypeStreamOperators<AkUnit>("AkUnit");
    QMetaType::registerDebugStreamOperator<AkUnit>();
    qRegisterMetaType<Unit>("Unit");
    qmlRegisterSingletonType<AkUnit>("Ak", 1, 0, "AkUnit",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkUnit();
    });
}

AkUnitPrivate::AkUnitPrivate(AkUnit *self):
    self(self)
{
    this->updateScaleFactor();
    this->updateScreenInfo(false);
    QObject::connect(qApp,
                     &QGuiApplication::primaryScreenChanged,
                     self,
                     [this] () {
        this->updateScreenInfo(true);
    });
}

void AkUnitPrivate::updateScaleFactor()
{
    auto scaleFactorStr = qgetenv("QT_SCALE_FACTOR");

    if (!scaleFactorStr.isEmpty()) {
        bool ok = false;
        auto factor = scaleFactorStr.toDouble(&ok);

        if (ok)
            this->m_scaleFactor = factor;
    }
}

qreal AkUnitPrivate::pixels(qreal value, AkUnit::Unit unit) const
{
    switch (unit) {
    case AkUnit::cm:
        return value * this->m_dotsPerInch / 2.54;
    case AkUnit::mm:
        return value * this->m_dotsPerInch / 25.4;
    case AkUnit::in:
        return value * this->m_dotsPerInch;
    case AkUnit::pt:
        return value * this->m_dotsPerInch / 72;
    case AkUnit::pc:
        return 12 * value * this->m_dotsPerInch / 72;
    case AkUnit::dp:
        return value * this->m_dotsPerInch / 160;
    case AkUnit::vw:
        return value * this->m_parentSize.width() / 100;
    case AkUnit::vh:
        return value * this->m_parentSize.height() / 100;
    case AkUnit::vmin: {
        auto min = qMin(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return value * min / 100;
    }
    case AkUnit::vmax: {
        auto max = qMax(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return value * max / 100;
    }
    default: return value;
    }
}

qreal AkUnitPrivate::fromPixels(qreal value, AkUnit::Unit unit) const
{
    switch (unit) {
    case AkUnit::cm:
        return 2.54 * value / this->m_dotsPerInch;
    case AkUnit::mm:
        return 25.4 * value / this->m_dotsPerInch;
    case AkUnit::in:
        return value / this->m_dotsPerInch;
    case AkUnit::pt:
        return 72  * value / this->m_dotsPerInch;
    case AkUnit::pc:
        return 72 * value / (12 * this->m_dotsPerInch);
    case AkUnit::dp:
        return 160 * value / this->m_dotsPerInch;
    case AkUnit::vw:
        return 100 * value / this->m_parentSize.width();
    case AkUnit::vh:
        return 100 * value / this->m_parentSize.height();
    case AkUnit::vmin: {
        auto min = qMin(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return 100 * value / min;
    }
    case AkUnit::vmax: {
        auto max = qMax(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return 100 * value / max;
    }
    default: return value;
    }
}

const UnitsMap &AkUnitPrivate::unitsMap()
{
    static const QMap<QString, AkUnit::Unit> unitsMap {
        {"cm"  , AkUnit::cm  },
        {"mm"  , AkUnit::mm  },
        {"in"  , AkUnit::in  },
        {"px"  , AkUnit::px  },
        {"pt"  , AkUnit::pt  },
        {"pc"  , AkUnit::pc  },
        {"dp"  , AkUnit::dp  },
        {"vw"  , AkUnit::vw  },
        {"vh"  , AkUnit::vh  },
        {"vmin", AkUnit::vmin},
        {"vmax", AkUnit::vmax},
    };

    return unitsMap;
}

void AkUnitPrivate::updateDpi(const QScreen *screen)
{
#ifdef Q_OS_ANDROID
    this->m_dotsPerInch = screen->physicalDotsPerInch();
    this->m_dotsPerInchX = screen->physicalDotsPerInchX();
    this->m_dotsPerInchY = screen->physicalDotsPerInchY();
#else
    static const qreal referenceDpi = 100.0;
    auto ldpi = screen->logicalDotsPerInch();
    auto pdpi = screen->physicalDotsPerInch();

    if (qAbs(ldpi - referenceDpi) < qAbs(pdpi - referenceDpi)) {
        this->m_dotsPerInch = ldpi;
        this->m_dotsPerInchX = screen->logicalDotsPerInchX();
        this->m_dotsPerInchY = screen->logicalDotsPerInchY();
    } else {
        this->m_dotsPerInch = pdpi;
        this->m_dotsPerInchX = screen->physicalDotsPerInchX();
        this->m_dotsPerInchY = screen->physicalDotsPerInchY();
    }
#endif
}

void AkUnitPrivate::updateScreenInfo(bool updatePixels)
{
    auto screen = QGuiApplication::primaryScreen();
    this->m_screenSize = screen->size();

    if (!this->m_hasParent)
        this->m_parentSize = this->m_screenSize;

    this->updateDpi(screen);

    if (updatePixels)
        this->updatePixels();

    QObject::connect(screen,
                     &QScreen::geometryChanged,
                     self,
                     [this, screen] () {
        this->m_screenSize = screen->size();

        if (!this->m_hasParent)
            this->m_parentSize = this->m_screenSize;

        this->updatePixels();
    });
    QObject::connect(screen,
                     &QScreen::logicalDotsPerInchChanged,
                     self,
                     [this, screen] () {
        this->updateDpi(screen);
        this->updatePixels();
    });
}

void AkUnitPrivate::updatePixels()
{
    auto pixels = this->m_scaleFactor * this->pixels(this->m_value, this->m_unit);

    if (qFuzzyCompare(this->m_pixels, pixels))
        return;

    this->m_pixels = pixels;
    emit self->pixelsChanged(qRound(this->m_pixels));
}

QString AkUnitPrivate::matchClassName(const QObject *obj,
                                      const QStringList &classes)
{
    if (!obj)
        return {};

    for (auto mobj = obj->metaObject(); mobj; mobj = mobj->superClass())
        if (classes.contains(mobj->className()))
            return {mobj->className()};

    return {};
}

QDebug operator <<(QDebug debug, const AkUnit &unit)
{
    auto unitStr =
            QString("%1 %2")
                .arg(unit.value())
                .arg(AkUnitPrivate::unitsMap().key(unit.unit(), "px"));
    debug.nospace() << unitStr.toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkUnit &unit)
{
    qreal value;
    int munit;
    istream >> value;
    istream >> munit;
    unit.setValue(value);
    unit.setUnit(AkUnit::Unit(munit));

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkUnit &unit)
{
    ostream << unit.value();
    ostream << unit.unit();

    return ostream;
}

QDebug operator <<(QDebug debug, const AkUnit::Unit &unit)
{
    auto unitStr = AkUnitPrivate::unitsMap().key(unit, "px");
    debug.nospace() << unitStr.toStdString().c_str();

    return debug.space();
}
