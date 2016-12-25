#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SerializationFuncPrototypeGen.h"

#include <unordered_set>
#include <string>

namespace decode {

class HeaderGen : public ConstAstVisitor<HeaderGen>, public SerializationFuncPrototypeGen<HeaderGen> {
public:
    HeaderGen(SrcBuilder* output);
    ~HeaderGen();

    void genHeader(const Ast* ast, const Type* type);

    SrcBuilder& output();
    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    bool visitBuiltinType(const BuiltinType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitSliceType(const SliceType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);

private:
    void appendSerializerFuncPrototypes(const Type* type);

    template <typename T, typename F>
    void genHeaderWithTypeDecl(const T* type, F&& declGen);

    void startIncludeGuard(const Type* type);
    void endIncludeGuard(const Type* type);

    void appendIncludes(const std::unordered_set<std::string>& src);
    void appendImplBlockIncludes(const Type* topLevelType);
    void appendIncludesAndFwdsForType(const Type* topLevelType);
    void appendCommonIncludePaths();

    void appendImplFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName);
    void appendImplFunctionPrototypes(const Type* type);

    void appendFieldVec(const std::vector<Rc<Type>>& fields, bmcl::StringView name);
    void appendFieldList(const FieldList* fields, bmcl::StringView name);
    void appendStruct(const StructType* type);
    void appendEnum(const EnumType* type);
    void appendVariant(const VariantType* type);

    const Ast* _ast;
    SrcBuilder* _output;
    IncludeCollector _includeCollector;
    TypeReprGen _typeReprGen;
};

inline SrcBuilder& HeaderGen::output()
{
    return *_output;
}

inline void HeaderGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen.genTypeRepr(type, fieldName);
}

inline bool HeaderGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool HeaderGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool HeaderGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool HeaderGen::visitSliceType(const SliceType* type)
{
    (void)type;
    return false;
}

inline bool HeaderGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}


inline bool HeaderGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}

}
