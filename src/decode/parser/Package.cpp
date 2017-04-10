#include "decode/parser/Package.h"

#include "decode/core/Diagnostics.h"
#include "decode/core/Configuration.h"
#include "decode/parser/Parser.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Component.h"
#include "decode/parser/Lexer.h"
#include "decode/core/Try.h"

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

#if defined(__linux__)
# include <dirent.h>
# include <sys/stat.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#endif

void libzpaq::error(const char* msg)
{
    throw std::runtime_error(msg);
}

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

class ZpaqReader : public libzpaq::Reader {
public:
    ZpaqReader(const bmcl::Buffer& buf)
        : _reader(buf.data(), buf.size())
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

Package::Package(Configuration* cfg, Diagnostics* diag)
    : _diag(diag)
    , _cfg(cfg)
{
}

Package::~Package()
{
}

PackageResult Package::readFromDirectory(Configuration* cfg, Diagnostics* diag, const char* path)
{

    std::string spath = path;

    Rc<Package> package = new Package(cfg, diag);
    std::size_t pathSize;
    Parser p(diag);

#if defined(__linux__)
#define NEXT_FILE() continue
    spath.push_back('/');

    DIR* dir = opendir(path);
    if (dir == NULL) {
        //TODO: handle error;
        return PackageResult();
    }

    struct dirent* ent;
#elif defined(_MSC_VER) || defined(__MINGW32__)
#define NEXT_FILE() goto nextFile
    spath.push_back('\\');

    std::string regexp = path;
    if (regexp.empty()) {
        //TODO: report error
        return PackageResult();
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
        if (!package->addFile(spath.c_str(), &p)) {
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

    if (!package->resolveAll()) {
        goto error;
    }

    return std::move(package);

error:
#if defined(__linux__)
    closedir(dir);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    FindClose(handle);
#endif
    return PackageResult();
}

typedef std::array<std::uint8_t, 4> MagicType;
const MagicType magic = {{ 0x7a, 0x70, 0x61, 0x71 }};

PackageResult Package::decodeFromMemory(Diagnostics* diag, const void* src, std::size_t size)
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

    bmcl::MemReader reader(buf.data(), buf.size());

    if (reader.readableSize() < (magic.size() + 2)) {
        //TODO: report error
        return PackageResult();
    }

    MagicType m;
    reader.read(m.data(), m.size());

    if (m != magic) {
        //TODO: report error
        return PackageResult();
    }

    Rc<Configuration> cfg = new Configuration;

    cfg->setDebugLevel(reader.readUint8());
    cfg->setCompressionLevel(reader.readUint8());

    uint64_t numOptions;
    if (!reader.readVarUint(&numOptions)) {
        //TODO: report error
        return PackageResult();
    }
    for (uint64_t i = 0; i < numOptions; i++) {
        uint64_t keySize;
        if (!reader.readVarUint(&keySize)) {
            //TODO: report error
            return PackageResult();
        }

        if (reader.readableSize() < keySize) {
            //TODO: report error
            return PackageResult();
        }

        bmcl::StringView key((const char*)reader.current(), keySize);
        reader.skip(keySize);

        if (reader.readableSize() < 1) {
            //TODO: report error
            return PackageResult();
        }

        bool hasValue = reader.readUint8();
        if (hasValue) {

            uint64_t valueSize;
            if (!reader.readVarUint(&valueSize)) {
                //TODO: report error
                return PackageResult();
            }

            if (reader.readableSize() < valueSize) {
                //TODO: report error
                return PackageResult();
            }

            bmcl::StringView value((const char*)reader.current(), keySize);
            reader.skip(valueSize);
            cfg->setCfgOption(key, value);
        } else {
            cfg->setCfgOption(key);
        }
    }

    Rc<Package> package = new Package(cfg.get(), diag);
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

        uint64_t contentsSize;
        if (!reader.readVarUint(&contentsSize)) {
            //TODO: report error
            return PackageResult();
        }

        if (reader.readableSize() < contentsSize) {
            //TODO: report error
            return PackageResult();
        }

        begin = (const char*)reader.current();
        end = begin + contentsSize;
        std::string contents(begin, end);
        reader.skip(contentsSize);

        Rc<FileInfo> finfo = new FileInfo(std::move(fname), std::move(contents));

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

bmcl::Buffer Package::encode() const
{
    return encode(_cfg->compressionLevel());
}

bmcl::Buffer Package::encode(unsigned compressionLevel) const
{
    if (compressionLevel > 5) {
        compressionLevel = 5;
    }
    bmcl::Buffer buf;
    buf.write(magic.data(), magic.size());

    buf.writeUint8(_cfg->debugLevel());
    buf.writeUint8(_cfg->compressionLevel());

    buf.writeVarUint(_cfg->numOptions());
    for (auto it : _cfg->optionsRange()) {
        buf.writeVarUint(it.first.size());
        buf.write(it.first.data(), it.first.size());
        if (it.second.isSome()) {
            buf.writeUint8(1);
            buf.writeVarUint(it.second->size());
            buf.write(it.second->data(), it.second->size());
        } else {
            buf.writeUint8(0);
        }
    }

    for (const Ast* it : modules()) {
        Rc<const FileInfo> finfo = it->moduleInfo()->fileInfo();

        const std::string& fname = finfo->fileName();
        assert(fname.size() <= std::numeric_limits<std::uint8_t>::max());
        buf.writeUint8(fname.size());
        buf.write((const void*)fname.data(), fname.size());

        const std::string& contents = finfo->contents();

        assert(contents.size() <= std::numeric_limits<std::uint32_t>::max());
        buf.writeVarUint(contents.size());
        buf.write((const void*)contents.data(), contents.size());
    }

    bmcl::Buffer result;
    ZpaqReader in(buf);
    ZpaqWriter out(&result);

    try {
        libzpaq::compress(&in, &out, std::to_string(compressionLevel).c_str());
    } catch (const std::exception& err) {
        BMCL_CRITICAL() << "error compressing zpaq archive: " << err.what();
        std::terminate();
    }

    return result;
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
            BMCL_CRITICAL() << "invalid import mod in " << ast->moduleInfo()->moduleName().toStdString() << ": " << import->path().toStdString();
            continue;
        }
        for (ImportedType* modifiedType : import->typesRange()) {
            bmcl::OptionPtr<NamedType> foundType = searchedAst->second->findTypeWithName(modifiedType->name());
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
                auto field  = fields.findIf([facc](const Field* f) -> bool {
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
        std::size_t id = _components.size(); //FIXME: make user-set
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
}
