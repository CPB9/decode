#include "decode/generator/ReportGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/parser/Project.h"
#include "decode/parser/Package.h"
#include "decode/ast/Component.h"
#include "decode/core/EncodedSizes.h"
#include "decode/ast/Function.h"

namespace decode {

ReportGen::ReportGen(SrcBuilder* output)
    : _output(output)
{
}

ReportGen::~ReportGen()
{
}

void genSizes(const EncodedSizes& sizes, SrcBuilder* output)
{
    output->append("[");
    output->appendNumericValue(sizes.min);
    output->append(", ");
    output->appendNumericValue(sizes.max);
    output->append("]");
}

template <typename T>
void genMsg(const Component* comp, const T* msg, EncodedSizes* max, SrcBuilder* output)
{
    output->append(" - ");
    output->append(comp->name());
    output->append("::");
    output->append(msg->name());
    output->appendSpace();
    EncodedSizes sizes = msg->encodedSizes();
    max->mergeMax(sizes);
    genSizes(sizes, output);
    output->appendEol();
}

void ReportGen::generateReport(const Project* project)
{
    EncodedSizes maxStatus(0, 0);
    EncodedSizes maxEvent(0, 0);
    EncodedSizes maxCmd(0, 0);
    _output->append("statuses:\n");
    for (const Component* comp : project->package()->components()) {
        for (const StatusMsg* msg : comp->statusesRange()) {
            genMsg(comp, msg, &maxStatus, _output);
        }
    }

    _output->append("\nevents:\n");
    for (const Component* comp : project->package()->components()) {
        for (const EventMsg* msg : comp->eventsRange()) {
            genMsg(comp, msg, &maxEvent, _output);
        }
    }

    _output->append("\ncommands:\n");
    for (const Component* comp : project->package()->components()) {
        for (const Command* msg : comp->cmdsRange()) {
            genMsg(comp, msg, &maxCmd, _output);
        }
    }

    bmcl::StringView sep = "----------------------------------------";
    _output->appendEol();
    _output->append(sep);
    _output->append(sep);
    _output->appendEol();
    _output->appendEol();
    _output->append(" - max status sizes ");
    genSizes(maxStatus, _output);
    _output->appendEol();
    _output->append(" - max event sizes ");
    genSizes(maxEvent, _output);
    _output->appendEol();
    _output->append(" - max cmd sizes ");
    genSizes(maxCmd, _output);
    _output->appendEol();
}

}
