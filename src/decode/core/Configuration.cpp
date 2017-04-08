#include "decode/core/Configuration.h"

namespace decode {

Configuration::Configuration()
    : _debugLevel(0)
    , _compressionLevel(5)
{
    setCfgOption("target_pointer_width", "32");
}

Configuration::~Configuration()
{
}

void Configuration::setCfgOption(bmcl::StringView key)
{
    _values.emplace(std::piecewise_construct,
                    std::forward_as_tuple(key.begin(), key.end()),
                    std::forward_as_tuple(bmcl::None));
}

void Configuration::setCfgOption(bmcl::StringView key, bmcl::StringView value)
{
    _values.emplace(std::piecewise_construct,
                    std::forward_as_tuple(key.begin(), key.end()),
                    std::forward_as_tuple(bmcl::InPlace, value.begin(), value.end()));
}

bool Configuration::isCfgOptionDefined(bmcl::StringView key) const
{
    return _values.find(key.toStdString()) != _values.end(); //TODO: remove toStdString
}

bool Configuration::isCfgOptionSet(bmcl::StringView key, bmcl::Option<bmcl::StringView> value) const
{
    auto it = _values.find(key.toStdString()); //TODO: remove toStdString
    if (it == _values.end()) {
        return false;
    }
    if (it->second.isSome() && value.isSome()) {
        return it->second.unwrap() == value.unwrap();
    }
    return false;
}

bmcl::Option<bmcl::StringView> Configuration::cfgOption(bmcl::StringView key) const
{
    auto it = _values.find(key.toStdString()); //TODO: remove toStdString
    if (it == _values.end()) {
        return bmcl::None;
    }
    if (it->second.isSome()) {
        return bmcl::StringView(it->second.unwrap());
    }
    return bmcl::None;
}

void Configuration::setDebugLevel(unsigned level)
{
    _debugLevel = level;
}

unsigned Configuration::debugLevel() const
{
    return _debugLevel;
}

void Configuration::setCompressionLevel(unsigned level)
{
    _compressionLevel = level;
}

unsigned Configuration::compressionLevel() const
{
    return _compressionLevel;
}

Configuration::OptionsConstIterator Configuration::optionsBegin() const
{
    return _values.cbegin();
}

Configuration::OptionsConstIterator Configuration::optionsEnd() const
{
    return _values.cend();
}

Configuration::OptionsConstRange Configuration::optionsRange() const
{
    return OptionsConstRange(_values);
}

std::size_t Configuration::numOptions() const
{
    return _values.size();
}
}
