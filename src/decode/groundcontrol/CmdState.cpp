#include "decode/groundcontrol/CmdState.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/parser/Project.h"
#include "decode/model/CmdModel.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/Encoder.h"
#include "decode/model/Decoder.h"
#include "decode/groundcontrol/GcInterface.h"
#include "decode/groundcontrol/Packet.h"
#include "decode/groundcontrol/GcCmd.h"
#include "decode/groundcontrol/TmParamUpdate.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/Result.h>

#include <caf/sec.hpp>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::TmParamUpdate);
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

using SendNextPointCmdAtom          = caf::atom_constant<caf::atom("sendnxtp")>;
using SendStartRouteCmdAtom         = caf::atom_constant<caf::atom("sendstrt")>;
using SendClearRouteCmdAtom         = caf::atom_constant<caf::atom("sendclrt")>;
using SendEndRouteCmdAtom           = caf::atom_constant<caf::atom("sendenrt")>;
using SendSetPointActiveCmdAtom     = caf::atom_constant<caf::atom("sendspac")>;
using SendSetRouteInvertedCmdAtom   = caf::atom_constant<caf::atom("sendsrti")>;
using SendSetRouteClosedCmdAtom     = caf::atom_constant<caf::atom("sendsrtc")>;

template <typename T>
class GcActorBase : public caf::event_based_actor {
public:
    GcActorBase(caf::actor_config& cfg,
                caf::actor exchange,
                const T* iface,
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
    template <typename R>
    void action(R* actor, bool (R::* encode)(Encoder* dest) const, void (R::* next)(const PacketResponse& resp))
    {
        auto rv = encodePacket(std::bind(encode, actor, std::placeholders::_1));
        if (rv.isErr()) {
            BMCL_CRITICAL() << "error encoding packet";
            _promise.deliver(caf::sec::invalid_argument);
            quit();
            return;
        }
        request(_exchange, caf::infinite, SendReliablePacketAtom::value, rv.unwrap()).then([=](const PacketResponse& resp) {
            if (resp.type == ReceiptType::Ok) {
                (actor->*next)(resp);
                return;
            }
            BMCL_CRITICAL() << "could not deliver packet";
            _promise.deliver(caf::sec::invalid_argument);
            quit();
        },
        [=](const caf::error& err) {
            BMCL_CRITICAL() << err.code();
            _promise.deliver(caf::sec::invalid_argument);
            quit();
        });
    }

    caf::actor _exchange;
    Rc<const T> _iface;
    caf::response_promise _promise;
};

using NavActorBase = GcActorBase<WaypointGcInterface>;

class RouteUploadActor : public NavActorBase {
public:
    RouteUploadActor(caf::actor_config& cfg,
                     caf::actor exchange,
                     const WaypointGcInterface* iface,
                     const caf::response_promise& promise,
                     UploadRouteGcCmd&& route)
        : NavActorBase(cfg, exchange, iface, promise)
        , _route(std::move(route))
        , _currentIndex(0)
    {
    }

    bool encodeRoutePoint(Encoder* dest) const
    {
        return _iface->encodeSetRoutePointCmd(_route.id, _currentIndex, _route.waypoints[_currentIndex], dest);
    }

    bool encodeBeginRoute(Encoder* dest) const
    {
        return _iface->encodeBeginRouteCmd(_route.id, _route.waypoints.size(), dest);
    }

    bool encodeClearRoute(Encoder* dest) const
    {
        return _iface->encodeClearRouteCmd(_route.id, dest);
    }

    bool encodeEndRoute(Encoder* dest) const
    {
        return _iface->encodeEndRouteCmd(_route.id, dest);
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

    void sendSetActivePoint(const PacketResponse&)
    {
        send(this, SendSetPointActiveCmdAtom::value);
    }

    void sendSetRouteClosed(const PacketResponse&)
    {
        send(this, SendSetRouteClosedCmdAtom::value);
    }

    void sendSetRouteInverted(const PacketResponse&)
    {
        send(this, SendSetRouteInvertedCmdAtom::value);
    }

    bool encodeSetActivePoint(Encoder* dest) const
    {
        return _iface->encodeSetRouteActivePointCmd(_route.id, _route.activePoint, dest);
    }

    bool encodeSetRouteClosed(Encoder* dest) const
    {
        return _iface->encodeSetRouteClosedCmd(_route.id, _route.isClosed, dest);
    }

    bool encodeSetRouteInverted(Encoder* dest) const
    {
        return _iface->encodeSetRouteInvertedCmd(_route.id, _route.isInverted, dest);
    }

    void endUpload(const PacketResponse&)
    {
        _promise.deliver(caf::unit);
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
            [this](SendStartRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeBeginRoute, &RouteUploadActor::sendFirstPoint);
            },
            [this](SendEndRouteCmdAtom) {
                action(this, &RouteUploadActor::encodeEndRoute, &RouteUploadActor::sendSetActivePoint);
            },
            [this](SendSetPointActiveCmdAtom) {
                action(this, &RouteUploadActor::encodeSetActivePoint, &RouteUploadActor::sendSetRouteClosed);
            },
            [this](SendSetRouteClosedCmdAtom) {
                action(this, &RouteUploadActor::encodeSetRouteClosed, &RouteUploadActor::sendSetRouteInverted);
            },
            [this](SendSetRouteInvertedCmdAtom) {
                action(this, &RouteUploadActor::encodeSetRouteInverted, &RouteUploadActor::endUpload);
            },
        };
    }

private:
    UploadRouteGcCmd _route;
    std::size_t _currentIndex;
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
        _promise.deliver(caf::unit);
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

class DownloadRouteInfoActor : public OneActionNavActor {
public:
    DownloadRouteInfoActor(caf::actor_config& cfg,
                           caf::actor exchange,
                           const WaypointGcInterface* iface,
                           const caf::response_promise& promise,
                           caf::actor handler)
        : OneActionNavActor(cfg, exchange, iface, promise)
        , _handler(handler)
    {
    }

