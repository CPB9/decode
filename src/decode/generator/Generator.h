#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

namespace decode {

class Ast;
class Diagnostics;
class Type;
class StructDecl;
class FieldList;
class Enum;
class Variant;

class Generator : public RefCountable {
public:
    Generator(const Rc<Diagnostics>& diag);

    void setOutPath(bmcl::StringView path);

    bool generateFromAst(const Rc<Ast>& ast);
private:
    void startIncludeGuard(const Type* type);
    void writeIncludesAndFwdsForType(const Type* type);

    void writeStruct(const FieldList* fields, bmcl::StringView name);
    void writeStruct(const StructDecl* type);
    void writeEnum(const Enum* type);
    void writeVariant(const Variant* type);

    void writeTagHeader(bmcl::StringView name);
    void writeTagFooter(bmcl::StringView typeName);

    void write(bmcl::StringView value);
    void writeWithFirstUpper(bmcl::StringView value);
    void writeModPrefix();

    void genTypeRepr(const Type* type, bmcl::StringView fieldName);

    void endIncludeGuard(const Type* type);

    void writeEol();

    Rc<Diagnostics> _diag;
    std::string _outPath;
    std::string _output;
    Rc<Ast> _ast;
};

}
