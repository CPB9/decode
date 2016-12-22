#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"

#include <bmcl/StringView.h>

#include <functional>
#include <unordered_set>

namespace decode {

class Ast;
class Diagnostics;
class EnumType;
class FnPointer;
class FunctionType;
class ReferenceType;
class BuiltinType;
class ArrayType;
class StructType;
class Type;
class Field;
class Package;
class FieldList;
class VariantType;
struct InlineSerContext;

class Target : public RefCountable {
public:
    Target(std::size_t pointerSize)
        : _pointerSize(pointerSize)
    {
    }

    std::size_t pointerSize() const
    {
        return _pointerSize;
    }

private:
    std::size_t _pointerSize;
};

typedef std::function<void()> Gen;

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromPackage(const Rc<Package>& ast);
private:
    bool generateFromAst(const Rc<Ast>& ast);

    void genHeader(const Type* type);
    void genSource(const Type* type);

    void startIncludeGuard(const Type* type);
    void collectIncludesAndFwdsForType(const Type* topLevelType, std::unordered_set<std::string>* dest);
    void writeIncludes(const std::unordered_set<std::string>& src);
    void writeIncludesAndFwdsForType(const Type* type);
    void writeImplBlockIncludes(const Type* type);

    void writeStruct(const FieldList* fields, bmcl::StringView name);
    void writeStruct(const StructType* type);
    void writeStruct(const std::vector<Rc<Type>>& fields, bmcl::StringView name);
    void writeEnum(const EnumType* type);
    void writeVariant(const VariantType* type);

    void writeImplFunctionPrototypes(const Type* type);
    void writeImplFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName);

    bool needsSerializers(const Type* type);
    void writeSerializerFuncPrototypes(const Type* type);

    void writeLocalIncludePath(bmcl::StringView path);
    void writeCommonIncludePaths(const Type* type);

    void writeEnumDeserizalizer(const EnumType* type);
    void writeEnumSerializer(const EnumType* type);

    void writeStructDeserizalizer(const StructType* type);
    void writeStructSerializer(const StructType* type);

    void writeVariantDeserizalizer(const VariantType* type);
    void writeVariantSerializer(const VariantType* type);

    void writeInlineArrayTypeDeserializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlineBuiltinTypeDeserializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlinePointerDeserializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlineTypeDeserializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeReadableSizeCheck(const InlineSerContext& ctx, std::size_t size);

    void writeInlineArrayTypeSerializer(const ArrayType* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlineBuiltinTypeSerializer(const BuiltinType* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlinePointerSerializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeInlineTypeSerializer(const Type* type, const InlineSerContext& ctx, const Gen& argNameGen);
    void writeWritableSizeCheck(const InlineSerContext& ctx, std::size_t size);

    void writeLoopHeader(const InlineSerContext& ctx, std::size_t loopSize);

    void writeWithTryMacro(const Gen& func);

    void writeSerializerFuncDecl(const Type* type);
    void writeDeserializerFuncDecl(const Type* type);

    void writeTagHeader(bmcl::StringView name);
    void writeTagFooter(bmcl::StringView typeName);
    void writeVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix = "");

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = "");

    void endIncludeGuard(const Type* type);

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path);

    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    Rc<Ast> _currentAst;
    Rc<Target> _target;
};

}
