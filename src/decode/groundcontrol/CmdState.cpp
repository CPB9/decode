#include "decode/groundcontrol/CmdState.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/parser/Project.h"
#include "decode/model/CmdModel.h"
#include "decode/model/ValueInfoCache.h"

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
        },
    };
}

void CmdState::on_exit()
{
    destroy(_exc);
    destroy(_handler);
}
}
