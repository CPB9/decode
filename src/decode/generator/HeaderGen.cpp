#include "decode/generator/HeaderGen.h"

namespace decode {

HeaderGen::HeaderGen(SrcBuilder* output)
    : _output(output)
    , _typeReprGen(output)
{
}

HeaderGen::~HeaderGen()
{
}

void HeaderGen::genHeader(const Ast* ast, const Type* type)
{
    _ast = ast;
    traverseType(type);
}

void HeaderGen::appendSerializerFuncPrototypes(const Type* type)
{
    appendSerializerFuncDecl(type);
    _output->append(";\n");
    appendDeserializerFuncDecl(type);
    _output->append(";\n\n");
}

void HeaderGen::startIncludeGuard(const Type* type)
{
    auto writeGuardMacro = [this, type]() {
        _output->append("__PHOTON_");
        _output->append(type->moduleInfo()->moduleName().toUpper());
        _output->append('_');
        _output->append(type->name().toUpper()); //FIXME
        _output->append("__\n");
    };
    _output->append("#ifndef ");
    writeGuardMacro();
    _output->append("#define ");
    writeGuardMacro();
    _output->appendEol();
}

void HeaderGen::endIncludeGuard(const Type* type)
{
    _output->append("#endif\n");
    _output->appendEol();
}

void HeaderGen::appendImplBlockIncludes(const Type* topLevelType)
{
    bmcl::Option<const Rc<ImplBlock>&> impl = _ast->findImplBlockWithName(topLevelType->name());
    std::unordered_set<std::string> dest;
    if (impl.isSome()) {
        for (const Rc<FunctionType>& fn : impl.unwrap()->functions()) {
            _includeCollector.collectIncludesAndFwdsForType(fn.get(), &dest);
        }
    }
    dest.insert("core/Reader");
    dest.insert("core/Writer");
    dest.insert("core/Error");
    appendIncludes(dest);
}

void HeaderGen::appendIncludes(const std::unordered_set<std::string>& src)
{
    for (const std::string& path : src) {
        _output->appendLocalIncludePath(path);
    }

    if (!src.empty()) {
        _output->appendEol();
    }
}

void HeaderGen::appendIncludesAndFwdsForType(const Type* topLevelType)
{
    std::unordered_set<std::string> includePaths;
    _includeCollector.collectIncludesAndFwdsForType(topLevelType, &includePaths);
    appendIncludes(includePaths);
}

void HeaderGen::appendImplFunctionPrototypes(const Type* type)
{
    bmcl::Option<const Rc<ImplBlock>&> block = _ast->findImplBlockWithName(type->name());
    if (block.isNone()) {
        return;
    }
    for (const Rc<FunctionType>& func : block.unwrap()->functions()) {
        appendImplFunctionPrototype(func, type->name());
    }
    if (!block.unwrap()->functions().empty()) {
        _output->append('\n');
    }
}

void HeaderGen::appendImplFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName)
{
    if (func->returnValue().isSome()) {
        genTypeRepr(func->returnValue()->get());
        _output->append(' ');
    } else {
        _output->append("void ");
    }
    _output->appendModPrefix();
    _output->append(typeName);
    _output->append('_');
    _output->appendWithFirstUpper(func->name());
    _output->append('(');

    if (func->selfArgument().isSome()) {
        SelfArgument self = func->selfArgument().unwrap();
        switch(self) {
        case SelfArgument::Reference:
            _output->append("const ");
        case SelfArgument::MutReference:
            _output->appendModPrefix();
            _output->append(typeName);
            _output->append("* self");
            break;
        case SelfArgument::Value:
            break;
        }
        if (!func->arguments().empty()) {
            _output->append(", ");
        }
    }

    if (func->arguments().size() > 0) {
        for (auto it = func->arguments().begin(); it < (func->arguments().end() - 1); it++) {
            const Field* field = it->get();
            genTypeRepr(field->type().get(), field->name());
            _output->append(", ");
        }
        genTypeRepr(func->arguments().back()->type().get(), func->arguments().back()->name());
    }

    _output->append(");\n");
}

