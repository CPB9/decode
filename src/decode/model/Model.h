#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/Node.h"

#include <bmcl/Fwd.h>

#include <cstdint>
#include <unordered_map>
#include <array>

namespace decode {

class Package;
class StatusDecoder;
class Component;
class FieldsNode;
class ValueInfoCache;
class ModelEventHandler;

class TmNode : public Node {
public:
    TmNode(const Package* package, const ValueInfoCache* cache, ModelEventHandler* handler, Node* parent);
    ~TmNode();

    void acceptTelemetry(bmcl::Bytes bytes);

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    Rc<ModelEventHandler> _handler;
    std::unordered_map<uint64_t, Rc<StatusDecoder>> _decoders;
    std::vector<Rc<FieldsNode>> _nodes;
};

class Model : public Node {
public:
    Model(const Package* package, ModelEventHandler* handler);
    ~Model();

    void acceptTelemetry(bmcl::Bytes bytes);

    TmNode* tmNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    Rc<const Package> _package;
    Rc<ValueInfoCache> _cache;
    Rc<ModelEventHandler> _handler;
    Rc<TmNode> _tmNode;
    std::array<Rc<Node>, 1> _nodes;
};
}
