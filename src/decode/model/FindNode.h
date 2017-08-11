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

#include <bmcl/Result.h>
#include <bmcl/StringView.h>

namespace decode {

class Node;

enum class FindNodeError {
    InvalidPath,
    InvalidType,
};

template <typename T>
using FindNodeResult = bmcl::Result<Rc<T>, FindNodeError>;

FindNodeResult<Node> findNode(Node* node, bmcl::StringView path);

template <typename T>
FindNodeResult<T> findTypedNode(Node* node, bmcl::StringView path)
{
    auto child = findNode(node, path);
    if (child.isErr()) {
        return child.unwrapErr();
    }
    T* typedNode = dynamic_cast<T*>(child.unwrap().get());
    if (!typedNode) {
        return FindNodeError::InvalidType;
    }
    return Rc<T>(typedNode);
}
}
