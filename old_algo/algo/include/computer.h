#pragma once

#include "leafnode.h"

class Computer : public LeafNode {
    friend class ElementFactory;
public:
    enum Type {
        NONE = 0,
        VNF = 1
    };

    Computer(bool vnf = false) : LeafNode(), computerType(0) {
        type = COMPUTER;
        if (vnf)
            computerType |= VNF;
    }

    bool isVnf() const {
    	return computerType & VNF == VNF;
    }

private:
    virtual bool typeCheck(const Element * other) const {
        return other->isComputer();
    }

private:

    int computerType; // vnf or not

};
