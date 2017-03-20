#include "decode/model/Value.h"

namespace decode {


Value::Value(const Value& other)
{
    construct(other);
}

Value::Value(Value&& other)
{
    construct(std::move(other));
}

Value::~Value()
{
    destruct();
}

void Value::construct(const Value& other)
{
    _kind = other.kind();
    switch (other.kind()) {
    case ValueKind::None:
    case ValueKind::Uninitialized:
        return;
    case ValueKind::Signed:
        *as<int64_t>() = other.asSigned();
        return;
    case ValueKind::Unsigned:
        *as<uint64_t>() = other.asUnsigned();
        return;
    case ValueKind::String:
        *as<std::string>() = other.asString();
        return;
    case ValueKind::StringView:
        *as<bmcl::StringView>() = other.asStringView();
        return;
    }
}

void Value::construct(Value&& other)
{
    _kind = other.kind();
    switch (other.kind()) {
    case ValueKind::None:
    case ValueKind::Uninitialized:
        return;
    case ValueKind::Signed:
        *as<int64_t>() = other.asSigned();
        return;
    case ValueKind::Unsigned:
        *as<uint64_t>() = other.asUnsigned();
        return;
    case ValueKind::String:
        *as<std::string>() = std::move(other.asString());
        return;
    case ValueKind::StringView:
        *as<bmcl::StringView>() = other.asStringView();
        return;
    }
}

void Value::destruct()
{
    switch (kind()) {
    case ValueKind::None:
    case ValueKind::Uninitialized:
    case ValueKind::Signed:
    case ValueKind::Unsigned:
        return;
    case ValueKind::String:
        as<std::string>()->~basic_string<char>();
        break;
    case ValueKind::StringView:
        as<bmcl::StringView>()->~StringView();
        break;
    }
}

Value& Value::operator=(const Value& other)
{
    destruct();
    construct(other);
    return *this;
}

Value& Value::operator=(Value&& other)
{
    destruct();
    construct(std::move(other));
    return *this;
}
}
