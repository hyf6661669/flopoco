//
// Created by Annika Oeste on 21.03.21.
//

// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitrary large
   entries */
#include "gmp.h"
#include "mpfr.h"

#include "BitHeapModuloTest.hpp"

using namespace std;
namespace flopoco {


    BitHeapModuloTest::BitHeapModuloTest(Target* target, int wIn_, int mod_, int maxInput_, string mode_, string pseudoCompMode_) : Operator(target), wIn(wIn_), mod(mod_), maxInput(maxInput_) {

        // definition of the source file name, used for info and error reporting using REPORT
        srcFileName="BitHeapModuloTest";

        // definition of the name of the operator
        ostringstream name;
        name << "BitHeapModuloTest_" << wIn << "_" << mod << "_" << abs(maxInput);
        setName(name.str()); // See also setNameWithFrequencyAndUID()
        // Copyright
        setCopyrightString("Annika Oeste, 2020");
        useNumericStd();

        /* SET UP THE IO SIGNALS
           Each IO signal is declared by addInput(name,n) or addOutput(name,n)
           where name is a string that stands for the name of the variable and
           n is an integer (int)   that stands for the length of the corresponding
           input/output */

        // wIn not needed for maxvalue -> only max needed bits as wIn
        if (maxInput != -1) {
            int maxInputSize = floor(log2(maxInput)+1);
            if (wIn > maxInputSize) {
                wIn = maxInputSize;
            }
        }
        // wIn too small for maxvalue -> use wIn range

        // declaring inputs
        addInput ("X" , wIn);
        addConstant("M", "positive", mod);
        // declaring output
        int modSize = floor(log2(mod)+1);
        addOutput("R" , modSize);

        addFullComment("Start of vhdl generation"); // this will be a large, centered comment in the VHDL
        // basic message
        REPORT(INFO,"Declaration of BitHeapModuloTest \n");
        REPORT(DETAILED, "this operator has received two parameters " << wIn << " and " << mod);

        // if wIn is smaller than the bit width of the modulo, no computation has to be done
        if (wIn < modSize) {
            addFullComment("Start of vhdl generation");
            vhdl << tab << "R <= (" << modSize-1 << " downto " << wIn << " => '0') & X;" << endl;
            addFullComment("End of vhdl generation");
            return;
        }

        transform(mode_.begin(), mode_.end(), mode_.begin(), ::tolower);

        BitHeap *bitHeap;
        bitHeap = new BitHeap(this, wIn, "moduloBitheap", 0, mod, maxInput, mode_, pseudoCompMode_);
        bitHeap->addSignal("X", 0);
        bitHeap->startCompression();

        int bitHeapResultBits = wIn;

        if (mode_.find("pos") != string::npos) {
            vhdl << tab << declare(
                    "STemp", bitHeapResultBits, false) << tab << "<= STD_LOGIC_VECTOR(SIGNED(" << bitHeap->getSumName(bitHeapResultBits-1,0) << ") - M);" << endl;
            vhdl << tab << "R <= " << bitHeap->getSumName(modSize-1,0) << " when ";
            vhdl << "UNSIGNED(" << bitHeap->getSumName(bitHeapResultBits-1,0) << ") < M";
            vhdl << tab << "else STemp" << range(modSize-1,0) << ";" << endl;
        } else {
            vhdl << tab << declare(
                    "STemp", bitHeapResultBits, false) << tab << "<= STD_LOGIC_VECTOR(SIGNED(" << bitHeap->getSumName(bitHeapResultBits-1,0) << ") + M);" << endl;
            vhdl << tab << "R <= " << bitHeap->getSumName(modSize-1,0) << " when ";
            vhdl << bitHeap->getSumName() << of(bitHeapResultBits-1) << " = '0'";
            vhdl << tab << "else STemp" << range(modSize-1,0) << ";" << endl;
        }
        delete bitHeap;

        addFullComment("End of vhdl generation"); // this will be a large, centered comment in the VHDL
    };

    int BitHeapModuloTest::reqBitsForRange2Complement(int min, int max) {
        REPORT(DEBUG, "reqBitsForRange 2 complement min: " << min << " max " << max);
        int bit = 0;
        if (max > 0) {
            while((max >> bit) != 0) {
                bit++;
            }
        }
        if (min < 0) {
            int negBit = 0;
            int currentMin = min;
            while(currentMin <= -1) {
                currentMin = currentMin / 2;
                negBit++;
            }
            if (bit < negBit) {
                bit = negBit + 1;
            } else {
                bit++;
            }
        }
        return bit;
    }


