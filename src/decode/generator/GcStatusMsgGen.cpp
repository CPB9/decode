#include "decode/generator/GcStatusMsgGen.h"

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

struct MsgPartsDesc {
    MsgPartsDesc(const Type* type, std::string&& fieldName)
        : type(type)
        , fieldName(std::move(fieldName))
    {
    }

    Rc<const Type> type;
    std::string fieldName;
};

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

    SrcBuilder partName;
    std::size_t i = 0;
    std::vector<MsgPartsDesc> descs;
    descs.reserve(msg->partsRange().size());
    for (const StatusRegexp* regexp : msg->partsRange()) {
        const SubscriptAccessor* lastSubscript = nullptr;
        const FieldAccessor* lastField = nullptr;
        foreachList(regexp->accessorsRange(), [&](const Accessor* acc) {
             switch (acc->accessorKind()) {
                case AccessorKind::Field:
                    lastField = acc->asFieldAccessor();
                    partName.append(lastField->field()->name());
                    break;
                case AccessorKind::Subscript:
                    lastSubscript = acc->asSubscriptAccessor();
                    break;
                default:
                    assert(false);
            }
        }, [&](const Accessor* acc) {
            partName.append("_");
        });

        Rc<const Type> contType = nullptr;
        assert(lastField);
        if (lastSubscript) {
            if (lastSubscript->type()->isArray()) {
                //TODO: support range
                contType = new ArrayType(lastSubscript->type()->asArray()->elementCount(), const_cast<Type*>(lastField->field()->type()));
            } else if (lastSubscript->type()->isDynArray()) {
                contType = new DynArrayType(lastSubscript->type()->asDynArray()->maxSize(), const_cast<Type*>(lastField->field()->type()));
            } else {
                assert(false);
            }
        } else {
            contType = lastField->field()->type();
        }
        descs.emplace_back(contType.get(), std::move(partName.result()));
        i++;
    }



    TypeReprGen reprGen(_output);
    for (const MsgPartsDesc& desc : descs) {
        _output->append("    ");
        reprGen.genGcTypeRepr(desc.type.get(), desc.fieldName);
        _output->append(";\n");
    }

    _output->append("};\n\n""}\n}\n}\n\n");

    _output->append("inline bool photongenDeserialize(");
    genStatusMsgType(comp, msg, _output);
    _output->append("* msg, bmcl::MemReader* src, photon::CoderState* state)\n{\n");

    InlineTypeInspector inspector(_output);
    InlineSerContext ctx;
    StringBuilder argName("msg->");
    for (const MsgPartsDesc& desc : descs) {
        argName.append(desc.fieldName);
        inspector.inspect<false, false>(desc.type.get(), ctx, argName.view());
        argName.resize(5);
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
