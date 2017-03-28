#include "decode/model/Decoder.h"
#include "decode/core/Try.h"
#include "decode/parser/Component.h"
#include "decode/parser/Type.h"
#include "decode/model/FieldsNode.h"
#include "decode/model/ValueNode.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/parser/Decl.h" //HACK

#include <bmcl/MemReader.h>

namespace decode {

StatusMsgDecoder::ChainElement::ChainElement(std::size_t index, DecoderAction* action, ValueNode* node)
    : nodeIndex(index)
    , action(action)
    , node(node)
{
}


class DecoderAction : public RefCountable {
public:
    virtual bool execute(std::size_t nodeIndex, ModelEventHandler* handler, ValueNode* node, bmcl::MemReader* src) = 0;

    void setNext(DecoderAction* next)
    {
        _next.reset(next);
    }

protected:
    Rc<DecoderAction> _next;
};

class DecodeNodeAction : public DecoderAction {
public:
    bool execute(std::size_t nodeIndex, ModelEventHandler* handler, ValueNode* node, bmcl::MemReader* src) override
    {
        return node->decode(nodeIndex, handler, src);
    }
};

class DescentAction : public DecoderAction {
public:
    DescentAction(std::size_t fieldIndex)
        : _fieldIndex(fieldIndex)
    {
    }

    bool execute(std::size_t nodeIndex, ModelEventHandler* handler, ValueNode* node, bmcl::MemReader* src) override
    {
        (void)src;
        assert(node->type()->isStruct());
        ContainerValueNode* cnode = static_cast<ContainerValueNode*>(node);
        ValueNode* child = cnode->nodeAt(_fieldIndex);
        return _next->execute(_fieldIndex, handler, child, src);
    }

private:
    std::size_t _fieldIndex;
};

class DecodeSlicePartsAction : public DecoderAction {
public:
    bool execute(std::size_t nodeIndex, ModelEventHandler* handler, ValueNode* node, bmcl::MemReader* src) override
    {
        uint64_t sliceSize;
        if (!src->readVarUint(&sliceSize)) {
            //TODO: report error
            return false;
        }
        //FIXME: implement range check
        SliceValueNode* cnode = static_cast<SliceValueNode*>(node);
        //TODO: add size check
        cnode->resize(sliceSize, nodeIndex, handler);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < sliceSize; i++) {
            TRY(_next->execute(i, handler, values[i].get(), src));
        }
        return true;
    }
};

class DecodeArrayPartsAction : public DecoderAction {
public:
    bool execute(std::size_t nodeIndex, ModelEventHandler* handler, ValueNode* node, bmcl::MemReader* src) override
    {
        //FIXME: implement range check
        ArrayValueNode* cnode = static_cast<ArrayValueNode*>(node);
        const std::vector<Rc<ValueNode>>& values = cnode->values();
        for (std::size_t i = 0; i < values.size(); i++) {
            TRY(_next->execute(i, handler, values[i].get(), src));
        }
        return true;
    }
};

static Rc<DecoderAction> createMsgDecoder(const StatusRegexp* part, const Type* lastType)
{
    assert(!part->accessors().empty());
    if (part->accessors().size() == 1) {
        return new DecodeNodeAction;
    }

    auto it = part->accessors().begin() + 1;
    auto end = part->accessors().end();

    Rc<DecoderAction> firstAction;
    Rc<DecoderAction> previousAction;

    auto updateAction = [&](DecoderAction* newAction) {
        if (!firstAction) {
            firstAction = newAction;
        }
        if (previousAction) {
            previousAction->setNext(newAction);
        }
        previousAction = newAction;
    };

    while (it != end) {
        const Rc<Accessor>& acc = *it;
        if (acc->accessorKind() == AccessorKind::Field) {
            auto facc = acc->asFieldAccessor();
            assert(lastType->isStruct());
            bmcl::Option<std::size_t> index = lastType->asStruct()->indexOfField(facc->field().get());
            assert(index.isSome());
            lastType = facc->field()->type();
            DescentAction* newAction = new DescentAction(index.unwrap());
            updateAction(newAction);
        } else if (acc->accessorKind() == AccessorKind::Subscript) {
            auto sacc = acc->asSubscriptAccessor();
            const Type* type = sacc->type().get();
            if (type->isArray()) {
                updateAction(new DecodeArrayPartsAction);
                lastType = type->asArray()->elementType();
            } else if (type->isSlice()) {
                updateAction(new DecodeSlicePartsAction);
                lastType = type->asSlice()->elementType();
            } else {
                assert(false);
            }
        } else {
            assert(false);
        }
        it++;
    }

    previousAction->setNext(new DecodeNodeAction);
    return firstAction;
}

StatusMsgDecoder::StatusMsgDecoder(StatusMsg* msg, FieldsNode* node)
{
    for (const Rc<StatusRegexp>& part : msg->parts()) {
        if (part->accessors().empty()) {
            continue;
        }
        assert(part->accessors()[0]->accessorKind() == AccessorKind::Field);
        auto facc = part->accessors()[0]->asFieldAccessor();
        auto op = node->nodeWithName(facc->field()->name());
        assert(op.isSome());
        assert(node->hasParent());
        auto i = node->childIndex(op.unwrap());
        std::size_t index = i.unwrap();
        Rc<DecoderAction> decoder = createMsgDecoder(part.get(), facc->field()->type());
        _chain.emplace_back(index, decoder.get(), op.unwrap());
    }
}

StatusMsgDecoder::~StatusMsgDecoder()
{
}

bool StatusMsgDecoder::decode(ModelEventHandler* handler, bmcl::MemReader* src)
{
    for (const ChainElement& elem : _chain) {
        if (!elem.action->execute(elem.nodeIndex, handler, elem.node.get(), src)) {
            return false;
        }
    }
    return true;
}

StatusDecoder::StatusDecoder(const Statuses* statuses, FieldsNode* node)
{
    for (auto it : statuses->statusMap()) {
        _decoders.emplace(std::piecewise_construct,
                          std::forward_as_tuple(it.first),
                          std::forward_as_tuple(it.second.get(), node));
    }
}

StatusDecoder::~StatusDecoder()
{
}

bool StatusDecoder::decode(ModelEventHandler* handler, bmcl::MemReader* src)
{
    uint64_t msgId;
    if (!src->readVarUint(&msgId)) {
        //TODO: report error
        return false;
    }
    auto it = _decoders.find(msgId);
    if (it == _decoders.end()) {
        //TODO: report error
        return false;
    }
    if (!it->second.decode(handler, src)) {
        //TODO: report error
        return false;
    }
    return true;
}
}
