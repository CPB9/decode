#include "decode/model/CmdNode.h"
#include "decode/parser/Component.h"
#include "decode/parser/Type.h"
#include "decode/model/ValueInfoCache.h"

namespace decode {

CmdNode::CmdNode(const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : FieldsNode(func->type()->argumentsRange(), cache, parent)
    , _func(func)
    , _cache(cache)
{
    setName(func->name());
}

CmdNode::~CmdNode()
{
}

bool CmdNode::encode(ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    //TODO: implement
    return true;
}

bmcl::StringView CmdNode::typeName() const
{
    return _cache->nameForType(_func->type());
}

CmdContainerNode::CmdContainerNode(RcVec<Function>::ConstRange range, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : Node(parent)
{
    for (const Function* f : range) {
        _nodes.emplace_back(new CmdNode(f, cache, this));
    }
}

CmdContainerNode::~CmdContainerNode()
{
}

std::size_t CmdContainerNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> CmdContainerNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> CmdContainerNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView CmdContainerNode::fieldName() const
{
    return "asd";
}
}
