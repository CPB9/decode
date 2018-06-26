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
#include "decode/generator/InlineFieldInspector.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/IncludeGen.h"

#include <bmcl/StringView.h>

namespace decode {

GcTypeGen::GcTypeGen(SrcBuilder* output)
    : _output(output)
    , _typeInspector(output)
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
    _output->append("photongen::");
    _output->append(type->moduleName());
    _output->append("::");
    _output->append(type->name());
    _output->append("::");
    _output->append(constant->name());
}

void GcTypeGen::generateHeader(const GenericInstantiationType* type)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("cstddef");
    _output->appendInclude("cstdint");
    _output->appendInclude("cstdbool");
    _output->appendInclude("vector");
    _output->appendEol();

    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("bmcl/Buffer.h");
    _output->appendInclude("photon/model/CoderState.h");
    _output->appendEol();

    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(type);

    if (type->genericType()->moduleName() == "core" && type->genericName() == "Option") {
        TypeReprGen gen(_output);
        const Type* inner = *type->substitutedTypesRange().begin();
        _output->append("inline bool photongenSerialize");
        TypeNameGen nameGen(_output);
        nameGen.genTypeName(type);
        _output->append("(const photongen::core::Option<");
        gen.genGcTypeRepr(inner);
        _output->append(">& self, bmcl::Buffer* dest, photon::CoderState* state)\n"
                        "{\n"
                        "    dest->writeVarInt(self.isSome());\n");
        InlineSerContext ctx;
        _typeInspector.inspect<false, true>(inner, ctx, "self.unwrap()");
        _output->append("    return true;\n"
                    "}\n\n");

        _output->append("inline bool photongenDeserialize");
        nameGen.genTypeName(type);
        _output->append("(photongen::core::Option<");
        gen.genGcTypeRepr(inner);
        _output->append(">* self, bmcl::MemReader* src, photon::CoderState* state)\n"
                        "{\n"
                        "    int64_t isSome;\n"
                        "    if (!src->readVarInt(&isSome)) {\n"
                        "        return false;\n"
                        "    }\n"
                        "    if (isSome) {\n"
                        "        self->emplace();\n");
        ctx = ctx.indent();
        _typeInspector.inspect<false, false>(inner, ctx, "self->unwrap()");
        _output->append("        return true;\n"
                        "    }\n"
                        "    self->clear();\n"
                        "    return true;\n"
                        "}\n");

        return;
    }

    appendSerPrefix(type, bmcl::None);
    _output->append("return true;}\n\n");

    appendDeserPrefix(type, bmcl::None);
    _output->append("return true;}\n\n");
}

