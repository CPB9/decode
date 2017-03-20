#include "decode/model/ValueNode.h"
#include "decode/model/Value.h"
#include "decode/core/Try.h"
#include "decode/core/Foreach.h"
#include "decode/generator/StringBuilder.h"
#include "decode/parser/Type.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>

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
        foreachList(func->arguments(), [dest](const Rc<Field>& arg){
            dest->append(arg->name());
            dest->append(": ");
            buildTypeName(arg->type(), dest);
        }, [dest](const Rc<Field>& arg){
            dest->append(", ");
        });
        if (func->returnValue().isSome()) {
            dest->append(") -> ");
            buildTypeName(func->returnValue().unwrap().get(), dest);
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

ValueNode::ValueNode(Node* parent)
    : Node(parent)
{
}

ValueNode::~ValueNode()
{
}

bmcl::StringView ValueNode::name() const
{
    return _name;
}

static Rc<ValueNode> createNodefromType(const Type* type, Node* parent)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        return BuiltinValueNode::fromType(type->asBuiltin(), parent);
    case TypeKind::Reference:
        return new ReferenceValueNode(type->asReference(), parent);
    case TypeKind::Array:
        return new ArrayValueNode(type->asArray(), parent);
    case TypeKind::Slice:
        return new SliceValueNode(type->asSlice(), parent);
    case TypeKind::Function:
        return new FunctionValueNode(type->asFunction(), parent);
    case TypeKind::Enum:
        return new EnumValueNode(type->asEnum(), parent);
    case TypeKind::Struct:
        return new StructValueNode(type->asStruct(), parent);
    case TypeKind::Variant:
        return new VariantValueNode(type->asVariant(), parent);
    case TypeKind::Imported:
        return ValueNode::fromType(type->asImported()->link(), parent);
    case TypeKind::Alias:
        return ValueNode::fromType(type->asAlias()->alias(), parent);
    }
    assert(false);
    return nullptr;
}

Rc<ValueNode> ValueNode::fromType(const Type* type, Node* parent)
{
    Rc<ValueNode> node = createNodefromType(type, parent);
    StringBuilder b;
    buildTypeName(type, &b);
    node->_name = std::move(b.result());
    return node;
}

ContainerValueNode::ContainerValueNode(Node* parent)
    : ValueNode(parent)
{
}

ContainerValueNode::ContainerValueNode(std::size_t count, const Type* type, Node* parent)
    : ValueNode(parent)
{
    _values.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        _values.push_back(ValueNode::fromType(type, parent));
    }
}

ContainerValueNode::~ContainerValueNode()
{
    for (const Rc<ValueNode>& v : _values) {
        v->setParent(nullptr);
    }
}

bool ContainerValueNode::isContainerValue() const
{
    return true;
}

bool ContainerValueNode::isInitialized() const
{
    for (const Rc<ValueNode>& v : _values) {
        if (!v->isInitialized()) {
            return false;
        }
    }
    return true;
}

bool ContainerValueNode::encode(bmcl::MemWriter* dest) const
{
    for (const Rc<ValueNode>& value : _values) {
        TRY(value->encode(dest));
    }
    return true;
}

bool ContainerValueNode::decode(bmcl::MemReader* src)
{
    for (const Rc<ValueNode>& value : _values) {
        TRY(value->decode(src));
    }
    return true;
}

const std::vector<Rc<ValueNode>> ContainerValueNode::values() const
{
    return _values;
}

std::size_t ContainerValueNode::numChildren() const
{
    return _values.size();
}

bmcl::Option<std::size_t> ContainerValueNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_values, node);
}

Node* ContainerValueNode::childAt(std::size_t idx)
{
    return childAtGeneric(_values, idx);
}

ArrayValueNode::ArrayValueNode(const ArrayType* type, Node* parent)
    : ContainerValueNode(type->elementCount(), type->elementType(), parent)
    , _type(type)
{
}

ArrayValueNode::~ArrayValueNode()
{
}

const Type* ArrayValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> ArrayValueNode::fixedSize() const
{
    return _type->elementCount();
}

SliceValueNode::SliceValueNode(const SliceType* type, Node* parent)
    : ContainerValueNode(parent)
    , _type(type)
{
}

SliceValueNode::~SliceValueNode()
{
}

bool SliceValueNode::encode(bmcl::MemWriter* dest) const
{
    TRY(dest->writeVarUint(_values.size()));
    return ContainerValueNode::encode(dest);
}

bool SliceValueNode::decode(bmcl::MemReader* src)
{
    uint64_t size;
    TRY(src->readVarUint(&size));
    resize(size);
    return ContainerValueNode::decode(src);
}

const Type* SliceValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> SliceValueNode::fixedSize() const
{
    return bmcl::None;
}

