/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"
#include "decode/core/HashMap.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <string>

namespace decode {

class Configuration : public RefCountable {
public:
    using Pointer = Rc<Configuration>;
    using ConstPointer = Rc<const Configuration>;
    using Options = HashMap<std::string, bmcl::Option<std::string>>;
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

    void setGeneratedCodeDebugLevel(unsigned level);
    unsigned generatedCodeDebugLevel() const;

    void setVerboseOutput(bool isVerbose);
    bool verboseOutput() const;

    void setCompressionLevel(unsigned level);
    unsigned compressionLevel() const;

    std::size_t numOptions() const;

private:
    Options _values;
    unsigned _codeDebugLevel;
    unsigned _compressionLevel;
    bool _verboseOutput;
};
}
