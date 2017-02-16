#pragma once

#include "decode/Config.h"
#include "decode/generator/TypeDefGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SerializationFuncPrototypeGen.h"

#include <unordered_set>
#include <string>

namespace decode {

class Component;
class FunctionType;

class HeaderGen : public FuncPrototypeGen<HeaderGen> {
public:
    HeaderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output);
    ~HeaderGen();

    void genTypeHeader(const Ast* ast, const Type* type);
    void genSliceHeader(const SliceType* type);
    void genComponentHeader(const Ast* ast, const Component* type);

    SrcBuilder& output();
    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    void startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName);
    void endIncludeGuard();

private:
    void appendSerializerFuncPrototypes(const Type* type);
    void appendSerializerFuncPrototypes(const Component* comp);

    void startIncludeGuard(const NamedType* type);
    void startIncludeGuard(const Component* comp);
    void startIncludeGuard(const SliceType* type);

    void appendIncludes(const std::unordered_set<std::string>& src);
    void appendImplBlockIncludes(const NamedType* topLevelType);
    void appendImplBlockIncludes(const Component* comp);
    void appendImplBlockIncludes(const SliceType* slice);
    void appendIncludesAndFwds(const Type* topLevelType);
    void appendIncludesAndFwds(const Component* comp);
    void appendCommonIncludePaths();

    void appendFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName);
    void appendFunctionPrototypes(const NamedType* type);
    void appendFunctionPrototypes(const Component* comp);
    void appendCommandPrototypes(const Component* comp);
    void appendFunctionPrototypes(const std::vector<Rc<FunctionType>>& funcs, bmcl::StringView typeName);

    const Ast* _ast;
    SrcBuilder* _output;
    IncludeCollector _includeCollector;
    TypeDefGen _typeDefGen;
    SrcBuilder _sliceName;
    Rc<TypeReprGen> _typeReprGen;
};

inline SrcBuilder& HeaderGen::output()
{
    return *_output;
}

inline void HeaderGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen->genTypeRepr(type, fieldName);
}
}
