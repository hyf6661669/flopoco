#include <iostream>
#include <sstream>

#include "gmp.h"
#include "mpfr.h"
#include "GenericMux.hpp"
#include "Primitive.hpp"

#include "Xilinx/Xilinx_GenericMux.hpp"

using namespace std;
namespace flopoco {
    GenericMux::GenericMux(Target* target, const uint32_t &wIn, const uint32_t &inputCount) : Operator(target) {
        setCopyrightString( UniKs::getAuthorsString( UniKs::AUTHOR_MKLEINLEIN ) );
        srcFileName="GenericMux";
		ostringstream name;
        name << "GenericMux_w" << wIn << "_s" << inputCount;
        setName(name.str());

        for( uint i=0;i<inputCount;++i )
            addInput(getInputName(i),wIn);

        addInput(getSelectName(), intlog2( inputCount ) );
        addOutput(getOutputName(),wIn);

        if( UserInterface::useTargetSpecificOptimization && target->getVendor() == "Xilinx" )
            buildXilinx(target,wIn,inputCount);
        else if( UserInterface::useTargetSpecificOptimization && target->getVendor()=="Altera" )
            buildAltera(target,wIn,inputCount);
        else
            buildCommon(target,wIn,inputCount);

    }

    void GenericMux::buildXilinx(Target* target, const uint32_t &wIn, const uint32_t &inputCount){
        Xilinx_GenericMux *mux = new Xilinx_GenericMux(target,inputCount,wIn );
        inPortMap(mux,"s_in",getSelectName());
        for( uint i=0;i<inputCount;++i )
            inPortMap(mux,join( "x", i, "_in" ), getInputName(i));

        outPortMap(mux,"x_out",getOutputName() ,false );
    }

    void GenericMux::buildAltera(Target *target, const uint32_t &wIn, const uint32_t &inputCount){
        REPORT(LIST,"Altera junction not fully implemented, fall back to common.");
        buildCommon(target,wIn,inputCount);
    }

    void GenericMux::buildCommon(Target* target, const uint32_t &wIn, const uint32_t &inputCount){
        const uint16_t select_ws = intlog2( inputCount );

        vhdl << "case " << getSelectName() << " is" << std::endl;
        for( uint i=0;i<inputCount;++i ){
            vhdl << "\t" << "when \"";
            for( uint j=select_ws-1;j>=0;--j )
                vhdl << (i&(1<<j)?"1":"0");
            vhdl << "\" => " << getOutputName() << " <= " << getInputName(i);
        }

        vhdl << "\t" << "when others => oSum <= (others=>'X');" << std::endl;
        vhdl << "end case;" << std::endl;

    }

    std::string GenericMux::getSelectName() const{
        return "iSel";
    }

    std::string GenericMux::getInputName(const uint32_t &index) const{
        std::stringstream o;
        o << "iS_" << index;
        return o.str();
    }

    std::string GenericMux::getOutputName() const{
        return "oMux";
    }

    void GenericMux::emulate(TestCase * tc) {

	}


    void GenericMux::buildStandardTestCases(TestCaseList * tcl) {
		// please fill me with regression tests or corner case tests!
	}





    OperatorPtr GenericMux::parseArguments(Target *target, vector<string> &args) {
        /*
		int param0, param1;
		UserInterface::parseInt(args, "param0", &param0); // param0 has a default value, this method will recover it if it doesnt't find it in args, 
		UserInterface::parseInt(args, "param1", &param1); 
        return new UserDefinedOperator(target, param0, param1);*/
        return nullptr;
	}
	
    void GenericMux::registerFactory(){
        /*UserInterface::add("UserDefinedOperator", // name
											 "An heavily commented example operator to start with FloPoCo.", // description, string
											 "Miscellaneous", // category, from the list defined in UserInterface.cpp
											 "", //seeAlso
											 // Now comes the parameter description string.
											 // Respect its syntax because it will be used to generate the parser and the docs
											 // Syntax is: a semicolon-separated list of parameterDescription;
											 // where parameterDescription is parameterName (parameterType)[=defaultValue]: parameterDescriptionString 
											 "param0(int)=16: A first parameter, here used as the input size; \
                        param1(int): A second parameter, here used as the output size",
											 // More documentation for the HTML pages. If you want to link to your blog, it is here.
											 "Feel free to experiment with its code, it will not break anything in FloPoCo. <br> Also see the developper manual in the doc/ directory of FloPoCo.",
											 UserDefinedOperator::parseArguments
                                             ) ;*/
	}

}//namespace