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
#include "decode/core/HashMap.h"

#include <bmcl/Fwd.h>

#include <vector>

namespace bmcl { class MemReader; }

namespace decode {

class FieldsNode;
class Statuses;
class StatusMsg;
class ValueNode;
class DecoderAction;

struct ChainElement {
    ChainElement(std::size_t index, DecoderAction* action, ValueNode* node);
    ChainElement(const ChainElement& other);
    ~ChainElement();

    std::size_t nodeIndex;
    Rc<DecoderAction> action;
    Rc<ValueNode> node;
};

class StatusMsgDecoder {
public:

    StatusMsgDecoder(const StatusMsg* msg, FieldsNode* node);
    ~StatusMsgDecoder();

    bool decode(bmcl::MemReader* src);

private:
    std::vector<ChainElement> _chain;
};

class StatusDecoder : public RefCountable {
public:
    using Pointer = Rc<StatusDecoder>;
    using ConstPointer = Rc<const StatusDecoder>;

    template <typename R>
    StatusDecoder(R statusRange, FieldsNode* node)
    {
        for (const auto& it : statusRange) {
            _decoders.emplace(std::piecewise_construct,
                              std::forward_as_tuple(it->number()),
                              std::forward_as_tuple(it, node));
        }
    }

    ~StatusDecoder();

    bool decode(uint64_t msgId, bmcl::Bytes payload);

private:
    HashMap<uint64_t, StatusMsgDecoder> _decoders;
};
}
