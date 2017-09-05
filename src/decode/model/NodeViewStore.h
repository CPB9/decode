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

namespace decode {

class NodeView;
class NodeViewUpdate;

class NodeViewStore {
public:
    NodeViewStore(NodeView* view);
    ~NodeViewStore();

    void setRoot(NodeView* view);
    bool apply(NodeViewUpdate* update);

private:
    void registerNodes(NodeView* view);
    void unregisterNodes(NodeView* view);

    Rc<NodeView> _root;
    HashMap<uintptr_t, Rc<NodeView>> _map;
};
}
