#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Option.h>
#include <bmcl/StringView.h>

namespace decode {

class Value;

class Node : public RefCountable {
public:
    explicit Node(Node* parent)
        : _parent(parent)
    {
    }

    ~Node();

    bool hasParent() const
    {
        return _parent != nullptr;
    }

    const Node* parent() const
    {
        return _parent;
    }

    Node* parent()
    {
        return _parent;
    }

    void setParent(Node* node)
    {
        _parent = node;
    }

    virtual std::size_t numChildren() const;
    virtual bmcl::Option<std::size_t> childIndex(const Node* node) const;
    virtual Node* childAt(std::size_t idx);
    virtual bmcl::StringView fieldName() const;
    virtual bmcl::StringView typeName() const;
    virtual Value value() const;

protected:
    template <typename C>
    static bmcl::Option<std::size_t> childIndexGeneric(const C& cont, const Node* node)
    {
        auto it = std::find(cont.cbegin(), cont.cend(), node);
        if (it == cont.cend()) {
            return bmcl::None;
        }
        return std::distance(cont.cbegin(), it);
    }

    template <typename C>
    static Node* childAtGeneric(const C& cont, std::size_t idx)
    {
        if (idx >= cont.size()) {
            return nullptr;
        }
        return cont[idx].get();
    }

private:
    Node* _parent;
};
}
