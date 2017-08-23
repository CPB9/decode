#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

struct Device;
class BuiltinType;
class StructType;
class VariantType;
class StructValueNode;
class ValueInfoCache;
class Function;
class Component;
class Ast;
class Node;

template <typename T>
using GcInterfaceResult = bmcl::Result<Rc<T>, std::string>;

class CoreGcInterface : public RefCountable {
public:
    ~CoreGcInterface();

    static GcInterfaceResult<CoreGcInterface> create(const Device* dev);

    const BuiltinType* u64Type() const;
    const BuiltinType* f64Type() const;
    const BuiltinType* varuintType() const;
    const BuiltinType* usizeType() const;
    const ValueInfoCache* cache() const;

private:
    CoreGcInterface();

    Rc<const BuiltinType> _u64Type;
    Rc<const BuiltinType> _f64Type;
    Rc<const BuiltinType> _varuintType;
    Rc<const BuiltinType> _usizeType;
    Rc<const ValueInfoCache> _cache;
};

class WaypointGcInterface : public RefCountable {
public:
    ~WaypointGcInterface();

    static GcInterfaceResult<WaypointGcInterface> create(const Device* dev, const CoreGcInterface* coreIface);

    bool encodeBeginRouteCmd(std::uintmax_t routeIndex, bmcl::MemWriter* dest) const;
    bool encodeSetRoutePointCmd(std::uintmax_t routeIndex, std::uintmax_t pointIndex, const Waypoint& wp, bmcl::MemWriter* dest) const;

private:
    bool toVec3(const Node* node, Vec3* dest) const;
    bool fromVec3(const Vec3& vec, Node* dest) const;

    WaypointGcInterface(const Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();

    Rc<const Device> _dev;
    Rc<const CoreGcInterface> _coreIface;
    Rc<const Ast> _navModule;
    Rc<const Component> _navComponent;
    Rc<const ValueInfoCache> _cache;
    Rc<const StructType> _latLonStruct;
    Rc<const StructType> _posStruct;
    Rc<const StructType> _vec3Struct;
    Rc<const StructType> _formationEntryStruct;
    Rc<const StructType> _waypointStruct;
    Rc<const VariantType> _actionVariant;
    Rc<const Function> _beginRouteCmd;
    Rc<const Function> _setRoutePointCmd;
    std::size_t _formationArrayMaxSize;
};

class AllGcInterfaces : public RefCountable {
public:
    AllGcInterfaces(const Device* dev);
    ~AllGcInterfaces();

    bmcl::OptionPtr<const CoreGcInterface> coreInterface() const;
    bmcl::OptionPtr<const WaypointGcInterface> waypointInterface() const;
    const std::string& errors();

private:
    Rc<CoreGcInterface> _coreIface;
    Rc<WaypointGcInterface> _waypointIface;
    std::string _errors;
};
}
