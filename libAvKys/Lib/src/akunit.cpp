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
        int m_pixels {0};
        QSize m_parentSize;

        // Screen info
        QSize m_screenSize;
        qreal m_physicalDotsPerInch {0.0};
        qreal m_physicalDotsPerInchX {0.0};
        qreal m_physicalDotsPerInchY {0.0};
        bool m_hasParent {false};

        explicit AkUnitPrivate(AkUnit *self);
        int pixels(qreal value, AkUnit::Unit unitsMap) const;
        static const UnitsMap &unitsMap();
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
    this->d->m_pixels = this->d->pixels(value, unit);
}

AkUnit::AkUnit(qreal value, const QString &unit):
    QObject()
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = AkUnitPrivate::unitsMap().value(unit, AkUnit::px);
    this->d->m_parentSize = this->d->m_screenSize;
    this->d->m_pixels = this->d->pixels(value, this->d->m_unit);
}

AkUnit::AkUnit(qreal value, Unit unit, QWindow *parent):
    QObject(parent)
{
    this->d = new AkUnitPrivate(this);
    this->d->m_value = value;
    this->d->m_unit = unit;
    this->d->m_hasParent = parent != nullptr;
    this->d->m_parentSize = parent? parent->size(): this->d->m_screenSize;
    this->d->m_pixels = this->d->pixels(value, unit);

    if (parent) {
        QObject::connect(parent,
                         &QWindow::widthChanged,
                         [this] (int width) {
            this->d->m_parentSize.setWidth(width);
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QWindow::heightChanged,
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
    this->d->m_pixels = this->d->pixels(value, this->d->m_unit);

    if (parent) {
        QObject::connect(parent,
                         &QWindow::widthChanged,
                         [this] (int width) {
            this->d->m_parentSize.setWidth(width);
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QWindow::heightChanged,
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
    this->d->m_pixels = this->d->pixels(value, unit);

    if (parent) {
        QObject::connect(parent,
                         &QQuickItem::widthChanged,
                         [this, parent] () {
            this->d->m_parentSize.setWidth(qRound(parent->width()));
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QQuickItem::heightChanged,
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
    this->d->m_pixels = this->d->pixels(value, this->d->m_unit);

    if (parent) {
        QObject::connect(parent,
                         &QQuickItem::widthChanged,
                         [this, parent] () {
            this->d->m_parentSize.setWidth(qRound(parent->width()));
            this->d->updatePixels();
        });
        QObject::connect(parent,
                         &QQuickItem::heightChanged,
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
    this->d->m_pixels = this->d->pixels(this->d->m_value, this->d->m_unit);
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
        this->d->m_physicalDotsPerInch = other.d->m_physicalDotsPerInch;
        this->d->m_physicalDotsPerInchX = other.d->m_physicalDotsPerInchX;
        this->d->m_physicalDotsPerInchY = other.d->m_physicalDotsPerInchY;
    }

    return *this;
}

bool AkUnit::operator ==(const AkUnit &other) const
{
    return this->d->m_pixels == other.d->m_pixels;
}

bool AkUnit::operator !=(const AkUnit &other) const
{
    return this->d->m_pixels != other.d->m_pixels;
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
    return this->d->m_pixels;
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
    return this->d->m_pixels;
}

void AkUnit::setValue(qreal value)
{
    if (qFuzzyCompare(this->d->m_value, value))
        return;

    this->d->m_value = value;
    auto pixels = this->d->pixels(this->d->m_value, this->d->m_unit);
    bool pixelsChanged = this->d->m_pixels != pixels;
    this->d->m_pixels = pixels;
    emit this->valueChanged(value);

    if (pixelsChanged)
        emit this->pixelsChanged(this->d->m_pixels);
}

void AkUnit::setUnit(AkUnit::Unit unit)
{
    if (this->d->m_unit == unit)
        return;

    this->d->m_unit = unit;
    auto pixels = this->d->pixels(this->d->m_value, this->d->m_unit);
    bool pixelsChanged = this->d->m_pixels != pixels;
    this->d->m_pixels = pixels;
    emit this->unitChanged(unit);

    if (pixelsChanged)
        emit this->pixelsChanged(this->d->m_pixels);
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
    this->updateScreenInfo(false);
    QObject::connect(qApp,
                     &QGuiApplication::primaryScreenChanged,
                     [this] () {
        this->updateScreenInfo(true);
    });
}

int AkUnitPrivate::pixels(qreal value, AkUnit::Unit unit) const
{
    switch (unit) {
    case AkUnit::cm:
        return qRound(value * this->m_physicalDotsPerInch / 2.54);
    case AkUnit::mm:
        return qRound(value * this->m_physicalDotsPerInch / 25.4);
    case AkUnit::in:
        return qRound(value * this->m_physicalDotsPerInch);
    case AkUnit::pt:
        return qRound(value * this->m_physicalDotsPerInch / 72);
    case AkUnit::pc:
        return qRound(12 * value * this->m_physicalDotsPerInch / 72);
    case AkUnit::dp:
        return qRound(value * this->m_physicalDotsPerInch / 160);
    case AkUnit::vw:
        return qRound(value * this->m_parentSize.width() / 100);
    case AkUnit::vh:
        return qRound(value * this->m_parentSize.height() / 100);
    case AkUnit::vmin: {
        auto min = qMin(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return qRound(value * min / 100);
    }
    case AkUnit::vmax: {
        auto max = qMax(this->m_parentSize.width(),
                        this->m_parentSize.height());

        return qRound(value * max / 100);
    }
    default: return qRound(value);
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

void AkUnitPrivate::updateScreenInfo(bool updatePixels)
{
    auto screen = QGuiApplication::primaryScreen();
    this->m_screenSize = screen->size();

    if (!this->m_hasParent)
        this->m_parentSize = this->m_screenSize;

    this->m_physicalDotsPerInch = screen->physicalDotsPerInch();
    this->m_physicalDotsPerInchX = screen->physicalDotsPerInchX();
    this->m_physicalDotsPerInchY = screen->physicalDotsPerInchY();

    if (updatePixels)
        this->updatePixels();

    QObject::connect(screen,
                     &QScreen::geometryChanged,
                     [this, screen] () {
        this->m_screenSize = screen->size();

        if (!this->m_hasParent)
            this->m_parentSize = this->m_screenSize;

        this->updatePixels();
    });
    QObject::connect(screen,
                     &QScreen::physicalDotsPerInchChanged,
                     [this, screen] () {
        this->m_physicalDotsPerInchX = screen->physicalDotsPerInchX();
        this->m_physicalDotsPerInchY = screen->physicalDotsPerInchY();
        this->updatePixels();
    });
}

void AkUnitPrivate::updatePixels()
{
    auto pixels = this->pixels(this->m_value, this->m_unit);

    if (this->m_pixels == pixels)
        return;

    this->m_pixels = pixels;
    emit self->pixelsChanged(this->m_pixels);
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
