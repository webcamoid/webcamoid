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

#include <queue>
#include <map>

#include "rcloader.h"
#include "rcnode.h"
#include "rcname.h"
#include "rcdata.h"
#include "../cstream/cstream.h"

#ifndef UNUSED
    #define UNUSED(x) (void)(x);
#endif

namespace AkVCam
{
    std::map<std::string, RcData> *rcLoaderResources()
    {
        static std::map<std::string, RcData> resources;

        return &resources;
    }
}

std::list<std::string> AkVCam::RcLoader::list()
{
    std::list<std::string> resources;

    for (auto &res: *rcLoaderResources())
        resources.push_back(res.first);

    return resources;
}

AkVCam::CStreamRead AkVCam::RcLoader::load(const std::string &resource)
{
    for (auto &res: *rcLoaderResources())
        if (res.first == resource)
            return CStreamRead(res.second.m_data, size_t(res.second.m_size));

    return CStreamRead();
}

namespace QT_NAMESPACE
{
    bool qRegisterResourceData(int rcVersion,
                               const unsigned char *rcTree,
                               const unsigned char *rcName,
                               const unsigned char *rcData)
    {
        if (rcVersion != 0x1 && rcVersion != 0x2)
            return false;

        std::queue<RcNode> nodes;
        nodes.push(RcNode::read(rcTree, rcVersion));
        bool isRoot = true;
        const unsigned int nodeSize = rcVersion == 1? 14: 22;

        while (!nodes.empty()) {
            auto node = nodes.front();
            nodes.pop();
            std::string path;

            if (isRoot) {
                path = ":";
                isRoot = false;
            } else {
                path = node.parent + "/" + RcName::read(rcName + node.nameOffset);
            }

            if (node.flags == RcNode::NodeType_Folder) {
                for (uint32_t i = 0; i < node.fd.count; i++) {
                    nodes.push(RcNode::read(rcTree + nodeSize * (node.firstChild + i), rcVersion));
                    nodes.back().parent = path;
                }
            } else {
                (*rcLoaderResources())[path] =
                        RcData::read(rcData + node.dataOffset);
            }
        };

        return true;
    }

    bool qUnregisterResourceData(int rcVersion,
                                 const unsigned char *rcTree,
                                 const unsigned char *rcName,
                                 const unsigned char *rcData)
    {
        UNUSED(rcVersion)
        UNUSED(rcTree)
        UNUSED(rcName)
        UNUSED(rcData)

        return true;
    }
}
