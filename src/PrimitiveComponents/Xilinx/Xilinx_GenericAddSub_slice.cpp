// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "Xilinx_GenericAddSub_slice.hpp"
#include "Xilinx_LUT6.hpp"
#include "Xilinx_CARRY4.hpp"

using namespace std;

namespace flopoco {
    Xilinx_GenericAddSub_slice::Xilinx_GenericAddSub_slice( Target *target, int wIn, bool initial, bool fixed, bool dss , const string &prefix ) : Operator( target ) {
        setCopyrightString( UniKs::getAuthorsString( UniKs::AUTHOR_MKLEINLEIN ) );
        UniKs::addUnisimLibrary( this );
        Xilinx_Primitive::checkTargetCompatibility( target );
        stringstream name;

        if( prefix.empty() ) {
            name << "Xilinx_GenericAddSub_slice_" << wIn;
        } else {
            name << prefix << "_slice" << wIn;
        }

        if( dss ) {
            name << "_dss";
        }

        if( initial ) {
            name << "_init";
        }

        setName( name.str() );
        setCombinatorial();
        srcFileName = "Xilinx_GenericAddSub_slice";
        REPORT( DEBUG , "Building" + this->getName() );

        if( dss ) {
            build_with_dss( target, wIn, initial );
        } else if( fixed ) {
            build_fixed_sign( target, wIn, initial );
        } else {
            build_normal( target, wIn, initial );
        }
    }

    void Xilinx_GenericAddSub_slice::build_normal( Target *target, int wIn, bool initial ) {
        lut_op carry_pre_o6;
        lut_op carry_pre_o5 = lut_in( 2 ) | lut_in( 3 );
        lut_op add_o5 =
            ( ~lut_in( 2 ) & ~lut_in( 3 ) &  lut_in( 0 ) ) |
            ( lut_in( 2 ) & ~lut_in( 3 ) & ~lut_in( 0 ) ) |
            ( ~lut_in( 2 ) &  lut_in( 3 ) &  lut_in( 0 ) ) ;
        lut_op add_o6 =
            ( ~lut_in( 2 ) & ~lut_in( 3 ) & ( lut_in( 0 )^ lut_in( 1 ) ) ) |
            ( lut_in( 2 ) & ~lut_in( 3 ) & ( ~lut_in( 0 )^ lut_in( 1 ) ) ) |
            ( ~lut_in( 2 ) &  lut_in( 3 ) & ( lut_in( 0 )^ ~lut_in( 1 ) ) ) ;
        lut_init carry_pre( carry_pre_o5, carry_pre_o6 );
        lut_init adder( add_o5, add_o6 );
        addInput( "x_in", wIn );
        addInput( "y_in", wIn );
        addInput( "neg_x_in", 1 , false );
        addInput( "neg_y_in", 1 , false );
        addInput( "carry_in",  1 , false );
        addOutput( "carry_out",  1 , 1, false );
        addOutput( "sum_out", wIn );
        declare( "cc_di", 4 );
        declare( "cc_s", 4 );
        declare( "cc_o", 4 );
        declare( "cc_co", 4 );
        declare( "lut_o5", wIn );
        declare( "lut_o6", wIn );

        if ( wIn < 4 ) {
            stringstream fillup;

            if ( 4 - wIn == 1 ) {
                fillup << "'0'";
            } else {
                fillup << "(3 downto " << ( wIn ) << " => '0')";
            }

            vhdl << tab << "cc_di" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_s" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_di" << range( wIn - 1, 0 ) << " <= lut_o5;" << std::endl;
            vhdl << tab << "cc_s" << range( wIn - 1, 0 ) << " <= lut_o6;" << std::endl;
        } else {
            vhdl << tab << "cc_di" << " <= lut_o5;" << std::endl;
            vhdl << tab << "cc_s" << " <= lut_o6;" << std::endl;
        }

        for( int i = 0; i < wIn; i++ ) {
            stringstream lut_name;
            lut_name << "lut_bit_" << i;
            Xilinx_LUT6_2 *initial_lut = new Xilinx_LUT6_2( target );

            if( initial && i == 0 ) {
                initial_lut->setGeneric( "init", carry_pre.get_hex() );
            } else {
                initial_lut->setGeneric( "init", adder.get_hex() );
            }

            inPortMap( initial_lut, "i0", "y_in" + of( i ) );
            inPortMap( initial_lut, "i1", "x_in" + of( i ) );
            inPortMap( initial_lut, "i2", "neg_y_in" );
            inPortMap( initial_lut, "i3", "neg_x_in" );

            if( initial && i == 0 ) {
                inPortMapCst( initial_lut, "i4", "'0'" );
            } else {
                inPortMapCst( initial_lut, "i4", "'1'" );
            }

            inPortMapCst( initial_lut, "i5", "'1'" );
            outPortMap( initial_lut, "o5", "lut_o5" + of( i ), false );
            outPortMap( initial_lut, "o6", "lut_o6" + of( i ), false );
            vhdl << initial_lut->primitiveInstance( lut_name.str() );
        }

        Xilinx_CARRY4 *further_cc = new Xilinx_CARRY4( target );
        outPortMap( further_cc, "co", "cc_co", false );
        outPortMap( further_cc, "o", "cc_o", false );
        inPortMapCst( further_cc, "cyinit", "'0'" );
        inPortMap( further_cc, "ci", "carry_in" );
        inPortMap( further_cc, "di", "cc_di" );
        inPortMap( further_cc, "s", "cc_s" );
        vhdl << further_cc->primitiveInstance( "slice_cc" );
        vhdl << "carry_out <= cc_co" << of( wIn - 1 ) << ";" << std::endl;
        vhdl << "sum_out <= cc_o" << range( wIn - 1, 0 ) << ";" << std::endl;
    }

