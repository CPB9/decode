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

#include <bmcl/Bytes.h>

#include <caf/event_based_actor.hpp>

namespace decode {

class TmModel;

class TmState : public caf::event_based_actor {
public:
    TmState(caf::actor_config& cfg);
    ~TmState();

    caf::behavior make_behavior() override;

protected:
    void acceptData(bmcl::Bytes packet);
    void acceptTmMsg(uint64_t compNum, uint64_t msgNum, bmcl::Bytes payload);

private:
    Rc<TmModel> _model;
};
}
