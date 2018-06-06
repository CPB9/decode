#pragma once

#include "decode/Config.h"

namespace decode {

class Project;
class SrcBuilder;

class ReportGen {
public:
    ReportGen(SrcBuilder* output);
    ~ReportGen();

    void generateReport(const Project* project);

private:
    SrcBuilder* _output;
};
}
