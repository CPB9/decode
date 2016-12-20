#include "decode/parser/Package.h"

#include "decode/core/Diagnostics.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"

#include <bmcl/Result.h>
#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>

#include <libzpaq.h>

#include <string>
#include <array>
#include <cstring>
#include <limits>
#include <exception>

#include <dirent.h>
#include <sys/stat.h>

void libzpaq::error(const char* msg)
{
    throw std::runtime_error(msg);
}

namespace decode {

class ZpaqReader : public libzpaq::Reader {
public:
    ZpaqReader(const bmcl::Buffer& buf)
        : _reader(buf.start(), buf.size())
    {
    }

    ZpaqReader(const void* src, std::size_t size)
        : _reader(src, size)
    {
    }

    int get() override
    {
        if (_reader.isEmpty()) {
            return -1;
        }
        return _reader.readUint8();
    }

    int read(char* buf, int n) override
    {
        std::size_t size = std::min<std::size_t>(n, _reader.readableSize());
        _reader.read(buf, size);
        return size;
    }

private:
    bmcl::MemReader _reader;
};


class ZpaqWriter : public libzpaq::Writer {
public:
    ZpaqWriter(bmcl::Buffer* buf)
        : _buf(buf)
    {
    }

    void put(int c) override
    {
        _buf->writeUint8(c);
    }

    void write(const char* buf, int n) override
    {
        _buf->write(buf, n);
    }

private:
    bmcl::Buffer* _buf;
};

#define DECODE_SUFFIX ".decode"

const std::size_t suffixSize = sizeof(DECODE_SUFFIX) - 1;

Package::Package(const Rc<Diagnostics>& diag)
    : _diag(diag)
{
}

PackageResult Package::readFromDirectory(const Rc<Diagnostics>& diag, const char* path)
{
    DIR* dir = opendir(path);
    if (dir == NULL) {
        //TODO: handle error;
        return PackageResult();
    }

    std::string spath = path;
    spath.push_back('/');
    std::size_t pathSize = spath.size();

    Rc<Package> package = new Package(diag);

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
                return PackageResult();
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
        if (!package->addFile(spath.c_str())) {
            closedir(dir);
            return PackageResult();
        }
        spath.resize(pathSize);
    }

    if (!package->resolveAll()) {
        return PackageResult();
    }

    return std::move(package);
}

typedef std::array<std::uint8_t, 4> MagicType;
const MagicType magic = {{ 0x7a, 0x70, 0x61, 0x71 }};

PackageResult Package::decodeFromMemory(const Rc<Diagnostics>& diag, const void* src, std::size_t size)
{
    bmcl::Buffer buf;
    ZpaqReader in(src, size);
    ZpaqWriter out(&buf);

    try {
        libzpaq::decompress(&in, &out);
    } catch(const std::exception& err) {
        BMCL_CRITICAL() << "error decompressing zpaq archive: " << err.what();
        return PackageResult();
    }

    Rc<Package> package = new Package(diag);
    bmcl::MemReader reader(buf.start(), buf.size());

    if (reader.readableSize() < magic.size()) {
        //TODO: report error
        return PackageResult();
    }

    MagicType m;
    reader.read(m.data(), m.size());

    if (m != magic) {
        //TODO: report error
        return PackageResult();
    }

    Parser p(diag);

    while (!reader.isEmpty()) {
        if (reader.readableSize() < 1) {
            //TODO: report error
            return PackageResult();
        }

        std::uint8_t fnameSize = reader.readUint8();
        if (reader.readableSize() < fnameSize) {
            //TODO: report error
            return PackageResult();
        }

        const char* begin = (const char*)reader.current();
        const char* end = begin + fnameSize;
        std::string fname(begin, end);
        reader.skip(fnameSize);

        if (reader.readableSize() < 4) {
            //TODO: report error
            return PackageResult();
        }

        std::uint32_t contentsSize = reader.readUint32Le();
        if (reader.readableSize() < contentsSize) {
            //TODO: report error
            return PackageResult();
        }

        begin = (const char*)reader.current();
        end = begin + contentsSize;
        std::string contents(begin, end);
        reader.skip(contentsSize);

        Rc<FileInfo> finfo = new FileInfo(std::move(fname), std::move(contents));

        ParseResult ast = p.parseFile(finfo);
        if (ast.isErr()) {
            //TODO: report error
            return PackageResult();
        }

        package->addAst(ast.unwrap());
    }

    return std::move(package);
}

bmcl::Buffer Package::encode() const
{
    bmcl::Buffer buf;
    buf.write(magic.data(), magic.size());

    for (const auto& it : _modNameToAstMap) {
        const Rc<FileInfo>& finfo = it.second->moduleInfo()->fileInfo();

        const std::string& fname = finfo->fileName();
        assert(fname.size() <= std::numeric_limits<std::uint8_t>::max());
        buf.writeUint8(fname.size());
        buf.write((const void*)fname.data(), fname.size());

        const std::string& contents = finfo->contents();

        assert(contents.size() <= std::numeric_limits<std::uint32_t>::max());
        buf.writeUint32Le(contents.size());
        buf.write((const void*)contents.data(), contents.size());
    }

    bmcl::Buffer result;
    ZpaqReader in(buf);
    ZpaqWriter out(&result);

    try {
        libzpaq::compress(&in, &out, "5");
    } catch(const std::exception& err) {
        BMCL_CRITICAL() << "error compressing zpaq archive: " << err.what();
        std::terminate();
    }

    return result;
}

