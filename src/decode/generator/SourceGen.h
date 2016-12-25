#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/SerializationFuncPrototypeGen.h"
#include "decode/generator/InlineTypeDeserializerGen.h"
#include "decode/generator/InlineTypeSerializerGen.h"

#include <unordered_set>
#include <string>

namespace decode {

class SourceGen : public ConstAstVisitor<SourceGen>, public SerializationFuncPrototypeGen<SourceGen> {
public:
    SourceGen(SrcBuilder* output);
    ~SourceGen();

    void genSource(const Type* type);

    bool visitBuiltinType(const BuiltinType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitArrayType(const ArrayType* type);
    bool visitSliceType(const SliceType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = bmcl::StringView::empty());

    SrcBuilder& output();

private:
    template <typename T, typename F>
    void genSource(const T* type, F&& serGen, F&& deserGen);

    void appendEnumSerializer(const EnumType* type);
    void appendEnumDeserializer(const EnumType* type);
    void appendStructSerializer(const StructType* type);
    void appendStructDeserializer(const StructType* type);
    void appendVariantSerializer(const VariantType* type);
    void appendVariantDeserializer(const VariantType* type);

    void appendIncludes(const Type* type);

    SrcBuilder* _output;
    TypeReprGen _typeReprGen;
    InlineTypeSerializerGen _inlineSer;
    InlineTypeDeserializerGen _inlineDeser;
};

inline SrcBuilder& SourceGen::output()
{
    return *_output;
}

inline void SourceGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen.genTypeRepr(type, fieldName);
}

inline bool SourceGen::visitBuiltinType(const BuiltinType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitReferenceType(const ReferenceType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitArrayType(const ArrayType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitSliceType(const SliceType* type)
{
    (void)type;
    return false;
}

inline bool SourceGen::visitFunctionType(const FunctionType* type)
{
    (void)type;
    return false;
}


inline bool SourceGen::visitImportedType(const ImportedType* type)
{
    (void)type;
    return false;
}
}
