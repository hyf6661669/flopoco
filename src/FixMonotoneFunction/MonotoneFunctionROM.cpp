//
// Created by Viktor Schmidt.
//

#include "MonotoneFunctionROM.hpp"
// general c++ library for manipulating streams
#include <iostream>
#include <sstream>
#include <bitset>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"
#include "ComparatorTable.hpp"

using namespace std;
namespace flopoco {
    MonotoneFunctionROM::MonotoneFunctionROM(OperatorPtr parentOp, Target* target, string functionString_, int inputWidth_, int outputWidth_) :
            FixMonotoneFunctionInterface(parentOp, target, functionString_, inputWidth_, outputWidth_) {

        srcFileName="MonotoneFunctionROM";

        // definition of the name of the operator
        ostringstream name;
        name << "MonotoneFunctionROM" << inputWidth << "_" << outputWidth;
        setName(name.str());

        REPORT(INFO,"Declaration of MonotoneFunctionROM \n");

        REPORT(DETAILED, "this operator has received two parameters " << inputWidth << " and " << outputWidth);

        build();
    };

//
//    void MonotoneFunctionROM::emulate(TestCase * tc) {
//        /* This function will be used when the TestBench command is used in the command line
//           we have to provide a complete and correct emulation of the operator, in order to compare correct output generated by this function with the test input generated by the vhdl code */
//        /* first we are going to format the entries */
//        mpz_class sx = tc->getInputValue("X");
//
//        /* then we are going to manipulate our bit vectors in order to get the correct output*/
//        mpz_class sr;
//        eval(sr, sx);
//
//        /* at the end, we indicate to the TestCase object what is the expected
//           output corresponding to the inputs */
//        tc->addExpectedOutput("Y",sr);
//    }


//    void MonotoneFunctionROM::buildStandardTestCases(TestCaseList * tcl) {
//        // please fill me with regression tests or corner case tests!
//    }


//    void MonotoneFunctionROM::eval(mpz_class& r, mpz_class x) const
//    {
//        mpfr_t mpX, mpR;
//        mpfr_init2(mpX, inputWidth+2);
//        mpfr_init2(mpR, outputWidth*3);
//        sollya_lib_set_prec(sollya_lib_constant_from_int(inputWidth*2));
//
//        mpfr_set_z(mpX, x.get_mpz_t(), GMP_RNDN);
//        mpfr_div_2si(mpX, mpX, inputWidth, GMP_RNDN);
//
//        sollya_lib_evaluate_function_at_point(mpR, fS, mpX, NULL);
//
//        mpfr_mul_2si(mpR, mpR, outputWidth, GMP_RNDN);
//        mpfr_get_z(r.get_mpz_t(), mpR, GMP_RNDN);
//
//        REPORT(FULL,"function() input is:"<<mpfr_get_d(mpX, GMP_RNDN));
//        REPORT(FULL,"function() output is:"<<mpfr_get_d(mpR, GMP_RNDN));
//
//        mpfr_clear(mpX);
//        mpfr_clear(mpR);
//    }


    OperatorPtr MonotoneFunctionROM::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
        string func;
        int inW, outW;
        UserInterface::parseString(args, "function", &func);
        UserInterface::parseInt(args, "inputWidth", &inW);
        UserInterface::parseInt(args, "outputWidth", &outW);
        return new MonotoneFunctionROM(parentOp, target, func, inW, outW);
    }

    void MonotoneFunctionROM::registerFactory(){
        UserInterface::add("MonotoneFunctionROM", // name
                           "Generates a LUT.", // description, string
                           "Miscellaneous", // category, from the list defined in UserInterface.cpp
                           "",
                           "function(string)=x: Algorithm Type: normal, diff, lut;\
                        inputWidth(int)=16: Input bit count; \
                        outputWidth(int)=8: Output bit count",
                           "Feel free to experiment with its code, it will not break anything in FloPoCo. <br> Also see the developer manual in the doc/ directory of FloPoCo.",
                           MonotoneFunctionROM::parseArguments
        ) ;
    }

    mpz_class MonotoneFunctionROM::function(int x) {
        mpz_class lut_in(x), lut_out;

        eval(lut_out, lut_in);

        return lut_out;
    }

    void MonotoneFunctionROM::build() {
        addInput("X", inputWidth);
        addOutput("Y", outputWidth);
//        string in = declare("inp", inputWidth);
//        string out = declare("outp", outputWidth);

        vector<mpz_class> values = vector<mpz_class>();

//        vhdl << tab << "with i select o <=" << endl;

        for(int x = 0; x < pow(2, inputWidth); ++x) {
            values.emplace_back(function(x));

//            vhdl << tab << tab << "\"" << unsignedBinary(function(x), outputWidth)
//                 << "\" when \"" << unsignedBinary(mpz_class(x), inputWidth) << "\"," << endl;

        }

//        vhdl << tab << tab << "\"";
//
//        for(int x = 0; x < outputWidth; ++x) {
//            vhdl << "-";
//        }

//        vhdl <<"\" when others;" << endl;
//        REPORT(DEBUG,"Last LUT value f(" << values.size() << ") = " << values.back().get_str(10) << " max y = " << y);

        ComparatorTable *ct = new ComparatorTable(nullptr, getTarget(), inputWidth, outputWidth, values);

        //ct->inPortMap(this, in, "X");
        //ct->outPortMap(this, out, "Y");
        this->inPortMap(ct, "X", "i");
        this->outPortMap(ct, "Y", "o");
        addSubComponent(ct);

        //vhdl << this->newInstance(ct, join("ct", inputWidth),);
        //vhdl << tab << in << " <= i;" << endl;
        //vhdl << tab << "o <= " << out << ";" << endl;
    }

}//namespace