#include "decode/generator/DynArrayCollector.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/ast/Component.h"

namespace decode {

DynArrayCollector::DynArrayCollector()
{
}

DynArrayCollector::~DynArrayCollector()
{
}

void DynArrayCollector::collectUniqueDynArrays(const Component* comp, NameToDynArrayMap* dest)
{
    _dest = dest;
    traverseComponentParameters(comp);
    traverseComponentCommands(comp);
}

void DynArrayCollector::collectUniqueDynArrays(const Type* type, NameToDynArrayMap* dest)
{
    _dest = dest;
    traverseType(type);
}

bool DynArrayCollector::visitDynArrayType(const DynArrayType* dynArray)
{
    _dynArrayName.clear();
    TypeNameGen gen(&_dynArrayName);
    gen.genTypeName(dynArray);
    _dest->emplace(_dynArrayName.view().toStdString(), dynArray);
    return true;
}
}
