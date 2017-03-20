#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

#include <string>
#include <unordered_map>

namespace decode {

class Type;

class ValueInfoCache : public RefCountable {
public:
    ValueInfoCache();
    ~ValueInfoCache();

    bmcl::StringView arrayIndex(std::size_t idx) const;
    bmcl::StringView nameForType(const Type* type) const;

private:
    void updateIndexes(std::size_t newSize) const;

    mutable std::vector<bmcl::StringView> _arrayIndexes;
    mutable std::unordered_map<const Type*, std::string> _names;
};
}
