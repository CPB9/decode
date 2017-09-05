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
#include "decode/model/NodeViewUpdate.h"

namespace decode {

class NodeViewStore;

class NodeViewUpdater : public RefCountable {
public:
    using Pointer = Rc<NodeViewUpdater>;
    using ConstPointer = Rc<const NodeViewUpdater>;

    NodeViewUpdater();
    ~NodeViewUpdater();

    void addValueUpdate(Value&& value, Node* parent);
    void addShrinkUpdate(std::size_t size, Node* parent);
    void addExtendUpdate(NodeViewVec&& vec, Node* parent);
    void apply(NodeViewStore* dest);

    const std::vector<NodeViewUpdate>& updates() const;

private:
    std::vector<NodeViewUpdate> _updates;
};
}
