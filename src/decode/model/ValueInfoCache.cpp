/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ValueInfoCache.h"
#include "decode/parser/Type.h"
#include "decode/generator/StringBuilder.h"
#include "decode/core/Foreach.h"

#include <bmcl/Logging.h>

namespace decode {

static void buildNamedTypeName(const NamedType* type, StringBuilder* dest)
{
    dest->append(type->moduleName());
    dest->append("::");
    dest->append(type->name());
}

static void buildTypeName(const Type* type, StringBuilder* dest)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        const BuiltinType* builtin = type->asBuiltin();
        switch (builtin->builtinTypeKind()) {
        case BuiltinTypeKind::USize:
            dest->append("usize");
            return;
        case BuiltinTypeKind::ISize:
            dest->append("isize");
            return;
        case BuiltinTypeKind::Varuint:
            dest->append("varuint");
            return;
        case BuiltinTypeKind::Varint:
            dest->append( "varint");
            return;
        case BuiltinTypeKind::U8:
            dest->append( "u8");
            return;
        case BuiltinTypeKind::I8:
            dest->append( "i8");
            return;
        case BuiltinTypeKind::U16:
            dest->append( "u16");
            return;
        case BuiltinTypeKind::I16:
            dest->append( "i16");
            return;
        case BuiltinTypeKind::U32:
            dest->append( "u32");
            return;
        case BuiltinTypeKind::I32:
            dest->append( "i32");
            return;
        case BuiltinTypeKind::U64:
            dest->append( "u64");
            return;
        case BuiltinTypeKind::I64:
            dest->append( "i64");
            return;
        case BuiltinTypeKind::Bool:
            dest->append( "bool");
            return;
        case BuiltinTypeKind::Void:
            dest->append("void");
            return;
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
        return;
    }
    case TypeKind::Array: {
        const ArrayType* array = type->asArray();
        dest->append('[');
        buildTypeName(array->elementType(), dest);
        dest->append(", ");
        dest->appendNumericValue(array->elementCount());
        dest->append(']');
        return;
    }
    case TypeKind::Slice: {
        const SliceType* slice = type->asSlice();
        dest->append("&[");
        buildTypeName(slice->elementType(), dest);
        dest->append(']');
        return;
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
        return;
    }
    case TypeKind::Enum:
        buildNamedTypeName(type->asEnum(), dest);
        return;
    case TypeKind::Struct:
        buildNamedTypeName(type->asStruct(), dest);
        return;
    case TypeKind::Variant:
        buildNamedTypeName(type->asVariant(), dest);
        return;
    case TypeKind::Imported:
        buildNamedTypeName(type->asImported(), dest);
        return;
    case TypeKind::Alias:
        buildNamedTypeName(type->asAlias(), dest);
        return;
    }
    assert(false);
}

ValueInfoCache::ValueInfoCache()
{
}

ValueInfoCache::~ValueInfoCache()
{
    for (bmcl::StringView view : _arrayIndexes) {
        delete [] view.data();
    }
}

// vector<string> when relocating copies strings instead of moving for some reason
// using allocated const char* instead
void ValueInfoCache::updateIndexes(std::size_t newSize) const
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

bmcl::StringView ValueInfoCache::arrayIndex(std::size_t idx) const
{
    if (idx >= _arrayIndexes.size()) {
        updateIndexes(idx + 1);
    }
    return _arrayIndexes[idx];
}

bmcl::StringView ValueInfoCache::nameForType(const Type* type) const
{
    auto pair = _names.emplace(type, std::string());
    if (pair.second) {
        StringBuilder b;
        buildTypeName(type, &b);
        pair.first->second = std::move(b.result());
    }
    return pair.first->second;
}
}
