#include "decode/generator/Generator.h"
#include "decode/generator/SliceNameGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/InlineSerContext.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/generator/TypeHeaderGen.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Package.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Decl.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"

#include <bmcl/Logging.h>

#include <unordered_set>
#include <iostream>
#include <deque>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER)
# include <windows.h>
#endif

//TODO: refact

namespace decode {

Generator::Generator(const Rc<Diagnostics>& diag)
    : _diag(diag)
    , _target(new Target(4)) //FIXME
    , _inlineSer(_target.get(), &_output)
    , _inlineDeser(_target.get(), &_output)
{
}

void Generator::setOutPath(bmcl::StringView path)
{
    _savePath = path.toStdString();
}

bool Generator::makeDirectory(const char* path)
{
#if defined(__linux__)
    int rv = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv == -1) {
        int rn = errno;
        if (rn == EEXIST) {
            return true;
        }
        //TODO: report error
        BMCL_CRITICAL() << "unable to create dir: " << path;
        return false;
    }
#elif defined(_MSC_VER)
    bool isOk = CreateDirectory(path, NULL);
    if (!isOk) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            BMCL_CRITICAL() << "error creating dir";
            return false;
        }
    }
#endif
    return true;
}

bool Generator::saveOutput(const char* path)
{
#if defined(__linux__)
    int fd;
    while (true) {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            //TODO: handle error
            BMCL_CRITICAL() << "unable to open file: " << path;
            return false;
        }
        break;
    }

    std::size_t size = _output.result().size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, _output.result().c_str() + total, size);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            BMCL_CRITICAL() << "unable to write file: " << path;
            //TODO: handle error
            close(fd);
            return false;
        }
        total += written;
    }

    close(fd);
#elif defined(_MSC_VER)
    HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        BMCL_CRITICAL() << "error creating file";
        //TODO: report error
        return false;
    }
    DWORD bytesWritten;
    bool isOk = WriteFile(handle, _output.result().c_str(), _output.result().size(), &bytesWritten, NULL);
    if (!isOk) {
        BMCL_CRITICAL() << "error writing file";
        //TODO: report error
        return false;
    }
    assert(_output.result().size() == bytesWritten);
#endif
    return true;
}

void Generator::genHeader(const Type* type)
{
    TypeHeaderGen gen(&_output);
    gen.genHeader(_currentAst.get(), type);
}

void Generator::genSource(const Type* type)
{
    auto writeIncludes = [this](const Type* type) {
        std::string path = type->moduleName().toStdString();
        path.push_back('/');
        path.append(type->name().toStdString());
        _output.appendLocalIncludePath(path);
    };

    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Array:
    case TypeKind::Reference:
    case TypeKind::Slice:
    case TypeKind::Imported:
    case TypeKind::Function:
        break;
    case TypeKind::Struct:
        writeIncludes(type);
        _output.appendEol();
        writeStructDeserizalizer(static_cast<const StructType*>(type));
        _output.appendEol();
        writeStructSerializer(static_cast<const StructType*>(type));
        break;
    case TypeKind::Variant:
        writeIncludes(type);
        _output.appendEol();
        writeVariantDeserizalizer(static_cast<const VariantType*>(type));
        _output.appendEol();
        writeVariantSerializer(static_cast<const VariantType*>(type));
        break;
    case TypeKind::Enum:
        writeIncludes(type);
        _output.appendEol();
        writeEnumDeserizalizer(static_cast<const EnumType*>(type));
        _output.appendEol();
        writeEnumSerializer(static_cast<const EnumType*>(type));
        break;
    }
    _output.appendEol();
}

