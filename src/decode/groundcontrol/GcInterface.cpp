#include "decode/groundcontrol/GcInterface.h"
#include "decode/core/Try.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Field.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Project.h"
#include "decode/model/ValueNode.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/Result.h>
#include <bmcl/OptionPtr.h>

namespace decode {

CoreGcInterface::CoreGcInterface()
{
}

CoreGcInterface::~CoreGcInterface()
{
}

GcInterfaceResult<CoreGcInterface> CoreGcInterface::create(const Device* dev)
{
    (void)dev;
    Rc<CoreGcInterface> self = new CoreGcInterface;
    self->_cache = new ValueInfoCache(dev->package.get());
    //TODO: search for builtin types
    self->_u64Type = new BuiltinType(BuiltinTypeKind::U64);
    self->_f64Type = new BuiltinType(BuiltinTypeKind::F64);
    return self;
}

const BuiltinType* CoreGcInterface::u64Type() const
{
    return _u64Type.get();
}

const BuiltinType* CoreGcInterface::f64Type() const
{
    return _f64Type.get();
}

const ValueInfoCache* CoreGcInterface::cache() const
{
    return _cache.get();
}

WaypointGcInterface::WaypointGcInterface()
{
}

WaypointGcInterface::~WaypointGcInterface()
{
}

static std::string wrapWithQuotes(bmcl::StringView str)
{
    std::string rv;
    rv.reserve(str.size() + 2);
    rv.push_back('`');
    rv.append(str.begin(), str.end());
    rv.push_back('`');
    return rv;
}

bmcl::Option<std::string> findModule(const Device* dev, bmcl::StringView name, Rc<const Ast>* dest)
{
    auto it = std::find_if(dev->modules.begin(), dev->modules.end(), [name](const Rc<Ast>& module) {
        return module->moduleInfo()->moduleName() == name;
    });
    if (it == dev->modules.end()) {
        return "failed to find module " + wrapWithQuotes(name);
    }
    *dest = *it;
    return bmcl::None;
}

template <typename T>
bmcl::Option<std::string> findType(const Ast* module, bmcl::StringView typeName, Rc<const T>* dest)
{
    bmcl::OptionPtr<const NamedType> type = module->findTypeWithName(typeName);
    if (type.isNone()) {
        return "failed to find type " + wrapWithQuotes(typeName);
    }
    if (type.unwrap()->typeKind() != deferTypeKind<T>()) {
        return "failed to find type " + wrapWithQuotes(typeName);
    }
    const T* t = static_cast<const T*>(type.unwrap());
    *dest = t;
    return bmcl::None;
}

static bmcl::Option<std::string> expectBuiltinField(const StructType* structType, std::size_t i, bmcl::StringView fieldName, BuiltinTypeKind kind)
{
    if (i >= structType->fieldsRange().size()) {
        return "struct " + wrapWithQuotes(structType->name()) + " is missing field with index " + std::to_string(i);
    }
    const Field* field = structType->fieldAt(i);
    if (field->name() != fieldName) {
        return "struct " + wrapWithQuotes(structType->name()) + " has invalid field name at index " + std::to_string(i);
    }
    const Type* type = field->type();
    if (!type->isBuiltin()) {
        return "field " + wrapWithQuotes(fieldName) + " of struct " + wrapWithQuotes(structType->name()) + " is of invalid type";
    }
    if (type->asBuiltin()->builtinTypeKind() != kind) {
        return "field " + wrapWithQuotes(fieldName) + " of struct " + wrapWithQuotes(structType->name()) + " is of invalid builtin type";
    }
    return bmcl::None;
}

static bmcl::Option<std::string> expectTypedField(const StructType* structType, std::size_t i, bmcl::StringView fieldName, const Type* other)
{
    if (i >= structType->fieldsRange().size()) {
        return "struct " + wrapWithQuotes(structType->name()) + " is missing field with index " + std::to_string(i);
    }
    const Field* field = structType->fieldAt(i);
    if (field->name() != fieldName) {
        return "struct " + wrapWithQuotes(structType->name()) + " has invalid field name at index " + std::to_string(i);
    }
    const Type* type = field->type();
    if (!type->equals(other)) {
        return "field " + wrapWithQuotes(fieldName) + " of struct " + wrapWithQuotes(structType->name()) + " is of invalid type";
    }
    return bmcl::None;
}

#define GC_TRY(expr)               \
    {                              \
        auto strErr = expr;        \
        if (strErr.isSome()) {     \
            return expr.unwrap();  \
        }                          \
    }

GcInterfaceResult<WaypointGcInterface> WaypointGcInterface::create(const Device* dev, const CoreGcInterface* coreIface)
{
    Rc<WaypointGcInterface> self = new WaypointGcInterface;
    self->_coreIface = coreIface;
    self->_cache = coreIface->cache();

    GC_TRY(findModule(dev, "nav", &self->_navModule));

    GC_TRY(findType<StructType>(self->_navModule.get(), "LatLon", &self->_latLonStruct));
    GC_TRY(expectBuiltinField(self->_latLonStruct.get(), 0, "latitude", BuiltinTypeKind::F64));
    GC_TRY(expectBuiltinField(self->_latLonStruct.get(), 1, "longitude", BuiltinTypeKind::F64));

    GC_TRY(findType<StructType>(self->_navModule.get(), "Position", &self->_posStruct));
    GC_TRY(expectTypedField(self->_posStruct.get(), 0, "latLon", self->_latLonStruct.get()));
    GC_TRY(expectBuiltinField(self->_posStruct.get(), 1, "altitude", BuiltinTypeKind::F64));

    GC_TRY(findType<StructType>(self->_navModule.get(), "Vec3", &self->_vec3Struct));
    GC_TRY(expectBuiltinField(self->_vec3Struct.get(), 0, "x", BuiltinTypeKind::F64));
    GC_TRY(expectBuiltinField(self->_vec3Struct.get(), 1, "y", BuiltinTypeKind::F64));
    GC_TRY(expectBuiltinField(self->_vec3Struct.get(), 2, "z", BuiltinTypeKind::F64));

    self->_vec3Node = new StructValueNode(self->_vec3Struct.get(), self->_cache.get(), bmcl::None);

    return self;
}

template <typename T>
void setNumericValue(ValueNode* node, T value)
{
    static_cast<NumericValueNode<T>*>(node)->setRawValue(value);
}

template <typename T>
bool getNumericValue(const ValueNode* node, T* dest)
{
    auto value = static_cast<const NumericValueNode<T>*>(node)->rawValue();
    if (value.isSome()) {
        *dest = value.unwrap();
        return true;
    }
    return false;
}

bool WaypointGcInterface::encodeVec3(const Vec3& vec, bmcl::MemWriter* dest) const
{
    setNumericValue(_vec3Node->nodeAt(0), vec.x);
    setNumericValue(_vec3Node->nodeAt(1), vec.y);
    setNumericValue(_vec3Node->nodeAt(2), vec.z);
    return _vec3Node->encode(dest);
}

bool WaypointGcInterface::decodeVec3(bmcl::MemReader* src, Vec3* dest) const
{
    if(_vec3Node->decode(src)) {
        return false;
    }

    TRY(getNumericValue(_vec3Node->nodeAt(0), &dest->x));
    TRY(getNumericValue(_vec3Node->nodeAt(1), &dest->y));
    TRY(getNumericValue(_vec3Node->nodeAt(2), &dest->z));
    return true;
}

AllGcInterfaces::AllGcInterfaces(const Device* dev)
{
    auto coreIface = CoreGcInterface::create(dev);
    if (!coreIface.isOk()) {
        _errors = coreIface.unwrapErr();
        return;
    }
    _coreIface = coreIface.unwrap();
    auto waypointIface = WaypointGcInterface::create(dev, _coreIface.get());
    if (waypointIface.isOk()) {
        _waypointIface = waypointIface.unwrap();
    } else {
        _errors = waypointIface.unwrapErr();
        return;
    }
}

AllGcInterfaces::~AllGcInterfaces()
{
}

bmcl::OptionPtr<const CoreGcInterface> AllGcInterfaces::coreInterface() const
{
    return _coreIface.get();
}

bmcl::OptionPtr<const WaypointGcInterface> AllGcInterfaces::waypointInterface() const
{
    return _waypointIface.get();
}

const std::string& AllGcInterfaces::errors()
{
    return _errors;
}
}