void SliceValueNode::resize(std::size_t size)
{
    if (size > _values.size()) {
        _values.reserve(size);
        for (std::size_t i = 0; i < (size - _values.size()); i++) {
            _values.push_back(ValueNode::fromType(_type->elementType(), this));
        }
    } else {
        _values.resize(size);
    }
}

StructValueNode::StructValueNode(const StructType* type, Node* parent)
    : ContainerValueNode(parent)
    , _type(type)
{
    _values.reserve(type->fields()->size());
    for (const Rc<Field>& field : *type->fields()) {
        _values.push_back(ValueNode::fromType(field->type(), this));
    }
}

StructValueNode::~StructValueNode()
{
}

const Type* StructValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> StructValueNode::fixedSize() const
{
    return _type->fields()->size();
}

bmcl::OptionPtr<ValueNode> StructValueNode::nodeWithName(bmcl::StringView name)
{
    bmcl::OptionPtr<const Field> field = _type->fieldWithName(name);
    if (field.isNone()) {
        return bmcl::None;
    }
    for (const Rc<ValueNode>& node : _values) {
        if (node->type() == field->type()) {
            return node.get();
        }
    }
    return bmcl::None;
}

VariantValueNode::VariantValueNode(const VariantType* type, Node* parent)
    : ContainerValueNode(parent)
    , _type(type)
    , _currentId(0)
{
}

VariantValueNode::~VariantValueNode()
{
}

bool VariantValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_values.empty()) {
        //TODO: report error
        return false;
    }
    TRY(dest->writeVarUint(_currentId));
    return ContainerValueNode::encode(dest);
}

bool VariantValueNode::decode(bmcl::MemReader* src)
{
    uint64_t id;
    TRY(src->readVarUint(&id));
    if (id >= _type->fields().size()) {
        //TODO: report error
        return false;
    }
    const Rc<VariantField>& field = _type->fields()[id];
    switch (field->variantFieldKind()) {
    case VariantFieldKind::Constant:
        break;
    case VariantFieldKind::Tuple: {
        const TupleVariantField* tField = static_cast<const TupleVariantField*>(field.get());
        std::size_t size = tField->types().size();
        _values.resize(size);
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(tField->types()[i].get(), this);
        }
        TRY(ContainerValueNode::decode(src));
        break;
    }
    case VariantFieldKind::Struct: {
        const StructVariantField* sField = static_cast<const StructVariantField*>(field.get());
        std::size_t size = sField->fields()->size();
        _values.resize(size);
        for (std::size_t i = 0; i < size; i++) {
            _values[i] = ValueNode::fromType(sField->fields()->at(i)->type(), this);
        }
        TRY(ContainerValueNode::decode(src));
        break;
    }
    default:
        assert(false);
    }

    _currentId = id;
    return true;
}

const Type* VariantValueNode::type() const
{
    return _type.get();
}

bmcl::Option<std::size_t> VariantValueNode::fixedSize() const
{
    return bmcl::None;
}

NonContainerValueNode::NonContainerValueNode(Node* parent)
    : ValueNode(parent)
{
}

bool NonContainerValueNode::isContainerValue() const
{
    return false;
}

AddressValueNode::AddressValueNode(Node* parent)
    : NonContainerValueNode(parent)
{
}

AddressValueNode::~AddressValueNode()
{
}

bool AddressValueNode::encode(bmcl::MemWriter* dest) const
{
    //TODO: get target word size
    if (_address.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < 8) {
        return false;
    }
    dest->writeUint64Le(_address.unwrap());
    return true;
}

bool AddressValueNode::decode(bmcl::MemReader* src)
{
    //TODO: get target word size
    if (src->readableSize() < 8) {
        return false;
    }
    _address.unwrap() = src->readUint64Le();
    return true;
}

bool AddressValueNode::isInitialized() const
{
    return _address.isSome();
}

Value AddressValueNode::value() const
{
    if (_address.isSome()) {
        return Value::makeUnsigned(_address.unwrap());
    }
    return Value::makeNone();
}

ReferenceValueNode::ReferenceValueNode(const ReferenceType* type, Node* parent)
    : AddressValueNode(parent)
    , _type(type)
{
}

ReferenceValueNode::~ReferenceValueNode()
{
}

const Type* ReferenceValueNode::type() const
{
    return _type.get();
}

FunctionValueNode::FunctionValueNode(const FunctionType* type, Node* parent)
    : AddressValueNode(parent)
    , _type(type)
{
}

FunctionValueNode::~FunctionValueNode()
{
}

const Type* FunctionValueNode::type() const
{
    return _type.get();
}

EnumValueNode::EnumValueNode(const EnumType* type, Node* parent)
    : NonContainerValueNode(parent)
    , _type(type)
{
}

EnumValueNode::~EnumValueNode()
{
}