void GcTypeGen::generateHeader(const NamedType* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::DynArray:
    case TypeKind::Function:
    case TypeKind::Imported:
    case TypeKind::Alias: {
        generateAlias(type->asAlias());
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
            default:
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

void GcTypeGen::appendSerPrefix(const Type* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix)
{
    TypeReprGen reprGen(_output);
    if (parent.isSome()) {
        appendTemplatePrefix(parent);
    }
    _output->append(bmcl::StringView(prefix));
    _output->append(" bool photongenSerialize");
    TypeNameGen nameGen(_output);
    nameGen.genTypeName(type);
    _output->append("(const ");
    reprGen.genGcTypeRepr(type);
    _output->append("& self, bmcl::Buffer* dest, photon::CoderState* state)\n{\n");
}

void GcTypeGen::appendDeserPrefix(const Type* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix)
{
    appendDeserPrototype(type, parent, prefix);
    _output->append("\n{\n");
}

void GcTypeGen::appendDeserPrototype(const Type* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix)
{
    TypeReprGen reprGen(_output);
    if (parent.isSome()) {
        appendTemplatePrefix(parent);
    }
    _output->append(bmcl::StringView(prefix));
    _output->append(" bool photongenDeserialize");
    TypeNameGen nameGen(_output);
    nameGen.genTypeName(type);
    _output->append("(");
    reprGen.genGcTypeRepr(type);
    _output->append("* self, bmcl::MemReader* src, photon::CoderState* state)");
}

void GcTypeGen::generateEnum(const EnumType* type, bmcl::OptionPtr<const GenericType> parent)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("bmcl/Buffer.h");
    _output->appendInclude("photon/model/CoderState.h");
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
    endNamespace();

    //ser
    appendSerPrefix(type, parent);

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
                    "dest->writeVarInt((int64_t)self);\n"
                    "    return true;\n}\n\n");

    //deser
    appendDeserPrefix(type, parent);
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
    _output->appendInclude("bmcl/Buffer.h");
    _output->appendInclude("photon/model/CoderState.h");
    _output->appendEol();

    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(type);

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

            auto appendPrefix = [this](const Field* field, bmcl::StringView init, char c) {
                _output->append("        ");
                _output->append(c);
                _output->append(" _");
                _output->append(field->name());
                _output->append('(');
                _output->append(init);
                _output->append(")\n");
            };

            if (isNull) {
                if (t->isBuiltin()) {
                    appendPrefix(field, "0", c);
                } else if (t->isReference()) {
                    appendPrefix(field, "nullptr", c);
                } else {
                    continue;
                }
            } else {
                appendPrefix(field, field->name(), c);
            }
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
            return t;
        case TypeKind::GenericParameter:
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
        assert(false);
        BMCL_UNREACHABLE();
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

        Rc<const Type> t = wrapType(field->type(), false);
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
            t = wrapType(field->type(), false);
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
    InlineSerContext ctx;
    if (parent.isNone()) {
        appendSerPrefix(serType, parent);
        builder.assign("self.");
        for (const Field* field : type->fieldsRange()) {
            builder.append(field->name());
            builder.append("()");
            _typeInspector.inspect<false, true>(field->type(), ctx, builder.view());
            builder.resize(5);
        }
        _output->append("    return true;\n}\n\n");
    }

    InlineStructInspector structInspector(_output, "self->_");
    appendDeserPrefix(serType, parent);
    structInspector.inspect<false, false>(type->fieldsRange(), &_typeInspector);
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
    //TODO: replace with ptr comparison
    if (type->moduleName() == "core" && type->name() == "Option") {
        _output->append("#pragma once\n\n"
                        "#include <photon/model/CoderState.h>\n\n"
                        "#include <bmcl/Option.h>\n"
                        "#include <bmcl/Buffer.h>\n"
                        "#include <bmcl/MemReader.h>\n\n"
                        "namespace photongen {\nnamespace core {\n\n"
                        "template <typename T>\n"
                        "using Option = bmcl::Option<T>;\n"
                        "}\n}\n\n");
        return;
    }

    const NamedType* serType = type;
    if (parent.isSome()) {
        serType = parent.unwrap();
    }
    _output->appendPragmaOnce();
    _output->appendEol();

    _output->appendInclude("cstddef");
    _output->appendInclude("cstdint");
    _output->appendInclude("cstdbool");
    _output->appendInclude("photon/model/CoderState.h");
    _output->appendInclude("bmcl/AlignedUnion.h");
    _output->appendInclude("bmcl/Buffer.h");
    _output->appendInclude("bmcl/MemReader.h");
    _output->appendEol();

    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(type);

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
            _output->appendWithFirstUpper(field->name());
            _output->append(" {\n");
            std::size_t i = 1;
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
            _output->appendWithFirstUpper(field->name());
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

    _output->append("    ");
    _output->appendWithFirstUpper(type->name());
    _output->append("()\n    {\n");
    if (type->fieldsRange().size() != 0) {
        const VariantField* field = *type->fieldsBegin();
        _output->append("        _kind = Kind::");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n");
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            _output->append("        _data.emplace<");
            _output->appendWithFirstUpper(field->name());
            _output->append(">();\n");
        }
    }
    _output->append("    }\n\n");

    _output->append("    ~");
    _output->appendWithFirstUpper(type->name());
    _output->append("()\n    {\n        destruct(); \n    }\n\n");

    _output->append("    Kind kind() const\n    {\n        return _kind;\n    }\n\n");

    bool hasNonConstantFields = false;
    for (const VariantField* field : type->fieldsRange()) {
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            hasNonConstantFields = true;
            break;
        }
    }

    for (const VariantField* field : type->fieldsRange()) {
        _output->append("    bool is");
        _output->appendWithFirstUpper(field->name());
        _output->append("() const\n    {\n        return _kind == Kind::");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n    }\n\n");
    }

    auto genAsMethod = [this](const VariantField* field, bool isConst) {
        if (field->variantFieldKind() == VariantFieldKind::Constant) {
            return;
        }
        _output->append("    ");
        if (isConst) {
            _output->append("const ");
        }
        _output->appendWithFirstUpper(field->name());
        _output->append("& as");
        _output->appendWithFirstUpper(field->name());
        _output->append("()");
        if (isConst) {
            _output->append(" const");
        }
        _output->append("\n    {\n        assert(_kind == Kind::");
        _output->appendWithFirstUpper(field->name());
        _output->append(");\n        return _data.as<");
        _output->appendWithFirstUpper(field->name());
        _output->append(">();\n    }\n\n");
    };

    for (const VariantField* field : type->fieldsRange()) {
        genAsMethod(field, true);
        genAsMethod(field, false);
    }

    for (const VariantField* field : type->fieldsRange()) {
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            _output->append("    template <typename... A>\n");
        }
        _output->append("    void emplace");
        _output->appendWithFirstUpper(field->name());
        _output->append("(");
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            _output->append("A&&... args");
        }
        _output->append(")\n    {\n        destruct();\n        _kind = Kind::");
        _output->appendWithFirstUpper(field->name());
        _output->append(";\n");
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            _output->append("        _data.emplace<");
            _output->appendWithFirstUpper(field->name());
            _output->append(">(std::forward<A>(args)...);\n");
        }
        _output->append("    }\n\n");
    }

    _output->append("private:\n");

    _output->append("    void destruct()\n    {\n        switch (_kind) {\n");
    for (const VariantField* field : type->fieldsRange()) {
        _output->append("        case Kind::");
        _output->appendWithFirstUpper(field->name());
        if (field->variantFieldKind() != VariantFieldKind::Constant) {
            _output->append(":\n            _data.destruct<");
            _output->appendWithFirstUpper(field->name());
            _output->append(">();\n            break;\n");
        } else {
            _output->append( ":\n            break;\n");
        }
    }
    _output->append("        }\n    }\n\n");

    _output->append("    Kind _kind;\n");
    if (hasNonConstantFields) {
        _output->append("    bmcl::AlignedUnion<");
        bool needComma = false;
        for (const VariantField* field : type->fieldsRange()) {
            if (field->variantFieldKind() != VariantFieldKind::Constant) {
                if (needComma) {
                    _output->append(", ");
                }
                _output->appendWithFirstUpper(field->name());
                needComma = true;
            }
        }
        _output->append("> _data;\n");
    }

    _output->append("};\n");
    endNamespace();
    _output->appendEol();

    if (parent.isNone()) {
        InlineSerContext ctx;
        ctx = ctx.indent();
        appendSerPrefix(serType, parent);
        _output->append("    dest->writeVarInt((std::int64_t)self.kind());\n");
        _output->append("    switch (self.kind()) {\n");
        for (const VariantField* field : type->fieldsRange()) {
            fieldName.clear();
            fieldName.append("self.as");
            fieldName.appendWithFirstUpper(field->name());
            fieldName.append("().");
            std::size_t nameSize = fieldName.size();

            _output->append("    case photongen::");
            _output->append(type->moduleName());
            _output->append("::");
            _output->appendWithFirstUpper(type->name());
            _output->append("::Kind::");
            _output->appendWithFirstUpper(field->name());
            _output->append(":\n");
            switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                std::size_t i = 1;
                for (const Type* t : field->asTupleField()->typesRange()) {
                    fieldName.append("_");
                    fieldName.appendNumericValue(i);
                    _typeInspector.inspect<false, true>(t, ctx, fieldName.view());
                    i++;
                    fieldName.resize(nameSize);
                }
                break;
            }
            case VariantFieldKind::Struct:
                for (const Field* f : field->asStructField()->fieldsRange()) {
                    fieldName.append(f->name());
                    _typeInspector.inspect<false, true>(f->type(), ctx, fieldName.view());
                    fieldName.resize(nameSize);
                }
                break;
            }
            _output->append("        break;\n");
        }
        _output->append("    default:\n        state->setError(\"Could not serialize variant `");
        _output->append(type->moduleName());
        _output->append("::");
        _output->appendWithFirstUpper(type->name());
        _output->append("` with invalid kind (\" + std::to_string((std::int64_t)self.kind()) + \")\");\n        return false;\n");
        _output->append("    }\n");
        _output->append("    return true;\n}\n\n");

        //deser
        appendDeserPrefix(serType, parent);

        _output->append("    int64_t value;\n    if (!src->readVarInt(&value)) {\n"
                    "        state->setError(\"Not enough data to deserialize variant `");
        appendFullTypeName(type);
        _output->append("`\");\n        return false;\n    }\n");

        _output->append("    switch (value) {\n");
        std::size_t enumIndex = 0;
        for (const VariantField* field : type->fieldsRange()) {
            fieldName.clear();
            fieldName.append("self->as");
            fieldName.appendWithFirstUpper(field->name());
            fieldName.append("().");
            std::size_t nameSize = fieldName.size();

            _output->append("    case ");
            _output->appendNumericValue(enumIndex);
            _output->append(":\n");
            _output->append("        self->emplace");
            _output->appendWithFirstUpper(field->name());
            _output->append("();\n");
            switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                std::size_t i = 1;
                for (const Type* t : field->asTupleField()->typesRange()) {
                    fieldName.append("_");
                    fieldName.appendNumericValue(i);
                    _typeInspector.inspect<false, false>(t, ctx, fieldName.view());
                    i++;
                    fieldName.resize(nameSize);
                }
                break;
            }
            case VariantFieldKind::Struct:
                for (const Field* f : field->asStructField()->fieldsRange()) {
                    fieldName.append(f->name());
                    _typeInspector.inspect<false, false>(f->type(), ctx, fieldName.view());
                    fieldName.resize(nameSize);
                }
                break;
            }
            _output->append("        return true;\n");
            enumIndex++;
        }
        _output->append("    }\n");
        _output->append("    state->setError(\"Could not deserialize variant `");
        _output->append(type->moduleName());
        _output->append("::");
        _output->appendWithFirstUpper(type->name());
        _output->append("`, got invalid kind (\" + std::to_string(value) + \")\");\n    return false;\n}\n\n");
    }
}

void GcTypeGen::generateAlias(const AliasType* type)
{
    _output->appendPragmaOnce();
    _output->appendInclude("vector");
    _output->appendInclude("bmcl/Buffer.h");
    _output->appendInclude("bmcl/MemReader.h");
    _output->appendInclude("photon/model/CoderState.h");
    IncludeGen includeGen(_output);
    includeGen.genGcIncludePaths(type);
    beginNamespace(type->moduleName());
    _output->append("using ");
    _output->appendWithFirstUpper(type->name());
    _output->append(" = ");
    TypeReprGen gen(_output);
    gen.genGcTypeRepr(type->alias());
    _output->append(";\n");
    endNamespace();
    _output->appendEol();

    InlineSerContext ctx;
    appendSerPrefix(type, bmcl::None);
    _typeInspector.inspect<false, true>(type->alias(), ctx, "self");
    _output->append("    return true;\n}\n\n");
    appendDeserPrefix(type, bmcl::None);
    _typeInspector.inspect<false, false>(type->alias(), ctx, "(*self)");
    _output->append("    return true;\n}\n\n");
}

}
