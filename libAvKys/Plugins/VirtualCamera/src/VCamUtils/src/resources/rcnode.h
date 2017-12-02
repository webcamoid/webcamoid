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

#ifndef AKVCAMUTILS_RCNODE_H
#define AKVCAMUTILS_RCNODE_H

#include <string>
#include <cstdint>

namespace AkVCam
{
    struct RcNode
    {
        enum
        {
            NodeType_File,
            NodeType_Compressed,
            NodeType_Folder
        };

        uint32_t nameOffset;
        uint16_t flags;

        union
        {
            uint32_t count;

            struct
            {
                uint16_t country;
                uint16_t language;
            } locale;
        } fd;

        union
        {
            uint32_t firstChild;
            uint32_t dataOffset;
        };

        uint64_t lastModified;
        std::string parent;

        RcNode();
        RcNode(const RcNode &other);
        static RcNode read(const unsigned char *rcTree, int rcVersion);
    };
}

#endif // AKVCAMUTILS_RCNODE_H
