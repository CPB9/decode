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
class DecoderAction;

class StatusMsgDecoder {
public:
    StatusMsgDecoder(StatusMsg* msg, FieldsNode* node);
    ~StatusMsgDecoder();

    bool decode(bmcl::MemReader* src);

private:
    std::vector<std::pair<Rc<DecoderAction>, Rc<ValueNode>>> _chain;
};

class StatusDecoder : public RefCountable {
public:
    StatusDecoder(const Statuses* statuses, FieldsNode* node);
    ~StatusDecoder();

    bool decode(bmcl::MemReader* src);
private:

    std::unordered_map<uint64_t, StatusMsgDecoder> _decoders;
};
}
