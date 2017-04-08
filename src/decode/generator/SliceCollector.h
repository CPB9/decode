#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/parser/Component.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"

#include <string>
#include <unordered_map>

namespace decode {

class Type;
class TypeReprGen;

class SliceCollector : public ConstAstVisitor<SliceCollector> {
public:
    using NameToSliceMap = RcSecondUnorderedMap<std::string, const SliceType>;

    void collectUniqueSlices(const Type* type, NameToSliceMap* dest);
    void collectUniqueSlices(const Component* type, NameToSliceMap* dest);

    bool visitSliceType(const SliceType* slice);

private:
    SrcBuilder _sliceName;
    NameToSliceMap* _dest;
};

inline void SliceCollector::collectUniqueSlices(const Component* comp, NameToSliceMap* dest)
{
    _dest = dest;
    traverseComponentParameters(comp);
}

inline void SliceCollector::collectUniqueSlices(const Type* type, NameToSliceMap* dest)
{
    _dest = dest;
    traverseType(type);
}

inline bool SliceCollector::visitSliceType(const SliceType* slice)
{
    _sliceName.clear();
    TypeNameGen gen(&_sliceName);
    gen.genTypeName(slice);
    _dest->emplace(_sliceName.result(), slice);
    return true;
}
}
