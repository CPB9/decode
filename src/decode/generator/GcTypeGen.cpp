/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/GcTypeGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/core/Foreach.h"
#include "decode/ast/Type.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/InlineTypeInspector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/IncludeGen.h"

#include <bmcl/StringView.h>

namespace decode {

GcTypeGen::GcTypeGen(SrcBuilder* output)
    : _output(output)
{
}

GcTypeGen::~GcTypeGen()
{
}

void GcTypeGen::appendFullTypeName(const NamedType* type)
{
    _output->append(type->moduleName());
    _output->append("::");
    _output->append(type->name());
}

void GcTypeGen::appendEnumConstantName(const EnumType* type, const EnumConstant* constant)
{
    _output->append(type->name());
    _output->append("::");
    _output->append(constant->name());
}

void GcTypeGen::generateHeader(const TopLevelType* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::DynArray:
    case TypeKind::Function:
    case TypeKind::Imported:
    case TypeKind::Alias: {
        _output->appendPragmaOnce();
        _output->appendInclude("vector");
        IncludeGen includeGen(_output);
        TypeDependsCollector coll;
        TypeDependsCollector::Depends deps;
        coll.collect(type, &deps);
        includeGen.genGcIncludePaths(&deps);
        beginNamespace(type->asAlias()->moduleName());
        _output->append("using ");
        _output->append(type->asAlias()->name());
        _output->append(" = ");
        TypeReprGen gen(_output);
        gen.genGcTypeRepr(type->asAlias()->alias());
        _output->append(";\n");
        endNamespace();
        break;
    }
    case TypeKind::GenericInstantiation:
    case TypeKind::GenericParameter:
        return;
    case TypeKind::Enum:
        generateEnum(type->asEnum(), bmcl::None);
        break;
    case TypeKind::Struct:
        generateStruct(type->asStruct(), bmcl::None);
        break;
    case TypeKind::Variant:
        generateVariant(type->asVariant(), bmcl::None);
        break;
    case TypeKind::Generic: {
        const GenericType* g = type->asGeneric();
        switch (g->innerType()->resolveFinalType()->typeKind()) {
            case TypeKind::Enum:
                generateEnum(g->innerType()->asEnum(), g);
                break;
            case TypeKind::Struct:
                generateStruct(g->innerType()->asStruct(), g);
                break;
            case TypeKind::Variant:
                generateVariant(g->innerType()->asVariant(), g);
                break;
        }
        break;
    }
    }
}

void GcTypeGen::beginNamespace(bmcl::StringView modName)
{
    _output->append("namespace photongen {\nnamespace ");
    _output->append(modName);
    _output->append(" {\n\n");
}

void GcTypeGen::endNamespace()
{
    _output->append("}\n}\n");
}

void GcTypeGen::appendSerPrefix(const NamedType* type, const char* prefix)
{
    TypeReprGen reprGen(_output);
    _output->append(bmcl::StringView(prefix));
    _output->append(" bool photongenSerialize");
    _output->append("(const ");
    reprGen.genGcTypeRepr(type);
    _output->append("& self, bmcl::MemWriter* dest, decode::CoderState* state)\n{\n");
}

void GcTypeGen::appendDeserPrefix(const NamedType* type, const char* prefix)
{
    appendDeserPrototype(type, prefix);
    _output->append("\n{\n");
}

void GcTypeGen::appendDeserPrototype(const NamedType* type, const char* prefix)
{
    TypeReprGen reprGen(_output);
    _output->append(bmcl::StringView(prefix));
    _output->append(" bool photongenDeserialize");
    _output->append("(");
    reprGen.genGcTypeRepr(type);
    _output->append("* self, bmcl::MemReader* src, decode::CoderState* state)");
}

