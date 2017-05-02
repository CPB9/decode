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
