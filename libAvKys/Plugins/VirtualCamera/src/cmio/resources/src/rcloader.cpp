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

#include <iostream>
#include <queue>
#include <map>

#include "rcloader.h"
#include "rcutils.h"
#include "rcnode.h"
#include "rcname.h"
#include "rcdata.h"

#ifndef UNUSED
    #define UNUSED(x) (void)(x);
#endif

static std::map<std::string, RcData> rcLoaderResourcesPrivate;

std::list<std::string> RcLoader::list()
{
    std::list<std::string> resources;

    for (auto &res: rcLoaderResourcesPrivate)
        resources.push_back(res.first);

    return resources;
}

size_t RcLoader::load(const std::string &resource, const unsigned char **data)
{
    for (auto &res: rcLoaderResourcesPrivate)
        if (res.first == resource && data) {
            *data = res.second.m_data;

            return res.second.m_size;
        }

    return 0;
}

bool qRegisterResourceData(int rcVersion,
                           const unsigned char *rcTree,
                           const unsigned char *rcName,
                           const unsigned char *rcData)
{
    if (rcVersion != 0x1 && rcVersion != 0x2)
        return false;

    std::queue<RcNode> nodes;
    nodes.push(RcNode::read(rcTree));
    bool isRoot = true;

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
                nodes.push(RcNode::read(rcTree + 22 * (node.firstChild + i)));
                nodes.back().parent = path;
            }
        } else {
            rcLoaderResourcesPrivate[path] =
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
