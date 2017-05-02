#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"
#include "decode/core/Hash.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace decode {

class Configuration : public RefCountable {
public:
    using Options = std::unordered_map<std::string, bmcl::Option<std::string>>;
    using OptionsConstIterator = Options::const_iterator;
    using OptionsConstRange = IteratorRange<Options::const_iterator>;

    Configuration();
    ~Configuration();

    OptionsConstIterator optionsBegin() const;
    OptionsConstIterator optionsEnd() const;
    OptionsConstRange optionsRange() const;

    void setCfgOption(bmcl::StringView key);
    void setCfgOption(bmcl::StringView key, bmcl::StringView value);

    bool isCfgOptionDefined(bmcl::StringView key) const;
    bool isCfgOptionSet(bmcl::StringView key, bmcl::Option<bmcl::StringView> value = bmcl::None) const;

    bmcl::Option<bmcl::StringView> cfgOption(bmcl::StringView key) const;

    void setDebugLevel(unsigned level);
    unsigned debugLevel() const;

    void setCompressionLevel(unsigned level);
    unsigned compressionLevel() const;

    std::size_t numOptions() const;

private:
    Options _values;
    unsigned _debugLevel;
    unsigned _compressionLevel;
};
}
