#pragma once

#include "decode/Config.h"
#include "decode/core/HashMap.h"
#include "decode/core/Rc.h"

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

class GcInterfaceGen {
public:
    GcInterfaceGen(SrcBuilder* dest);
    ~GcInterfaceGen();

    void generateHeader(const Package* package);
    void generateSource(const Package* package);

private:
    bool appendTypeValidator(const Type* type);
    void appendStructValidator(const StructType* type);
    void appendEnumValidator(const EnumType* type);
    void appendVariantValidator(const VariantType* type);
    void appendCmdValidator(const Component* comp, const Command* cmd);
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
    static void appendTestedType(const Type* type, SrcBuilder* dest);

    bool insertValidatedType(const Type* type);

    SrcBuilder* _output;
    HashMap<std::string, Rc<const Type>> _validatedTypes;
};
}
