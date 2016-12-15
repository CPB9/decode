#pragma once

#include "decode/core/Rc.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Ast.h"

namespace decode {

template<class T> struct Ptr { typedef T* type; };
template<class T> struct ConstPtr { typedef const T* type; };

template <template <typename> class P, typename T, typename R>
typename P<T>::type ptrCast(const R* type)
{
    return static_cast<typename P<T>::type>(type);
}

template <typename B, template <typename> class P>
class AstVisitorBase {
public:
    void traverseType(typename P<Type>::type type);

    B& base();

    constexpr bool shouldFollowOtherAst() const;
    constexpr bool shouldFollowOtherAstOnlyOnce() const;

protected:
    void traverseType(typename P<Type>::type type, typename P<Type>::type parent);
    void ascendTypeOnce(typename P<Type>::type type);

    bool visitBuiltin(typename P<BuiltinType>::type builtin);
    bool visitArray(typename P<ArrayType>::type array);
    bool visitSlice(typename P<SliceType>::type slice);
    bool visitReference(typename P<ReferenceType>::type ref);
    bool visitFunction(typename P<Function>::type fn);
    bool visitEnum(typename P<Enum>::type enumeration);
    bool visitStruct(typename P<StructType>::type str);
    bool visitVariant(typename P<Variant>::type variant);
    bool visitUnresolved(typename P<UnresolvedType>::type u);
};

template <typename B, template <typename> class P>
inline B& AstVisitorBase<B, P>::base()
{
    return *static_cast<B*>(this);
}

template <typename B, template <typename> class P>
constexpr bool AstVisitorBase<B, P>::shouldFollowOtherAst() const
{
    return true;
}

template <typename B, template <typename> class P>
constexpr bool AstVisitorBase<B, P>::shouldFollowOtherAstOnlyOnce() const
{
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitFunction(typename P<Function>::type fn)
{
    (void)fn;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitBuiltin(typename P<BuiltinType>::type builtin)
{
    (void)builtin;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitArray(typename P<ArrayType>::type array)
{
    (void)array;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitSlice(typename P<SliceType>::type slice)
{
    (void)slice;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitEnum(typename P<Enum>::type enumeration)
{
    (void)enumeration;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitStruct(typename P<StructType>::type str)
{
    (void)str;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitVariant(typename P<Variant>::type variant)
{
    (void)variant;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitReference(typename P<ReferenceType>::type ref)
{
    (void)ref;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitUnresolved(typename P<UnresolvedType>::type u)
{
    (void)u;
    return true;
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::ascendTypeOnce(typename P<Type>::type type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        base().visitBuiltin(builtin);
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        base().visitReference(ref);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        base().visitArray(array);
        break;
    }
    case TypeKind::Slice: {
        typename P<SliceType>::type ref = ptrCast<P, SliceType>(type);
        base().visitSlice(ref);
        break;
    }
    case TypeKind::Function: {
        typename P<Function>::type fn = ptrCast<P, Function>(type);
        base().visitFunction(fn);
        break;
    }
    case TypeKind::Enum: {
        typename P<Enum>::type en = ptrCast<P, Enum>(type);
        base().visitEnum(en);
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        base().visitStruct(str);
        break;
    }
    case TypeKind::Variant: {
        typename P<Variant>::type variant = ptrCast<P, Variant>(type);
        base().visitVariant(variant);
        break;
    }
    case TypeKind::Unresolved: {
        typename P<UnresolvedType>::type u = ptrCast<P, UnresolvedType>(type);
        base().visitUnresolved(u);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseType(typename P<Type>::type type, typename P<Type>::type parent)
{
    if (base().shouldFollowOtherAst() && !base().shouldFollowOtherAstOnlyOnce()) {
        traverseType(type);
        return;
    }

    if (type->moduleInfo() == parent->moduleInfo()) {
        traverseType(type);
        return;
    }

    if (base().shouldFollowOtherAst() && base().shouldFollowOtherAstOnlyOnce()) {
        ascendTypeOnce(type);
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseType(typename P<Type>::type type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        if (!base().visitBuiltin(builtin)) {
            return;
        }
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        if (!base().visitReference(ref)) {
            return;
        }
        traverseType(ref->pointee().get(), type);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        if (!base().visitArray(array)) {
            return;
        }
        traverseType(array->elementType().get(), type);
        break;
    }
    case TypeKind::Slice: {
        typename P<SliceType>::type ref = ptrCast<P, SliceType>(type);
        if (!base().visitSlice(ref)) {
            return;
        }
        traverseType(ref->elementType().get(), type);
        break;
    }
    case TypeKind::Function: {
        typename P<Function>::type fn = ptrCast<P, Function>(type);
        if (!base().visitFunction(fn)) {
            return;
        }
        if (fn->returnValue().isSome()) {
            traverseType(fn->returnValue().unwrap().get(), type);
        }
        for (const Rc<Field>& field : fn->arguments()) {
            traverseType(field->type().get(), type);
        }
        break;
    }
    case TypeKind::Enum: {
        typename P<Enum>::type en = ptrCast<P, Enum>(type);
        if (!base().visitEnum(en)) {
            return;
        }
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        if (!base().visitStruct(str)) {
            return;
        }
        Rc<FieldList> fieldList = str->fields();
        for (const Rc<Field>& field : *fieldList) {
            traverseType(field->type().get(), type);
        }
        break;
    }
    case TypeKind::Variant: {
        typename P<Variant>::type variant = ptrCast<P, Variant>(type);
        if (!base().visitVariant(variant)) {
            return;
        }
        for (const Rc<VariantField>& field : variant->fields()) {
            switch (field->variantFieldKind()) {
            case VariantFieldKind::Constant:
                break;
            case VariantFieldKind::Tuple: {
                const TupleVariantField* tupleField = static_cast<const TupleVariantField*>(field.get());
                for (const Rc<Type>& t : tupleField->types()) {
                    traverseType(t.get(), type);
                }
                break;
            }
            case VariantFieldKind::Struct: {
                Rc<FieldList> fieldList = static_cast<const StructVariantField*>(field.get())->fields();
                for (const Rc<Field>& field : *fieldList) {
                    traverseType(field->type().get(), type);
                }
                break;
            }
            }
        }
        break;
    }
    case TypeKind::Unresolved: {
        typename P<UnresolvedType>::type u = ptrCast<P, UnresolvedType>(type);
        if (!base().visitUnresolved(u)) {
            return;
        }
        break;
    }
    }
}

template <typename B>
using AstVisitor = AstVisitorBase<B, Ptr>;

template <typename B>
using ConstAstVisitor = AstVisitorBase<B, ConstPtr>;
}
