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

class Component;

class TypeDefGen : public ConstAstVisitor<TypeDefGen> {
public:
    TypeDefGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output);
    ~TypeDefGen();

    void genTypeDef(const Type* type);
    void genComponentDef(const Component* comp);

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
    void appendFieldVec(const std::vector<Rc<Type>>& fields, bmcl::StringView name);
    void appendFieldList(const FieldList* fields, bmcl::StringView name);
    void appendStruct(const StructType* type);
    void appendEnum(const EnumType* type);
    void appendVariant(const VariantType* type);

    Rc<TypeReprGen> _typeReprGen;
    SrcBuilder* _output;
};

inline bool TypeDefGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool TypeDefGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}


inline bool TypeDefGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}
}
