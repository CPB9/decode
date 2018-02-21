#pragma once

namespace decode {

enum class CmdArgPassKind {
    Default,
    StackValue,
    StackPtr,
    AllocPtr,
};
}
