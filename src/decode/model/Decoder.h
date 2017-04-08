#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <unordered_map>
#include <vector>

namespace bmcl { class MemReader; }

namespace decode {

class FieldsNode;
class Statuses;
class StatusMsg;
class ValueNode;
class ModelEventHandler;
class DecoderAction;

class StatusMsgDecoder {
public:
    struct ChainElement {
        ChainElement(std::size_t index, DecoderAction* action, ValueNode* node);

        std::size_t nodeIndex;
        Rc<DecoderAction> action;
        Rc<ValueNode> node;
    };

    StatusMsgDecoder(const StatusMsg* msg, FieldsNode* node);
    ~StatusMsgDecoder();

    bool decode(ModelEventHandler* handler, bmcl::MemReader* src);

private:
    std::vector<ChainElement> _chain;
};

class StatusDecoder : public RefCountable {
public:
    template <typename R>
    StatusDecoder(R statusRange, FieldsNode* node)
    {
        for (auto it : statusRange) {
            _decoders.emplace(std::piecewise_construct,
                              std::forward_as_tuple(it->number()),
                              std::forward_as_tuple(it, node));
        }
    }

    ~StatusDecoder();

    bool decode(ModelEventHandler* handler, bmcl::MemReader* src);

private:
    std::unordered_map<uint64_t, StatusMsgDecoder> _decoders;
};
}
