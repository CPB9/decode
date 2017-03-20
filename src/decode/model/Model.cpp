#include "decode/model/Model.h"
#include "decode/core/Try.h"
#include "decode/parser/Package.h"
#include "decode/model/Decoder.h"
#include "decode/model/FieldsNode.h"
#include "decode/parser/Decl.h" //HACK
#include "decode/parser/Type.h" //HACK
#include "decode/parser/Component.h" //HACK

#include <bmcl/ArrayView.h>
#include <bmcl/MemReader.h>

namespace decode {

Model::Model(const Package* package)
    : Node(nullptr)
    , _package(package)
{
    for (auto it : package->components()) {
        if (it.second->parameters().isNone()) {
            continue;
        }

        Rc<FieldsNode> node = new FieldsNode(it.second->parameters()->get(), this);
        node->setName(it.second->name());
        _nodes.emplace_back(node);

        if (it.second->statuses().isNone()) {
            continue;
        }

        Rc<StatusDecoder> decoder = new StatusDecoder(it.second->statuses()->get(), node.get());
        _decoders.emplace(it.first, decoder);
    }
}

Model::~Model()
{
}

std::size_t Model::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> Model::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

Node* Model::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

void Model::acceptTelemetry(bmcl::ArrayView<uint8_t> bytes)
{
    bmcl::MemReader reader(bytes.data(), bytes.size());

    while (!reader.isEmpty()) {
        uint64_t compId;
        if (!reader.readVarUint(&compId)) {
            //TODO: report error
            return;
        }
        auto it = _decoders.find(compId);
        if (it == _decoders.end()) {
            //TODO: report error
            return;
        }
        if (!it->second->decode(&reader)) {
            //TODO: report error
            return;
        }
    }
}
}
