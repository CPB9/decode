/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/FindNode.h"
#include "decode/model/ValueNode.h"
#include "decode/model/NodeWithNamedChildren.h"

namespace decode {

FindNodeResult<Node> findNode(Node* node, bmcl::StringView path)
{
    Node* current = node;
    while (true) {
        if (path.isEmpty()) {
            return Rc<Node>(current);
        }
        auto dotPos = path.findFirstOf('.');
        bmcl::StringView name;
        if (dotPos.isNone()) {
            name = path;
            path = bmcl::StringView::empty();
        } else {
            name = path.sliceTo(dotPos.unwrap());
            path = path.sliceFrom(dotPos.unwrap() + 1);
            if (path.isEmpty()) {
                return FindNodeError::InvalidPath;
            }
        }
        if (NodeWithNamedChildren* fnode = dynamic_cast<NodeWithNamedChildren*>(current)) {
            auto next = fnode->nodeWithName(name);
            if (next.isNone()) {
                return FindNodeError::InvalidPath;
            }
            current = next.unwrap();
        } else if (StructValueNode* snode = dynamic_cast<StructValueNode*>(current)) {
            auto next = snode->nodeWithName(name);
            if (next.isNone()) {
                return FindNodeError::InvalidPath;
            }
            current = next.unwrap();
        } else {
            return FindNodeError::InvalidType;
        }
    }
}
}
