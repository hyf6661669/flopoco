#include <iostream>
#include <sstream>
#include <string>
#include "gmp.h"
#include "mpfr.h"
#include <vector>
#include <gmpxx.h>
#include <stdio.h>
#include <stdlib.h>
#include "FourToTwoCompressor.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT6.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_CARRY4.hpp"
#include "../PrimitiveComponents/Xilinx/Xilinx_LUT_compute.h"

using namespace std;
namespace flopoco{

    FourToTwoCompressor::FourToTwoCompressor(Target * target, int width, bool useLastColumn) : VariableColumnCompressor(target)
	{
        this->useLastColumn = useLastColumn;
        setWidth(width);

        ostringstream name;
        name << "Compressor_4_to_2_type" << useLastColumn << "_width_" << width;
        setName(name.str());

        for(unsigned i=0;i<height.size();i++)
        {
            if(height[height.size()-i-1] > 0)
                addInput(join("X",i), height[height.size()-i-1]);
        }

        addOutput("R0", wOut);
        addOutput("R1", wOut);


        int needed_cc = ( width / 4 ) + ( width % 4 > 0 ? 1 : 0 ); //no. of required carry chains

        cout << "no of required carry-chains for width=" << width << " is " << needed_cc << endl;

        declare( "cc_s", needed_cc * 4 );
        declare( "cc_di", needed_cc * 4 );
        declare( "cc_co", needed_cc * 4 );
        declare( "cc_o", needed_cc * 4 );

        //init unused carry-chain inputs to zero:
        if(needed_cc*4 > width)
        {
            vhdl << tab << "cc_s(" << needed_cc*4-1 << " downto " << width << ") <= (others => '0');" << endl;
            vhdl << tab << "cc_di(" << needed_cc*4-1 << " downto " << width << ") <= (others => '0');" << endl;
        }
        vhdl << tab << "cc_di(" << width-1 << " downto 0) <= ";

        if(useLastColumn)
            vhdl << "X" << width-1 << "(1)";
        else
            vhdl << "'0'";

        for(int i=1; i < width; i++)
        {
            vhdl << " & X" << width-i-1 << "(3)";
        }
        vhdl << ";" << endl;

        vhdl << endl;

        //create LUTs, except the last LUT:
        for(int i=0; i < width-1; i++)
        {
            //LUT content of the LUTs exept the last LUT:
            lut_op lutop_o6 = lut_in(0) ^ lut_in(1) ^ lut_in(2) ^ lut_in(3); //sum out of full adder xor input 3
            lut_op lutop_o5 = (lut_in(0) & lut_in(1)) | (lut_in(0) & lut_in(2)) | (lut_in(1) & lut_in(2)); //carry out of full adder
            lut_init lutop( lutop_o5, lutop_o6 );

            Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2( target );
            cur_lut->setGeneric( "init", lutop.get_hex() );

            inPortMap(cur_lut,"i0",join("X",i) + of(0));
            inPortMap(cur_lut,"i1",join("X",i) + of(1));
            inPortMap(cur_lut,"i2",join("X",i) + of(2));
            inPortMap(cur_lut,"i3",join("X",i) + of(3));
            inPortMapCst(cur_lut, "i4","'0'");
            inPortMapCst(cur_lut, "i5","'1'");

            outPortMap(cur_lut,"o5","R1" + of(i+1),false);
            outPortMap(cur_lut,"o6","cc_s" + of(i),false);

            vhdl << cur_lut->primitiveInstance( join("lut",i), this ) << endl;
            //        addToGlobalOpList(cur_lut);
        }

        if(useLastColumn)
        {
            //create last LUT:
            lut_op lutop_o6 = lut_in(0) ^ lut_in(3); //input 0 xor input 3
            lut_op lutop_o5 = lut_in(3); //identical with input 3
            lut_init lutop( lutop_o5, lutop_o6 );

            Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2( target );
            cur_lut->setGeneric( "init", lutop.get_hex() );

            inPortMap(cur_lut,"i0",join("X",width-1) + of(0));
            inPortMapCst(cur_lut,"i1","'0'");
            inPortMapCst(cur_lut,"i2","'0'");
            inPortMap(cur_lut,"i3",join("X",width-1) + of(1));
            inPortMapCst(cur_lut, "i4","'0'");
            inPortMapCst(cur_lut, "i5","'1'");
            outPortMap(cur_lut,"o5","open",false);
            outPortMap(cur_lut,"o6","cc_s" + of(width-1),false);

            vhdl << cur_lut->primitiveInstance( join("lut",width-1), this ) << endl;
        }
        else
        {
            vhdl << "cc_s" + of(width-1) << " <= '0';" << endl;
        }

        for( int i = 0; i < needed_cc; i++ ) {
            Xilinx_CARRY4 *cur_cc = new Xilinx_CARRY4( target );

            inPortMapCst( cur_cc, "cyinit", "'0'" );
            if( i == 0 ) {
                inPortMapCst( cur_cc, "ci", "'0'" ); //carry-in can not be used as AX input is blocked!!
            } else {
                inPortMap( cur_cc, "ci", "cc_co" + of( i * 4 - 1 ) );
            }
            inPortMap( cur_cc, "di", "cc_di" + range( i * 4 + 3, i * 4 ) );
            inPortMap( cur_cc, "s", "cc_s" + range( i * 4 + 3, i * 4 ) );
            outPortMap( cur_cc, "co", "cc_co" + range( i * 4 + 3, i * 4 ), false);
            outPortMap( cur_cc, "o", "cc_o" + range( i * 4 + 3, i * 4 ), false);

            stringstream cc_name;
            cc_name << "cc_" << i;
            vhdl << cur_cc->primitiveInstance( cc_name.str(), this );
        }

        vhdl << endl;

        vhdl << tab << "R0 <= cc_co(" << width-1 << ") & cc_o(" << width-1 << " downto 0);" << endl;
    }
	
    FourToTwoCompressor::~FourToTwoCompressor()
	{
	}
	
    void FourToTwoCompressor::setWidth(int width)
    {
        this->width = width;

        //adjust size of basic compressor to the size of a variable column compressor of this specific size:
        height.resize(width);

        //no of bits at MSB position
        if(useLastColumn)
            height[0] = 2;
        else
            height[0] = 0;

        for(int i=1; i < width-1; i++)
        {
            height[i] = 4;
        }
        height[width-1] = 4;

        wOut=width+1;

        outputs.resize(wOut);
        if(useLastColumn)
            outputs[0] = 1; //there is one output at MSB
        else
            outputs[0] = 0; //there is no output at MSB

        for(int i=1; i < wOut-1; i++)
        {
            outputs[i] = 2;
        }
        outputs[wOut-1] = 1; //there is one output at LSB
    }
}
	


	
