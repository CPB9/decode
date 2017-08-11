#pragma once

#include "decode/Config.h"
#include "decode/model/Node.h"

#include <bmcl/Fwd.h>

namespace decode {

class NodeWithNamedChildren : public Node {
public:
    using Node::Node;

    virtual bmcl::OptionPtr<Node> nodeWithName(bmcl::StringView name) = 0;
};

}
