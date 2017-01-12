#pragma once

#include "decode/core/Rc.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Ast.h"

namespace decode {

//TODO: traverse impl functions

template<class T> struct Ptr { typedef T* type; };
template<class T> struct ConstPtr { typedef const T* type; };

template <template <typename> class P, typename T, typename R>
typename P<T>::type ptrCast(const R* type)
{
    return static_cast<typename P<T>::type>(type);
}

template <template <typename> class P, typename T, typename R>
typename P<T>::type ptrCast(R* type)
{
    return static_cast<typename P<T>::type>(type);
}

template <typename B, template <typename> class P>
class AstVisitorBase {
public:
    void traverseType(typename P<Type>::type type);
    void traverseComponentParameters(typename P<Component>::type comp);

    void traverseBuiltinType(typename P<BuiltinType>::type builtin);
    void traverseArrayType(typename P<ArrayType>::type array);
    void traverseSliceType(typename P<SliceType>::type slice);
    void traverseReferenceType(typename P<ReferenceType>::type ref);
    void traverseFunctionType(typename P<FunctionType>::type fn);
    void traverseEnumType(typename P<EnumType>::type enumeration);
    void traverseStructType(typename P<StructType>::type str);
    void traverseVariantType(typename P<VariantType>::type variant);
    void traverseImportedType(typename P<ImportedType>::type u);

    void traverseVariantField(typename P<VariantField>::type field);
    void traverseConstantVariantField(typename P<ConstantVariantField>::type field);
    void traverseTupleVariantField(typename P<TupleVariantField>::type field);
    void traverseStructVariantField(typename P<StructVariantField>::type field);

    B& base();

    constexpr bool shouldFollowImportedType() const;

protected:
    void ascendTypeOnce(typename P<Type>::type type);

    bool visitBuiltinType(typename P<BuiltinType>::type builtin);
    bool visitArrayType(typename P<ArrayType>::type array);
    bool visitSliceType(typename P<SliceType>::type slice);
    bool visitReferenceType(typename P<ReferenceType>::type ref);
    bool visitFunctionType(typename P<FunctionType>::type fn);
    bool visitEnumType(typename P<EnumType>::type enumeration);
    bool visitStructType(typename P<StructType>::type str);
    bool visitVariantType(typename P<VariantType>::type variant);
    bool visitImportedType(typename P<ImportedType>::type u);

