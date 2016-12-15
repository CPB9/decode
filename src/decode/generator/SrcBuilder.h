#pragma once

#include "decode/generator/StringBuilder.h"

namespace decode {

class SrcBuilder : public StringBuilder {
public:
    void setModName(bmcl::StringView modName);

    void appendModPrefix();

    template <typename... A>
    void appendInclude(A&&... args);

private:
    bmcl::StringView _modName;
};

inline void SrcBuilder::setModName(bmcl::StringView modName)
{
    _modName = modName;
}

inline void SrcBuilder::appendModPrefix()
{
    append("Photon");
    appendWithFirstUpper(_modName);
}

template <typename... A>
void SrcBuilder::appendInclude(A&&... args)
{
    append("#include <");
    append(std::forward<A>(args)...);
    append(">\n");
}
}
