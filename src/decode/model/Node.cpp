#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>

namespace decode {

Node::~Node()
{
}

bool Node::canHaveChildren() const
{
    return true;
}

std::size_t Node::numChildren() const
{
    return 0;
}

bmcl::Option<std::size_t> Node::childIndex(const Node* node) const
{
    return bmcl::None;
}

bmcl::OptionPtr<Node> Node::childAt(std::size_t idx)
{
    return bmcl::None;
}

bmcl::StringView Node::fieldName() const
{
    return bmcl::StringView::empty();
}

bmcl::StringView Node::typeName() const
{
    return bmcl::StringView::empty();
}

Value Node::value() const
{
    return Value::makeNone();
}

ValueKind Node::valueKind() const
{
    return ValueKind::None;
}

bool Node::canSetValue() const
{
    return false;
}

bool Node::setValue(const Value& value)
{
    (void)value;
    return false;
}
}
