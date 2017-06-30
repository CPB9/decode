/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ModelEventHandler.h"

#include <bmcl/Bytes.h>

namespace decode {

ModelEventHandler::~ModelEventHandler()
{
}

void ModelEventHandler::beginHashDownload()
{
}

void ModelEventHandler::endHashDownload(const std::string&, bmcl::Bytes)
{
}

void ModelEventHandler::beginFirmwareStartCommand()
{
}

void ModelEventHandler::endFirmwareStartCommand()
{
}


void ModelEventHandler::beginFirmwareDownload(std::size_t)
{
}

void ModelEventHandler::firmwareDownloadProgress(std::size_t)
{
}

void ModelEventHandler::firmwareError(const std::string&)
{
}

void ModelEventHandler::endFirmwareDownload()
{
}

void ModelEventHandler::packetQueued(bmcl::Bytes)
{
}

void ModelEventHandler::modelUpdated(const Rc<Model>&)
{
}

void ModelEventHandler::nodeValueUpdated(const Node*, std::size_t)
{
}

void ModelEventHandler::nodesInserted(const Node*, std::size_t, std::size_t, std::size_t)
{
}

void ModelEventHandler::nodesRemoved(const Node*, std::size_t, std::size_t, std::size_t)
{
}
}