bool EnumValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_currentId.isSome()) {
        TRY(dest->writeVarUint(_currentId.unwrap()));
        return true;
    }
    //TODO: report error
    return false;
}

bool EnumValueNode::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    _currentId.unwrap() = value;
    return true;
}

bool EnumValueNode::isInitialized() const
{
    return _currentId.isSome();
}

const Type* EnumValueNode::type() const
{
    return _type.get();
}

BuiltinValueNode::BuiltinValueNode(const BuiltinType* type, Node* parent)
    : NonContainerValueNode(parent)
    , _type(type)
{
}

BuiltinValueNode::~BuiltinValueNode()
{
}

Rc<BuiltinValueNode> BuiltinValueNode::fromType(const BuiltinType* type, Node* parent)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        //TODO: check target ptr size
        return new NumericValueNode<std::uint64_t>(type, parent);
    case BuiltinTypeKind::ISize:
        //TODO: check target ptr size
        return new NumericValueNode<std::int64_t>(type, parent);
    case BuiltinTypeKind::Varint:
        return new VarintValueNode(type, parent);
    case BuiltinTypeKind::Varuint:
        return new VaruintValueNode(type, parent);
    case BuiltinTypeKind::U8:
        return new NumericValueNode<std::uint8_t>(type, parent);
    case BuiltinTypeKind::I8:
        return new NumericValueNode<std::int8_t>(type, parent);
    case BuiltinTypeKind::U16:
        return new NumericValueNode<std::uint16_t>(type, parent);
    case BuiltinTypeKind::I16:
        return new NumericValueNode<std::int16_t>(type, parent);
    case BuiltinTypeKind::U32:
        return new NumericValueNode<std::uint32_t>(type, parent);
    case BuiltinTypeKind::I32:
        return new NumericValueNode<std::int32_t>(type, parent);
    case BuiltinTypeKind::U64:
        return new NumericValueNode<std::uint64_t>(type, parent);
    case BuiltinTypeKind::I64:
        return new NumericValueNode<std::int64_t>(type, parent);
    case BuiltinTypeKind::Bool:
        //TODO: make bool value
        return new NumericValueNode<std::uint8_t>(type, parent);
    case BuiltinTypeKind::Void:
        assert(false);
        return nullptr;
    }
    assert(false);
    return nullptr;
}

const Type* BuiltinValueNode::type() const
{
    return _type.get();
}

template <typename T>
NumericValueNode<T>::NumericValueNode(const BuiltinType* type, Node* parent)
    : BuiltinValueNode(type, parent)
{
}

template <typename T>
NumericValueNode<T>::~NumericValueNode()
{
}

template <typename T>
bool NumericValueNode<T>::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    if (dest->writableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    dest->writeType<T>(bmcl::htole<T>(_value.unwrap()));
    return true;
}

template <typename T>
bool NumericValueNode<T>::decode(bmcl::MemReader* src)
{
    if (src->readableSize() < sizeof(T)) {
        //TODO: report error
        return false;
    }
    _value.emplace(bmcl::letoh<T>(src->readType<T>()));
    return true;
}

template <typename T>
bool NumericValueNode<T>::isInitialized() const
{
    return _value.isSome();
}

template class NumericValueNode<std::uint8_t>;
template class NumericValueNode<std::int8_t>;
template class NumericValueNode<std::uint16_t>;
template class NumericValueNode<std::int16_t>;
template class NumericValueNode<std::uint32_t>;
template class NumericValueNode<std::int32_t>;
template class NumericValueNode<std::uint64_t>;
template class NumericValueNode<std::int64_t>;

VarintValueNode::VarintValueNode(const BuiltinType* type, Node* parent)
    : BuiltinValueNode(type, parent)
{
}

VarintValueNode::~VarintValueNode()
{
}

bool VarintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarInt(_value.unwrap());
}

bool VarintValueNode::decode(bmcl::MemReader* src)
{
    int64_t value;
    TRY(src->readVarInt(&value));
    _value.emplace(value);
    return true;
}

bool VarintValueNode::isInitialized() const
{
    return _value.isSome();
}

VaruintValueNode::VaruintValueNode(const BuiltinType* type, Node* parent)
    : BuiltinValueNode(type, parent)
{
}

VaruintValueNode::~VaruintValueNode()
{
}

bool VaruintValueNode::encode(bmcl::MemWriter* dest) const
{
    if (_value.isNone()) {
        //TODO: report error
        return false;
    }
    return dest->writeVarUint(_value.unwrap());
}

bool VaruintValueNode::decode(bmcl::MemReader* src)
{
    uint64_t value;
    TRY(src->readVarUint(&value));
    _value.emplace(value);
    return true;
}

bool VaruintValueNode::isInitialized() const
{
    return _value.isSome();
}
}
