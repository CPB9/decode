#pragma once

#include "decode/Config.h"
#include "decode/model/FieldsNode.h"

#include <bmcl/Fwd.h>

namespace decode {

class Function;
class Component;
class ValueInfoCache;
class ModelEventHandler;

class CmdNode : public FieldsNode {
public:
    CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~CmdNode();

    bool encode(ModelEventHandler* handler, bmcl::MemWriter* dest) const;

    bmcl::StringView typeName() const override;

    Rc<CmdNode> clone(bmcl::OptionPtr<Node> parent);

private:
    Rc<const Component> _comp;
    Rc<const Function> _func;
    Rc<const ValueInfoCache> _cache;
};

class CmdContainerNode : public Node {
public:
    CmdContainerNode(bmcl::OptionPtr<Node> parent);
    ~CmdContainerNode();

    static Rc<CmdContainerNode> withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);

    void addCmdNode(CmdNode* node);

    bool encode(ModelEventHandler* handler, bmcl::MemWriter* dest) const;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    RcVec<CmdNode> _nodes;
};
}
