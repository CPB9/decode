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
    req.payload = bmcl::SharedBytes::create(writer.writenData());
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
using SendClearRouteCmdAtom = caf::atom_constant<caf::atom("sendclrt")>;
using SendEndRouteCmdAtom   = caf::atom_constant<caf::atom("sendenrt")>;

class NavActorBase : public caf::event_based_actor {
public:
    NavActorBase(caf::actor_config& cfg,
                 caf::actor exchange,
                 const WaypointGcInterface* iface,
                 const caf::response_promise& promise)
        : caf::event_based_actor(cfg)
        , _exchange(exchange)
        , _iface(iface)
        , _promise(promise)
    {
    }

    void on_exit() override
    {
        destroy(_exchange);
    }

protected:
    template <typename T>
    void action(T* actor, bool (T::* encode)(Encoder* dest) const, void (T::* next)(const PacketResponse& resp))
    {
        auto rv = encodePacket(std::bind(encode, actor, std::placeholders::_1));
        if (rv.isErr()) {
            _promise.deliver(caf::sec::invalid_argument);
            quit();
            return;
        }
        request(_exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([=](const PacketResponse& resp) {
            (actor->*next)(resp);
        },
        [=](const caf::error& err) {
            BMCL_CRITICAL() << err.code();
            _promise.deliver(caf::sec::invalid_argument);
            quit();
        });
    }

    caf::actor _exchange;
    Rc<const WaypointGcInterface> _iface;
    caf::response_promise _promise;
};

class RouteUploadActor : public NavActorBase {
public:
    RouteUploadActor(caf::actor_config& cfg,
                     caf::actor exchange,
                     const WaypointGcInterface* iface,
                     const caf::response_promise& promise,
                     Route&& route)
        : NavActorBase(cfg, exchange, iface, promise)
        , _route(std::move(route))
        , _currentIndex(0)
        , _routeIndex(0)
    {
    }

    bool encodeRoutePoint(Encoder* dest) const
    {
        return _iface->encodeSetRoutePointCmd(_routeIndex, _currentIndex, _route.waypoints[_currentIndex], dest);
    }

    bool encodeBeginRoute(Encoder* dest) const
    {
        return _iface->encodeBeginRouteCmd(_routeIndex, dest);
    }

    bool encodeClearRoute(Encoder* dest) const
    {
        return _iface->encodeClearRouteCmd(_routeIndex, dest);
    }

    bool encodeEndRoute(Encoder* dest) const
    {
        return _iface->encodeEndRouteCmd(_routeIndex, dest);
    }

    void sendFirstPoint(const PacketResponse&)
    {
        send(this, SendNextPointCmdAtom::value);
    }

    void sendClearRoute(const PacketResponse&)
    {
        send(this, SendClearRouteCmdAtom::value);
    }

    void sendNextPoint(const PacketResponse&)
    {
        _currentIndex++;
        send(this, SendNextPointCmdAtom::value);
    }

    void endUpload(const PacketResponse&)
    {
        _promise.deliver(caf::none);
        quit();
    }

    caf::behavior make_behavior() override
    {
        send(this, SendStartRouteCmdAtom::value);
        return caf::behavior{
            [this](SendNextPointCmdAtom) {
                if (_currentIndex >= _route.waypoints.size()) {
                    send(this, SendEndRouteCmdAtom::value);
                    return;
                }
                action(this, &RouteUploadActor::encodeRoutePoint, &RouteUploadActor::sendNextPoint);
            },
            [this](SendClearRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeClearRoute, &RouteUploadActor::sendFirstPoint);
            },
            [this](SendStartRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeBeginRoute, &RouteUploadActor::sendClearRoute);
            },
            [this](SendEndRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeEndRoute, &RouteUploadActor::endUpload);
            },
        };
    }

private:
    Route _route;
    std::size_t _currentIndex;
    std::size_t _routeIndex;
};

class OneActionNavActor : public NavActorBase {
public:
    OneActionNavActor(caf::actor_config& cfg,
                      caf::actor exchange,
                      const WaypointGcInterface* iface,
                      const caf::response_promise& promise)
        : NavActorBase(cfg, exchange, iface, promise)
    {
    }

    virtual bool encode(Encoder* dest) const = 0;

    virtual void end(const PacketResponse&)
    {
        _promise.deliver(caf::none);
        quit();
    }

