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
class StructValueNode;
class ValueInfoCache;
class Ast;

template <typename T>
using GcInterfaceResult = bmcl::Result<Rc<T>, std::string>;

class CoreGcInterface : public RefCountable {
public:
    ~CoreGcInterface();

    static GcInterfaceResult<CoreGcInterface> create(const Device* dev);

    const BuiltinType* u64Type() const;
    const BuiltinType* f64Type() const;
    const ValueInfoCache* cache() const;

private:
    CoreGcInterface();

    Rc<const BuiltinType> _u64Type;
    Rc<const BuiltinType> _f64Type;
    Rc<const ValueInfoCache> _cache;
};

class WaypointGcInterface : public RefCountable {
public:
    ~WaypointGcInterface();

    static GcInterfaceResult<WaypointGcInterface> create(const Device* dev, const CoreGcInterface* coreIface);

    bool encodeVec3(const Vec3& vec, bmcl::MemWriter* dest) const;
    bool decodeVec3(bmcl::MemReader* src, Vec3* dest) const;

private:
    WaypointGcInterface();

    Rc<const CoreGcInterface> _coreIface;
    Rc<const Ast> _navModule;
    Rc<const ValueInfoCache> _cache;
    Rc<const StructType> _latLonStruct;
    Rc<const StructType> _posStruct;
    Rc<const StructType> _vec3Struct;
    mutable Rc<StructValueNode> _vec3Node;
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
