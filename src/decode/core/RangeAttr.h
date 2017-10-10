#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Utils.h"

#include <bmcl/Variant.h>

#include <functional>

namespace decode {

enum class NumberVariantKind {
    None,
    Signed,
    Unsigned,
    Double,
};

using NumberVariant =
    bmcl::Variant<NumberVariantKind, NumberVariantKind::None,
        bmcl::VariantElementDesc<NumberVariantKind, std::intmax_t, NumberVariantKind::Signed>,
        bmcl::VariantElementDesc<NumberVariantKind, std::uintmax_t, NumberVariantKind::Unsigned>,
        bmcl::VariantElementDesc<NumberVariantKind, double, NumberVariantKind::Double>
    >;

class RangeAttr : public RefCountable {
public:
    RangeAttr();
    ~RangeAttr();

    template <typename T>
    bool valueIsDefault(T value) const
    {
        switch (_default.kind()) {
        case NumberVariantKind::None:
            return false;
        case NumberVariantKind::Signed:
            if (std::is_unsigned<T>()) {
                if (_default.as<std::intmax_t>() < 0) {
                    return false;
                }
                return value == uintmax_t(_default.as<std::intmax_t>());
            } else {
                return value == _default.as<std::intmax_t>();
            }
        case NumberVariantKind::Unsigned:
            if (std::is_signed<T>()) {
                if (value < 0) {
                    return false;
                }
                return uintmax_t(value) == _default.as<std::uintmax_t>();
            } else {
                return value == _default.as<std::uintmax_t>();
            }
        case NumberVariantKind::Double:
            return doubleEq(value, _default.as<double>());
        }
        return false;
    }

    template <typename T>
    bool valueIsInRange(T value) const
    {
        bool isInRangeLeft;
        switch (_min.kind()) {
        case NumberVariantKind::None:
            isInRangeLeft = true;
            break;
        case NumberVariantKind::Signed:
            if (std::is_unsigned<T>()) {
                if (_min.as<std::intmax_t>() < 0) {
                    isInRangeLeft = true;
                } else {
                    isInRangeLeft = value >= std::uintmax_t(_min.as<std::intmax_t>());
                }
            } else {
                isInRangeLeft = value >= _min.as<std::intmax_t>();
            }
            break;
        case NumberVariantKind::Unsigned:
            if (std::is_signed<T>()) {
                if (value < 0) {
                    isInRangeLeft = false;
                } else {
                    isInRangeLeft = std::uintmax_t(value) >= _min.as<std::uintmax_t>();
                }
            } else {
                isInRangeLeft = value >= _min.as<std::uintmax_t>();
            }
            break;
        case NumberVariantKind::Double:
            isInRangeLeft = value >= _min.as<double>();
            break;
        }
        bool isInRangeRight;
        switch (_max.kind()) {
        case NumberVariantKind::None:
            isInRangeRight = true;
            break;
        case NumberVariantKind::Signed:
            if (std::is_unsigned<T>()) {
                if (_max.as<std::intmax_t>() < 0) {
                    isInRangeRight = false;
                } else {
                    isInRangeRight = value >= std::uintmax_t(_max.as<std::intmax_t>());
                }
            } else {
                isInRangeRight = value >= _max.as<std::intmax_t>();
            }
            break;
        case NumberVariantKind::Unsigned:
            if (std::is_signed<T>()) {
                if (value < 0) {
                    isInRangeRight = true;
                } else {
                    isInRangeRight = std::uintmax_t(value) >= _max.as<std::uintmax_t>();
                }
            } else {
                isInRangeRight = value >= _max.as<std::uintmax_t>();
            }
            break;
        case NumberVariantKind::Double:
            isInRangeRight = value >= _max.as<double>();
            break;
        }
        return isInRangeLeft && isInRangeRight;
    }

    void setMinValue(NumberVariant&& value);
    void setMaxValue(NumberVariant&& value);
    void setDefaultValue(NumberVariant&& value);

    const NumberVariant& minValue() const;
    const NumberVariant& maxValue() const;
    const NumberVariant& defaultValue() const;

private:
    NumberVariant _min;
    NumberVariant _max;
    NumberVariant _default;
};
}
