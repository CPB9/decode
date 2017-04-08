#include "decode/parser/Containers.h"
#include "decode/parser/Field.h"
#include "decode/parser/Type.h"

#include <bmcl/OptionPtr.h>
#include <bmcl/StringView.h>

namespace decode {

bmcl::OptionPtr<Field> FieldVec::fieldWithName(bmcl::StringView name)
{
    for (const Rc<Field>& value : *this) {
        if (value->name() == name) {
            return value.get();
        }
    }
    return bmcl::None;
}
}
