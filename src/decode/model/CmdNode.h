/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/model/FieldsNode.h"
#include "decode/model/ValueNode.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/Fwd.h>

namespace decode {

class Function;
class Component;
class ValueInfoCache;

class CmdNode : public FieldsNode {
public:
    using Pointer = Rc<CmdNode>;
    using ConstPointer = Rc<const CmdNode>;

    CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs = true);
    ~CmdNode();

    bool encode(bmcl::MemWriter* dest) const;
    std::size_t numChildren() const override;
    bool canHaveChildren() const override;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;
    bmcl::StringView shortDescription() const override;

    Rc<CmdNode> clone(bmcl::OptionPtr<Node> parent);

    const Function* function() const
    {
        return _func.get();
    }

    const ValueInfoCache* cache() const
    {
        return _cache.get();
    }


private:
    Rc<const Component> _comp;
    Rc<const Function> _func;
    Rc<const ValueInfoCache> _cache;

    bool _expandArgs;
};

template <typename T, typename B = Node>
class GenericContainerNode : public B {
public:
    template <typename... A>
    GenericContainerNode(A&&... args)
        : B(std::forward<A>(args)...)
    {
    }

    ~GenericContainerNode()
    {
    }

    std::size_t numChildren() const override
    {
        return _nodes.size();
    }

    bmcl::Option<std::size_t> childIndex(const Node* node) const override
    {
        return Node::childIndexGeneric(_nodes, node);
    }

    bmcl::OptionPtr<Node> childAt(std::size_t idx) override
    {
        return Node::childAtGeneric(_nodes, idx);
    }

    typename RcVec<T>::ConstRange nodes() const
    {
        return _nodes;
    }

    typename RcVec<T>::Range nodes()
    {
        return _nodes;
    }

protected:
    RcVec<T> _nodes;
};

extern template class GenericContainerNode<CmdNode, Node>;

class ScriptNode : public GenericContainerNode<CmdNode, Node> {
public:
    ScriptNode(bmcl::OptionPtr<Node> parent);
    ~ScriptNode();

    static Rc<ScriptNode> withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs);

    void addCmdNode(CmdNode* node);

    bool encode(bmcl::MemWriter* dest) const;

    bmcl::StringView fieldName() const override;

    void swapNodes(std::size_t i1, std::size_t i2);

private:
    bmcl::StringView _fieldName;
};

extern template class GenericContainerNode<ValueNode, Node>;

class ScriptResultNode : public GenericContainerNode<ValueNode, Node> {
public:
    ScriptResultNode(bmcl::OptionPtr<Node> parent);
    ~ScriptResultNode();

    static Rc<ScriptResultNode> fromScriptNode(const ScriptNode* node, bmcl::OptionPtr<Node> parent);

    bool decode(bmcl::MemReader* src);

    bmcl::StringView fieldName() const override;

private:
    StrIndexCache _indexCache;
};
}
