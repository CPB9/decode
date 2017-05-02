#include "decode/groundcontrol/TmState.h"

#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>

namespace decode {

TmState::TmState()
    : Client(2)
{
}

TmState::~TmState()
{
}


void TmState::acceptData(Sender*, bmcl::Bytes packet)
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

void TmState::start(Sender*)
{
}
}

