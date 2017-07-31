#pragma once

#include "decode/Config.h"
#include "decode/model/Node.h"

namespace decode {

struct Device;
class ValueInfoCache;
class CmdContainerNode;

class CmdModel : public Node {
public:
    CmdModel(const Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~CmdModel();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    std::vector<Rc<CmdContainerNode>> _nodes;
};
}
