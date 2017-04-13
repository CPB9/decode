#pragma once

#include "decode/Config.h"
#include "decode/model/FieldsNode.h"

#include <bmcl/Fwd.h>

namespace decode {

class Function;
class ValueInfoCache;
class ModelEventHandler;

class CmdNode : public FieldsNode {
public:
    CmdNode(const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~CmdNode();

    bool encode(ModelEventHandler* handler, bmcl::MemWriter* dest) const;

    bmcl::StringView typeName() const override;

private:
    Rc<const Function> _func;
    Rc<const ValueInfoCache> _cache;
};

class CmdContainerNode : public Node {
public:
    CmdContainerNode(RcVec<Function>::ConstRange, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~CmdContainerNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    std::vector<Rc<CmdNode>> _nodes;
};
}
