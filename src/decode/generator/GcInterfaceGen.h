#pragma once

#include "decode/Config.h"
#include "decode/core/HashMap.h"
#include "decode/core/Rc.h"
#include "decode/generator/SrcBuilder.h"

#include "bmcl/Fwd.h"

#include <string>

namespace decode {

class SrcBuilder;
class Package;
class Ast;
class Type;
class StructType;
class EnumType;
class VariantType;
class NamedType;
class ModuleInfo;
class Component;
class Command;
class TmMsg;
class StatusMsg;
class EventMsg;
class GenericType;

class GcInterfaceGen {
public:
    GcInterfaceGen(SrcBuilder* dest);
    ~GcInterfaceGen();

    void generateHeader(const Package* package);
    void generateValidatorHeader(const Package* package);
    void generateSource(const Package* package);

private:
    bool appendTypeValidator(const Type* type);
    void appendStructValidator(const StructType* type);
    void appendEnumValidator(const EnumType* type);
    void appendVariantValidator(const VariantType* type);
    void appendCmdValidator(const Component* comp, const Command* cmd);
    void appendTmMsgValidator(const Component* comp, const TmMsg* msg, bmcl::StringView typeName);
    void appendStatusValidator(const Component* comp, const StatusMsg* msg);
    void appendEventValidator(const Component* comp, const EventMsg* msg);
    void appendComponentFieldName(const Component* comp);
    void appendCmdFieldName(const Component* comp, const Command* cmd);
    void appendMsgFieldName(const Component* comp, const TmMsg* msg, bmcl::StringView msgTypeName);
    void appendCmdMethods(const Component* comp, const Command* cmd);
    void appendTmMethods(const Component* comp, const TmMsg* msg, bmcl::StringView msgTypeName, bmcl::StringView namespaceName);
    void appendCmdDecls(const Component* comp, const Command* cmd);
    void appendTmDecls(const Component* comp, const TmMsg* msg, bmcl::StringView msgTypeName, bmcl::StringView namespaceName);
    void appendNamedTypeInit(const NamedType* type, bmcl::StringView name);
    void appendTestedType(const Type* type);
    bool appendFwd(const Type* type, bmcl::OptionPtr<const GenericType> parent);
    static void appendTestedType(const Type* type, SrcBuilder* dest);

    void appendComponentCheck(const Component* comp, bmcl::StringView returnValue);
    void appendComponentNumberInlineGetter(const Component* comp);
    void appendWriteComponentNumber(const Component* comp);

    void appendTypeCheckBitDecl(const Component* comp, bmcl::StringView kind, bmcl::StringView name);
    void appendTypeCheckBitInlineGetter(const Component* comp, bmcl::StringView kind, bmcl::StringView name);

    void appendTypeNumDecl(const Component* comp, bmcl::StringView kind, bmcl::StringView name);
    void appendTypeNumDeclInlineGetter(const Component* comp, bmcl::StringView kind, bmcl::StringView name);

    bool insertValidatedType(const Type* type);
    bool insertForwardedType(const Type* type);

    SrcBuilder* _output;
    SrcBuilder _nameBuilder;
    HashMap<std::string, Rc<const Type>> _validatedTypes;
};
}