    void Xilinx_GenericAddSub_slice::build_fixed_sign( Target *target, int wIn, bool initial ) {
        lut_op add_o5 =   ( ~lut_in( 2 ) & ~lut_in( 3 ) &  lut_in( 0 ) ) |
                          ( lut_in( 2 ) & ~lut_in( 3 ) & ~lut_in( 0 ) ) |
                          ( ~lut_in( 2 ) &  lut_in( 3 ) &  lut_in( 0 ) ) ;
        lut_op add_o6 =   ( ~lut_in( 2 ) & ~lut_in( 3 ) & ( lut_in( 0 )^ lut_in( 1 ) ) ) |
                          ( lut_in( 2 ) & ~lut_in( 3 ) & ( ~lut_in( 0 )^ lut_in( 1 ) ) ) |
                          ( ~lut_in( 2 ) &  lut_in( 3 ) & ( lut_in( 0 ) ^ ~lut_in( 1 ) ) ) ;
        lut_init adder( add_o5, add_o6 );
        addInput( "x_in", wIn );
        addInput( "y_in", wIn );
        addInput( "neg_x_in", 1, false );
        addInput( "neg_y_in", 1, false );
        addInput( "carry_in",  1 , false );
        addOutput( "carry_out",  1 , 1, false );
        addOutput( "sum_out", wIn );
        declare( "cc_di", 4 );
        declare( "cc_s", 4 );
        declare( "cc_o", 4 );
        declare( "cc_co", 4 );
        declare( "lut_o5", wIn );
        declare( "lut_o6", wIn );

        if ( wIn < 4 ) {
            stringstream fillup;

            if ( 4 - wIn == 1 ) {
                fillup << "'0'";
            } else {
                fillup << "(3 downto " << ( wIn ) << " => '0')";
            }

            vhdl << tab << "cc_di" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_s" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_di" << range( wIn - 1, 0 ) << " <= lut_o5;" << std::endl;
            vhdl << tab << "cc_s" << range( wIn - 1, 0 ) << " <= lut_o6;" << std::endl;
        } else {
            vhdl << tab << "cc_di" << " <= lut_o5;" << std::endl;
            vhdl << tab << "cc_s" << " <= lut_o6;" << std::endl;
        }

        for( int i = 0; i < wIn; i++ ) {
            stringstream lut_name;
            lut_name << "lut_bit_" << i;
            Xilinx_LUT6_2 *initial_lut = new Xilinx_LUT6_2( target );
            initial_lut->setGeneric( "init", adder.get_hex() );
            inPortMap( initial_lut, "i0", "y_in" + of( i ) );
            inPortMap( initial_lut, "i1", "x_in" + of( i ) );
            inPortMap( initial_lut, "i2", "neg_y_in" );
            inPortMap( initial_lut, "i3", "neg_x_in" );
            inPortMapCst( initial_lut, "i4", "'1'" );
            inPortMapCst( initial_lut, "i5", "'1'" );
            outPortMap( initial_lut, "o5", "lut_o5" + of( i ), false );
            outPortMap( initial_lut, "o6", "lut_o6" + of( i ), false );
            vhdl << initial_lut->primitiveInstance( lut_name.str() );
        }

        Xilinx_CARRY4 *further_cc = new Xilinx_CARRY4( target );
        outPortMap( further_cc, "co", "cc_co", false );
        outPortMap( further_cc, "o", "cc_o", false );
        inPortMapCst( further_cc, "cyinit", "'0'" );
        inPortMap( further_cc, "ci", "carry_in" );
        inPortMap( further_cc, "di", "cc_di" );
        inPortMap( further_cc, "s", "cc_s" );
        vhdl << further_cc->primitiveInstance( "slice_cc" );
        vhdl << "carry_out <= cc_co" << of( wIn - 1 ) << ";" << std::endl;
        vhdl << "sum_out <= cc_o" << range( wIn - 1, 0 ) << ";" << std::endl;
    }

