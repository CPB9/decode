#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/ValueKind.h"

#include <bmcl/Option.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/StringView.h>

namespace decode {

class Value;

class Node : public RefCountable {
public:
    explicit Node(bmcl::OptionPtr<Node> parent);
    ~Node();

    void setParent(Node* node);
    bool hasParent() const;
    bmcl::OptionPtr<const Node> parent() const;
    bmcl::OptionPtr<Node> parent();

    virtual bool canHaveChildren() const;
    virtual std::size_t numChildren() const;
    virtual bmcl::Option<std::size_t> childIndex(const Node* node) const;
    virtual bmcl::OptionPtr<Node> childAt(std::size_t idx);
    virtual bmcl::StringView fieldName() const;
    virtual bmcl::StringView typeName() const;
    virtual Value value() const;
    virtual ValueKind valueKind() const;
    virtual bool canSetValue() const;
    virtual bool setValue(const Value& value);

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
    static bmcl::OptionPtr<Node> childAtGeneric(const C& cont, std::size_t idx)
    {
        if (idx >= cont.size()) {
            return bmcl::None;
        }
        return cont[idx].get();
    }

private:
    bmcl::OptionPtr<Node> _parent; // not owned
};

inline Node::Node(bmcl::OptionPtr<Node> node)
    : _parent(node)
{
}

inline bool Node::hasParent() const
{
    return _parent.isSome();
}

inline bmcl::OptionPtr<const Node> Node::parent() const
{
    return _parent;
}

inline bmcl::OptionPtr<Node> Node::parent()
{
    return _parent;
}

inline void Node::setParent(Node* node)
{
    _parent = node;
}
}
