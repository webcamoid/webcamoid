/* Webcamoid, webcam capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
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
#include "../cstream/cstreamread.h"

AkVCam::RcNode::RcNode()
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

AkVCam::RcNode::RcNode(const RcNode &other)
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

AkVCam::RcNode AkVCam::RcNode::read(const unsigned char *rcTree,
                                    int rcVersion)
{
    CStreamRead treeStream(rcTree, true);
    RcNode node;
    node.nameOffset = treeStream.read<uint32_t>();
    node.flags = treeStream.read<uint16_t>();

    if (node.flags == NodeType_Folder) {
        node.fd.count = treeStream.read<uint32_t>();
        node.firstChild = treeStream.read<uint32_t>();
    } else {
        node.fd.locale.country = treeStream.read<uint16_t>();
        node.fd.locale.language = treeStream.read<uint16_t>();
        node.dataOffset = treeStream.read<uint32_t>();
    }

    if (rcVersion > 1)
        node.lastModified = treeStream.read<uint64_t>();

    return node;
}
