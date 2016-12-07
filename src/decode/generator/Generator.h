#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/StringBuilder.h"

#include <bmcl/StringView.h>

namespace decode {

class Ast;
class Diagnostics;
class Enum;
class FieldList;
class FnPointer;
class Function;
class StructDecl;
class Type;
class Variant;

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromAst(const Rc<Ast>& ast);
private:

    void genHeader(const Type* type);
    void genSource(const Type* type);

    void startIncludeGuard(const Type* type);
    void writeIncludesAndFwdsForType(const Type* type);

    void writeStruct(const FieldList* fields, bmcl::StringView name);
    void writeStruct(const StructDecl* type);
    void writeStruct(const std::vector<Rc<Type>>& fields, bmcl::StringView name);
    void writeEnum(const Enum* type);
    void writeVariant(const Variant* type);

    void writeImplFunctionPrototypes(const Type* type);
    void writeImplFunctionPrototype(const Rc<Function>& func, bmcl::StringView typeName);

    bool needsSerializers(const Type* type);
    void writeSerializerFuncPrototypes(const Type* type);

    void writeLocalIncludePath(bmcl::StringView path);

    void writeEnumDeserizalizer(const Enum* type);
    void writeEnumSerializer(const Enum* type);

    template <typename F, typename... A>
    void writeWithTryMacro(F&& func, A&&... args);

    void writeSerializerFuncDecl(const Type* type);
    void writeDeserializerFuncDecl(const Type* type);

    void writeTagHeader(bmcl::StringView name);
    void writeTagFooter(bmcl::StringView typeName);
    void writeVarDecl(bmcl::StringView typeName, bmcl::StringView varName, bmcl::StringView prefix = "");

    void writeModPrefix();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = "");
    void genFnPointerTypeRepr(const FnPointer* type, bmcl::StringView fieldName = "");

    void endIncludeGuard(const Type* type);

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path);

    Rc<Diagnostics> _diag;
    std::string _outPath;
    StringBuilder _output;
    Rc<Ast> _ast;
    bool _shouldWriteModPrefix;
};

}
