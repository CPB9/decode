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
#include "decode/parser/Ast.h"
#include "decode/parser/Component.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Lexer.h"
#include "decode/parser/Parser.h"

#include <bmcl/Buffer.h>
#include <bmcl/Logging.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>

#include <array>
#include <cstring>
#include <exception>
#include <limits>
#include <string>

#if defined(__linux__)
# include <dirent.h>
# include <sys/stat.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#endif

namespace decode {

//TODO: move to Component.c

ComponentAndMsg::ComponentAndMsg(const Rc<Component>& component, const Rc<StatusMsg>& msg)
    : component(component)
    , msg(msg)
{
}

ComponentAndMsg::~ComponentAndMsg()
{
}

#define DECODE_SUFFIX ".decode"

const std::size_t suffixSize = sizeof(DECODE_SUFFIX) - 1;

Package::Package(Configuration* cfg, Diagnostics* diag)
    : _diag(diag)
    , _cfg(cfg)
{
}

Package::~Package()
{
}

bool Package::addDir(const char* path, Package* package, Parser* p)
{
    std::size_t pathSize;
    std::string spath = path;
#if defined(__linux__)
#define NEXT_FILE() continue
    spath.push_back('/');

    DIR* dir = opendir(path);
    if (dir == NULL) {
        //TODO: handle error;
        return false;
    }

    struct dirent* ent;
#elif defined(_MSC_VER) || defined(__MINGW32__)
#define NEXT_FILE() goto nextFile
    spath.push_back('\\');

    std::string regexp = path;
    if (regexp.empty()) {
        //TODO: report error
        return false;
    }
    if (regexp.back() != '\\') {
        regexp.push_back('\\');
    }
    regexp.push_back('*');

    WIN32_FIND_DATA currentFile;
    HANDLE handle = FindFirstFile(regexp.c_str(), &currentFile);

    //TODO: check ERROR_FILE_NOT_FOUND
    if (handle == INVALID_HANDLE_VALUE) {
        BMCL_CRITICAL() << "error opening directory";
        goto error;
    }
#endif
    pathSize = spath.size();

    while (true) {
#if defined(__linux__)
        errno = 0;
        ent = readdir(dir);
        if (ent == NULL) {
            if (errno == 0) {
                closedir(dir);
                break;
            } else {
                goto error;
            }
        }
        const char* name = &ent->d_name[0];
#elif defined(_MSC_VER) || defined(__MINGW32__)
        const char* name = currentFile.cFileName;
#endif
        std::size_t nameSize;
        if (name[0] == '.') {
            if (name[1] == '\0') {
                NEXT_FILE();
            } else if (name[1] == '.') {
                if (name[2] == '\0') {
                    NEXT_FILE();
                }
            }
        }

        nameSize = std::strlen(name);
        // length of .decode suffix
        if (nameSize >= suffixSize) {
            if (std::memcmp(name + nameSize - suffixSize, DECODE_SUFFIX, suffixSize) != 0) {
                NEXT_FILE();
            }
        }
        spath.append(name, nameSize);
        if (!package->addFile(spath.c_str(), p)) {
            goto error;
        }
        spath.resize(pathSize);

#if defined(_MSC_VER) || defined(__MINGW32__)
nextFile:
        bool isOk = FindNextFile(handle, &currentFile);
        if (!isOk) {
            DWORD err = GetLastError();
            if (err == ERROR_NO_MORE_FILES) {
                FindClose(handle);
                break;
            } else {
                goto error;
            }
        }
#endif
    }

    return true;

error:
#if defined(__linux__)
    closedir(dir);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    FindClose(handle);
#endif
    return false;
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

PackageResult Package::decodeFromMemory(Configuration* cfg, Diagnostics* diag, const void* src, std::size_t size)
{
    bmcl::MemReader reader(src, size);

    Rc<Package> package = new Package(cfg, diag);
    Parser p(diag);

    while (!reader.isEmpty()) {
        auto fname = deserializeString(&reader);
        if (fname.isErr()) {
            //TODO: report error
            return PackageResult();
        }

        auto contents = deserializeString(&reader);
        if (contents.isErr()) {
            //TODO: report error
            return PackageResult();
        }

        Rc<FileInfo> finfo = new FileInfo(fname.unwrap().toStdString(), contents.unwrap().toStdString());

        ParseResult ast = p.parseFile(finfo.get());
        if (ast.isErr()) {
            //TODO: report error
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
    bmcl::StringView modName = ast->moduleInfo()->moduleName();
    _modNameToAstMap.emplace(modName, ast);
}

bool Package::addFile(const char* path, Parser* p)
{
    BMCL_DEBUG() << "reading file " << path;
    ParseResult ast = p->parseFile(path);
    if (ast.isErr()) {
        return false;
    }

    addAst(ast.unwrap().get());

    BMCL_DEBUG() << "finished " << ast.unwrap()->moduleInfo()->fileName();
    return true;
}

bool Package::resolveTypes(Ast* ast)
{
    bool isOk = true;
    for (TypeImport* import : ast->importsRange()) {
        auto searchedAst = _modNameToAstMap.find(import->path().toStdString());
        if (searchedAst == _modNameToAstMap.end()) {
            isOk = false;
            BMCL_CRITICAL() << "invalid import mod in "
                            << ast->moduleInfo()->moduleName().toStdString() << ": "
                            << import->path().toStdString();
            continue;
        }
        for (ImportedType* modifiedType : import->typesRange()) {
            bmcl::OptionPtr<NamedType> foundType = searchedAst->second->findTypeWithName(modifiedType->name());
            if (foundType.isNone()) {
                isOk = false;
                //TODO: report error
                BMCL_CRITICAL() << "invalid import type in "
                                << ast->moduleInfo()->moduleName().toStdString() << ": "
                                << modifiedType->name().toStdString();
            } else {
                if (modifiedType->typeKind() == foundType.unwrap()->typeKind()) {
                    isOk = false;
                    //TODO: report error - circular imports
                    BMCL_CRITICAL() << "circular imports "
                                    << ast->moduleInfo()->moduleName().toStdString()
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

    if (!comp->hasParams() && comp->hasStatuses()) {
        //TODO: report error
        BMCL_CRITICAL() << "no params, has statuses";
        return false;
    }

    for (StatusMsg* it : comp->statusesRange()) {
        _statusMsgs.emplace_back(comp.unwrap(), it);
        for (StatusRegexp* re : it->partsRange()) {
            FieldVec::Range fields = comp->paramsRange();
            Rc<Type> lastType;
            Rc<Field> lastField;

            if (!re->hasAccessors()) {
                continue;
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
                    sacc->setType(lastType.get());
                    if (lastType->isSlice()) {
                        SliceType* slice = lastType->asSlice();
                        lastType = slice->elementType();
                    } else if (lastType->isArray()) {
                        ArrayType* array = lastType->asArray();
                        lastType = array->elementType();
                        //TODO: check ranges
                    } else {
                        //TODO: report error
                        BMCL_CRITICAL() << "subscript accessor can only access array or slice";
                        return false;
                    }
                } else {
                    BMCL_CRITICAL() << "invalid accessor kind";
                    return false;
                }
            }
        }
    }

    return isOk;
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
    for (Ast* modifiedAst : modules()) {
        BMCL_DEBUG() << "resolving " << modifiedAst->moduleInfo()->moduleName().toStdString();
        TRY(mapComponent(modifiedAst));
        isOk &= resolveTypes(modifiedAst);
        isOk &= resolveStatuses(modifiedAst);
    }
    if (!isOk) {
        BMCL_CRITICAL() << "asd";
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
}
