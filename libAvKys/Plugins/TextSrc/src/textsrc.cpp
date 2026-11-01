/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include "textsrc.h"
#include "textsrcelement.h"

QObject *TextSrc::create()
{
    qRegisterMetaType<TextSrcElement::TextSource>("TextSrcElementTextSource");
    qRegisterMetaType<TextSrcElement::HorizontalAlignment>("TextSrcElementHorizontalAlignment");
    qRegisterMetaType<TextSrcElement::VerticalAlignment>("TextSrcElementVerticalAlignment");
    qRegisterMetaType<TextSrcElement::BackgroundShape>("TextSrcElementBackgroundShape");
    qRegisterMetaType<TextSrcElement::TailStyle>("TextSrcElementTailStyle");
    qmlRegisterType<TextSrcElement>("TextSrcElement", 1, 0, "TextSrcElement");

    return new TextSrcElement();
}

#include "moc_textsrc.cpp"
