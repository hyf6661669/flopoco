// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <stdexcept>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "Xilinx_CARRY4.hpp"

using namespace std;
namespace flopoco {
    Xilinx_CARRY4::Xilinx_CARRY4(Target* target) : Xilinx_Primitive(target) {
        setName("CARRY4");
        srcFileName="Xilinx_CARRY4";

        addInput( "ci",1 );
        addInput( "cyinit",1 );
        addInput( "di",4 );
        addInput( "s",4 );

        addOutput("co",4);
        addOutput("o",4);
    }
}//namespace
