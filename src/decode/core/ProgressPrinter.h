#pragma once

#include "decode/Config.h"

#include <bmcl/ColorStream.h>
#include <bmcl/StringView.h>

#include <ostream>
#include <iomanip>

namespace decode {

class ProgressPrinter {
public:

    ProgressPrinter(bool verbose = false)
        : _isVerbose(verbose)
    {
    }

    void printActionProgress(bmcl::StringView actionMsgPart, bmcl::StringView additionalMsgPart = bmcl::StringView::empty())
    {
        constexpr const char* prefix = "> ";
        constexpr std::size_t actionSize = 10;

        _dest << bmcl::ColorAttr::Reset;
        _dest << prefix;
        _dest << bmcl::ColorAttr::FgGreen << bmcl::ColorAttr::Bright;
        _dest << std::setw(actionSize) << std::left;
        _dest << actionMsgPart.toStdString();
        _dest << bmcl::ColorAttr::Reset;
        _dest << ' ';
        _dest << additionalMsgPart.toStdString();
        _dest << std::endl;
    }

private:
    bmcl::ColorStdOut _dest;
    bool _isVerbose;
};

}
