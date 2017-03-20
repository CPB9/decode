#include "decode/generator/TypeDefGen.h"
#include "decode/parser/Component.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

TypeDefGen::TypeDefGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
{
}

TypeDefGen::~TypeDefGen()
{
}

void TypeDefGen::genTypeDef(const Type* type)
{
    traverseType(type);
}

void TypeDefGen::genComponentDef(const Component* comp)
{
    if (comp->parameters().isNone()) {
        return;
    }
    appendFieldList(comp->parameters().unwrap().get(), bmcl::StringView::empty());
}

bool TypeDefGen::visitEnumType(const EnumType* type)
{
    appendEnum(type);
    return false;
}

bool TypeDefGen::visitStructType(const StructType* type)
{
    appendStruct(type);
    return false;
}

bool TypeDefGen::visitVariantType(const VariantType* type)
{
    appendVariant(type);
    return false;
}

bool TypeDefGen::visitSliceType(const SliceType* type)
{
    TypeNameGen gen(_output);
    _output->appendTagHeader("struct");

    _output->appendIndent(1);
    _typeReprGen->genTypeRepr(type->elementType());
    _output->append("* data;\n");

    _output->appendIndent(1);
    _output->append("size_t size;\n");

    _output->append("} Photon");
    gen.genTypeName(type);
    _output->append(";\n");
    _output->appendEol();
    return false;
}

void TypeDefGen::appendFieldVec(const std::vector<Rc<Type>>& fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    std::size_t i = 1;
    for (const Rc<Type>& type : fields) {
        _output->appendIndent(1);
        _typeReprGen->genTypeRepr(type.get(), "_" + std::to_string(i));
        _output->append(";\n");
        i++;
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendFieldList(const FieldList* fields, bmcl::StringView name)
{
    _output->appendTagHeader("struct");

    for (const Rc<Field>& field : *fields) {
        _output->appendIndent(1);
        _typeReprGen->genTypeRepr(field->type(), field->name());
        _output->append(";\n");
    }

    _output->appendTagFooter(name);
    _output->appendEol();
}

void TypeDefGen::appendStruct(const StructType* type)
{
    appendFieldList(type->fields(), type->name());
}

void TypeDefGen::appendEnum(const EnumType* type)
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

void TypeDefGen::appendVariant(const VariantType* type)
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
                const FieldList* fields = static_cast<const StructVariantField*>(field.get())->fields();
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

bool TypeDefGen::visitAliasType(const AliasType* type)
{
    _output->append("typedef ");
    const Type* link = type->alias();
    if (link->isFunction()) {
        StringBuilder typedefName = "Photon";
        typedefName.append(type->name());
        _typeReprGen->genTypeRepr(link, typedefName.result());
    } else {
        _typeReprGen->genTypeRepr(link);
        _output->append(" Photon");
        _output->append(type->name());
    }
    _output->append(";\n\n");
    return false;
}
}