    caf::behavior make_behavior() override
    {
        send(this, StartAtom::value);
        return caf::behavior{
            [this](StartAtom) {
                action(this, &OneActionNavActor::encode, &OneActionNavActor::end);
            },
        };
    }
};

class SetActiveRouteActor : public OneActionNavActor {
public:
    SetActiveRouteActor(caf::actor_config& cfg,
                        caf::actor exchange,
                        const WaypointGcInterface* iface,
                        const caf::response_promise& promise,
                        SetActiveRouteGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise)
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetActiveRouteCmd(_cmd.id, dest);
    }

private:
    SetActiveRouteGcCmd _cmd;
};

class SetRouteActivePointActor : public OneActionNavActor {
public:
    SetRouteActivePointActor(caf::actor_config& cfg,
                             caf::actor exchange,
                             const WaypointGcInterface* iface,
                             const caf::response_promise& promise,
                             SetRouteActivePointGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise)
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteActivePointCmd(_cmd.id, _cmd.index, dest);
    }

private:
    SetRouteActivePointGcCmd _cmd;
};

class SetRouteInvertedActor : public OneActionNavActor {
public:
    SetRouteInvertedActor(caf::actor_config& cfg,
                          caf::actor exchange,
                          const WaypointGcInterface* iface,
                          const caf::response_promise& promise,
                          SetRouteInvertedGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise)
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteInvertedCmd(_cmd.id, _cmd.isInverted, dest);
    }

private:
    SetRouteInvertedGcCmd _cmd;
};

class SetRouteClosedActor : public OneActionNavActor {
public:
    SetRouteClosedActor(caf::actor_config& cfg,
                        caf::actor exchange,
                        const WaypointGcInterface* iface,
                        const caf::response_promise& promise,
                        SetRouteClosedGcCmd&& cmd)
        : OneActionNavActor(cfg, exchange, iface, promise)
        , _cmd(std::move(cmd))
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeSetRouteClosedCmd(_cmd.id, _cmd.isClosed, dest);
    }

private:
    SetRouteClosedGcCmd _cmd;
};

caf::behavior CmdState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, Project::ConstPointer& proj, Device::ConstPointer& dev) {
            _model = new CmdModel(dev.get(), new ValueInfoCache(proj->package()), bmcl::None);
            _proj = proj;
            _dev = dev;
            _ifaces = new AllGcInterfaces(dev.get());
//             Route rt;
//             Waypoint wp;
//             wp.position = Position{1,2,3};
//             wp.action = SnakeWaypointAction{};
//             rt.waypoints.push_back(wp);
//             rt.waypoints.push_back(wp);
//             send(this, SendGcCommandAtom::value, GcCmd(rt));
//             SetActiveRouteGcCmd cmd;
//             cmd.id.emplace(0);
//             send(this, SendGcCommandAtom::value, GcCmd(cmd));
//             SetRouteActivePointGcCmd cmd2;
//             cmd2.id = 0;
//             cmd2.index.emplace(2);
//             send(this, SendGcCommandAtom::value, GcCmd(cmd2));
//             SetRouteInvertedGcCmd cmd3;
//             cmd3.id = 0;
//             cmd3.isInverted = true;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd3));
//             SetRouteClosedGcCmd cmd4;
//             cmd4.id = 0;
//             cmd4.isClosed = true;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd4));
        },
        [this](SendGcCommandAtom, GcCmd& cmd) -> caf::result<void> {
            if (_ifaces->waypointInterface().isNone()) {
                return caf::sec::invalid_argument;
            }
            caf::response_promise promise = make_response_promise();
            switch (cmd.kind()) {
            case GcCmdKind::None:
                return caf::sec::invalid_argument;
            case GcCmdKind::UploadRoute:
                spawn<RouteUploadActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<Route>()));
                return promise;
            case GcCmdKind::SetActiveRoute:
                spawn<SetActiveRouteActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetActiveRouteGcCmd>()));
                return promise;
            case GcCmdKind::SetRouteActivePoint:
                spawn<SetRouteActivePointActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteActivePointGcCmd>()));
                return promise;
            case GcCmdKind::SetRouteInverted:
                spawn<SetRouteInvertedActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteInvertedGcCmd>()));
                return promise;
            case GcCmdKind::SetRouteClosed:
                spawn<SetRouteClosedActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<SetRouteClosedGcCmd>()));
                return promise;
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