void Generator::writeStructDeserizalizer(const StructType* type)
{
    InlineSerContext ctx;
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");
    StringBuilder argName("self->");

    for (const Rc<Field>& field : *type->fields()) {
        argName.append(field->name().begin(), field->name().end());
        _inlineDeser.inspect(field->type().get(), ctx, argName.view());
        argName.resize(6);
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeStructSerializer(const StructType* type)
{
    InlineSerContext ctx;
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");
    StringBuilder argName("self->");

    for (const Rc<Field>& field : *type->fields()) {
        argName.append(field->name().begin(), field->name().end());
        _inlineSer.inspect(field->type().get(), ctx, argName.view());
        argName.resize(6);
    }

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeVariantDeserizalizer(const VariantType* type)
{
    writeDeserializerFuncDecl(type);
     _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendVarDecl("int64_t", "value");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output.append("    switch(value) {\n");
    std::size_t i = 0;
    StringBuilder argName("self->data.");
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        _output.appendNumericValue(i);
        i++;
        _output.append(": {\n");
        _output.append("        self->type = ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("Type");
        _output.append("_");
        _output.appendWithFirstUpper(field->name());
        _output.append(";\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<TupleVariantField*>(field.get());
            std::size_t j = 1;
            for (const Rc<Type>& t : tupField->types()) {
                    argName.appendWithFirstLower(field->name());
                    argName.append(type->name());
                    argName.append("._");
                    argName.appendNumericValue(j);
                    _inlineDeser.inspect(t.get(), ctx, argName.view());
                    argName.resize(11);
                    j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<StructVariantField*>(field.get());
            for (const Rc<Field>& f : *varField->fields()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append(".");
                argName.append(f->name());
                _inlineDeser.inspect(f->type().get(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output.append("        break;\n");
        _output.append("    }\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeVariantSerializer(const VariantType* type)
{
    writeSerializerFuncDecl(type);
     _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, (uint64_t)self->type)");
    });

    _output.append("    switch(self->type) {\n");
    StringBuilder argName("self->data.");
    for (const Rc<VariantField>& field : type->fields()) {
        _output.append("    case ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("Type");
        _output.append("_");
        _output.appendWithFirstUpper(field->name());
        _output.append(": {\n");

        InlineSerContext ctx(2);
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* tupField = static_cast<TupleVariantField*>(field.get());
            std::size_t j = 1;
            for (const Rc<Type>& t : tupField->types()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append("._");
                argName.appendNumericValue(j);
                _inlineSer.inspect(t.get(), ctx, argName.view());
                argName.resize(11);
                j++;
            }
            break;
        }
        case VariantFieldKind::Struct: {
            const StructVariantField* varField = static_cast<StructVariantField*>(field.get());
            for (const Rc<Field>& f : *varField->fields()) {
                argName.appendWithFirstLower(field->name());
                argName.append(type->name());
                argName.append(".");
                argName.append(f->name());
                _inlineSer.inspect(f->type().get(), ctx, argName.view());
                argName.resize(11);
            }
            break;
        }
        }

        _output.append("        break;\n");
        _output.append("    }\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeEnumDeserizalizer(const EnumType* type)
{
    writeDeserializerFuncDecl(type);
    _output.append("\n{\n");
    _output.appendIndent(1);
    _output.appendVarDecl("int64_t", "value");
    _output.appendIndent(1);
    _output.appendModPrefix();
    _output.appendVarDecl(type->name(), "result");
    _output.appendIndent(1);
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output.append("    switch(value) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        _output.appendNumericValue(pair.first);
        _output.append(":\n");
        _output.append("        result = ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name());
        _output.append(";\n");
        _output.append("        break;\n");
    }
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n");

    _output.append("    *self = result;\n");
    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

void Generator::writeEnumSerializer(const EnumType* type)
{
    writeSerializerFuncDecl(type);
    _output.append("\n{\n");

    _output.append("    switch(*self) {\n");
    for (const auto& pair : type->constants()) {
        _output.append("    case ");
        _output.appendModPrefix();
        _output.append(type->name());
        _output.append("_");
        _output.append(pair.second->name());
        _output.append(":\n");
    }
    _output.append("        break;\n");
    _output.append("    default:\n");
    _output.append("        return PhotonError_InvalidValue;\n");
    _output.append("    }\n    ");
    _output.appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)*self)");
    });
    _output.append("    return PhotonError_Ok;\n");
    _output.append("}\n");
}

bool Generator::generateFromPackage(const Rc<Package>& package)
{
    for (auto it : package->modules()) {
        if (!generateFromAst(it.second)) {
            return false;
        }
    }

    return true;
}

bool Generator::generateFromAst(const Rc<Ast>& ast)
{
    _currentAst = ast;
    if (!ast->moduleInfo()->moduleName().equals("core")) {
        _output.setModName(ast->moduleInfo()->moduleName());
    } else {
        _output.setModName(bmcl::StringView::empty());
    }

    StringBuilder photonPath;
    photonPath.append(_savePath);
    photonPath.append("/photon");
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');
    photonPath.append(_currentAst->moduleInfo()->moduleName());
    TRY(makeDirectory(photonPath.result().c_str()));
    photonPath.append('/');

    auto dump = [this, &photonPath](const Type* type, bmcl::StringView ext) -> bool {
        if (!_output.result().empty()) {
            photonPath.append(type->name());
            photonPath.append(ext);
            TRY(saveOutput(photonPath.result().c_str()));
            photonPath.removeFromBack(type->name().size() + 2);
            _output.clear();
        }
        return true;
    };

    for (const auto& it : ast->typeMap()) {
        const Type* type = it.second.get();
        if (type->typeKind() == TypeKind::Imported) {
            return true;
        }

        genHeader(type);
        TRY(dump(type, ".h"));

        genSource(type);
        TRY(dump(type, ".c"));
    }

    _currentAst = nullptr;
    return true;
}

void Generator::writeSerializerFuncDecl(const Type* type)
{
    _output.append("PhotonError ");
    genTypeRepr(type);
    _output.append("_Serialize(const ");
    genTypeRepr(type);
    _output.append("* self, PhotonWriter* dest)");
}

void Generator::writeDeserializerFuncDecl(const Type* type)
{
    _output.append("PhotonError ");
    genTypeRepr(type);
    _output.append("_Deserialize(");
    genTypeRepr(type);
    _output.append("* self, PhotonReader* src)");
}

void Generator::genTypeRepr(const Type* topLevelType, bmcl::StringView fieldName)
{
    TypeReprGen trg(&_output);
    trg.genTypeRepr(topLevelType, fieldName);
}

}
