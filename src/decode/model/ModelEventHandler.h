/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

class Node;
class Model;

class ModelEventHandler : public RefCountable {
public:
    virtual ~ModelEventHandler();
    virtual void beginHashDownload();
    virtual void endHashDownload(const std::string& deviceName, bmcl::Bytes firmwareHash);
    virtual void beginFirmwareStartCommand();
    virtual void endFirmwareStartCommand();
    virtual void beginFirmwareDownload(std::size_t firmwareSize);
    virtual void firmwareError(const std::string& errorMsg);
    virtual void firmwareDownloadProgress(std::size_t sizeDownloaded);
    virtual void endFirmwareDownload();
    virtual void packetQueued(bmcl::Bytes packet);
    virtual void modelUpdated(const Rc<Model>& model);
    virtual void nodeValueUpdated(const Node* node, std::size_t nodeIndex);
    virtual void nodesInserted(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
    virtual void nodesRemoved(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
};
}