void Package::addAst(const Rc<Ast>& ast)
{
    bmcl::StringView modName = ast->moduleInfo()->moduleName();
    _modNameToAstMap.emplace(modName, ast);
}

bool Package::addFile(const char* path)
{
    BMCL_DEBUG() << "reading file " << path;
    Parser p(_diag);
    ParseResult ast = p.parseFile(path);
    if (ast.isErr()) {
        return false;
    }

    addAst(ast.unwrap());

    BMCL_DEBUG() << "finished " << ast.unwrap()->moduleInfo()->fileName();
    return true;
}

bool Package::resolveTypes(const Rc<Ast>& ast)
{
    bool isOk = true;
    for (const Rc<Import>& import : ast->imports()) {
        auto searchedAst = _modNameToAstMap.find(import->path().toStdString());
        if (searchedAst == _modNameToAstMap.end()) {
            isOk = false;
            BMCL_CRITICAL() << "invalid import mod in " << ast->moduleInfo()->moduleName().toStdString() << ": " << import->path().toStdString();
            continue;
        }
        for (const Rc<ImportedType>& modifiedType : import->types()) {
            bmcl::Option<const Rc<Type>&> foundType = searchedAst->second->findTypeWithName(modifiedType->name());
            if (foundType.isNone()) {
                isOk = false;
                //TODO: report error
                BMCL_CRITICAL() << "invalid import type in " << ast->moduleInfo()->moduleName().toStdString() << ": " << modifiedType->name().toStdString();
            } else {
                if (modifiedType->typeKind() == foundType.unwrap()->typeKind()) {
                    isOk = false;
                    //TODO: report error - circular imports
                    BMCL_CRITICAL() << "circular imports " << ast->moduleInfo()->moduleName().toStdString() << ": " << modifiedType->name().toStdString();
                    BMCL_CRITICAL() << "circular imports " << searchedAst->first.toStdString() << ".decode: " << foundType.unwrap()->name().toStdString();
                }
                modifiedType->_link = foundType.unwrap();
            }
        }
    }
    return isOk;
}

bool Package::resolveStatuses(const Rc<Ast>& ast)
{
    bool isOk = true;
    const bmcl::Option<Rc<Component>>& comp = ast->component();
    if (comp.isNone()) {
        return true;
    }

    const bmcl::Option<Rc<Statuses>>& statuses = comp.unwrap()->statuses();
    if (statuses.isNone()) {
        return true;
    }

    StatusMap& map = statuses.unwrap()->_regexps;
    const bmcl::Option<Rc<Parameters>>& params = comp.unwrap()->parameters();
    if (params.isNone() && !map.empty()) {
        //TODO: report error
        return false;
    }

    for (auto it : map) {
        for (const Rc<StatusRegexp>& re : it.second) {

            Rc<FieldList> fields = params.unwrap()->fields();
            Rc<Type> lastType;
            Rc<Field> lastField;

            if (re->_accessors.empty()) {
                continue;
            }
            auto resolveField = [&](FieldAccessor* facc) -> bool {
                bmcl::Option<const Rc<Field>&> field  = fields->fieldWithName(facc->value());
                if (field.isNone()) {
                    //TODO: report error
                    return false;
                }
                facc->_field = field.unwrap();
                lastField = field.unwrap();
                lastType = field.unwrap()->type();
                return true;
            };
            if (re->_accessors.front()->accessorKind() != AccessorKind::Field) {
                return false;
            }
            if (!resolveField(static_cast<FieldAccessor*>(re->_accessors.front().get()))) {
                return false;
            }
            for (auto jt = re->_accessors.begin() + 1; jt < re->_accessors.end(); jt++) {
                Rc<Accessor> acc = *jt;
                if (acc->accessorKind() == AccessorKind::Field) {
                    if (!lastType->isStruct()) {
                        //TODO: report error
                        return false;
                    }
                    fields = lastType->asStruct()->fields();
                    FieldAccessor* facc = static_cast<FieldAccessor*>(acc.get());
                    if (!resolveField(facc)) {
                        return false;
                    }
                } else if (acc->accessorKind() == AccessorKind::Subscript) {
                    SubscriptAccessor* sacc = static_cast<SubscriptAccessor*>(acc.get());
                    sacc->_type = lastType;
                    if (lastType->isSlice()) {
                        SliceType* slice = lastType->asSlice();
                        lastType = slice->elementType();
                    } else if (lastType->isArray()) {
                        ArrayType* array = lastType->asArray();
                        lastType = array->elementType();
                        //TODO: check ranges
                    } else {
                        //TODO: report error
                        return false;
                    }
                } else {
                    return false;
                }

            }
        }
    }

    return isOk;
}

bool Package::resolveAll()
{
    BMCL_DEBUG() << "resolving";
    bool isOk = true;
    for (const auto& modifiedAst : _modNameToAstMap) {
        isOk &= resolveTypes(modifiedAst.second);
        isOk &= resolveStatuses(modifiedAst.second);

    }
    if (!isOk) {
        BMCL_CRITICAL() << "asd";
    }
    return isOk;
}
}
