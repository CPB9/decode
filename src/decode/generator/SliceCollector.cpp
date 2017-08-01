#include "decode/generator/SliceCollector.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/ast/Component.h"

namespace decode {

SliceCollector::SliceCollector()
{
}

SliceCollector::~SliceCollector()
{
}

void SliceCollector::collectUniqueSlices(const Component* comp, NameToSliceMap* dest)
{
    _dest = dest;
    traverseComponentParameters(comp);
}

void SliceCollector::collectUniqueSlices(const Type* type, NameToSliceMap* dest)
{
    _dest = dest;
    traverseType(type);
}

bool SliceCollector::visitSliceType(const SliceType* slice)
{
    _sliceName.clear();
    TypeNameGen gen(&_sliceName);
    gen.genTypeName(slice);
    _dest->emplace(_sliceName.result(), slice);
    return true;
}
}
