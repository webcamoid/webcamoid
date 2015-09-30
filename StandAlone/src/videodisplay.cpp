/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "videodisplay.h"

VideoDisplay::VideoDisplay(QQuickItem *parent):
    QQuickItem(parent)
{
    this->m_fillDisplay = false;
    this->setFlag(ItemHasContents, true);
}

VideoDisplay::~VideoDisplay()
{
}

bool VideoDisplay::fillDisplay() const
{
    return this->m_fillDisplay;
}

QSGNode *VideoDisplay::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    if (this->m_videoFrame.textureSize().isEmpty()) {
        if (oldNode)
            delete oldNode;

        return NULL;
    }

    QSGSimpleTextureNode *node = NULL;

    if (oldNode)
        node = dynamic_cast<QSGSimpleTextureNode *>(oldNode);
    else
        node = new QSGSimpleTextureNode();

    if (this->m_fillDisplay)
        node->setRect(this->boundingRect());
    else {
        QSizeF size(this->m_videoFrame.textureSize());
        size.scale(this->boundingRect().size(), Qt::KeepAspectRatio);
        QRectF rect(QPointF(), size);
        rect.moveCenter(this->boundingRect().center());

        node->setRect(rect);
    }

    node->setTexture(&this->m_videoFrame);

    return node;
}

void VideoDisplay::setFrame(const QbPacket &packet)
{
    this->m_videoFrame = packet;
    this->update();
}

void VideoDisplay::setFillDisplay(bool fillDisplay)
{
    if (this->m_fillDisplay == fillDisplay)
        return;

    this->m_fillDisplay = fillDisplay;
    emit this->fillDisplayChanged();
}

void VideoDisplay::resetFillDisplay()
{
    this->setFillDisplay(false);
}
