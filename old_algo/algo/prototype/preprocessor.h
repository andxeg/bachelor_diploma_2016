#pragma once

#include "defs.h"

class Preprocessor {
public:
    static Request * fakeNetElements(Request * r);
private:
    static Link * getFakeLink(Element * first, Element * second);
};