    bool visitVariantField(typename P<VariantField>::type field);
    bool visitConstantVariantField(typename P<ConstantVariantField>::type field);
    bool visitTupleVariantField(typename P<TupleVariantField>::type field);
    bool visitStructVariantField(typename P<StructVariantField>::type field);
};

template <typename B, template <typename> class P>
inline B& AstVisitorBase<B, P>::base()
{
    return *static_cast<B*>(this);
}

template <typename B, template <typename> class P>
constexpr bool AstVisitorBase<B, P>::shouldFollowImportedType() const
{
    return false;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitFunctionType(typename P<FunctionType>::type fn)
{
    (void)fn;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitBuiltinType(typename P<BuiltinType>::type builtin)
{
    (void)builtin;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitArrayType(typename P<ArrayType>::type array)
{
    (void)array;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitSliceType(typename P<SliceType>::type slice)
{
    (void)slice;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitEnumType(typename P<EnumType>::type enumeration)
{
    (void)enumeration;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitStructType(typename P<StructType>::type str)
{
    (void)str;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitVariantType(typename P<VariantType>::type variant)
{
    (void)variant;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitReferenceType(typename P<ReferenceType>::type ref)
{
    (void)ref;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitImportedType(typename P<ImportedType>::type u)
{
    (void)u;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitVariantField(typename P<VariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitConstantVariantField(typename P<ConstantVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitTupleVariantField(typename P<TupleVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
inline bool AstVisitorBase<B, P>::visitStructVariantField(typename P<StructVariantField>::type field)
{
    (void)field;
    return true;
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::ascendTypeOnce(typename P<Type>::type type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        base().visitBuiltinType(builtin);
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        base().visitReferenceType(ref);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        base().visitArrayType(array);
        break;
    }
    case TypeKind::Slice: {
        typename P<SliceType>::type ref = ptrCast<P, SliceType>(type);
        base().visitSliceType(ref);
        break;
    }
    case TypeKind::Function: {
        typename P<FunctionType>::type fn = ptrCast<P, FunctionType>(type);
        base().visitFunctionType(fn);
        break;
    }
    case TypeKind::Enum: {
        typename P<EnumType>::type en = ptrCast<P, EnumType>(type);
        base().visitEnumType(en);
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        base().visitStructType(str);
        break;
    }
    case TypeKind::Variant: {
        typename P<VariantType>::type variant = ptrCast<P, VariantType>(type);
        base().visitVariantType(variant);
        break;
    }
    case TypeKind::Imported: {
        typename P<ImportedType>::type u = ptrCast<P, ImportedType>(type);
        base().visitImportedType(u);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseBuiltinType(typename P<BuiltinType>::type builtin)
{
    if (!base().visitBuiltinType(builtin)) {
        return;
    }
    //TODO: visit builtin types
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseReferenceType(typename P<ReferenceType>::type ref)
{
    if (!base().visitReferenceType(ref)) {
        return;
    }
    traverseType(ref->pointee().get());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseArrayType(typename P<ArrayType>::type array)
{
    if (!base().visitArrayType(array)) {
        return;
    }
    traverseType(array->elementType().get());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseSliceType(typename P<SliceType>::type slice)
{
    if (!base().visitSliceType(slice)) {
        return;
    }
    traverseType(slice->elementType().get());
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseFunctionType(typename P<FunctionType>::type fn)
{
    if (!base().visitFunctionType(fn)) {
        return;
    }
    if (fn->returnValue().isSome()) {
        traverseType(fn->returnValue().unwrap().get());
    }
    for (const Rc<Field>& field : fn->arguments()) {
        traverseType(field->type().get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseEnumType(typename P<EnumType>::type en)
{
    if (!base().visitEnumType(en)) {
        return;
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseStructType(typename P<StructType>::type str)
{
    if (!base().visitStructType(str)) {
        return;
    }
    Rc<FieldList> fieldList = str->fields();
    for (const Rc<Field>& field : *fieldList) {
        traverseType(field->type().get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseConstantVariantField(typename P<ConstantVariantField>::type field)
{
    if (!base().visitConstantVariantField(field)) {
        return;
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseTupleVariantField(typename P<TupleVariantField>::type field)
{
    if (!base().visitTupleVariantField(field)) {
        return;
    }
    for (const Rc<Type>& t : field->types()) {
        traverseType(t.get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseStructVariantField(typename P<StructVariantField>::type field)
{
    if (!base().visitStructVariantField(field)) {
        return;
    }
    Rc<FieldList> fieldList = field->fields();
    for (const Rc<Field>& field : *fieldList) {
        traverseType(field->type().get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseVariantField(typename P<VariantField>::type field)
{
    if (!base().visitVariantField(field)) {
        return;
    }
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant: {
        typename P<ConstantVariantField>::type f = ptrCast<P, ConstantVariantField>(field);
        traverseConstantVariantField(f);
        break;
    }
    case VariantFieldKind::Tuple: {
        typename P<TupleVariantField>::type f = ptrCast<P, TupleVariantField>(field);
        traverseTupleVariantField(f);
        break;
    }
    case VariantFieldKind::Struct: {
        typename P<StructVariantField>::type f = ptrCast<P, StructVariantField>(field);
        traverseStructVariantField(f);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseVariantType(typename P<VariantType>::type variant)
{
    if (!base().visitVariantType(variant)) {
        return;
    }
    for (const Rc<VariantField>& field : variant->fields()) {
        traverseVariantField(field.get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseImportedType(typename P<ImportedType>::type u)
{
    if (!base().visitImportedType(u)) {
        return;
    }
    if (base().shouldFollowImportedType()) {
        traverseType(u->link().get());
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseType(typename P<Type>::type type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin: {
        typename P<BuiltinType>::type builtin = ptrCast<P, BuiltinType>(type);
        traverseBuiltinType(builtin);
        break;
    }
    case TypeKind::Reference: {
        typename P<ReferenceType>::type ref = ptrCast<P, ReferenceType>(type);
        traverseReferenceType(ref);
        break;
    }
    case TypeKind::Array: {
        typename P<ArrayType>::type array = ptrCast<P, ArrayType>(type);
        traverseArrayType(array);
        break;
    }
    case TypeKind::Slice: {
        typename P<SliceType>::type slice = ptrCast<P, SliceType>(type);
        traverseSliceType(slice);
        break;
    }
    case TypeKind::Function: {
        typename P<FunctionType>::type fn = ptrCast<P, FunctionType>(type);
        traverseFunctionType(fn);
        break;
    }
    case TypeKind::Enum: {
        typename P<EnumType>::type en = ptrCast<P, EnumType>(type);
        traverseEnumType(en);
        break;
    }
    case TypeKind::Struct: {
        typename P<StructType>::type str = ptrCast<P, StructType>(type);
        traverseStructType(str);
        break;
    }
    case TypeKind::Variant: {
        typename P<VariantType>::type variant = ptrCast<P, VariantType>(type);
        traverseVariantType(variant);
        break;
    }
    case TypeKind::Imported: {
        typename P<ImportedType>::type u = ptrCast<P, ImportedType>(type);
        traverseImportedType(u);
        break;
    }
    }
}

template <typename B, template <typename> class P>
void AstVisitorBase<B, P>::traverseComponentParameters(typename P<Component>::type comp)
{
    if (comp->parameters().isSome()) {
        for (const Rc<Field>& field : *comp->parameters().unwrap()->fields()) {
            traverseType(field->type().get());
        }
    }
}

template <typename B>
using AstVisitor = AstVisitorBase<B, Ptr>;

template <typename B>
using ConstAstVisitor = AstVisitorBase<B, ConstPtr>;
}
