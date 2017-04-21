#include "decode/model/CmdNode.h"
#include "decode/core/Try.h"
#include "decode/parser/Component.h"
#include "decode/parser/Type.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/MemWriter.h>

namespace decode {

CmdNode::CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : FieldsNode(func->type()->argumentsRange(), cache, parent)
    , _comp(comp)
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
    TRY(dest->writeVarUint(_comp->number()));
    auto it = std::find(_comp->cmdsBegin(), _comp->cmdsEnd(), _func.get());
    if (it == _comp->cmdsEnd()) {
        //TODO: report error
        return false;
    }

    TRY(dest->writeVarUint(std::distance(_comp->cmdsBegin(), it)));
    return encodeFields(handler, dest);
}

Rc<CmdNode> CmdNode::clone(bmcl::OptionPtr<Node> parent)
{
    return new CmdNode(_comp.get(), _func.get(), _cache.get(), parent);
}

bmcl::StringView CmdNode::typeName() const
{
    return _cache->nameForType(_func->type());
}

CmdContainerNode::CmdContainerNode(bmcl::OptionPtr<Node> parent)
    : Node(parent)
{
}

CmdContainerNode::~CmdContainerNode()
{
}

Rc<CmdContainerNode> CmdContainerNode::withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    CmdContainerNode* self = new CmdContainerNode(parent);
    for (const Function* f : comp->cmdsRange()) {
        self->_nodes.emplace_back(new CmdNode(comp, f, cache, self));
    }
    return self;
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
    return "packet";
}

void CmdContainerNode::addCmdNode(CmdNode* node)
{
    _nodes.emplace_back(node);
}

bool CmdContainerNode::encode(ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    for (const CmdNode* node : RcVec<CmdNode>::ConstRange(_nodes)) {
        TRY(node->encode(handler, dest));
    }
    return true;
}

void CmdContainerNode::swapNodes(std::size_t i1, std::size_t i2)
{
    std::swap(_nodes[i1], _nodes[i2]);
}
}
