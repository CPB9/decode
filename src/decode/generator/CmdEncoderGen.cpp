#include "decode/generator/CmdEncoderGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/core/Foreach.h"

namespace decode {

CmdEncoderGen::CmdEncoderGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _reprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, output)
{
}

CmdEncoderGen::~CmdEncoderGen()
{
}

void CmdEncoderGen::appendEncoderPrototype(const Component* comp, const Function* func)
{
    _output->append("PhotonError Photon");
    _output->appendWithFirstUpper(comp->name());
    _output->append("_GenCmd_");
    _output->appendWithFirstUpper(func->name());
    if (func->type()->argumentsRange().isEmpty()) {
        _output->append("(PhotonWriter* dest)");
    } else {
        _output->append("(");
        foreachList(func->type()->argumentsRange(), [this](const Field* arg) {
            _reprGen->genTypeRepr(arg->type(), arg->name());
        }, [this](const Field*) {
            _output->append(", ");
        });
        _output->append(", PhotonWriter* dest)");
    }
}

void CmdEncoderGen::generateHeader(ComponentMap::ConstRange comps)
{
    (void)comps;
    _output->startIncludeGuard("PRIVATE", "CMD_ENCODER");
    _output->appendEol();

    _output->appendLocalIncludePath("core/Error");
    _output->appendLocalIncludePath("core/Reader");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendEol();

    _output->startCppGuard();

    for (auto it : comps) {
        for (const Function* jt : it->cmdsRange()) {
            appendEncoderPrototype(it, jt);
            _output->append(";\n");
        }
    }

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void CmdEncoderGen::generateSource(ComponentMap::ConstRange comps)
{
    _output->appendLocalIncludePath("CmdEncoder.Private"); //TODO: pass as argument
    _output->append("\n");

    for (auto it : comps) {
        std::size_t funcNum = 0;
        for (const Function* jt : it->cmdsRange()) {
            InlineSerContext ctx;

            appendEncoderPrototype(it, jt);
            _output->append("\n{\n");

            _output->append("    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, ");
            _output->appendNumericValue(it->number());
            _output->append("));\n");

            _output->append("    PHOTON_TRY(PhotonWriter_WriteVaruint(dest, ");
            _output->appendNumericValue(funcNum);
            _output->append("));\n");

            for (const Field* arg : jt->type()->argumentsRange()) {
                _inlineSer.inspect(arg->type(), ctx, arg->name());
            }

            _output->append("    return PhotonError_Ok;\n}\n\n");
            funcNum++;
        }
    }
}
}
