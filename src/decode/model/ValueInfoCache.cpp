/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ValueInfoCache.h"
#include "decode/ast/Type.h"
#include "decode/parser/Package.h"
#include "decode/ast/AstVisitor.h"
#include "decode/generator/StringBuilder.h"
#include "decode/core/Foreach.h"

#include <bmcl/Logging.h>

namespace decode {

StrIndexCache::StrIndexCache()
{
}

StrIndexCache::~StrIndexCache()
{
    for (bmcl::StringView view : _arrayIndexes) {
        delete [] view.data();
    }
}

// vector<string> when relocating copies strings instead of moving for some reason
// using allocated const char* instead
void StrIndexCache::updateIndexes(std::size_t newSize)
{
    _arrayIndexes.reserve(newSize);
    for (std::size_t i = _arrayIndexes.size(); i < newSize; i++) {
        char* buf = (char*)alloca(sizeof(std::size_t) * 4);
        int stringSize = std::sprintf(buf, "%zu", i);
        assert(stringSize > 0);
        char* allocatedBuf  = new char[stringSize];
        std::memcpy(allocatedBuf, buf, stringSize);
        _arrayIndexes.push_back(bmcl::StringView(allocatedBuf, stringSize));
    }
}

bmcl::StringView StrIndexCache::arrayIndex(std::size_t idx)
{
    if (idx >= _arrayIndexes.size()) {
        updateIndexes(idx + 1);
    }
    return _arrayIndexes[idx];
}

static void buildNamedTypeName(const NamedType* type, StringBuilder* dest)
{
    dest->append(type->moduleName());
    dest->append("::");
    dest->append(type->name());
}

static bool buildTypeName(const Type* type, StringBuilder* dest)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        const BuiltinType* builtin = type->asBuiltin();
        switch (builtin->builtinTypeKind()) {
        case BuiltinTypeKind::USize:
            dest->append("usize");
            return true;
        case BuiltinTypeKind::ISize:
            dest->append("isize");
            return true;
        case BuiltinTypeKind::Varuint:
            dest->append("varuint");
            return true;
        case BuiltinTypeKind::Varint:
            dest->append( "varint");
            return true;
        case BuiltinTypeKind::U8:
            dest->append( "u8");
            return true;
        case BuiltinTypeKind::I8:
            dest->append( "i8");
            return true;
        case BuiltinTypeKind::U16:
            dest->append( "u16");
            return true;
        case BuiltinTypeKind::I16:
            dest->append( "i16");
            return true;
        case BuiltinTypeKind::U32:
            dest->append( "u32");
            return true;
        case BuiltinTypeKind::I32:
            dest->append( "i32");
            return true;
        case BuiltinTypeKind::U64:
            dest->append( "u64");
            return true;
        case BuiltinTypeKind::I64:
            dest->append( "i64");
            return true;
        case BuiltinTypeKind::F32:
            dest->append( "f32");
            return true;
        case BuiltinTypeKind::F64:
            dest->append( "f64");
            return true;
        case BuiltinTypeKind::Bool:
            dest->append( "bool");
            return true;
        case BuiltinTypeKind::Void:
            dest->append("void");
            return true;
        case BuiltinTypeKind::Char:
            dest->append("char");
            return true;
        }
        assert(false);
        break;
    }
    case TypeKind::Reference: {
        const ReferenceType* ref = type->asReference();
        if (ref->referenceKind() == ReferenceKind::Pointer) {
            if (ref->isMutable()) {
                dest->append("*mut ");
            } else {
                dest->append("*const ");
            }
        } else {
            if (ref->isMutable()) {
                dest->append("&mut ");
            } else {
                dest->append('&');
            }
        }
        buildTypeName(ref->pointee(), dest);
        return true;
    }
    case TypeKind::Array: {
        const ArrayType* array = type->asArray();
        dest->append('[');
        buildTypeName(array->elementType(), dest);
        dest->append("; ");
        dest->appendNumericValue(array->elementCount());
        dest->append(']');
        return true;
    }
    case TypeKind::DynArray: {
        const DynArrayType* dynArray = type->asDynArray();
        dest->append("&[");
        buildTypeName(dynArray->elementType(), dest);
        dest->append("; ");
        dest->appendNumericValue(dynArray->maxSize());
        dest->append(']');
        return true;
    }
    case TypeKind::Function: {
        const FunctionType* func = type->asFunction();
        dest->append("&Fn(");
        foreachList(func->argumentsRange(), [dest](const Field* arg){
            dest->append(arg->name());
            dest->append(": ");
            buildTypeName(arg->type(), dest);
        }, [dest](const Field*){
            dest->append(", ");
        });
        bmcl::OptionPtr<const Type> rv = func->returnValue();
        if (rv.isSome()) {
            dest->append(") -> ");
            buildTypeName(rv.unwrap(), dest);
        } else {
            dest->append(')');
        }
        return true;
    }
    case TypeKind::Enum:
        buildNamedTypeName(type->asEnum(), dest);
        return true;
    case TypeKind::Struct:
        buildNamedTypeName(type->asStruct(), dest);
        return true;
    case TypeKind::Variant:
        buildNamedTypeName(type->asVariant(), dest);
        return true;
    case TypeKind::Imported:
        buildNamedTypeName(type->asImported(), dest);
        return true;
    case TypeKind::Alias:
        buildNamedTypeName(type->asAlias(), dest);
        return true;
    case TypeKind::Generic:
        return false;
    case TypeKind::GenericInstantiation: {
        const GenericInstantiationType* t = type->asGenericInstantiation();
        dest->append(t->moduleName());
        dest->append("::");
        dest->append(t->genericName());
        dest->append("<");
        foreachList(t->substitutedTypesRange(), [dest](const Type* type) {
            buildTypeName(type, dest);
        }, [dest](const Type* type) {
            dest->append(", ");
        });
        dest->append(">");
        return false;
    }
    case TypeKind::GenericParameter:
        assert(false);
        return false;
    }
    assert(false);
}

class NameBuilder : public ConstAstVisitor<NameBuilder> {
public:
    void collectNames(const Package* package, ValueInfoCache::MapType* dest)
    {
        _dest = dest;
        for (const Ast* ast : package->modules()) {
            for (const Type* type : ast->typesRange()) {
                traverseType(type);
            }
        }
    }

    bool visitType(const Type* type)
    {
        bool rv = true;
        auto pair = _dest->emplace(type, std::string());
        if (pair.second) {
            StringBuilder b;
            rv = buildTypeName(type, &b);
            pair.first->second = std::move(b.result());
            if (type->isGenericInstantiation()) {
                _dest->emplace(type->asGenericInstantiation()->instantiatedType(), pair.first->second);
            }
        }
        return rv;
    }

private:
    ValueInfoCache::MapType* _dest;
};

ValueInfoCache::ValueInfoCache(const Package* package)
{
    NameBuilder b;
    b.collectNames(package, &_names);
}

ValueInfoCache::~ValueInfoCache()
{
}

bmcl::StringView ValueInfoCache::nameForType(const Type* type) const
{
    auto it = _names.find(type);
    if (it == _names.end()) {
        return bmcl::StringView("UNITIALIZED");
    }
    return it->second;
}
}
