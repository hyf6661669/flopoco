#ifndef BaseMultiplier2xk_HPP
#define BaseMultiplier2xk_HPP

#include <string>
#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include "Target.hpp"
#include "Operator.hpp"
#include "Table.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {


    class BaseMultiplier2xk : public BaseMultiplier
    {

	public:
        BaseMultiplier2xk(bool isSignedX, bool isSignedY, int width);

        virtual Operator *generateOperator(Target *target);

    private:

	};

    class BaseMultiplier2xkOp : public Operator
    {
    public:
        BaseMultiplier2xkOp(Target* target, bool isSignedX, bool isSignedY, int width);
    };

}
#endif
