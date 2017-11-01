#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/GcStructs.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

struct Device;
struct AllRoutesInfo;
class BuiltinType;
class StructType;
class VariantType;
class StructValueNode;
class ValueInfoCache;
class Function;
class Component;
class Ast;
class Node;
class Encoder;
class Decoder;
class Command;
class ValueInfoCache;

template <typename T>
using GcInterfaceResult = bmcl::Result<Rc<T>, std::string>;

class CoreGcInterface : public RefCountable {
public:
    ~CoreGcInterface();

    static GcInterfaceResult<CoreGcInterface> create(const Device* dev, const ValueInfoCache* cache);

    const BuiltinType* u8Type() const;
    const BuiltinType* u16Type() const;
    const BuiltinType* u64Type() const;
    const BuiltinType* f64Type() const;
    const BuiltinType* varuintType() const;
    const BuiltinType* usizeType() const;
    const BuiltinType* boolType() const;
    const ValueInfoCache* cache() const;

private:
    CoreGcInterface();

    Rc<const BuiltinType> _u8Type;
    Rc<const BuiltinType> _u16Type;
    Rc<const BuiltinType> _u64Type;
    Rc<const BuiltinType> _f64Type;
    Rc<const BuiltinType> _varuintType;
    Rc<const BuiltinType> _usizeType;
    Rc<const BuiltinType> _boolType;
    Rc<const ValueInfoCache> _cache;
};

class WaypointGcInterface : public RefCountable {
public:
    ~WaypointGcInterface();

    static GcInterfaceResult<WaypointGcInterface> create(const Device* dev, const CoreGcInterface* coreIface);

    bool encodeBeginRouteCmd(std::uintmax_t id, std::uintmax_t size, Encoder* dest) const;
    bool encodeEndRouteCmd(std::uintmax_t id, Encoder* dest) const;
    bool encodeClearRouteCmd(std::uintmax_t id, Encoder* dest) const;
    bool encodeSetRoutePointCmd(std::uintmax_t id, std::uintmax_t pointIndex, const Waypoint& wp, Encoder* dest) const;
    bool encodeSetActiveRouteCmd(bmcl::Option<uintmax_t> id, Encoder* dest) const;
    bool encodeSetRouteActivePointCmd(uintmax_t id, bmcl::Option<uintmax_t> index, Encoder* dest) const;
    bool encodeSetRouteInvertedCmd(uintmax_t id, bool flag, Encoder* dest) const;
    bool encodeSetRouteClosedCmd(uintmax_t id, bool flag, Encoder* dest) const;
    bool encodeGetRouteInfoCmd(uintmax_t id, Encoder* dest) const;
    bool encodeGetRoutePointCmd(uintmax_t id, uintmax_t pointIndex, Encoder* dest) const;
    bool encodeGetRoutesInfoCmd(Encoder* dest) const;

    bool decodeGetRoutesInfoResponse(Decoder* src, AllRoutesInfo* dest) const;
    bool decodeGetRouteInfoResponse(Decoder* src, RouteInfo* dest) const;
    bool decodeGetRoutePointResponse(Decoder* src, Waypoint* dest) const;

private:
    WaypointGcInterface(const Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();

    bool beginNavCmd(const Function* func, Encoder* dest) const;

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
    Rc<const StructType> _routeInfoStruct;
    Rc<const StructType> _allRoutesInfoStruct;
    Rc<const VariantType> _actionVariant;
    Rc<const VariantType> _optionalRouteIdStruct;
    Rc<const VariantType> _optionalIndexStruct;
    Rc<const VariantType> _optionalF64Struct;
    Rc<const Function> _beginRouteCmd;
    Rc<const Function> _clearRouteCmd;
    Rc<const Function> _endRouteCmd;
    Rc<const Function> _setRoutePointCmd;
    Rc<const Function> _setRouteClosedCmd;
    Rc<const Function> _setRouteInvertedCmd;
    Rc<const Function> _setActiveRouteCmd;
    Rc<const Function> _setRouteActivePointCmd;
    Rc<const Function> _getRoutesInfoCmd;
    Rc<const Function> _getRouteInfoCmd;
    Rc<const Function> _getRoutePointCmd;
    std::size_t _formationArrayMaxSize;
    std::size_t _allRoutesInfoMaxSize;
};

class FileGcInterface : public RefCountable {
public:
    ~FileGcInterface();

    static GcInterfaceResult<FileGcInterface> create(const Device* dev, const CoreGcInterface* coreIface);

    bool encodeBeginFile(uintmax_t id, uintmax_t size, Encoder* dest) const;
    bool encodeWriteFile(uintmax_t id, uintmax_t offset, bmcl::Bytes data, Encoder* dest) const;
    bool encodeEndFile(uintmax_t id, uintmax_t size, Encoder* dest) const;

    std::uintmax_t maxFileChunkSize() const;

private:
    FileGcInterface(const Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();
    bool beginCmd(const Function* func, Encoder* dest) const;

    Rc<const Device> _dev;
    Rc<const CoreGcInterface> _coreIface;
    Rc<const Ast> _flModule;
    Rc<const Component> _flComponent;
    Rc<const Function> _beginFileCmd;
    Rc<const Function> _writeFileCmd;
    Rc<const Function> _endFileCmd;
    std::uintmax_t _maxChunkSize;
};

class UdpGcInterface : public RefCountable {
public:
    ~UdpGcInterface();

    static GcInterfaceResult<UdpGcInterface> create(const Device* dev, const CoreGcInterface* coreIface);

    bool encodeAddClient(uintmax_t id, bmcl::SocketAddressV4 address, Encoder* dest) const;

private:
    UdpGcInterface(const Device* dev, const CoreGcInterface* coreIface);
    bmcl::Option<std::string> init();
    bool beginCmd(const Function* func, Encoder* dest) const;

    Rc<const Device> _dev;
    Rc<const Component> _comp;
    Rc<const Ast> _udpModule;
    Rc<const Function> _addClientCmd;
    Rc<const StructType> _ipAddressStruct;
    Rc<const CoreGcInterface> _coreIface;
};

class AllGcInterfaces : public RefCountable {
public:
    AllGcInterfaces(const Device* dev, const ValueInfoCache* cache);
    ~AllGcInterfaces();

    bmcl::OptionPtr<const CoreGcInterface> coreInterface() const;
    bmcl::OptionPtr<const WaypointGcInterface> waypointInterface() const;
    bmcl::OptionPtr<const FileGcInterface> fileInterface() const;
    bmcl::OptionPtr<const UdpGcInterface> udpInterface() const;
    const std::string& errors() const;

private:
    Rc<CoreGcInterface> _coreIface;
    Rc<WaypointGcInterface> _waypointIface;
    Rc<FileGcInterface> _fileIface;
    Rc<UdpGcInterface> _udpIface;
    std::string _errors;
};
}
