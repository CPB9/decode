#include "decode/groundcontrol/CmdState.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/parser/Project.h"
#include "decode/model/CmdModel.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/Encoder.h"
#include "decode/groundcontrol/GcInterface.h"
#include "decode/groundcontrol/Packet.h"
#include "decode/groundcontrol/GcCmd.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/Result.h>

#include <caf/sec.hpp>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::GcCmd);

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

static CmdState::EncodeResult encodePacket(CmdState::EncodeHandler handler)
{
    PacketRequest req;
    req.streamType = StreamType::CmdTelem;
    uint8_t tmp[1024];
    Encoder writer(tmp, sizeof(tmp));
    if (!handler(&writer)) {
        return writer.errorMsgOr("unknown error").toStdString();
    }

    return req;
}

struct RouteUploadState {
    RouteUploadState()
        : currentIndex(0)
        , routeIndex(0)
    {
    }

    caf::actor exchange;
    Rc<const WaypointGcInterface> iface;
    caf::response_promise promise;
    Route route;
    std::size_t currentIndex;
    std::size_t routeIndex;
};

using SendNextPointCmdAtom  = caf::atom_constant<caf::atom("sendnxtp")>;
using SendStartRouteCmdAtom = caf::atom_constant<caf::atom("sendstrt")>;
using SendEndRouteCmdAtom   = caf::atom_constant<caf::atom("sendenrt")>;

caf::behavior routeUploadActor(caf::stateful_actor<RouteUploadState>* self,
                               caf::actor exchange,
                               const WaypointGcInterface* iface,
                               const caf::response_promise& promise,
                               Route&& route)
{
    self->state.exchange = exchange;
    self->state.iface = iface;
    self->state.promise = promise;
    self->state.route = std::move(route);
    self->send(self, SendStartRouteCmdAtom::value);
    return caf::behavior{
        [self](SendNextPointCmdAtom) {
            if (self->state.currentIndex >= self->state.route.waypoints.size()) {
                self->send(self, SendEndRouteCmdAtom::value);
                return;
            }
            auto rv = encodePacket([self](Encoder* dest) {
                return self->state.iface->encodeSetRoutePointCmd(self->state.routeIndex, self->state.currentIndex,
                                                                 self->state.route.waypoints[self->state.currentIndex], dest);
            });
            if (rv.isErr()) {
                self->state.promise.deliver(caf::sec::invalid_argument);
                return;
            }
            self->request(self->state.exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([self](const PacketResponse& resp) {
                self->send(self, SendNextPointCmdAtom::value);
                self->state.currentIndex++;
            });
        },
        [self](SendStartRouteCmdAtom) {
            auto rv = encodePacket([self](Encoder* dest) {
                return self->state.iface->encodeBeginRouteCmd(self->state.routeIndex, dest);
            });
            if (rv.isErr()) {
                self->state.promise.deliver(caf::sec::invalid_argument);
                return;
            }
            self->request(self->state.exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([self](const PacketResponse& resp) {
                self->send(self, SendNextPointCmdAtom::value);
            });
        },
        [self](SendEndRouteCmdAtom) {
             auto rv = encodePacket([self](Encoder* dest) {
                return self->state.iface->encodeEndRouteCmd(self->state.routeIndex, dest);
            });
            if (rv.isErr()) {
                self->state.promise.deliver(caf::sec::invalid_argument);
                return;
            }
            self->request(self->state.exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([self](const PacketResponse& resp) {
                self->state.promise.deliver(caf::none);
                BMCL_DEBUG() << "ooook";
                self->send_exit(self, caf::exit_reason::normal);
            });
        },
    };
}

caf::behavior CmdState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, Project::ConstPointer& proj, Device::ConstPointer& dev) {
            _model = new CmdModel(dev.get(), new ValueInfoCache(proj->package()), bmcl::None);
            _proj = proj;
            _dev = dev;
            _ifaces = new AllGcInterfaces(dev.get());
        },
        [this](SendGcCommandAtom, const GcCmd& cmd) -> caf::result<void> {
            caf::response_promise promise = make_response_promise();
            switch (cmd.kind()) {
            case GcCmdKind::None:
                return caf::sec::invalid_argument;
            case GcCmdKind::UploadRoute:
                if (_ifaces->waypointInterface().isNone()) {
                    return caf::sec::invalid_argument;
                }
                spawn(routeUploadActor, _exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<Route>()));
                return promise;
            case GcCmdKind::Test:
                return caf::sec::invalid_argument;
            };
            return caf::sec::invalid_argument;
        },
    };
}

void CmdState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}
}
