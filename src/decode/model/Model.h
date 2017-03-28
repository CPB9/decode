#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/Node.h"

#include <cstdint>
#include <unordered_map>

namespace bmcl {
template <typename T>
class ArrayView;
}

namespace decode {

class Package;
class StatusDecoder;
class Component;
class FieldsNode;
class ValueInfoCache;
class ModelEventHandler;

class Model : public Node {
public:
    Model(const Package* package, ModelEventHandler* handler);
    ~Model();

    void acceptTelemetry(bmcl::ArrayView<uint8_t> bytes);

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    Node* childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    Rc<const Package> _package;
    std::unordered_map<uint64_t, Rc<StatusDecoder>> _decoders;
    std::vector<Rc<FieldsNode>> _nodes;
    Rc<ValueInfoCache> _cache;
    Rc<ModelEventHandler> _handler;
};
}
