/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/TmState.h"
#include "decode/groundcontrol/Atoms.h"

#include "decode/ast/Type.h"
#include "decode/parser/Project.h"
#include "decode/model/TmModel.h"
#include "decode/model/NodeView.h"
#include "decode/model/NodeViewUpdater.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ValueNode.h"
#include "decode/model/FindNode.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/groundcontrol/TmParamUpdate.h"

#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::NodeView::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::NodeViewUpdater::Pointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::TmParamUpdate);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Value);

namespace decode {

TmState::Sub::Sub(const BuiltinValueNode* node, const caf::actor& dest)
    : node(node)
    , actor(dest)
{
}

TmState::Sub::~Sub()
{
}

TmState::TmState(caf::actor_config& cfg, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _handler(handler)
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
            initTmNodes();
            Rc<NodeView> view = new NodeView(_model.get());
            send(_handler, SetTmViewAtom::value, view);
            _updateCount++;
            delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value, _updateCount);
        },
        [this](RecvPacketPayloadAtom, const bmcl::SharedBytes& data) {
            acceptData(data.view());
        },
        [this](PushTmUpdatesAtom, uint64_t count) {
            if (count != _updateCount) {
                return;
            }
            pushTmUpdates();
        },
        [this](SubscribeTmAtom, const std::string& path, const caf::actor& dest) {
            return subscribeTm(path, dest);
        },
        [this](StartAtom) {
            (void)this;
        },
        [this](StopAtom) {
            (void)this;
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            (void)this;
            (void)isEnabled;
        },
    };
}

bool TmState::subscribeTm(bmcl::StringView path, const caf::actor& dest)
{
    auto rv = findNode(_model.get(), path);
    if (rv.isErr()) {
        return false;
    }
    Node* node = rv.unwrap().get();
    BuiltinValueNode* builtinNode = dynamic_cast<BuiltinValueNode*>(node);
    if (!builtinNode) {
        return false;
    }
    _subscriptions.emplace_back(builtinNode, dest);
    return true;
}

void TmState::pushTmUpdates()
{
    Rc<NodeViewUpdater> updater = new NodeViewUpdater;
    _model->collectUpdates(updater.get());
    if (_features.hasPosition) {
        Position pos;
        updateParam(_latNode, &pos.latLon.latitude);
        updateParam(_lonNode, &pos.latLon.longitude);
        updateParam(_altNode, &pos.altitude);
        send(_handler, UpdateTmParams::value, TmParamUpdate(pos));
    }
    if (_features.hasVelocity) {
        Velocity3 vel;
        updateParam(_velXNode, &vel.x);
        updateParam(_velYNode, &vel.y);
        updateParam(_velZNode, &vel.z);
        send(_handler, UpdateTmParams::value, TmParamUpdate(vel));
    }
    if (_features.hasOrientation) {
        Orientation orientation;
        updateParam(_headingNode, &orientation.heading);
        updateParam(_pitchNode, &orientation.pitch);
        updateParam(_rollNode, &orientation.roll);
        send(_handler, UpdateTmParams::value, TmParamUpdate(orientation));
    }
    send(_handler, UpdateTmViewAtom::value, updater);
    for (const Sub& sub : _subscriptions) {
        send(sub.actor, sub.node->value());
    }
    _updateCount++;
    delayed_send(this, std::chrono::milliseconds(1000), PushTmUpdatesAtom::value, _updateCount);
}

template <typename T>
void TmState::updateParam(const Rc<NumericValueNode<T>>& src, T* dest, T defaultValue)
{
    if (src) {
        auto value = src->rawValue();
        if (value.isSome()) {
            *dest = value.unwrap();
        } else {
            *dest = defaultValue;
        }
    }
}

template <typename T>
void TmState::initTypedNode(const char* name, Rc<T>* dest)
{
    auto node = findTypedNode<T>(_model.get(), name);
    if (node.isOk()) {
        *dest = node.unwrap();
    }
}

void TmState::initTmNodes()
{
    initTypedNode("nav.latLon.latitude", &_latNode);
    initTypedNode("nav.latLon.longitude", &_lonNode);
    initTypedNode("nav.altitude", &_altNode);
    _features.hasPosition = _latNode || _lonNode || _altNode;
    initTypedNode("nav.velocity.x", &_velXNode);
    initTypedNode("nav.velocity.y", &_velYNode);
    initTypedNode("nav.velocity.z", &_velZNode);
    _features.hasVelocity = _velXNode || _velYNode || _velZNode;
    initTypedNode("nav.orientation.heading", &_headingNode);
    initTypedNode("nav.orientation.pitch", &_pitchNode);
    initTypedNode("nav.orientation.roll", &_rollNode);
    _features.hasOrientation = _headingNode || _pitchNode || _rollNode;
}

void TmState::acceptData(bmcl::Bytes packet)
{
    if (!_model) {
        return;
    }

    bmcl::MemReader src(packet);
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