void GcTypeGen::generateEnum(const EnumType* type, bmcl::OptionPtr<const GenericType> parent)
{

    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("bmcl/MemWriter.h");
    _output->appendInclude("decode/model/CoderState.h");
    _output->appendEol();

    beginNamespace(type->moduleName());

    //type
    appendTemplatePrefix(parent);
    _output->append("enum class ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\n");

    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    ");
        _output->append(c->name());
        _output->append(" = ");
        _output->appendNumericValue(c->value());
        _output->append(",\n");
    }

    _output->append("};\n\n");

    //ser
    appendSerPrefix(type);

    _output->append("    switch(self) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        appendEnumConstantName(type, c);
        _output->append(":\n");
    }
    _output->append("        break;\n"
                    "    default:\n"
                    "        state->setError(\"Could not serialize enum `");
    appendFullTypeName(type);
    _output->append("` with invalid value (\" + std::to_string((int64_t)self) + \")\");\n"
                    "        return false;\n"
                    "    }\n    "
                    "if(!dest->writeVarInt((int64_t)self)) {\n"
                    "        state->setError(\"Not enough space to serialize enum `");
    appendFullTypeName(type);
    _output->append("`\");\n        return false;\n    }\n"
                    "    return true;\n}\n\n");

    //deser
    appendDeserPrefix(type);
    _output->append("    int64_t value;\n    if (!src->readVarInt(&value)) {\n"
                    "        state->setError(\"Not enough data to deserialize enum `");
    appendFullTypeName(type);
    _output->append("`\");\n        return false;\n    }\n");

    _output->append("    switch(value) {\n");
    for (const EnumConstant* c : type->constantsRange()) {
        _output->append("    case ");
        _output->appendNumericValue(c->value());
        _output->append(":\n        *self = ");
        appendEnumConstantName(type, c);
        _output->append(";\n        return true;\n");
    }
    _output->append("    }\n    state->setError(\"Failed to deserialize enum `");
    appendFullTypeName(type);
    _output->append("`, got invalid value (\" + std::to_string(value) + \")\");\n"
                    "    return false;\n}");

    endNamespace();
}

void GcTypeGen::generateStruct(const StructType* type, bmcl::OptionPtr<const GenericType> parent)
{
    const NamedType* serType = type;
    if (parent.isSome()) {
        serType = parent.unwrap();
    }
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("cstddef");
    _output->appendInclude("cstdint");
    _output->appendInclude("cstdbool");
    _output->appendInclude("vector");
    _output->appendEol();

    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("bmcl/MemWriter.h");
    _output->appendInclude("decode/model/CoderState.h");
    _output->appendEol();

    TypeDependsCollector coll;
    TypeDependsCollector::Depends depends;
    coll.collect(type, &depends);
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(&depends);
    if (!depends.empty()) {
        _output->appendEol();
    }

    beginNamespace(type->moduleName());

    appendTemplatePrefix(parent);
    _output->append("class ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\npublic:\n");

    _output->appendIndent();
    _output->appendWithFirstUpper(type->name());
    _output->append("()\n");

    auto constructVars = [this](const StructType* type, bool isNull) {
        char c = ':';
        for (const Field* field : type->fieldsRange()) {
            const Type* t = field->type()->resolveFinalType();
            if (!t->isBuiltin() && !t->isReference()) {
                continue;
            }

            _output->append("        ");
            _output->append(c);
            _output->append(" _");
            _output->append(field->name());
            _output->append('(');
            if (isNull) {
                if (t->isBuiltin()) {
                    _output->append('0');
                } else if (t->isReference()) {
                    _output->append("nullptr");
                }
            } else {
                _output->append(field->name());
            }
            _output->append(")\n");
            c = ',';
        }
    };
    constructVars(type, true);
    _output->append("    {\n    }\n\n");

    Rc<ReferenceType> ref = new ReferenceType(ReferenceKind::Reference, false, nullptr);
    auto wrapType = [&](const Type* t, bool isMutable) -> const Type* {
        switch (t->typeKind()) {
        case TypeKind::Builtin:
        case TypeKind::Reference:
        case TypeKind::Function:
        case TypeKind::GenericParameter:
            return t;
        case TypeKind::Array:
        case TypeKind::DynArray:
        case TypeKind::Enum:
        case TypeKind::Struct:
        case TypeKind::Variant:
        case TypeKind::Imported:
        case TypeKind::Alias:
        case TypeKind::Generic:
        case TypeKind::GenericInstantiation:
            ref->setMutable(isMutable);
            ref->setPointee(const_cast<Type*>(t));
            return ref.get();
        }
    };

    TypeReprGen gen(_output);
    _output->appendIndent();
    _output->appendWithFirstUpper(type->name());
    _output->append('(');
    foreachList(type->fieldsRange(), [&](const Field* arg) {
        const Type* t = wrapType(arg->type()->resolveFinalType(), false);
        gen.genGcTypeRepr(t, arg->name());
    }, [this](const Field*) {
         _output->append(", ");
    });

    _output->append(")\n");
    constructVars(type, false);
    _output->append("    {\n    }\n\n");

    for (const Field* field : type->fieldsRange()) {
        _output->append("    void set");
        _output->appendWithFirstUpper(field->name());
        _output->append('(');

        Rc<const Type> t = wrapType(field->type()->resolveFinalType(), false);
        gen.genGcTypeRepr(t.get(), field->name());
        _output->append(")\n    {\n        _");
        _output->append(field->name());
        _output->append(" = ");
        _output->append(field->name());
        _output->append(";\n    }\n\n");
    }

    auto appendGetter = [&](const Field* field, bool isMutable) {
        _output->append("    ");
        Rc<const Type> t = field->type()->resolveFinalType();
        if (isMutable) {
            ref->setPointee(const_cast<Type*>(t.get()));
            ref->setMutable(isMutable);
            t = ref;
        } else {
            t = wrapType(field->type()->resolveFinalType(), false);
        }
        SrcBuilder fieldName;
        fieldName.append(field->name());
        fieldName.append("()");
        gen.genGcTypeRepr(t.get(), fieldName.view());
        if (!isMutable) {
            _output->append(" const");
        }
        _output->append("\n    {\n        return _");
        _output->append(field->name());
        _output->append(";\n    }\n\n");
    };

    for (const Field* field : type->fieldsRange()) {
        appendGetter(field, false);
        //appendGetter(field, true);
    }

    _output->append("\n");
    SrcBuilder builder("_");
    for (const Field* field : type->fieldsRange()) {
        _output->appendIndent();
        builder.append(field->name());
        gen.genGcTypeRepr(field->type(), builder.view());
        builder.resize(1);
        _output->append(";\n");
    }

    _output->append("};\n\n");

    endNamespace();

    //TODO: use field inspector
    InlineTypeInspector inspector(&gen, _output);
    InlineSerContext ctx;

    appendTemplatePrefix(parent);
    appendSerPrefix(serType);
    builder.result().assign("self.");
    for (const Field* field : type->fieldsRange()) {
        builder.append(field->name());
        builder.append("()");
        inspector.inspect<false, true>(field->type(), ctx, builder.view());
        builder.resize(5);
    }
    _output->append("    return true;\n}\n\n");

    appendTemplatePrefix(parent);
    appendDeserPrefix(serType);
    builder.result().assign("self->_");
    for (const Field* field : type->fieldsRange()) {
        builder.append(field->name());
        inspector.inspect<false, false>(field->type(), ctx, builder.view());
        builder.resize(7);
    }
    _output->append("    return true;\n}\n");

}

