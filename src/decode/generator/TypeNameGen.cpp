/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeNameGen.h"
#include "decode/parser/Decl.h"

#include <bmcl/StringView.h>

namespace decode {

TypeNameGen::TypeNameGen(StringBuilder* dest)
    : _output(dest)
{
}

static bmcl::StringView builtinToName(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        return "USize";
    case BuiltinTypeKind::ISize:
        return "ISize";
    case BuiltinTypeKind::Varuint:
        return "Varuint";
    case BuiltinTypeKind::Varint:
        return "Varint";
    case BuiltinTypeKind::U8:
        return "U8";
    case BuiltinTypeKind::I8:
        return "I8";
    case BuiltinTypeKind::U16:
        return "U16";
    case BuiltinTypeKind::I16:
        return "I16";
    case BuiltinTypeKind::U32:
        return "U32";
    case BuiltinTypeKind::I32:
        return "I32";
    case BuiltinTypeKind::U64:
        return "U64";
    case BuiltinTypeKind::I64:
        return "I64";
    case BuiltinTypeKind::F32:
        return "F32";
    case BuiltinTypeKind::F64:
        return "F64";
    case BuiltinTypeKind::Bool:
        return "Bool";
    case BuiltinTypeKind::Void:
        return "Void";
    }

    return nullptr;
}

inline bool TypeNameGen::visitBuiltinType(const BuiltinType* type)
{
    _output->append(builtinToName(type));
    return false;
}

inline bool TypeNameGen::visitArrayType(const ArrayType* type)
{
    _output->append("ArrOf");
    return true;
}

bool TypeNameGen::visitReferenceType(const ReferenceType* type)
{
    if (type->isMutable()) {
        _output->append("Mut");
    }
    switch (type->referenceKind()) {
    case ReferenceKind::Pointer:
        _output->append("PtrTo");
        break;
    case ReferenceKind::Reference:
        _output->append("RefTo");
        break;
    }
    return true;
}

inline bool TypeNameGen::visitSliceType(const SliceType* type)
{
    _output->append("SliceOf");
    return true;
}

inline bool TypeNameGen::appendTypeName(const NamedType* type)
{
    _output->appendWithFirstUpper(type->moduleName());
    _output->append(type->name());
    return false;
}

void TypeNameGen::genTypeName(const Type* type)
{
    traverseType(type);
}

std::string TypeNameGen::genTypeNameAsString(const Type* type)
{
    StringBuilder output;
    TypeNameGen gen(&output);
    gen.genTypeName(type);
    return std::move(output.result());
}
}
