/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/TypeNameGen.h"
#include "decode/core/StringBuilder.h"
#include "decode/ast/Decl.h"

#include <bmcl/StringView.h>

namespace decode {

TypeNameGen::TypeNameGen(SrcBuilder* dest)
    : _output(dest)
{
}

TypeNameGen::~TypeNameGen()
{
}

bool TypeNameGen::visitBuiltinType(const BuiltinType* type)
{
    _output->appendWithFirstUpper(BuiltinType::renderedTypeName(type->builtinTypeKind()));
    return false;
}

bool TypeNameGen::visitArrayType(const ArrayType* type)
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

bool TypeNameGen::visitDynArrayType(const DynArrayType* type)
{
    _output->append("DynArrayOf");
    traverseType(type->elementType());
    _output->append("MaxSize");
    _output->appendNumericValue(type->maxSize());
    return false;
}

bool TypeNameGen::visitGenericInstantiationType(const GenericInstantiationType* type)
{
    if (type->moduleName() != "core") {
        _output->appendWithFirstUpper(type->moduleName());
    }
    _output->append(type->genericName().toStdString());
    for (const Type* t : type->substitutedTypesRange()) {
        ascendTypeOnce(t);
    }
    return false;
}

bool TypeNameGen::visitFunctionType(const FunctionType* type)
{
    _output->append("Fn");
    for (const Field* field : type->argumentsRange()) {
        genTypeName(field->type());
    }
    if (type->selfArgument().isSome()) {
        _output->append("SelfArg");
        switch (type->selfArgument().unwrap()) {
        case SelfArgument::Reference:
            _output->append("Reference");
            break;
        case SelfArgument::MutReference:
            _output->append("MutReference");
            break;
        case SelfArgument::Value:
            _output->append("Value");
            break;
        }
    }
    if (type->hasReturnValue()) {
        _output->append("Rv");
        genTypeName(type->returnValue().unwrap());
    }
    return false;
}

bool TypeNameGen::appendTypeName(const NamedType* type)
{
    if (type->moduleName() != "core") {
        _output->appendWithFirstUpper(type->moduleName());
    }
    _output->appendWithFirstUpper(type->name());
    return false;
}

void TypeNameGen::genTypeName(const Type* type)
{
    traverseType(type);
}
}
