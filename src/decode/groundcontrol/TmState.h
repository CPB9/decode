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
#include "decode/groundcontrol/Client.h"

namespace decode {

class TmState : public Client {
public:
    TmState();
    ~TmState();

    void acceptData(Sender* parent, bmcl::Bytes packet) override;
    void start(Sender* parent) override;

protected:
    virtual void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload) = 0;

private:
};
}
