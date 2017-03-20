#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"
#include "decode/model/Node.h"

#include <bmcl/StringView.h>
#include <bmcl/OptionPtr.h>

#include <cstdint>
#include <unordered_map>

namespace decode {

class FieldList;
class ValueNode;

class FieldsNode : public Node {
public:
    FieldsNode(FieldList* params, Node* parent);
    ~FieldsNode();

    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

    void setName(bmcl::StringView name)
    {
        _name = name;
    }

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    Node* childAt(std::size_t idx) override;
    bmcl::StringView name() const override;

public:
    std::unordered_map<bmcl::StringView, Rc<ValueNode>> _nameToNodeMap;
    std::vector<Rc<ValueNode>> _nodes;
    bmcl::StringView _name;
};
}
