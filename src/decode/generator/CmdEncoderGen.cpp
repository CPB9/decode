#include "decode/generator/CmdEncoderGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/IncludeGen.h"
#include "decode/ast/Function.h"
#include "decode/core/Foreach.h"
#include "decode/core/HashSet.h"

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
            _reprGen->genOnboardTypeRepr(arg->type(), arg->name());
        }, [this](const Field*) {
            _output->append(", ");
        });
        _output->append(", PhotonWriter* dest)");
    }
}

void CmdEncoderGen::generateHeader(ComponentMap::ConstRange comps)
{
    _output->startIncludeGuard("PRIVATE", "CMD_ENCODER");
    _output->appendEol();

    TypeDependsCollector::Depends includes;
    TypeDependsCollector col;

    for (const char* path : {"core/Error", "core/Reader", "core/Writer", "core/Try"}) {
        _output->appendOnboardIncludePath(path);
    }
    _output->appendEol();

    IncludeGen includeGen(_output);
    for (const Component* it : comps) {
        for (const Function* jt : it->cmdsRange()) {
            col.collect(jt, &includes);
        }
        if (includes.empty()) {
            continue;
        }
        _output->appendTargetModIfdef(it->moduleName());
        includeGen.genOnboardIncludePaths(&includes);
        _output->appendEndif();

        includes.clear();
    }

    _output->appendEol();

    _output->startCppGuard();

    for (const Component* it : comps) {
        if (!it->hasCmds()) {
            continue;
        }
        _output->appendTargetModIfdef(it->moduleName());
        for (const Function* jt : it->cmdsRange()) {
            appendEncoderPrototype(it, jt);
            _output->append(";\n");
        }
        _output->appendEndif();
    }

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

class SimpleInlineFieldInspector : public InlineFieldInspector<SimpleInlineFieldInspector> {
public:
    SimpleInlineFieldInspector(SrcBuilder* dest)
        : InlineFieldInspector<SimpleInlineFieldInspector>(dest)
        , _current(nullptr)
    {
    }

    void beginField(const Field* field)
    {
        _current = field;
    }

    void endField(const Field*)
    {
    }

    bmcl::StringView currentFieldName() const
    {
        return _current->name();
    }

private:
    const Field* _current;
};

void CmdEncoderGen::generateSource(ComponentMap::ConstRange comps)
{
    _output->appendOnboardIncludePath("CmdEncoder.Private"); //TODO: pass as argument
    _output->appendOnboardIncludePath("core/Logging");
    _output->append("\n");

    _output->append("#define _PHOTON_FNAME \"CmdDecoder.Private.c\"\n\n");

    for (const Component* it : comps) {
        if (!it->hasCmds()) {
            continue;
        }

        std::size_t funcNum = 0;
        _output->appendTargetModIfdef(it->moduleName());
        _output->appendEol();
        for (const Function* jt : it->cmdsRange()) {
            appendEncoderPrototype(it, jt);
            _output->append("\n{\n");

            _output->append("    PHOTON_TRY_MSG(PhotonWriter_WriteVaruint(dest, ");
            _output->appendNumericValue(it->number());
            _output->append("), \"Failed to write component number\");\n");

            _output->append("    PHOTON_TRY_MSG(PhotonWriter_WriteVaruint(dest, ");
            _output->appendNumericValue(funcNum);
            _output->append("), \"Failed to write cmd number\");\n");

            SimpleInlineFieldInspector inspector(_output);
            inspector.inspect(jt->type()->argumentsRange(), &_inlineSer);

            _output->append("    return PhotonError_Ok;\n}\n\n");
            funcNum++;
        }
        _output->appendEndif();
        _output->appendEol();
    }

    _output->append("#undef _PHOTON_FNAME\n");
}
}
