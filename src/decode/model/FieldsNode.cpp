#include "decode/model/FieldsNode.h"
#include "decode/parser/Component.h"
#include "decode/model/ValueNode.h"
#include "decode/parser/Decl.h" //HACK

namespace decode {

FieldsNode::FieldsNode(FieldVec::ConstRange params, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : Node(parent)
{
    for (const Field* field : params) {
        Rc<ValueNode> node = ValueNode::fromType(field->type(), cache, this);
        node->setFieldName(field->name());
        _nameToNodeMap.emplace(field->name(), node);
        _nodes.emplace_back(node);
    }
}

FieldsNode::~FieldsNode()
{
}

bmcl::StringView FieldsNode::fieldName() const
{
    return _name;
}

bmcl::OptionPtr<ValueNode> FieldsNode::nodeWithName(bmcl::StringView name)
{
    auto it = _nameToNodeMap.find(name);
    if (it == _nameToNodeMap.end()) {
        return bmcl::None;
    }
    return it->second.get();
}

std::size_t FieldsNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> FieldsNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> FieldsNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

void FieldsNode::setName(bmcl::StringView name)
{
    _name = name;
}

bool FieldsNode::encodeFields(ModelEventHandler* handler, bmcl::MemWriter* dest) const
{
    for (std::size_t i = 0; i < _nodes.size(); i++) {
        const ValueNode* node = _nodes[i].get();
        if (!node->encode(i, handler, dest)) {
            //TODO: report error
            return false;
        }
    }
    return true;
}
}