    void BitHeapModuloTest::emulate(TestCase * tc) {
        /* This function will be used when the TestBench command is used in the command line
           we have to provide a complete and correct emulation of the operator, in order to compare correct output generated by this function with the test input generated by the vhdl code */
        /* first we are going to format the entries */
        mpz_class sx = tc->getInputValue("X");

        /* then we are going to manipulate our bit vectors in order to get the correct output*/
        mpz_class sr;
        sr = sx % mod;

        /* at the end, we indicate to the TestCase object what is the expected
           output corresponding to the inputs */
        tc->addExpectedOutput("R",sr);
    }


    void BitHeapModuloTest::buildStandardTestCases(TestCaseList * tcl) {
        // please fill me with regression tests or corner case tests!
        TestCase *tc;
        if (wIn >= 4) {
            tc = new TestCase(this);
            tc->addInput("X", mpz_class(14));
            emulate(tc);
            tcl->add(tc);
        }
        if (wIn >= 5) {
            tc = new TestCase(this);
            tc->addInput("X", mpz_class(21));
            emulate(tc);
            tcl->add(tc);
        }
        if (wIn >= 6) {
            tc = new TestCase(this);
            tc->addInput("X", mpz_class(40));
            emulate(tc);
            tcl->add(tc);

            tc = new TestCase(this);
            tc->addInput("X", mpz_class(44));
            emulate(tc);
            tcl->add(tc);

            tc = new TestCase(this);
            tc->addInput("X", mpz_class(63));
            emulate(tc);
            tcl->add(tc);
        }
        if (wIn >= 9) {
            tc = new TestCase(this);
            tc->addInput("X", mpz_class(506));
            emulate(tc);
            tcl->add(tc);
        }
    }

    TestList BitHeapModuloTest::unitTest(int index)
    {
        // the static list of mandatory tests
        TestList testStateList;
        vector<pair<string,string>> paramList;
        string modes[] = {"msbCasesSingleBitSEVector"};

        if(index==-1) 	{ // The unit tests
            for (int i = 0; i < 1; i++) {
                for (int wIn=64; wIn<=64; wIn++) {
                    for(int m=11; m<61; m+=2) {
                        paramList.push_back(make_pair("target","GenericAsic"));
                        paramList.push_back(make_pair("wIn",to_string(wIn)));
                        paramList.push_back(make_pair("mod",to_string(m)));
                        paramList.push_back(make_pair("mode",modes[i]));
                        paramList.push_back(make_pair("pseudoCompMode", "lMinBits"));
                        paramList.push_back(make_pair("TestBench n=",to_string(100)));
                        testStateList.push_back(paramList);

                        paramList.clear();
                    }
                }
            }
        }
        return testStateList;
    }

    OperatorPtr BitHeapModuloTest::parseArguments(OperatorPtr parentOp, Target *target, vector<string> &args) {
        int wIn, mod, maxInput;
        string mode, pseudoCompMode;
        UserInterface::parseInt(args, "wIn", &wIn); // wIn has a default value, this method will recover it if it doesn't find it in args,
        UserInterface::parseInt(args, "mod", &mod);
        UserInterface::parseInt(args, "maxInput", &maxInput);
        UserInterface::parseString(args, "mode", &mode);
        UserInterface::parseString(args, "pseudoCompMode", &pseudoCompMode);
        return new BitHeapModuloTest(target, wIn, mod, maxInput, mode, pseudoCompMode);
    }

    void BitHeapModuloTest::registerFactory(){
        UserInterface::add("BitHeapModuloTest", // name
                           "An operator to test a bitheap with an integrated modulo operation", // description, string
                           "Miscellaneous", // category, from the list defined in UserInterface.cpp
                           "", //seeAlso
                // Now comes the parameter description string.
                // Respect its syntax because it will be used to generate the parser and the docs
                // Syntax is: a semicolon-separated list of parameterDescription;
                // where parameterDescription is parameterName (parameterType)[=defaultValue]: parameterDescriptionString
                           "wIn(int)=8: input size; \
                            mod(int): modulus for the modulo computation; \
                            maxInput(int)=-1: maximal allowed input value; \
                            mode(string)=default: mode for testing different variations"
                           "available are:"
                           "pos:"
                           " - seVector:"
                           " - singleBitPos:"
                           " - singleBitSEVector:"
                           " - msbCasesPos:"
                           " - msbCasesSEVector:"
                           " - default: ; \
                           pseudoCompMode(string)=minRange: mode for pseudocompression to choose which pseudocomrpessor to use"
                           "available are: minRange, lMinBits",
                // More documentation for the HTML pages. If you want to link to your blog, it is here.
                           "<br> Also see the developer manual in the doc/ directory of FloPoCo.",
                           BitHeapModuloTest::parseArguments,
                           BitHeapModuloTest::unitTest
        ) ;
    }

}//namespace
