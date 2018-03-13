#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

namespace decode {

class Component;
class StatusMsg;
class EventMsg;
class TmMsg;
class SrcBuilder;

class GcMsgGen {
public:
    GcMsgGen(SrcBuilder* dest);
    ~GcMsgGen();

    void generateStatusHeader(const Component* comp, const StatusMsg* msg);
    void generateEventHeader(const Component* comp, const EventMsg* msg);

    static void genTmMsgType(const Component* comp, const TmMsg* msg, bmcl::StringView namespaceName, SrcBuilder* dest);

private:
    template <typename T>
    void appendPrelude(const Component* comp, const T* msg, bmcl::StringView namespaceName);

    SrcBuilder* _output;
};
}