    void Xilinx_GenericAddSub_slice::build_with_dss( Target *target, int wIn, bool initial ) {
        addInput( "x_in", wIn );
        addInput( "y_in", wIn );
        addInput( "neg_x_in", 1, false );
        addInput( "neg_y_in", 1, false );
        addInput( "carry_in",  1 , false );
        addOutput( "carry_out",  1 , 1, false );
        addOutput( "sum_out", wIn );
        addInput( "bbus_in", wIn );
        addOutput( "bbus_out", wIn );
        declare( "cc_di", 4 );
        declare( "cc_s", 4 );
        declare( "cc_o", 4 );
        declare( "cc_co", 4 );
        declare( "lut_o5", wIn );
        declare( "lut_o6", wIn );
        declare( "bb_t", wIn );

        if ( wIn < 4 ) {
            stringstream fillup;

            if ( 4 - wIn == 1 ) {
                fillup << "'0'";
            } else {
                fillup << "(3 downto " << ( wIn ) << " => '0')";
            }

            vhdl << tab << "cc_di" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_s" << range( 3, wIn ) << " <= " << fillup.str() << ";" << std::endl;
            vhdl << tab << "cc_di" << range( wIn - 1, 0 ) << " <= bbus_in;" << std::endl;
            vhdl << tab << "cc_s" << range( wIn - 1, 0 ) << " <= lut_o6;" << std::endl;
        } else {
            vhdl << tab << "cc_di" << " <= bbus_in;" << std::endl;
            vhdl << tab << "cc_s" << " <= lut_o6;" << std::endl;
        }

        for( int i = 0; i < wIn; i++ ) {
            stringstream lut_name;
            lut_name << "lut_bit_" << i;
            Xilinx_LUT6_2 *cur_lut = new Xilinx_LUT6_2( target );

            if( initial && i == 0 ) {
                cur_lut->setGeneric( "init", getLUT_dss_init() );
            } else if( initial && i == 1 ) {
                cur_lut->setGeneric( "init", getLUT_dss_sec() );
            } else {
                cur_lut->setGeneric( "init", getLUT_dss_std() );
            }

            inPortMap( cur_lut, "i0", "y_in" + of( i ) );
            inPortMap( cur_lut, "i1", "x_in" + of( i ) );
            inPortMap( cur_lut, "i2", "neg_y_in" );
            inPortMap( cur_lut, "i3", "neg_x_in" );
            inPortMap( cur_lut, "i4", "bbus_in" + of( i ) );
            inPortMapCst( cur_lut, "i5", "'1'" );
            outPortMap( cur_lut, "o5", "bb_t" + of( i ), false );
            outPortMap( cur_lut, "o6", "lut_o6" + of( i ), false );
            vhdl << cur_lut->primitiveInstance( lut_name.str() );
        }

        Xilinx_CARRY4 *further_cc = new Xilinx_CARRY4( target );
        outPortMap( further_cc, "co", "cc_co", false );
        outPortMap( further_cc, "o", "cc_o", false );
        inPortMapCst( further_cc, "cyinit", "'0'" );
        inPortMap( further_cc, "ci", "carry_in" );
        inPortMap( further_cc, "di", "cc_di" );
        inPortMap( further_cc, "s", "cc_s" );
        vhdl << further_cc->primitiveInstance( "slice_cc" );
        vhdl << "carry_out <= cc_co" << of( wIn - 1 ) << ";" << std::endl;
        vhdl << "sum_out <= cc_o" << range( wIn - 1, 0 ) << ";" << std::endl;
        vhdl << "bbus_out <= bb_t;" << std::endl;
    }