void HeaderGen::appendFieldVec(const std::vector<Rc<Type>>& fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    std::size_t i = 1;
    for (const Rc<Type>& type : fields) {
        _output->appendIndent(1);
        genTypeRepr(type.get(), "_" + std::to_string(i));
        _output->append(";\n");
        i++;
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void HeaderGen::appendFieldList(const FieldList* fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    for (const Rc<Field>& field : *fields) {
        _output->appendIndent(1);
        genTypeRepr(field->type().get(), field->name());
        _output->append(";\n");
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void HeaderGen::appendStruct(const StructType* type)
{
    appendFieldList(type->fields().get(), type->name());
}

void HeaderGen::appendEnum(const EnumType* type)
{
    _output->appendTagHeader("enum");

    for (const auto& pair : type->constants()) {
        _output->appendIndent(1);
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(pair.second->name().toStdString());
        if (pair.second->isUserSet()) {
            _output->append(" = ");
            _output->append(std::to_string(pair.second->value()));
        }
        _output->append(",\n");
    }

    _output->appendTagFooter(type->name());
    _output->appendEol();
}

void HeaderGen::appendVariant(const VariantType* type)
{
    std::vector<bmcl::StringView> fieldNames;

    _output->appendTagHeader("enum");

    for (const Rc<VariantField>& field : type->fields()) {
        _output->appendIndent(1);
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type_");
        _output->append(field->name());
        _output->append(",\n");
    }

    _output->append("} ");
    _output->appendModPrefix();
    _output->append(type->name());
    _output->append("Type;\n");
    _output->appendEol();

    for (const Rc<VariantField>& field : type->fields()) {
        switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                const std::vector<Rc<Type>>& types = static_cast<const TupleVariantField*>(field.get())->types();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                appendFieldVec(types, name);
                break;
            }
            case VariantFieldKind::Struct: {
                const FieldList* fields = static_cast<const StructVariantField*>(field.get())->fields().get();
                std::string name = field->name().toStdString();
                name.append(type->name().begin(), type->name().end());
                appendFieldList(fields, name);
                break;
            }
        }
    }

    _output->appendTagHeader("struct");
    _output->append("    union {\n");

    for (const Rc<VariantField>& field : type->fields()) {
        if (field->variantFieldKind() == VariantFieldKind::Constant) {
            continue;
        }
        _output->append("        ");
        _output->appendModPrefix();
        _output->append(field->name());
        _output->append(type->name());
        _output->appendSpace();
        _output->appendWithFirstLower(field->name());
        _output->append(type->name());
        _output->append(";\n");
    }

    _output->append("    } data;\n");
    _output->appendIndent(1);
    _output->appendModPrefix();
    _output->append(type->name());
    _output->append("Type");
    _output->append(" type;\n");

    _output->appendTagFooter(type->name());
    _output->appendEol();
}

void HeaderGen::appendCommonIncludePaths()
{
    _output->appendInclude("stdbool.h");
    _output->appendInclude("stddef.h");
    _output->appendInclude("stdint.h");
    _output->appendEol();
}

template <typename T, typename F>
void HeaderGen::genHeaderWithTypeDecl(const T* type, F&& declGen)
{
    startIncludeGuard(type);
    appendIncludesAndFwdsForType(type);
    appendCommonIncludePaths();
    (this->*declGen)(type);
    appendImplBlockIncludes(type);
    appendImplFunctionPrototypes(type);
    appendSerializerFuncPrototypes(type);
    endIncludeGuard(type);
}

bool HeaderGen::visitEnumType(const EnumType* type)
{
    genHeaderWithTypeDecl(type, &HeaderGen::appendEnum);
    return false;
}

bool HeaderGen::visitStructType(const StructType* type)
{
    genHeaderWithTypeDecl(type, &HeaderGen::appendStruct);
    return false;
}

bool HeaderGen::visitVariantType(const VariantType* type)
{
    genHeaderWithTypeDecl(type, &HeaderGen::appendVariant);
    return false;
}
}
