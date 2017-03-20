#include "decode/model/FieldsNode.h"
#include "decode/parser/Component.h"
#include "decode/model/ValueNode.h"
#include "decode/parser/Decl.h" //HACK

namespace decode {

FieldsNode::FieldsNode(const FieldList* params, const ValueInfoCache* cache, Node* parent)
    : Node(parent)
{
    for (const Rc<Field>& field : *params) {
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

Node* FieldsNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}
}
