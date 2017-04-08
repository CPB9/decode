#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/Containers.h"
#include "decode/generator/SerializationFuncPrototypeGen.h"
#include "decode/generator/InlineTypeDeserializerGen.h"
#include "decode/generator/InlineTypeSerializerGen.h"

#include <vector>
#include <functional>

namespace decode {

struct ComponentAndMsg;
class TypeReprGen;
class SrcBuilder;
class Type;

class CmdDecoderGen : public FuncPrototypeGen<CmdDecoderGen> {
public:
    CmdDecoderGen(TypeReprGen* reprGen, SrcBuilder* output);
    ~CmdDecoderGen();

    void generateHeader(ComponentMap::ConstRange comps); //TODO: make generic
    void generateSource(ComponentMap::ConstRange comps);

    SrcBuilder& output();
    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

private:

    void appendFunctionPrototype(unsigned componenNum, unsigned cmdNum);
    void appendMainFunctionPrototype();
    void appendFunctionName(unsigned componenNum, unsigned cmdNum);

    template <typename C>
    void foreachParam(const Function* func, C&& callable);

    void generateFunc(const Component* comp, const Function* func, unsigned componenNum, unsigned cmdNum);
    void generateMainFunc(ComponentMap::ConstRange comps);

    void writePointerOp(const Type* type);

    Rc<TypeReprGen> _typeReprGen;
    SrcBuilder* _output;
    InlineTypeSerializerGen _inlineSer;
    InlineTypeDeserializerGen _inlineDeser;
    StringBuilder _paramName;
};

inline SrcBuilder& CmdDecoderGen::output()
{
    return *_output;
}
}
