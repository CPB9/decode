#include "decode/generator/GcStatusMsgGen.h"

#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/InlineTypeInspector.h"
#include "decode/ast/Component.h"
#include "decode/core/Foreach.h"

namespace decode {

GcStatusMsgGen::GcStatusMsgGen(SrcBuilder* dest)
    : _output(dest)
{
}

GcStatusMsgGen::~GcStatusMsgGen()
{
}

struct MsgPartsDesc {
    MsgPartsDesc(const Type* type, std::string&& fieldName)
        : type(type)
        , fieldName(std::move(fieldName))
    {
    }

    const Type* type;
    std::string fieldName;
};

void GcStatusMsgGen::generateHeader(const Component* comp, const StatusMsg* msg)
{
    _output->appendPragmaOnce();
    _output->appendEol();

    TypeDependsCollector coll;
    TypeDependsCollector::Depends deps;
    coll.collect(msg, &deps);
    IncludeGen gen(_output);
    gen.genGcIncludePaths(&deps);
    _output->appendEol();

    _output->append("namespace photongen {\nnamespace ");
    _output->append(comp->name());
    _output->append(" {\n\n");

    _output->append("struct Msg");
    _output->appendWithFirstUpper(msg->name());
    _output->append(" {\n");

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

        const Type* contType = nullptr;
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
        descs.emplace_back(contType, std::move(partName.result()));
        i++;
    }

    _output->append("    static constexpr uint64_t COMP_NUM = ");
    _output->appendNumericValue(comp->number());
    _output->append(";\n    static constexpr uint64_t MSG_NUM = ");
    _output->appendNumericValue(msg->number());
    _output->append(";\n\n");
    TypeReprGen reprGen(_output);
    for (const MsgPartsDesc& desc : descs) {
        _output->append("    ");
        reprGen.genGcTypeRepr(desc.type, desc.fieldName);
        _output->append(";\n");
    }

    _output->append("};\n\n");


    _output->append("}\n}\n\n");

    _output->append("inline bool photongenDeserialize(");
    genMsgType(comp, msg, _output);
    _output->append("* msg, bmcl::MemReader* src, photon::CoderState* state)\n{\n");

    InlineTypeInspector inspector(&reprGen, _output);
    InlineSerContext ctx;
    for (const MsgPartsDesc& desc : descs) {
        std::string argName("msg->");
        argName.append(desc.fieldName);
        inspector.inspect<false, false>(desc.type, ctx, argName);
    }

    _output->append("    return true;\n}\n\n");
}

void GcStatusMsgGen::genMsgType(const Component* comp, const StatusMsg* msg, SrcBuilder* dest)
{
    dest->append("photongen::");
    dest->append(comp->name());
    dest->append("::Msg");
    dest->appendWithFirstUpper(msg->name());
}
}
