#include "decode/generator/CmdEncoderGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/InlineFieldInspector.h"
#include "decode/generator/FuncPrototypeGen.h"
#include "decode/generator/Utils.h"
#include "decode/ast/Function.h"
#include "decode/core/Foreach.h"
#include "decode/core/HashSet.h"

namespace decode {

CmdEncoderGen::CmdEncoderGen(SrcBuilder* output)
    : _output(output)
    , _inlineSer(output)
{
}

CmdEncoderGen::~CmdEncoderGen()
{
}

void CmdEncoderGen::generateSource(ComponentMap::ConstRange comps)
{
    _output->appendImplIncludePath("core/Logging");
    _output->append("\n");

    _output->append("#define _PHOTON_FNAME \"CmdEncoder.c\"\n\n");

    TypeReprGen reprGen(_output);
    WrappingInlineStructInspector inspector(_output);
    FuncPrototypeGen prototypeGen(_output);
    for (const Component* comp : comps) {
        if (!comp->hasCmds()) {
            continue;
        }

        _output->appendTargetModIfdef(comp->moduleName());
        _output->appendEol();
        for (const Command* cmd : comp->cmdsRange()) {
            prototypeGen.appendCmdEncoderFunctionPrototype(comp, cmd, &reprGen);

            if (cmd->cmdId() <= 240) {//one byte for varuint
                _output->append("\n{\n"
                                "    (void)dest;\n"
                                "    if (PhotonWriter_WritableSize(dest) < 1) {\n"
                                "        PHOTON_DEBUG(\"Not enough space to serialize cmd header\");\n"
                                "        return PhotonError_NotEnoughSpace;\n"
                                "    }\n");
                _output->append("    PhotonWriter_WriteU8(dest, ");
                _output->appendNumericValue(cmd->cmdId());
                _output->append(");\n");
            } else {
                _output->append("    PHOTON_TRY_MSG(PhotonWriter_WriteVaruint(dest, ");
                _output->appendNumericValue(cmd->cmdId());
                _output->append("), \"Not enough space to serialize cmd header\");\n");
            }

            inspector.inspect<true, true>(cmd->type()->argumentsRange(), &_inlineSer);

            _output->append("    return PhotonError_Ok;\n}\n\n");
        }
        _output->appendEndif();
        _output->appendEol();
    }

    _output->append("#undef _PHOTON_FNAME\n");
}
}
