#include "decode/groundcontrol/GcInterface.h"
#include "decode/core/Try.h"
#include "decode/ast/Type.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Field.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/parser/Project.h"
#include "decode/model/ValueNode.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/CmdNode.h"
#include "decode/model/Value.h"
#include "decode/model/Encoder.h"

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
    self->_varuintType = new BuiltinType(BuiltinTypeKind::Varuint);
    self->_usizeType = new BuiltinType(BuiltinTypeKind::USize);
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

const BuiltinType* CoreGcInterface::varuintType() const
{
    return _varuintType.get();
}

const BuiltinType* CoreGcInterface::usizeType() const
{
    return _usizeType.get();
}

const ValueInfoCache* CoreGcInterface::cache() const
{
    return _cache.get();
}

WaypointGcInterface::WaypointGcInterface(const Device* dev, const CoreGcInterface* coreIface)
    : _dev(dev)
    , _coreIface(coreIface)
    , _cache(coreIface->cache())
{
    (void)dev;
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

bmcl::Option<std::string> findCmd(const Component* comp, bmcl::StringView name, Rc<const Function>* dest)
{
    auto it = std::find_if(comp->cmdsBegin(), comp->cmdsEnd(), [name](const Function* func) {
        return func->name() == name;
    });
    if (it == comp->cmdsEnd()) {
        return "failed to find command " + wrapWithQuotes(name);
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

template <typename T>
static bmcl::Option<std::string> findVariantField(const VariantType* variantType, bmcl::StringView fieldName, Rc<const T>* dest)
{
    auto it = std::find_if(variantType->fieldsBegin(), variantType->fieldsEnd(), [&fieldName](const VariantField* field) {
        return field->name() == fieldName;
    });
    if (it == variantType->fieldsEnd()) {
        return std::string("no field with name " + wrapWithQuotes(fieldName) + " in variant " + wrapWithQuotes(variantType->name()));
    }
    if (it->variantFieldKind() != deferVariantFieldKind<T>()) {
        return std::string("variant field ") + wrapWithQuotes(fieldName) + " has invalid type";
    }

    const T* t = static_cast<const T*>(*it);
    *dest = t;
    return bmcl::None;
}

template <typename T>
static bmcl::Option<std::string> expectFieldNum(const T* container, std::size_t num)
{
    if (container->fieldsRange().size() != num) {
        return "struct " + wrapWithQuotes(container->name()) + " has invalid number of fields";
    }
    return bmcl::None;
}

template <typename T>
static bmcl::Option<std::string> expectField(const T* container, std::size_t i, bmcl::StringView fieldName, const Type* other)
{
    if (i >= container->fieldsRange().size()) {
        return "struct " + wrapWithQuotes(container->name()) + " is missing field with index " + std::to_string(i);
    }
    const Field* field = container->fieldAt(i);
    if (field->name() != fieldName) {
        return "struct " + wrapWithQuotes(container->name()) + " has invalid field name at index " + std::to_string(i);
    }
    const Type* type = field->type();
    if (!type->equals(other)) {
        return "field " + wrapWithQuotes(fieldName) + " of struct " + wrapWithQuotes(container->name()) + " is of invalid type";
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
    Rc<WaypointGcInterface> self = new WaypointGcInterface(dev, coreIface);
    GC_TRY(self->init());
    return self;
}

bmcl::Option<std::string> WaypointGcInterface::init()
{
    GC_TRY(findModule(_dev.get(), "nav", &_navModule));

    const BuiltinType* f64Type = _coreIface->f64Type();
    GC_TRY(findType<StructType>(_navModule.get(), "LatLon", &_latLonStruct));
    GC_TRY(expectFieldNum(_latLonStruct.get(), 2));
    GC_TRY(expectField(_latLonStruct.get(), 0, "latitude", f64Type));
    GC_TRY(expectField(_latLonStruct.get(), 1, "longitude", f64Type));

    GC_TRY(findType<StructType>(_navModule.get(), "Position", &_posStruct));
    GC_TRY(expectFieldNum(_posStruct.get(), 2));
    GC_TRY(expectField(_posStruct.get(), 0, "latLon", _latLonStruct.get()));
    GC_TRY(expectField(_posStruct.get(), 1, "altitude", f64Type));

    GC_TRY(findType<StructType>(_navModule.get(), "Vec3", &_vec3Struct));
    GC_TRY(expectFieldNum(_vec3Struct.get(), 3));
    GC_TRY(expectField(_vec3Struct.get(), 0, "x", f64Type));
    GC_TRY(expectField(_vec3Struct.get(), 1, "y", f64Type));
    GC_TRY(expectField(_vec3Struct.get(), 2, "z", f64Type));

    const BuiltinType* varuintType = _coreIface->varuintType();
    GC_TRY(findType<StructType>(_navModule.get(), "FormationEntry", &_formationEntryStruct));
    GC_TRY(expectFieldNum(_formationEntryStruct.get(), 2));
    GC_TRY(expectField(_formationEntryStruct.get(), 0, "pos", _vec3Struct.get()));
    GC_TRY(expectField(_formationEntryStruct.get(), 1, "id", varuintType));

    GC_TRY(findType<VariantType>(_navModule.get(), "Action", &_actionVariant));
    Rc<const ConstantVariantField> noneField;
    GC_TRY(findVariantField<ConstantVariantField>(_actionVariant.get(), "None", &noneField));
    Rc<const ConstantVariantField> loopField;
    GC_TRY(findVariantField<ConstantVariantField>(_actionVariant.get(), "Loop", &loopField));
    Rc<const ConstantVariantField> snakeField;
    GC_TRY(findVariantField<ConstantVariantField>(_actionVariant.get(), "Snake", &snakeField));
    Rc<const ConstantVariantField> reynoldsField;
    GC_TRY(findVariantField<ConstantVariantField>(_actionVariant.get(), "Reynolds", &reynoldsField));
    Rc<const StructVariantField> sleepField;
    GC_TRY(findVariantField<StructVariantField>(_actionVariant.get(), "Sleep", &sleepField));
    GC_TRY(expectField(sleepField.get(), 0, "timeout", varuintType));
    Rc<const StructVariantField> formationField;
    GC_TRY(findVariantField<StructVariantField>(_actionVariant.get(), "Formation", &formationField));
    GC_TRY(expectFieldNum(formationField.get(), 1));

    const Type* formationArrayType = formationField->fieldAt(0)->type();
    if (!formationArrayType->isDynArray()) {
        return std::string("`Formation` field is not a dyn array");
    }
    if (!formationArrayType->asDynArray()->elementType()->equals(_formationEntryStruct.get())) {
        return std::string("`Formation` field dyn array element is not `FormationEntry`");
    }
    _formationArrayMaxSize = formationArrayType->asDynArray()->maxSize();

    GC_TRY(findType<StructType>(_navModule.get(), "Waypoint", &_waypointStruct));
    GC_TRY(expectFieldNum(_waypointStruct.get(), 2));
    GC_TRY(expectField(_waypointStruct.get(), 0, "position", _posStruct.get()));
    GC_TRY(expectField(_waypointStruct.get(), 1, "action", _actionVariant.get()));

    if (_navModule->component().isNone()) {
        return std::string("`nav` module has no component");
    }

    _navComponent = _navModule->component().unwrap();
    const BuiltinType* usizeType = _coreIface->usizeType();
    GC_TRY(findCmd(_navComponent.get(), "beginRoute", &_beginRouteCmd));
    GC_TRY(expectFieldNum(_beginRouteCmd.get(), 1));
    GC_TRY(expectField(_beginRouteCmd.get(), 0, "routeIndex", usizeType));

    GC_TRY(findCmd(_navComponent.get(), "endRoute", &_endRouteCmd));
    GC_TRY(expectFieldNum(_endRouteCmd.get(), 1));
    GC_TRY(expectField(_endRouteCmd.get(), 0, "routeIndex", usizeType));

    GC_TRY(findCmd(_navComponent.get(), "setRoutePoint", &_setRoutePointCmd));
    GC_TRY(expectFieldNum(_setRoutePointCmd.get(), 3));
    GC_TRY(expectField(_setRoutePointCmd.get(), 0, "routeIndex", usizeType));
    GC_TRY(expectField(_setRoutePointCmd.get(), 1, "pointIndex", usizeType));
    GC_TRY(expectField(_setRoutePointCmd.get(), 2, "waypoint", _waypointStruct.get()));

    return bmcl::None;
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

bool WaypointGcInterface::beginNavCmd(const Function* cmd, Encoder* dest) const
{
    TRY(dest->writeVarUint(_navComponent->number()));
    auto it = std::find(_navComponent->cmdsBegin(), _navComponent->cmdsEnd(), cmd);
    if (it == _navComponent->cmdsEnd()) {
        //TODO: report error
        return false;
    }

    return dest->writeVarUint(std::distance(_navComponent->cmdsBegin(), it));
}

bool WaypointGcInterface::encodeBeginRouteCmd(std::uintmax_t index, Encoder* dest) const
{
    TRY(beginNavCmd(_beginRouteCmd.get(), dest));
    return dest->writeUSize(index);
}

bool WaypointGcInterface::encodeEndRouteCmd(std::uintmax_t index, Encoder* dest) const
{
    TRY(beginNavCmd(_endRouteCmd.get(), dest));
    return dest->writeUSize(index);
}

bool WaypointGcInterface::encodeSetRoutePointCmd(std::uintmax_t routeIndex, std::uintmax_t pointIndex, const Waypoint& wp, Encoder* dest) const
{
    TRY(beginNavCmd(_setRoutePointCmd.get(), dest));
    TRY(dest->writeUSize(routeIndex));
    TRY(dest->writeUSize(pointIndex));
    TRY(dest->writeF64(wp.position.latLon.latitude));
    TRY(dest->writeF64(wp.position.latLon.longitude));
    TRY(dest->writeF64(wp.position.altitude));
    switch (wp.action.kind()) {
    case WaypointActionKind::None:
        TRY(dest->writeVariantTag(0));
        break;
    case WaypointActionKind::Sleep:
        TRY(dest->writeVariantTag(1));
        TRY(dest->writeVarUint(wp.action.as<SleepWaypointAction>().timeout));
        break;
    case WaypointActionKind::Formation:
        TRY(dest->writeVariantTag(2));
        for (const FormationEntry& entry: wp.action.as<FormationWaypointAction>().entries) {
            //TODO: check max size
            TRY(dest->writeDynArraySize(wp.action.as<FormationWaypointAction>().entries.size()));
            TRY(dest->writeF64(entry.pos.x));
            TRY(dest->writeF64(entry.pos.y));
            TRY(dest->writeF64(entry.pos.z));
            TRY(dest->writeVarUint(entry.id));
        }
        break;
    case WaypointActionKind::Reynolds:
        TRY(dest->writeVariantTag(3));
        break;
    case WaypointActionKind::Snake:
        TRY(dest->writeVariantTag(4));
        break;
    case WaypointActionKind::Loop:
        TRY(dest->writeVariantTag(5));
        break;
    default:
        return false;
    }
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

const std::string& AllGcInterfaces::errors() const
{
    return _errors;
}
}
