#include "decode/model/Model.h"

#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"

#include <bmcl/Result.h>
#include <bmcl/Logging.h>

#include <string>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>

namespace decode {

#define DECODE_SUFFIX ".decode"

const std::size_t suffixSize = sizeof(DECODE_SUFFIX) - 1;

Rc<Model> Model::readFromDirectory(const Rc<Diagnostics>& diag, const char* path)
{
    DIR* dir = opendir(path);
    if (dir == NULL) {
        //TODO: handle error;
        return nullptr;
    }

    std::string spath = path;
    spath.push_back('/');
    std::size_t pathSize = spath.size();

    Rc<Model> model = makeRc<Model>();
    model->_diag = diag;

    struct dirent* ent;
    while (true) {
        errno = 0;
        ent = readdir(dir);
        if (ent == NULL) {
            closedir(dir);
            if (errno == 0) {
                break;
            } else {
                //TODO: handle error
                return nullptr;
            }
        }

        if (ent->d_name[0] == '.') {
            if (ent->d_name[1] == '\0') {
                continue;
            } else if (ent->d_name[1] == '.') {
                if (ent->d_name[2] == '\0') {
                    continue;
                }
            }
        }

        std::size_t nameSize = std::strlen(ent->d_name);
         // length of .decode suffix
        if (nameSize >= suffixSize) {
            if (std::memcmp(ent->d_name + nameSize - suffixSize, DECODE_SUFFIX, suffixSize) != 0) {
                continue;
            }
        }
        spath.append(ent->d_name, nameSize);
        if (!model->addFile(spath.c_str(), ent->d_name, nameSize - suffixSize)) {
            //TODO: handle error
            closedir(dir);
            return nullptr;
        }
        spath.resize(pathSize);
    }

    if (!model->resolveAll()) {
        return nullptr;
    }

    return model;
}

bool Model::addFile(const char* path, const char* fileName, std::size_t fileNameLen)
{
    BMCL_DEBUG() << "reading file " << path;
    Parser p(_diag);
    ParseResult<Rc<Ast>> ast = p.parseFile(path);
    if (ast.isErr()) {
        //TODO: report error
        return false;
    }

    _parsedFiles.emplace(std::piecewise_construct,
                         std::forward_as_tuple(fileName, fileName + fileNameLen),
                         std::forward_as_tuple(ast.take()));

    BMCL_DEBUG() << "finished " << path;
    return true;
}

bool Model::resolveAll()
{
    BMCL_DEBUG() << "resolving";
    bool isOk = true;
    for (const auto& modifiedAst : _parsedFiles) {
        for (const Rc<Import>& import : modifiedAst.second->imports()) {
            auto searchedAst = _parsedFiles.find(import->path().toStdString());
            if (searchedAst == _parsedFiles.end()) {
                isOk = false;
                BMCL_CRITICAL() << "invalid import mod in " << modifiedAst.first << ".decode: " << import->path().toStdString();
                continue;
            }
            for (const Rc<ImportedType>& modifiedType : import->types()) {
                bmcl::Option<const Rc<Type>&> foundType = searchedAst->second->findTypeWithName(modifiedType->name());
                if (foundType.isNone()) {
                        isOk = false;
                    //TODO: report error
                    BMCL_CRITICAL() << "invalid import type in " << modifiedAst.first << ".decode: " << modifiedType->name().toStdString();
                } else {
                    if (modifiedType->typeKind() == foundType.unwrap()->typeKind()) {
                        isOk = false;
                        //TODO: report error - circular imports
                        BMCL_CRITICAL() << "circular imports " << modifiedAst.first << ".decode: " << modifiedType->name().toStdString();
                        BMCL_CRITICAL() << "circular imports " << searchedAst->first << ".decode: " << foundType.unwrap()->name().toStdString();
                    }
                    modifiedType->_link = foundType.unwrap();
                }
            }
        }
    }
    return isOk;
}
}
