/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/model/Node.h"
#include "decode/core/HashMap.h"
#include "decode/model/NodeWithNamedChildren.h"

#include <vector>

namespace decode {

struct Device;
class ValueInfoCache;
class FieldsNode;
class StatusDecoder;
class NodeViewUpdater;
template <typename T>
class NumericValueNode;
class ComponentParamsNode;

class TmModel : public NodeWithNamedChildren {
public:
    using Pointer = Rc<TmModel>;
    using ConstPointer = Rc<const TmModel>;

    TmModel(const Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent = bmcl::None);
    ~TmModel();

    void acceptTmMsg(uint64_t compNum, uint64_t msgNum, bmcl::Bytes payload);

    bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) override;

    void collectUpdates(NodeViewUpdater* dest) override;
    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    HashMap<uint64_t, Rc<StatusDecoder>> _decoders;
    std::vector<Rc<ComponentParamsNode>> _nodes;
};
}
