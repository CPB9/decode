#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
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
    CmdDecoderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output);
    ~CmdDecoderGen();

    void generateHeader(const std::map<std::size_t, Rc<Component>>& comps);
    void generateSource(const std::map<std::size_t, Rc<Component>>& comps);

    SrcBuilder& output();
    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

private:

    void appendFunctionPrototype(unsigned componenNum, unsigned cmdNum);
    void appendMainFunctionPrototype();
    void appendFunctionName(unsigned componenNum, unsigned cmdNum);

    template <typename C>
    void foreachParam(const Rc<FunctionType>& func, C&& callable);

    void generateFunc(const Rc<Component>& comp, const Rc<FunctionType>& func, unsigned componenNum, unsigned cmdNum);
    void generateMainFunc(const std::map<std::size_t, Rc<Component>>& comps);

    void writePointerOp(const Rc<Type>& type);

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
