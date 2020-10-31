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

#include "scale.h"
#include "scaleelement.h"

QObject *Scale::create(const QString &key, const QString &specification)
{
    Q_UNUSED(specification)

    if (key == AK_PLUGIN_TYPE_ELEMENT) {
        qRegisterMetaType<ScaleElement::ScalingMode>("ScaleElementScalingMode");
        qRegisterMetaTypeStreamOperators<ScaleElement::ScalingMode>("ScaleElementScalingMode");
        qRegisterMetaType<ScaleElement::AspectRatioMode>("ScaleElementAspectRatioMode");
        qRegisterMetaTypeStreamOperators<ScaleElement::AspectRatioMode>("ScaleElementAspectRatioMode");
        qmlRegisterType<ScaleElement>("ScaleElement", 1, 0, "ScaleElement");

        return new ScaleElement();
    }

    return nullptr;
}

QStringList Scale::keys() const
{
    return QStringList();
}

#include "moc_scale.cpp"