    bool encode(Encoder* dest) const override
    {
        return _iface->encodeGetRoutesInfoCmd(dest);
    }

    void end(const PacketResponse& resp) override
    {
        Decoder dec(resp.payload.view());
        AllRoutesInfo info;
        if (!_iface->decodeGetRoutesInfoResponse(&dec, &info)) {
            _promise.deliver(caf::sec::invalid_argument);
        } else {
            send(_handler, UpdateTmParams::value, TmParamUpdate(std::move(info)));
            _promise.deliver(caf::unit);
        }
        quit();
    }

private:
    caf::actor _handler;
};

using SendGetInfoAtom       = caf::atom_constant<caf::atom("sendgein")>;
using SendGetNextPointAtom  = caf::atom_constant<caf::atom("sendgenp")>;

class DownloadRouteActor : public NavActorBase {
public:
    DownloadRouteActor(caf::actor_config& cfg,
                       caf::actor exchange,
                       const WaypointGcInterface* iface,
                       const caf::response_promise& promise,
                       DownloadRouteGcCmd&& cmd,
                       caf::actor handler)
        : NavActorBase(cfg, exchange, iface, promise)
        , _currentIndex(0)
        , _handler(handler)
    {
        _route.id = cmd.id;
    }

    bool encodeGetInfo(Encoder* dest) const
    {
        return _iface->encodeGetRouteInfoCmd(_route.id, dest);
    }

    bool encodeGetRoutePoint(Encoder* dest) const
    {
        return _iface->encodeGetRoutePointCmd(_route.id, _currentIndex, dest);
    }

    void unpackInfoAndSendGetPoint(const PacketResponse& resp)
    {
        Decoder dec(resp.payload.view());
        if (!_iface->decodeGetRouteInfoResponse(&dec, &_route.route.info)) {
            _promise.deliver(caf::sec::invalid_argument);
            return;
        }
        send(this, SendGetNextPointAtom::value);
    }

    void unpackPointAndSendGetPoint(const PacketResponse& resp)
    {
        Decoder dec(resp.payload.view());
        Waypoint point;
        if (!_iface->decodeGetRoutePointResponse(&dec, &point)) {
            _promise.deliver(caf::sec::invalid_argument);
            return;
        }
        _route.route.waypoints.push_back(point);
        _currentIndex++;
        send(this, SendGetNextPointAtom::value);
    }

    void endDownload()
    {
        _promise.deliver(caf::unit);
        send(_handler, UpdateTmParams::value, TmParamUpdate(std::move(_route)));
        quit();
    }

    caf::behavior make_behavior() override
    {
        send(this, SendGetInfoAtom::value);
        return caf::behavior{
            [this](SendGetNextPointAtom) {
                if (_currentIndex >= _route.route.info.size) {
                    endDownload();
                    return;
                }
                action(this, &DownloadRouteActor::encodeGetRoutePoint, &DownloadRouteActor::unpackPointAndSendGetPoint);
            },
            [this](SendGetInfoAtom) {
                action(this, &DownloadRouteActor::encodeGetInfo, &DownloadRouteActor::unpackInfoAndSendGetPoint);
            },
        };
    }

private:
    RouteTmParam _route;
    std::size_t _currentIndex;
    caf::actor _handler;
};

using SendBeginFileAtom     = caf::atom_constant<caf::atom("sendbef")>;
using SendNextFileChunkAtom = caf::atom_constant<caf::atom("sendnfc")>;
using SendEndFileAtom       = caf::atom_constant<caf::atom("sendenf")>;

class UploadFileActor : public GcActorBase<FileGcInterface> {
public:
    UploadFileActor(caf::actor_config& cfg,
                    caf::actor exchange,
                    const FileGcInterface* iface,
                    const caf::response_promise& promise,
                    UploadFileGcCmd&& cmd)
        : GcActorBase<FileGcInterface>(cfg, exchange, iface, promise)
        , _reader(cmd.reader)
        , _id(cmd.id)
    {
    }

