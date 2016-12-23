#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/InlineTypeDeserializerGen.h"
#include "decode/generator/InlineTypeSerializerGen.h"
#include "decode/core/Target.h"

#include <bmcl/StringView.h>

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

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromPackage(const Rc<Package>& ast);
private:
    bool generateFromAst(const Rc<Ast>& ast);

    void genHeader(const Type* type);
    void genSource(const Type* type);

    void writeEnumDeserizalizer(const EnumType* type);
    void writeEnumSerializer(const EnumType* type);

    void writeStructDeserizalizer(const StructType* type);
    void writeStructSerializer(const StructType* type);

    void writeVariantDeserizalizer(const VariantType* type);
    void writeVariantSerializer(const VariantType* type);

    void writeSerializerFuncDecl(const Type* type);
    void writeDeserializerFuncDecl(const Type* type);

    void genTypeRepr(const Type* type, bmcl::StringView fieldName = "");

    bool makeDirectory(const char* path);

    bool saveOutput(const char* path);

    Rc<Diagnostics> _diag;
    std::string _savePath;
    SrcBuilder _output;
    Rc<Ast> _currentAst;
    Rc<Target> _target;
    InlineTypeSerializerGen _inlineSer;
    InlineTypeDeserializerGen _inlineDeser;
};

}
