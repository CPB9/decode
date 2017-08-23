#include "decode/groundcontrol/CmdState.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/parser/Project.h"
#include "decode/model/CmdModel.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/groundcontrol/GcInterface.h"
#include "decode/groundcontrol/Packet.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketResponse);

namespace decode {

CmdState::CmdState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _exc(exchange)
    , _handler(handler)
{
}

CmdState::~CmdState()
{
}

caf::behavior CmdState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, Project::ConstPointer& proj, Device::ConstPointer& dev) {
            _model = new CmdModel(dev.get(), new ValueInfoCache(proj->package()), bmcl::None);
            _proj = proj;
            _dev = dev;
            Rc<AllGcInterfaces> ifaces = new AllGcInterfaces(dev.get());
            BMCL_CRITICAL() << ifaces->errors();
            assert(ifaces->waypointInterface().isSome());

            PacketRequest req;
            req.streamType = StreamType::CmdTelem;
            uint8_t tmp[1024];
            bmcl::MemWriter writer(tmp, sizeof(tmp));
            assert(ifaces->waypointInterface()->encodeBeginRouteCmd(0, &writer));
            req.payload = bmcl::SharedBytes::create(writer.writenData());
            request(_exc, caf::infinite, SendReliablePacketAtom::value, req).then([this](const PacketResponse& resp) {

            });
        },
    };
}

void CmdState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}
}