    bool encodeBeginFile(Encoder* dest) const
    {
        return _iface->encodeBeginFile(_id, _reader->size(), dest);
    }

    bool encodeEndFile(Encoder* dest) const
    {
        return _iface->encodeEndFile(_id, _reader->size(), dest);
    }

    bool encodeNextChunk(Encoder* dest) const
    {
        bmcl::Bytes chunk = _reader->readNext(_iface->maxFileChunkSize());
        return _iface->encodeWriteFile(_id, _reader->offset(), chunk, dest);
    }

    void sendNextChunk(const PacketResponse&)
    {
        send(this, SendNextFileChunkAtom::value);
    }

    void endUpload(const PacketResponse&)
    {
        _promise.deliver(caf::unit);
        quit();
    }

    const char* name() const override
    {
        return "FileUploadActor";
    }

    caf::behavior make_behavior() override
    {
        send(this, SendBeginFileAtom::value);
        return caf::behavior{
            [this](SendBeginFileAtom) {
                action(this, &UploadFileActor::encodeBeginFile, &UploadFileActor::sendNextChunk);
            },
            [this](SendNextFileChunkAtom) {
                if (_reader->hasData()) {
                    action(this, &UploadFileActor::encodeNextChunk, &UploadFileActor::sendNextChunk);
                    return;
                }
                send(this, SendEndFileAtom::value);
            },
            [this](SendEndFileAtom) {
                action(this, &UploadFileActor::encodeEndFile, &UploadFileActor::endUpload);
            },
        };
    }

private:
    Rc<DataReader> _reader;
    std::uintmax_t _id;
};

caf::behavior CmdState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, Project::ConstPointer& proj, Device::ConstPointer& dev) {
            _model = new CmdModel(dev.get(), new ValueInfoCache(proj->package()), bmcl::None);
            _proj = proj;
            _dev = dev;
            _ifaces = new AllGcInterfaces(dev.get());
            if (!_ifaces->errors().empty()) {
                BMCL_CRITICAL() << _ifaces->errors();
            }
//             assert(_ifaces->waypointInterface().isSome());
//             UploadRouteGcCmd rt;
//             Waypoint wp;
//             wp.position = Position{1,2,3};
//             wp.speed.emplace(7);
//             wp.action = SnakeWaypointAction{};
//             rt.waypoints.push_back(wp);
//             rt.waypoints.push_back(wp);
//             rt.id = 0;
//             rt.isClosed = true;
//             rt.isInverted = true;
//             rt.activePoint.emplace(2);
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
//             DownloadRouteInfoGcCmd cmd5;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd5));
//             DownloadRouteGcCmd cmd6;
//             cmd6.id = 0;
//             send(this, SendGcCommandAtom::value, GcCmd(cmd6));
//             UploadFileGcCmd cmd7;
//             cmd7.id = 12;
//             cmd7.reader = new MemDataReader(std::vector<uint8_t>(3000, 4));
//             send(this, SendGcCommandAtom::value, GcCmd(cmd7));
        },
        [this](SendGcCommandAtom, GcCmd& cmd) -> caf::result<void> {
            //FIXME: check per command
            if (!_proj) {
                return caf::sec::invalid_argument;
            }
            if (_ifaces->waypointInterface().isNone()) {
                return caf::sec::invalid_argument;
            }
            if (_ifaces->fileInterface().isNone()) {
                return caf::sec::invalid_argument;
            }
            caf::response_promise promise = make_response_promise();
            switch (cmd.kind()) {
            case GcCmdKind::None:
                return caf::sec::invalid_argument;
            case GcCmdKind::UploadRoute:
                spawn<RouteUploadActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<UploadRouteGcCmd>()));
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
            case GcCmdKind::DownloadRouteInfo:
                spawn<DownloadRouteInfoActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, _handler);
                return promise;
            case GcCmdKind::DownloadRoute:
                spawn<DownloadRouteActor>(_exc, _ifaces->waypointInterface().unwrap(), promise, std::move(cmd.as<DownloadRouteGcCmd>()), _handler);
                return promise;
            case GcCmdKind::UploadFile:
                spawn<UploadFileActor>(_exc, _ifaces->fileInterface().unwrap(), promise, std::move(cmd.as<UploadFileGcCmd>()));
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

const char* CmdState::name() const
{
    return "CmdStateActor";
}
}
