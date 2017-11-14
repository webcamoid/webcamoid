/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "rcnode.h"
#include "rcutils.h"

RcNode::RcNode()
{
    this->nameOffset = 0;
    this->flags = 0;
    this->fd.count = 0;
    this->fd.locale.country = 0;
    this->fd.locale.language = 0;
    this->firstChild = 0;
    this->dataOffset = 0;
    this->lastModified = 0;
}

RcNode::RcNode(const RcNode &other)
{
    this->nameOffset = other.nameOffset;
    this->flags = other.flags;
    this->fd.count = other.fd.count;
    this->fd.locale.country = other.fd.locale.country;
    this->fd.locale.language = other.fd.locale.language;
    this->firstChild = other.firstChild;
    this->dataOffset = other.dataOffset;
    this->lastModified = other.lastModified;
    this->parent = other.parent;
}

RcNode RcNode::read(const unsigned char *rcTree)
{
    RcNode node;
    node.nameOffset = RcUtils::fromBigEndian(*reinterpret_cast<const uint32_t *>(rcTree));
    rcTree += sizeof(uint32_t);
    node.flags = RcUtils::fromBigEndian(*reinterpret_cast<const uint16_t *>(rcTree));
    rcTree += sizeof(uint16_t);

    if (node.flags == NodeType_Folder) {
        node.fd.count = RcUtils::fromBigEndian(*reinterpret_cast<const uint32_t *>(rcTree));
        rcTree += sizeof(uint32_t);
        node.firstChild = RcUtils::fromBigEndian(*reinterpret_cast<const uint32_t *>(rcTree));
    } else {
        node.fd.locale.country = RcUtils::fromBigEndian(*reinterpret_cast<const uint16_t *>(rcTree));
        rcTree += sizeof(uint16_t);
        node.fd.locale.language = RcUtils::fromBigEndian(*reinterpret_cast<const uint16_t *>(rcTree));
        rcTree += sizeof(uint16_t);
        node.dataOffset = RcUtils::fromBigEndian(*reinterpret_cast<const uint32_t *>(rcTree));
    }

    rcTree += sizeof(uint16_t);
    node.lastModified = RcUtils::fromBigEndian(*reinterpret_cast<const uint64_t *>(rcTree));

    return node;
}
