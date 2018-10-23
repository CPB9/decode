/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/parser/Package.h"

#include "decode/core/Configuration.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"
#include "decode/core/Utils.h"
#include "decode/core/FileInfo.h"
#include "decode/core/ProgressPrinter.h"
#include "decode/ast/Ast.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Component.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/parser/Parser.h"

#include <bmcl/Buffer.h>
#include <bmcl/Logging.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>

#include <cstring>
#include <limits>
#include <string>

#if defined(__linux__)
# include <dirent.h>
# include <sys/stat.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#endif

namespace decode {

Package::Package(Configuration* cfg, Diagnostics* diag)
    : _diag(diag)
    , _cfg(cfg)
{
}

Package::~Package()
{
}

PackageResult Package::readFromFiles(Configuration* cfg, Diagnostics* diag, bmcl::ArrayView<std::string> files)
{
    Rc<Package> package = new Package(cfg, diag);
    Parser p(diag);

    for (const std::string& path : files) {
        if (!package->addFile(path.c_str(), &p)) {
            return PackageResult();
        }
    }

    if (!package->resolveAll()) {
        return PackageResult();
    }

    return std::move(package);
}

static void addDecodeError(Diagnostics* diag, bmcl::StringView msg)
{
    diag->buildSystemErrorReport("could not decode package from memory", msg);
}

PackageResult Package::decodeFromMemory(Configuration* cfg, Diagnostics* diag, const void* src, std::size_t size)
{
    bmcl::MemReader reader(src, size);

    Rc<Package> package = new Package(cfg, diag);
    Parser p(diag);

    while (!reader.isEmpty()) {
        auto fname = deserializeString(&reader);
        if (fname.isErr()) {
            addDecodeError(diag, fname.unwrapErr());
            return PackageResult();
        }

        auto contents = deserializeString(&reader);
        if (contents.isErr()) {
            addDecodeError(diag, contents.unwrapErr());
            return PackageResult();
        }

        Rc<FileInfo> finfo = new FileInfo(fname.unwrap().toStdString(), contents.unwrap().toStdString());

        ParseResult ast = p.parseFile(finfo.get());
        if (ast.isErr()) {
            return PackageResult();
        }

        package->addAst(ast.unwrap().get());
    }

    if (!package->resolveAll()) {
        return PackageResult();
    }

    return std::move(package);
}

void Package::encode(bmcl::Buffer* dest) const
{
    for (const Ast* it : modules()) {
        const FileInfo* finfo = it->moduleInfo()->fileInfo();
        serializeString(finfo->fileName(), dest);
        serializeString(finfo->contents(), dest);
    }
}

void Package::addAst(Ast* ast)
{
    bmcl::StringView modName = ast->moduleName();
    _modNameToAstMap.emplace(modName, ast);
}

bool Package::addFile(const char* path, Parser* p)
{
    ProgressPrinter printer(_cfg->verboseOutput());
    printer.printActionProgress("Parsing", "file `" + std::string(path) + "`");
    ParseResult ast = p->parseFile(path);
    if (ast.isErr()) {
        return false;
    }

    addAst(ast.unwrap().get());

    return true;
}

bool Package::resolveGenerics(Ast* ast)
{
    bool isOk = true;
    for (GenericInstantiationType* type : ast->genericInstantiationsRange()) {
        NamedType* t = type->instantiatedType();
        if (t->isImported()) {
            t = t->asImported()->link();
        }
        if (t->isGeneric()) {
            auto rv = t->asGeneric()->instantiate(type->substitutedTypes());
            if (rv.isErr()) {
                BMCL_CRITICAL() << "failed to instantiate type " + type->genericName().toStdString() + ": " + rv.unwrapErr();
                isOk = false;
                continue;
            }
            type->setModuleInfo(t->moduleInfo());
            type->setInstantiatedType(rv.unwrap().get());
            type->setGenericType(t->asGeneric());
        } else {
            BMCL_CRITICAL() << "Type " + type->genericName().toStdString() + " is not generic";
            isOk = false;
        }
    }
    return isOk;
}

bool Package::resolveImports(Ast* ast)
{
    bool isOk = true;
    for (ImportDecl* import : ast->importsRange()) {
        auto searchedAst = _modNameToAstMap.find(import->path().toStdString());
        if (searchedAst == _modNameToAstMap.end()) {
            isOk = false;
            BMCL_CRITICAL() << "invalid import mod in "
                            << ast->moduleName().toStdString() << ": "
                            << import->path().toStdString();
            continue;
        }
        for (ImportedType* modifiedType : import->typesRange()) {
            bmcl::OptionPtr<NamedType> foundType = searchedAst->second->findTypeWithName(modifiedType->name());
            if (foundType.isNone()) {
                isOk = false;
                //TODO: report error
                BMCL_CRITICAL() << "invalid import type in "
                                << ast->moduleName().toStdString() << ": "
                                << modifiedType->name().toStdString();
            } else {
                if (modifiedType->typeKind() == foundType.unwrap()->typeKind()) {
                    isOk = false;
                    //TODO: report error - circular imports
                    BMCL_CRITICAL() << "circular imports "
                                    << ast->moduleName().toStdString()
                                    << ": " << modifiedType->name().toStdString();
                    BMCL_CRITICAL() << "circular imports "
                                    << searchedAst->first.toStdString() << ".decode: "
                                    << foundType.unwrap()->name().toStdString();
                }
                modifiedType->setLink(foundType.unwrap());
            }
        }
    }
    return isOk;
}

bool Package::resolveParameters(Ast* ast, uint64_t* paramNum)
{
    bmcl::OptionPtr<Component> comp = ast->component();
    if (comp.isNone()) {
        return true;
    }

    if (!comp->hasParams()) {
        return true;
    }

    if (!comp->hasVars() && comp->hasParams()) {
        //TODO: report error
        BMCL_CRITICAL() << "no vars, has params";
        return false;
    }

    for (Parameter* param : comp->paramsRange()) {
        param->setNumber(*paramNum);
        (*paramNum)++;
        if (param->pathPartsRange().empty()) {
            param->addPathPart(new FieldAccessor(param->name(), nullptr));
        }
        bmcl::OptionPtr<Field> lastField;
        Type* lastType = nullptr;
        for (FieldAccessor* acc : param->pathPartsRange()) {
            bmcl::StringView path = acc->value();
            if (lastField.isNone()) {
                lastField = comp->varWithName(path);
            } else {
                Type* type = lastField->type();
                if (!type->isStruct()) {
                    BMCL_CRITICAL() << "intermediate param paths can only reference structs: " << path.toStdString();
                    return false;
                }
                lastField = type->asStruct()->fieldWithName(path);
            }
            if (lastField.isNone()) {
                BMCL_CRITICAL() << "component has no var with name: " << path.toStdString();
                return false;
            }
            lastType = lastField->type();
            acc->setField(lastField.unwrap());
        }
        if (!lastType->resolveFinalType()->isBuiltin()) {
            BMCL_CRITICAL() << "variable can only be of builtin type";
            return false;
        }
    }
    return true;
}

bool Package::resolveStatuses(Ast* ast)
{
    bool isOk = true;
    bmcl::OptionPtr<Component> comp = ast->component();
    if (comp.isNone()) {
        return true;
    }

    if (!comp->hasStatuses()) {
        return true;
    }

    if (!comp->hasVars() && comp->hasStatuses()) {
        //TODO: report error
        BMCL_CRITICAL() << "no vars, has statuses";
        return false;
    }

    for (StatusMsg* it : comp->statusesRange()) {
        _statusMsgs.emplace_back(comp.unwrap(), it);
        for (VarRegexp* re : it->partsRange()) {
            if (!resolveVarRegexp(ast, comp.unwrap(), re)) {
                return false;
            }
        }
    }

    for (VarRegexp* re : comp->savedVarsRange()) {
        if (!resolveVarRegexp(ast, comp.unwrap(), re)) {
            return false;
        }
    }

    return isOk;
}

bool Package::resolveVarRegexp(Ast* ast, Component* comp, VarRegexp* re)
{
    FieldVec::Range fields = comp->varsRange();
    Rc<Type> lastType = nullptr;
    Rc<Field> lastField = nullptr;
    Rc<SubscriptAccessor> lastSubscript = nullptr;

    if (!re->hasAccessors()) {
        return true;
    }
    auto resolveField = [&](FieldAccessor* facc) -> bool {
        auto field = fields.findIf([facc](const Field* f) -> bool {
            return f->name() == facc->value();
        });
        if (field == fields.end()) {
            //TODO: report error
            BMCL_CRITICAL() << "no field with name: " << facc->value().toStdString();
            return false;
        }
        facc->setField(*field);
        lastField = *field;
        lastType = field->type();
        return true;
    };
    if (re->accessorsBegin()->accessorKind() != AccessorKind::Field) {
        BMCL_CRITICAL() << "first accessor must be field";
        return false;
    }
    if (!resolveField(re->accessorsBegin()->asFieldAccessor())) {
        return false;
    }
    for (auto jt = re->accessorsBegin() + 1; jt < re->accessorsEnd(); jt++) {
        Rc<Accessor> acc = *jt;
        if (acc->accessorKind() == AccessorKind::Field) {
            if (!lastType->isStruct()) {
                //TODO: report error
                BMCL_CRITICAL() << "field accessor can only access struct";
                return false;
            }
            fields = lastType->asStruct()->fieldsRange();
            FieldAccessor* facc = static_cast<FieldAccessor*>(acc.get());
            if (!resolveField(facc)) {
                return false;
            }
        } else if (acc->accessorKind() == AccessorKind::Subscript) {
            SubscriptAccessor* sacc = static_cast<SubscriptAccessor*>(acc.get());
            lastSubscript = sacc;
            sacc->setType(lastType.get());
            if (lastType->isDynArray()) {
                DynArrayType* dynArray = lastType->asDynArray();
                lastType = dynArray->elementType();
            } else if (lastType->isArray()) {
                ArrayType* array = lastType->asArray();
                lastType = array->elementType();
                //TODO: check ranges
            } else {
                //TODO: report error
                BMCL_CRITICAL() << "subscript accessor can only access array or dynArray";
                return false;
            }
        } else {
            BMCL_CRITICAL() << "invalid accessor kind";
            return false;
        }
    }
    Rc<Type> contType = nullptr;
    if (!lastSubscript.isNull()) {
        if (lastSubscript->type()->isArray()) {
            //TODO: support range
            contType = new ArrayType(lastSubscript->type()->asArray()->elementCount(), lastField->type());
        } else if (lastSubscript->type()->isDynArray()) {
            contType = new DynArrayType(lastSubscript->type()->asDynArray()->maxSize(), lastField->type());
        } else {
            assert(false);
        }
        ast->addType(contType.get());
    } else {
        contType = lastField->type();
    }
    re->setType(contType.get());
    return true;
}

bool Package::mapComponent(Ast* ast)
{
    if (ast->component().isSome()) {
        std::size_t id = _components.size(); // FIXME: make user-set
        ast->component()->setNumber(id);
        _components.emplace(id, ast->component().unwrap());
    }
    return true;
}

bool Package::resolveAll()
{
    bool isOk = true;
    uint64_t paramNum = 0;
    for (Ast* modifiedAst : modules()) {
        //BMCL_DEBUG() << "resolving " << modifiedAst->moduleInfo()->moduleName().toStdString();
        TRY(mapComponent(modifiedAst));
        isOk &= resolveImports(modifiedAst);
        isOk &= resolveGenerics(modifiedAst);
        isOk &= resolveStatuses(modifiedAst);
        isOk &= resolveParameters(modifiedAst, &paramNum);
    }
    if (!isOk) {
        BMCL_CRITICAL() << "failed to resolve package";
    }
    return isOk;
}

bmcl::OptionPtr<Ast> Package::moduleWithName(bmcl::StringView name)
{
    auto it = _modNameToAstMap.find(name);
    if (it == _modNameToAstMap.end()) {
        return bmcl::None;
    }
    return it->second.get();
}

bmcl::OptionPtr<const Ast> Package::moduleWithName(bmcl::StringView name) const
{
    auto it = _modNameToAstMap.find(name);
    if (it == _modNameToAstMap.end()) {
        return bmcl::None;
    }
    return it->second.get();
}

ComponentMap::ConstRange Package::components() const
{
    return _components;
}

ComponentMap::Range Package::components()
{
    return _components;
}

Package::AstMap::ConstRange Package::modules() const
{
    return _modNameToAstMap;
}

Package::AstMap::Range Package::modules()
{
    return _modNameToAstMap;
}

const Diagnostics* Package::diagnostics() const
{
    return _diag.get();
}

Diagnostics* Package::diagnostics()
{
    return _diag.get();
}

CompAndMsgVecConstRange Package::statusMsgs() const
{
    return _statusMsgs;
}

void Package::sortComponentsByNumber()
{
    ComponentMap tmp;
    for (Component* comp : components()) {
        tmp.emplace(comp->number(), comp);
    }
    _components = std::move(tmp);
}
}
