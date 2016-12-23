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

class TypeHeaderGen : public ConstAstVisitor<TypeHeaderGen>, public SerializationFuncPrototypeGen<TypeHeaderGen> {
public:
    TypeHeaderGen(SrcBuilder* output);
    ~TypeHeaderGen();

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


template <typename T, typename F>
void TypeHeaderGen::genHeaderWithTypeDecl(const T* type, F&& declGen)
{
    startIncludeGuard(type);
    appendIncludesAndFwdsForType(type);
    appendCommonIncludePaths();
    (this->*declGen)(type);
    appendImplBlockIncludes(type);
    appendImplFunctionPrototypes(type);
    appendSerializerFuncPrototypes(type);
    endIncludeGuard(type);
}

inline SrcBuilder& TypeHeaderGen::output()
{
    return *_output;
}

inline void TypeHeaderGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen.genTypeRepr(type, fieldName);
}

inline bool TypeHeaderGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool TypeHeaderGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool TypeHeaderGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool TypeHeaderGen::visitSliceType(const SliceType* type)
{
    (void)type;
    return false;
}

inline bool TypeHeaderGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}


inline bool TypeHeaderGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}

}
