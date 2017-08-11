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
#include "decode/model/TmModel.h"
#include "decode/model/NodeView.h"
#include "decode/model/NodeViewUpdater.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ValueNode.h"
#include "decode/model/FindNode.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"

#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::NodeView::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::NodeViewUpdater::Pointer);

namespace decode {

TmState::TmState(caf::actor_config& cfg, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _handler(handler)
    , _lastCounter(0)
{
}

TmState::~TmState()
{
}

void TmState::on_exit()
{
    destroy(_handler);
}

caf::behavior TmState::make_behavior()
{
    return caf::behavior{
        [this](SetProjectAtom, Project::ConstPointer& proj, Device::ConstPointer& dev) {
            _model = new TmModel(dev.get(), new ValueInfoCache(proj->package()));
            //auto n = findTypedNode<NumericValueNode<uint16_t>>(_model.get(), "exc.outCounter");
            Rc<NodeView> view = new NodeView(_model.get());
            send(_handler, SetTmViewAtom::value, view);
            delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value);
        },
        [this](RecvUserPacketAtom, const bmcl::SharedBytes& data) {
            acceptData(data.view());
        },
        [this](PushTmUpdatesAtom) {
            Rc<NodeViewUpdater> updater = new NodeViewUpdater;
            _model->collectUpdates(updater.get());
            send(_handler, UpdateTmViewAtom::value, updater);
            delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value);
        },
        [this](StartAtom) {
        },
        [this](StopAtom) {
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            (void)isEnabled;
        },
    };
}

void TmState::acceptData(bmcl::Bytes packet)
{
    if (!_model) {
        return;
    }

    bmcl::MemReader src(packet);
    int64_t streamType;
    if (!src.readVarInt(&streamType)) {
        //TODO: report error
        return;
    }

    int64_t dataType;
    if (!src.readVarInt(&dataType)) {
        //TODO: report error
        return;
    }

    if (src.sizeLeft() < 2) {
        //TODO: report error
        return;
    }
    uint16_t counter = src.readUint16Le();

    uint16_t expectedCounter = _lastCounter + 1;
    if (counter != expectedCounter) {
        //TODO: estimate lost packets
    }
    _lastCounter = counter;

    uint64_t tickTime;
    if (!src.readVarUint(&tickTime)) {
        //TODO: report error
        return;
    }

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

        _model->acceptTmMsg(compNum, msgNum, bmcl::Bytes(msg.current(), msg.sizeLeft()));
    }
}
}