    string Xilinx_GenericAddSub_slice::getLUT_dss_init() {
        lut_op fa_s = ( lut_in( 0 )^lut_in( 2 ) ) ^ ( lut_in( 1 )^lut_in( 3 ) ) ^ ( lut_in( 2 )^lut_in( 3 ) );
        lut_op fa_c =
            ( ( lut_in( 0 )^lut_in( 2 ) ) & ( lut_in( 1 )^lut_in( 3 ) ) ) |
            ( ( lut_in( 0 )^lut_in( 2 ) ) & ( lut_in( 2 )^lut_in( 3 ) ) ) |
            ( ( lut_in( 1 )^lut_in( 3 ) ) & ( lut_in( 2 )^lut_in( 3 ) ) );
        lut_op o5 = fa_c;
        lut_op o6 = fa_s;
        lut_init op( o5, o6 );
        return op.get_hex();
    }

    string Xilinx_GenericAddSub_slice::getLUT_dss_sec() {
        lut_op fa_s = ( lut_in( 0 )^lut_in( 2 ) ) ^ ( lut_in( 1 )^lut_in( 3 ) ) ^ ( lut_in( 2 )&lut_in( 3 ) );
        lut_op fa_c =
            ( ( lut_in( 0 )^lut_in( 2 ) ) & ( lut_in( 1 )^lut_in( 3 ) ) ) |
            ( ( lut_in( 0 )^lut_in( 2 ) ) & ( lut_in( 2 )&lut_in( 3 ) ) ) |
            ( ( lut_in( 1 )^lut_in( 3 ) ) & ( lut_in( 2 )&lut_in( 3 ) ) );
        lut_op o5 = fa_c;
        lut_op o6 = fa_s ^ lut_in( 4 );
        lut_init op( o5, o6 );
        return op.get_hex();
    }

    string Xilinx_GenericAddSub_slice::getLUT_dss_std() {
        lut_op fa_s = ( lut_in( 0 )^lut_in( 2 ) ) ^ ( lut_in( 1 )^lut_in( 3 ) );
        lut_op fa_c = ( ( lut_in( 0 )^lut_in( 2 ) ) & ( lut_in( 1 )^lut_in( 3 ) ) );
        lut_op o5 = fa_c;
        lut_op o6 = fa_s ^ lut_in( 4 );
        lut_init op( o5, o6 );
        return op.get_hex();
    };
}//namespace
