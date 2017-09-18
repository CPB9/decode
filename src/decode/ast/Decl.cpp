/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Field.h"

#include <bmcl/Result.h>
#include <bmcl/ArrayView.h>

namespace decode {

Decl::Decl(const ModuleInfo* info, Location start, Location end)
    : _moduleInfo(info)
    , _start(start)
    , _end(end)
{
}

Decl::Decl()
{
}

Decl::~Decl()
{
}

const ModuleInfo* Decl::moduleInfo() const
{
    return _moduleInfo.get();
}

void Decl::cloneDeclTo(Decl* dest)
{
    dest->_start = _start;
    dest->_end = _end;
    dest->_moduleInfo = _moduleInfo;
}


NamedDecl::NamedDecl()
{
}

NamedDecl::~NamedDecl()
{
}

bmcl::StringView NamedDecl::name() const
{
    return _name;
}

TypeDecl::TypeDecl()
{
}

TypeDecl::~TypeDecl()
{
}

const Type* TypeDecl::type() const
{
    return _type.get();
}

Type* TypeDecl::type()
{
    return _type.get();
}

ModuleDecl::ModuleDecl(const ModuleInfo* info, Location start, Location end)
    : Decl(info, start, end)
{
}

ModuleDecl::~ModuleDecl()
{
}

bmcl::StringView ModuleDecl::moduleName() const
{
    return moduleInfo()->moduleName();
}

ImportDecl::ImportDecl(const ModuleInfo* modInfo, bmcl::StringView path)
    : _importPath(path)
    , _modInfo(modInfo)
{
}

ImportDecl::~ImportDecl()
{
}

bmcl::StringView ImportDecl::path() const
{
    return _importPath;
}

ImportDecl::Types::ConstRange ImportDecl::typesRange() const
{
    return _types;
}

ImportDecl::Types::Range ImportDecl::typesRange()
{
    return _types;
}

bool ImportDecl::addType(ImportedType* type)
{
    auto it = std::find_if(_types.begin(), _types.end(), [type](const Rc<ImportedType>& current) {
        return current->name() == type->name();
    });
    if (it != _types.end()) {
        return false;
    }
    _types.emplace_back(type);
    return true;
}

ImplBlock::ImplBlock()
{
}

ImplBlock::~ImplBlock()
{
}

ImplBlock::Functions::ConstIterator ImplBlock::functionsBegin() const
{
    return _funcs.cbegin();
}

ImplBlock::Functions::ConstIterator ImplBlock::functionsEnd() const
{
    return _funcs.cend();
}

ImplBlock::Functions::ConstRange ImplBlock::functionsRange() const
{
    return _funcs;
}

void ImplBlock::addFunction(Function* func)
{
    _funcs.emplace_back(func);
}

GenericTypeDecl::GenericTypeDecl(bmcl::StringView name, bmcl::ArrayView<Rc<GenericParameterType>> parameters, NamedType* genericType)
    : NamedRc(name)
    , _parameters(parameters.begin(), parameters.end())
    , _type(genericType)
{
}

GenericTypeDecl::~GenericTypeDecl()
{
}

bmcl::Result<Rc<NamedType>, std::string> GenericTypeDecl::instantiate(const bmcl::ArrayView<Rc<Type>> types)
{
    if (_parameters.size() != types.size()) {
        return std::string("invalid number of parameters");
    }

    Rc<Type> cloned = cloneAndSubstitute(_type.get(), types);
    if (!cloned) {
        return std::string("failed to substitute generic parameters");
    }
    return Rc<NamedType>(static_cast<NamedType*>(cloned.get()));
}

Rc<Field> GenericTypeDecl::cloneAndSubstitute(Field* field, bmcl::ArrayView<Rc<Type>> types)
{
    Rc<Type> clonedType = cloneAndSubstitute(field->type(), types);
    Rc<Field> clonedField = new Field(field->name(), clonedType.get());
    if (field->rangeAttribute().isSome()) {
        clonedField->setRangeAttribute(field->rangeAttribute().unwrap());
    }
    return clonedField;
}

Rc<VariantField> GenericTypeDecl::cloneAndSubstitute(VariantField* varField, bmcl::ArrayView<Rc<Type>> types)
{
    switch (varField->variantFieldKind()) {
    case VariantFieldKind::Constant:
        return varField;
    case VariantFieldKind::Tuple: {
        TupleVariantField* f = static_cast<TupleVariantField*>(varField);
        Rc<TupleVariantField> newField = new TupleVariantField(f->id(), f->name());
        for (Type* type : f->typesRange()) {
            Rc<Type> cloned = cloneAndSubstitute(type, types);
            newField->addType(cloned.get());
        }
        return newField;
    }
    case VariantFieldKind::Struct: {
        StructVariantField* f = static_cast<StructVariantField*>(varField);
        Rc<StructVariantField> newField = new StructVariantField(f->id(), f->name());
        for (Field* field : f->fieldsRange()) {
            Rc<Field> cloned = cloneAndSubstitute(field, types);
            newField->addField(cloned.get());
        }
        return newField;
    }
    }
}

Rc<Type> GenericTypeDecl::cloneAndSubstitute(Type* type, bmcl::ArrayView<Rc<Type>> types)
{
    switch (type->typeKind()) {
        case TypeKind::Builtin:
            return type;
        case TypeKind::Reference: {
            ReferenceType* ref = type->asReference();
            Rc<Type> cloned = cloneAndSubstitute(ref->pointee(), types);
            return new ReferenceType(ref->referenceKind(), ref->isMutable(), cloned.get());
        }
        case TypeKind::Array: {
            ArrayType* array = type->asArray();
            Rc<Type> cloned = cloneAndSubstitute(array->elementType(), types);
            return new ArrayType(array->elementCount(), cloned.get());
        }
        case TypeKind::DynArray: {
            DynArrayType* dynArray = type->asDynArray();
            Rc<Type> cloned = cloneAndSubstitute(dynArray->elementType(), types);
            return new DynArrayType(dynArray->moduleInfo(), dynArray->maxSize(), cloned.get());
        }
        case TypeKind::Function: {
            FunctionType* func = type->asFunction();
            Rc<FunctionType> newFunc = new FunctionType(func->moduleInfo());
            if (func->hasReturnValue()) {
                Rc<Type> cloned = cloneAndSubstitute(func->returnValue().unwrap(), types);
                newFunc->setReturnValue(cloned.get());
            }
            newFunc->setSelfArgument(func->selfArgument());
            for (Field* field : func->argumentsRange()) {
                Rc<Field> cloned = cloneAndSubstitute(field, types);
                newFunc->addArgument(cloned.get());
            }
            return newFunc;
        }
        case TypeKind::Enum: {
            return type;
        }
        case TypeKind::Struct: {
            StructType* structType = type->asStruct();
            Rc<StructType> newStruct = new StructType(structType->name(), structType->moduleInfo());
            for (Field* field : structType->fieldsRange()) {
                Rc<Field> cloned = cloneAndSubstitute(field, types);
                newStruct->addField(cloned.get());
            }
            return newStruct;
        }
        case TypeKind::Variant: {
            VariantType* variant = type->asVariant();
            Rc<VariantType> newVariant = new VariantType(variant->name(), variant->moduleInfo());
            for (VariantField* field : variant->fieldsRange()) {
                Rc<VariantField> cloned = cloneAndSubstitute(field, types);
                newVariant->addField(cloned.get());
            }
            return newVariant;

        }
        case TypeKind::Imported: {
            return type;
        }
        case TypeKind::Alias: {
            AliasType* imported = type->asAlias();
            Rc<Type> cloned = cloneAndSubstitute(imported->alias(), types);
            return new AliasType(imported->name(), imported->moduleInfo(), cloned.get());
        }
        case TypeKind::Generic: {
            GenericType* generic = type->asGeneric();
            Rc<Type> cloned = cloneAndSubstitute(generic->type(), types);
            return new GenericType(generic->name(), generic->substitutedTypes(), static_cast<NamedType*>(cloned.get()));
        }
        case TypeKind::GenericParameter: {
            GenericParameterType* genericParam = type->asGenericParemeter();
            auto it = std::find_if(_parameters.begin(), _parameters.end(), [genericParam](const Rc<GenericParameterType>& type) {
                return type->name() == genericParam->name();
            });
            if (it == _parameters.end()) {
                return nullptr;
            }
            std::size_t index = it - _parameters.begin();
            return types[index];
        }
    }
}
}
