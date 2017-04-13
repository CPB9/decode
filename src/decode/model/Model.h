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

class PackageTmNode : public Node {
public:
    PackageTmNode(const Package* package, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent);
    ~PackageTmNode();

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

class CmdContainerNode;

class PackageCmdsNode : public Node {
public:
    PackageCmdsNode(const Package* package, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent);
    ~PackageCmdsNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    Rc<ModelEventHandler> _handler;
    std::vector<Rc<CmdContainerNode>> _nodes;
};

class Model : public Node {
public:
    Model(const Package* package, ModelEventHandler* handler);
    ~Model();

    void acceptTelemetry(bmcl::Bytes bytes);

    PackageTmNode* tmNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    Rc<const Package> _package;
    Rc<ValueInfoCache> _cache;
    Rc<ModelEventHandler> _handler;
    Rc<PackageTmNode> _tmNode;
    Rc<PackageCmdsNode> _cmdsNode;
    std::vector<Rc<Node>> _nodes;
};
}
