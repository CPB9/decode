#include "decode/generator/GcMsgGen.h"

#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/InlineTypeInspector.h"
#include "decode/ast/Component.h"
#include "decode/core/Foreach.h"

namespace decode {

GcMsgGen::GcMsgGen(SrcBuilder* dest)
    : _output(dest)
{
}

GcMsgGen::~GcMsgGen()
{
}

template <typename T>
void GcMsgGen::appendPrelude(const Component* comp, const T* msg, bmcl::StringView namespaceName)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    TypeDependsCollector coll;
    TypeDependsCollector::Depends deps;
    coll.collect(msg, &deps);
    IncludeGen gen(_output);
    gen.genGcIncludePaths(&deps);
    _output->appendEol();

    _output->appendInclude("photon/groundcontrol/NumberedSub.h");
    _output->appendEol();

    _output->append("namespace photongen {\nnamespace ");
    _output->append(comp->name());
    _output->append(" {\nnamespace ");
    _output->append(namespaceName);
    _output->append(" {\n\n");

    _output->append("struct ");
    _output->appendWithFirstUpper(msg->name());
    _output->append(" {\n"
                    "    static constexpr uint32_t COMP_NUM = ");
    _output->appendNumericValue(comp->number());
    _output->append(";\n    static constexpr uint32_t MSG_NUM = ");
    _output->appendNumericValue(msg->number());
    _output->append(";\n");
    _output->append("    static constexpr uint64_t MSG_ID = (uint64_t(COMP_NUM) << 32) | uint64_t(MSG_NUM);\n\n");

    _output->append("    static photon::NumberedSub sub_()\n    {\n        return photon::NumberedSub::fromMsg<");
    _output->appendWithFirstUpper(msg->name());
    _output->append(">();\n    }\n\n");
}


void GcMsgGen::generateStatusHeader(const Component* comp, const StatusMsg* msg)
{
    appendPrelude(comp, msg, "statuses");

    TypeReprGen reprGen(_output);
    StringBuilder fieldName;
    fieldName.reserve(31);
    for (const StatusRegexp* regexp : msg->partsRange()) {
        _output->append("    ");
        regexp->buildFieldName(&fieldName);
        reprGen.genGcTypeRepr(regexp->type(), fieldName.view());
        _output->append(";\n");
        fieldName.clear();
    }

    _output->append("};\n\n""}\n}\n}\n\n");

    _output->append("inline bool photongenDeserialize(");
    genStatusMsgType(comp, msg, _output);
    _output->append("* msg, bmcl::MemReader* src, photon::CoderState* state)\n{\n");

    InlineTypeInspector inspector(_output);
    InlineSerContext ctx;
    fieldName.assign("msg->");
    for (const StatusRegexp* regexp : msg->partsRange()) {
        regexp->buildFieldName(&fieldName);
        inspector.inspect<false, false>(regexp->type(), ctx, fieldName.view());
        fieldName.resize(5);
    }

    _output->append("    return true;\n}\n\n");
}

void GcMsgGen::generateEventHeader(const Component* comp, const EventMsg* msg)
{
    appendPrelude(comp, msg, "events");

    TypeReprGen reprGen(_output);
    for (const Field* field : msg->partsRange()) {
        _output->append("    ");
        reprGen.genGcTypeRepr(field->type(), field->name());
        _output->append(";\n");
    }
    _output->append("};\n\n""}\n}\n}\n\n");

    _output->append("inline bool photongenDeserialize(");
    _output->append("photongen::");
    _output->append(comp->name());
    _output->append("::events::");
    _output->appendWithFirstUpper(msg->name());
    _output->append("* msg, bmcl::MemReader* src, photon::CoderState* state)\n{\n");

    InlineTypeInspector inspector(_output);
    InlineSerContext ctx;
    StringBuilder argName("msg->");
    for (const Field* field : msg->partsRange()) {
        argName.append(field->name());
        inspector.inspect<false, false>(field->type(), ctx, argName.view());
        argName.resize(5);
    }

    _output->append("    return true;\n}\n\n");
}

void GcMsgGen::genStatusMsgType(const Component* comp, const StatusMsg* msg, SrcBuilder* dest)
{
    dest->append("photongen::");
    dest->append(comp->name());
    dest->append("::statuses::");
    dest->appendWithFirstUpper(msg->name());
}
}
