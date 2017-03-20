#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>

namespace decode {

Node::~Node()
{
}

std::size_t Node::numChildren() const
{
    return 0;
}

bmcl::Option<std::size_t> Node::childIndex(const Node* node) const
{
    return bmcl::None;
}

Node* Node::childAt(std::size_t idx)
{
    return nullptr;
}

bmcl::StringView Node::name() const
{
    return bmcl::StringView::empty();
}

Value Node::value() const
{
    return Value::makeNone();
}
}
