#include "sat/sat.h"

namespace SAT
{
    Literal negate(Literal lit)
    {
        return -lit;
    }

    bool is_negated(Literal lit)
    {
        return lit < 0;
    }

    Variable strip(Literal lit)
    {
        return is_negated(lit) ? -lit : lit;
    }
}

