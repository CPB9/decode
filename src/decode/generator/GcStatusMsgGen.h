#pragma once

namespace decode {

class Component;
class StatusMsg;
class SrcBuilder;

class GcStatusMsgGen {
public:
    GcStatusMsgGen(SrcBuilder* dest);
    ~GcStatusMsgGen();

    void generateHeader(const Component* comp, const StatusMsg* msg);

    static void genMsgType(const Component* comp, const StatusMsg* msg, SrcBuilder* dest);

private:
    SrcBuilder* _output;
};
}
