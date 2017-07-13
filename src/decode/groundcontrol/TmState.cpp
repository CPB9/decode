/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/TmState.h"
#include "decode/groundcontrol/Atoms.h"

#include "decode/parser/Project.h"
#include "decode/model/Model.h"

#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

namespace decode {

TmState::TmState(caf::actor_config& cfg)
    : caf::event_based_actor(cfg)
{
}

TmState::~TmState()
{
}

caf::behavior TmState::make_behavior()
{
    return caf::behavior{
        [this](UpdateProjectAtom, const Rc<const Project>& proj, const std::string& name) {
        },
        [this](RecvUserPacketAtom, const bmcl::SharedBytes& data) {
            acceptData(data.view());
        },
    };
}

void TmState::acceptData(bmcl::Bytes packet)
{
    bmcl::MemReader src(packet);
    src.skip(11);

    while (src.sizeLeft() != 0) {
        if (src.sizeLeft() < 2) {
            //TODO: report error
            return;
        }

        uint16_t msgSize = src.readUint16Le();
        if (src.sizeLeft() < msgSize) {
            //TODO: report error
            return;
        }

        bmcl::MemReader msg(src.current(), msgSize);
        src.skip(msgSize);

        uint64_t compNum;
        if (!msg.readVarUint(&compNum)) {
            //TODO: report error
            return;
        }

        uint64_t msgNum;
        if (!msg.readVarUint(&msgNum)) {
            //TODO: report error
            return;
        }

        acceptTmMsg(compNum, msgNum, bmcl::Bytes(msg.current(), msg.sizeLeft()));
    }
}

void TmState::acceptTmMsg(uint64_t compNum, uint64_t msgNum, bmcl::Bytes payload)
{
}
}
