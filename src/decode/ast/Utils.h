//TODO: move to photon

#pragma once

#include "decode/Config.h"
#include "decode/ast/Ast.h"
#include "decode/parser/Project.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"

#include <bmcl/StringView.h>
#include <bmcl/Result.h>

namespace decode {

static const decode::Ast* findModule(const decode::Device* dev, bmcl::StringView name)
{
    if (!dev) {
        return nullptr;
    }
    auto it = std::find_if(dev->modules().begin(), dev->modules().end(), [name](const decode::Ast* module) {
        return module->moduleName() == name;
    });
    if (it == dev->modules().end()) {
        return nullptr;
    }
    return *it;
}

static const decode::Component* getComponent(const decode::Ast* ast)
{
    if (!ast) {
        return nullptr;
    }
    if (ast->component().isNone()) {
        return nullptr;
    }
    return ast->component().unwrap();
}

template <typename T>
const T* findType(const decode::Ast* module, bmcl::StringView typeName)
{
    if (!module) {
        return nullptr;
    }
    bmcl::OptionPtr<const decode::NamedType> type = module->findTypeWithName(typeName);
    if (type.isNone()) {
        return nullptr;
    }
    if (type.unwrap()->typeKind() != decode::deferTypeKind<T>()) {
        return nullptr;
    }
    return static_cast<const T*>(type.unwrap());
}

template <typename T>
static void expectFieldNum(Rc<const T>* container, std::size_t num)
{
    const T* t = (*container).get();
    if (!t) {
        return;
    }
    if (t->fieldsRange().size() != num) {
        *container = nullptr;
        return;
    }
}

template <typename T>
static void expectField(Rc<const T>* container, std::size_t i, bmcl::StringView fieldName, const decode::Type* other)
{
    const T* t = (*container).get();
    if (!t) {
        return;
    }
    if (!other) {
        *container = nullptr;
        return;
    }
    if (i >= t->fieldsRange().size()) {
        *container = nullptr;
        return;
    }
    const decode::Field* field = t->fieldAt(i);
    if (field->name() != fieldName) {
        *container = nullptr;
        return;
    }
    const decode::Type* type = field->type();
    if (!type->equals(other)) {
        *container = nullptr;
        return;
    }
}

static const decode::Command* findCmd(const decode::Component* comp, bmcl::StringView name, std::size_t argNum)
{
    if (!comp) {
        return nullptr;
    }
    auto it = std::find_if(comp->cmdsBegin(), comp->cmdsEnd(), [name](const decode::Command* func) {
        return func->name() == name;
    });
    if (it == comp->cmdsEnd()) {
        return nullptr;
        //return "failed to find command " + wrapWithQuotes(name);
    }
    if (it->fieldsRange().size() != argNum) {
        return nullptr;
    }
    return *it;
}

static void expectCmdArg(Rc<const Command>* cmd, std::size_t i, const Type* type)
{
    if (!cmd->get()) {
        *cmd = nullptr;
        return;
    }
    if (!type) {
        *cmd = nullptr;
        return;
    }
    if (cmd->get()->fieldsRange().size() <= i) {
        *cmd = nullptr;
        return;
    }
    if (!cmd->get()->fieldAt(i)->type()->equals(type)) {
        *cmd = nullptr;
        return;
    }
}

static void expectCmdRv(Rc<const Command>* cmd, const Type* type)
{
    if (!cmd->get()) {
        *cmd = nullptr;
        return;
    }
    if (!type) {
        *cmd = nullptr;
        return;
    }
    if (cmd->get()->type()->returnValue().isNone()) {
        *cmd = nullptr;
        return;
    }
    if (!cmd->get()->type()->returnValue()->equals(type)) {
        *cmd = nullptr;
        return;
    }
}

static void expectCmdNoRv(Rc<const Command>* cmd)
{
    if (!cmd->get()) {
        *cmd = nullptr;
        return;
    }
    if (cmd->get()->type()->returnValue().isSome()) {
        *cmd = nullptr;
        return;
    }
}

static Rc<GenericInstantiationType> instantiateGeneric(Rc<const GenericType>* type, bmcl::ArrayView<Rc<Type>> params)
{

    if (!type->get()) {
        *type = nullptr;
        return nullptr;
    }
    auto rv = ((GenericType*)type->get())->instantiate(params);
    if (rv.isErr()) {
        *type = nullptr;
        return nullptr;
    }
    return new GenericInstantiationType((*type)->name(), params, rv.unwrap().get());
}
}
