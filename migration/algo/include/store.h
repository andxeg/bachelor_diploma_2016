#pragma once

#include "leafnode.h"

class Store : public LeafNode {
    friend class ElementFactory;
public:
    enum Attributes {
        NONE = 0,
        REPLICABLE = 1
    };

    Store() : 
        LeafNode(),
        cl(0)
    {
        type = STORE;
    }

    void setClass(int cl) {
        this->cl = cl;
    }

private:
    virtual bool typeCheck(const Element * other) const {
        return other->isStore();
    }

    virtual bool physicalCheck(const Element * other) const {
        if ( !LeafNode::physicalCheck(other) ) return false;
        return classCheck(other);
    }

    bool classCheck(const Element * other) const {
        return cl >= other->toStore()->cl;
    }

private:
    int cl;
};
