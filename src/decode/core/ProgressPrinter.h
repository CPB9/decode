#pragma once

#include "decode/Config.h"

#include <bmcl/ColorStream.h>
#include <bmcl/StringView.h>

namespace decode {

class ProgressPrinter {
public:
    ProgressPrinter(bool verbose = false);

    void printActionProgress(bmcl::StringView actionMsgPart, bmcl::StringView additionalMsgPart = bmcl::StringView::empty());

private:
    bmcl::ColorStdOut _dest;
    bool _isVerbose;
};

}
