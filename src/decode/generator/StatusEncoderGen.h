#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SerializationFuncPrototypeGen.h"
#include "decode/generator/InlineTypeSerializerGen.h"

#include <vector>

namespace decode {

struct ComponentAndMsg;
class TypeReprGen;
class SrcBuilder;
class Type;

class StatusEncoderGen : public FuncPrototypeGen<StatusEncoderGen> {
public:
    StatusEncoderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output);
    ~StatusEncoderGen();

    void generateHeader(const std::vector<ComponentAndMsg>& messages);
    void generateSource(const std::vector<ComponentAndMsg>& messages);

    SrcBuilder& output();
    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

private:

    void appendInlineSerializer(const Component* comp, const StatusRegexp* part);

    Rc<TypeReprGen> _typeReprGen;
    SrcBuilder* _output;
    InlineTypeSerializerGen _inlineSer;
};

inline SrcBuilder& StatusEncoderGen::output()
{
    return *_output;
}


}