void GcTypeGen::appendTemplatePrefix(bmcl::OptionPtr<const GenericType> parent)
{
    if (parent.isNone()) {
        return;
    }
    _output->append("template <");
    foreachList(parent->parametersRange(), [this](const GenericParameterType* type) {
        _output->append("typename ");
        _output->append(type->name());
    }, [this](const GenericParameterType*) {
        _output->append(", ");
    });
    _output->append(">\n");
}

void GcTypeGen::generateVariant(const VariantType* type, bmcl::OptionPtr<const GenericType> parent)
{
    const NamedType* serType = type;
    if (parent.isSome()) {
        serType = parent.unwrap();
    }
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("cstddef");
    _output->appendInclude("cstdint");
    _output->appendInclude("cstdbool");
    _output->appendInclude("decode/model/CoderState.h");
    _output->appendEol();

    TypeDependsCollector coll;
    TypeDependsCollector::Depends depends;
    coll.collect(type, &depends);
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(&depends);
    if (!depends.empty()) {
        _output->appendEol();
    }

    beginNamespace(type->moduleName());

    appendTemplatePrefix(parent);
    _output->append("class ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" {\npublic:\n");

    _output->append("    enum class Kind {\n");

    for (const VariantField* field : type->fieldsRange()) {
        _output->append("        ");
        _output->appendWithFirstUpper(field->name());
        _output->append(",\n");
    }
    _output->append("    };\n\n");

    TypeReprGen gen(_output);
    SrcBuilder fieldName("_");
    for (const VariantField* field : type->fieldsRange()) {
        switch (field->variantFieldKind()) {
        case VariantFieldKind::Constant:
            break;
        case VariantFieldKind::Tuple: {
            const TupleVariantField* f = field->asTupleField();
            _output->append("    struct ");
            _output->append(field->name());
            _output->append(" {\n");
            std::size_t i = 0;
            for (const Type* t : f->typesRange()) {
                fieldName.appendNumericValue(i);
                _output->append("        ");
                gen.genGcTypeRepr(t, fieldName.view());
                _output->append(";\n");
                fieldName.resize(1);
                i++;
            }
            _output->append("    };\n\n");
            break;
        }
        case VariantFieldKind::Struct:
            const StructVariantField* f = field->asStructField();
            _output->append("    struct ");
            _output->append(field->name());
            _output->append(" {\n");
            for (const Field* t : f->fieldsRange()) {
                _output->append("        ");
                gen.genGcTypeRepr(t->type(), t->name());
                _output->append(";\n");
            }
            _output->append("    };\n\n");
            break;
        }
    }

    _output->append("};\n");
    endNamespace();

    appendTemplatePrefix(parent);
    appendSerPrefix(serType);
    _output->append("return true;}\n\n");
    appendTemplatePrefix(parent);
    appendDeserPrefix(serType);
    _output->append("return true;}\n\n");
}
}
