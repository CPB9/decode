#include "decode/model/Model.h"
#include "decode/core/Try.h"
#include "decode/parser/Package.h"
#include "decode/model/Decoder.h"
#include "decode/model/FieldsNode.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/parser/Decl.h" //HACK
#include "decode/parser/Type.h" //HACK
#include "decode/parser/Component.h" //HACK

#include <bmcl/ArrayView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>

namespace decode {

Model::Model(const Package* package, ModelEventHandler* handler)
    : Node(nullptr)
    , _package(package)
    , _cache(new ValueInfoCache)
    , _handler(handler)
{
    for (const Component* it : package->components()) {
        if (!it->hasParams()) {
            continue;
        }

        Rc<FieldsNode> node = new FieldsNode(it, _cache.get(), this);
        node->setName(it->name());
        _nodes.emplace_back(node);

        if (!it->hasStatuses()) {
            continue;
        }

        Rc<StatusDecoder> decoder = new StatusDecoder(it->statusesRange(), node.get());
        _decoders.emplace(it->number(), decoder);
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

bmcl::StringView Model::fieldName() const
{
    return "photon";
}

void Model::acceptTelemetry(bmcl::Bytes bytes)
{
    bmcl::MemReader stream(bytes.data(), bytes.size());

    while (!stream.isEmpty()) {
        if (stream.sizeLeft() < 2) {
            //TODO: report error
            return;
        }

        uint16_t msgSize = stream.readUint16Le();
        if (stream.sizeLeft() < msgSize) {
            //TODO: report error
            return;
        }

        bmcl::MemReader msg(stream.current(), msgSize);
        uint64_t compId;
        if (!msg.readVarUint(&compId)) {
            //TODO: report error
            return;
        }

        auto it = _decoders.find(compId);
        if (it == _decoders.end()) {
            //TODO: report error
            return;
        }

        if (!it->second->decode(_handler.get(), &msg)) {
            //TODO: report error
            return;
        }

        stream.skip(msgSize);
    }
}
}
