#include "decode/core/RangeAttr.h"

namespace decode {

RangeAttr::RangeAttr()
{
}

RangeAttr::~RangeAttr()
{
}

void RangeAttr::setMinValue(NumberVariant&& value)
{
    _min = std::move(value);
}

void RangeAttr::setMaxValue(NumberVariant&& value)
{
    _max = std::move(value);
}

void RangeAttr::setDefaultValue(NumberVariant&& value)
{
    _default = std::move(value);
}

const NumberVariant& RangeAttr::minValue() const
{
    return _min;
}

const NumberVariant& RangeAttr::maxValue() const
{
    return _max;
}

const NumberVariant& RangeAttr::defaultValue() const
{
    return _default;
}
}
