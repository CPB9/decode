#include "decode/model/CmdModel.h"
#include "decode/parser/Project.h"
#include "decode/parser/Component.h"
#include "decode/parser/Ast.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/CmdNode.h"

namespace decode {

CmdModel::CmdModel(const Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : Node(parent)
{
    for (const Rc<Ast>& ast : dev->modules) {
        if (ast->component().isNone()) {
            continue;
        }

        const Component* it = ast->component().unwrap();

        if (!it->hasCmds()) {
            continue;
        }

        Rc<CmdContainerNode> node = CmdContainerNode::withAllCmds(it, cache, this, false);
        _nodes.emplace_back(node);
    }
}

CmdModel::~CmdModel()
{
}

std::size_t CmdModel::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> CmdModel::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> CmdModel::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView CmdModel::fieldName() const
{
    return "cmds";
}
}
