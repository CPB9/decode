#include "decode/generator/SourceGen.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

//TODO: refact

SourceGen::SourceGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output)
    : _output(output)
    , _typeReprGen(reprGen)
    , _inlineSer(reprGen, output)
    , _inlineDeser(reprGen, output)
{
}

SourceGen::~SourceGen()
{
}

void SourceGen::appendIncludes(const NamedType* type)
{
    StringBuilder path = type->moduleName().toStdString();
    path.append('/');
    path.append(type->name());
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
}

void SourceGen::appendEnumSerializer(const EnumType* type)
{
    appendSerializerFuncDecl(type);
    _output->append("\n{\n");

    _output->append("    switch(*self) {\n");
    for (const auto& pair : type->constants()) {
        _output->append("    case ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(pair.second->name());
        _output->append(":\n");
    }
    _output->append("        break;\n");
    _output->append("    default:\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n    ");
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVarint(dest, (int64_t)*self)");
    });
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");

}

void SourceGen::appendEnumDeserializer(const EnumType* type)
{
    appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->appendModPrefix();
    _output->appendVarDecl(type->name(), "result");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output->append("    switch(value) {\n");
    for (const auto& pair : type->constants()) {
        _output->append("    case ");
        _output->appendNumericValue(pair.first);
        _output->append(":\n");
        _output->append("        result = ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("_");
        _output->append(pair.second->name());
        _output->append(";\n");
        _output->append("        break;\n");
    }
    _output->append("    default:\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");

    _output->append("    *self = result;\n");
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendStructSerializer(const StructType* type)
{
    InlineSerContext ctx;
    appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    StringBuilder argName("self->");

    for (const Rc<Field>& field : *type->fields()) {
        argName.append(field->name().begin(), field->name().end());
        _inlineSer.inspect(field->type().get(), ctx, argName.view());
        argName.resize(6);
    }

    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendStructDeserializer(const StructType* type)
{
    InlineSerContext ctx;
    appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    StringBuilder argName("self->");

    for (const Rc<Field>& field : *type->fields()) {
        argName.append(field->name().begin(), field->name().end());
        _inlineDeser.inspect(field->type().get(), ctx, argName.view());
        argName.resize(6);
    }

    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendVariantSerializer(const VariantType* type)
{
    appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonWriter_WriteVaruint(dest, (uint64_t)self->type)");
    });

    _output->append("    switch(self->type) {\n");
    StringBuilder argName("self->data.");
    for (const Rc<VariantField>& field : type->fields()) {
        _output->append("    case ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(": {\n");

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

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");

    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendVariantDeserializer(const VariantType* type)
{
    appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    _output->appendIndent(1);
    _output->appendVarDecl("int64_t", "value");
    _output->appendIndent(1);
    _output->appendWithTryMacro([](SrcBuilder* output) {
        output->append("PhotonReader_ReadVarint(src, &");
        output->append("value");
        output->append(")");
    });

    _output->append("    switch(value) {\n");
    std::size_t i = 0;
    StringBuilder argName("self->data.");
    for (const Rc<VariantField>& field : type->fields()) {
        _output->append("    case ");
        _output->appendNumericValue(i);
        i++;
        _output->append(": {\n");
        _output->append("        self->type = ");
        _output->appendModPrefix();
        _output->append(type->name());
        _output->append("Type");
        _output->append("_");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n");

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

        _output->append("        break;\n");
        _output->append("    }\n");
    }
    _output->append("    default:\n");
    _output->append("        return PhotonError_InvalidValue;\n");
    _output->append("    }\n");

    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendSliceSerializer(const SliceType* type)
{
    appendSerializerFuncDecl(type);
    _output->append("\n{\n");
    InlineSerContext ctx;
    _output->appendLoopHeader(ctx, "self->size");
    InlineSerContext lctx = ctx.indent();
    _inlineSer.inspect(type->elementType().get(), lctx, "self->data[a]");
    _output->append("    }\n");
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

void SourceGen::appendSliceDeserializer(const SliceType* type)
{
    appendDeserializerFuncDecl(type);
    _output->append("\n{\n");
    InlineSerContext ctx;
    _output->appendLoopHeader(ctx, "self->size");
    InlineSerContext lctx = ctx.indent();
    _inlineDeser.inspect(type->elementType().get(), lctx, "self->data[a]");
    _output->append("    }\n");
    _output->append("    return PhotonError_Ok;\n");
    _output->append("}\n");
}

template <typename T, typename F>
void SourceGen::genSource(const T* type, F&& serGen, F&& deserGen)
{
    appendIncludes(type);
    _output->appendEol();
    (this->*serGen)(type);
    _output->appendEol();
    (this->*deserGen)(type);
    _output->appendEol();
}

bool SourceGen::visitSliceType(const SliceType* type)
{
    StringBuilder path("_slices_/");
    path.append(TypeNameGen::genTypeNameAsString(type));
    _output->appendLocalIncludePath(path.view());
    _output->appendLocalIncludePath("core/Try");
    _output->appendEol();
    appendSliceSerializer(type);
    _output->appendEol();
    appendSliceDeserializer(type);
    _output->appendEol();
    return false;
}

bool SourceGen::visitEnumType(const EnumType* type)
{
    genSource(type, &SourceGen::appendEnumSerializer, &SourceGen::appendEnumDeserializer);
    return false;
}

bool SourceGen::visitStructType(const StructType* type)
{
    genSource(type, &SourceGen::appendStructSerializer, &SourceGen::appendStructDeserializer);
    return false;
}

bool SourceGen::visitVariantType(const VariantType* type)
{
    genSource(type, &SourceGen::appendVariantSerializer, &SourceGen::appendVariantDeserializer);
    return false;
}

void SourceGen::genTypeSource(const Type* type)
{
    traverseType(type);
}
}
