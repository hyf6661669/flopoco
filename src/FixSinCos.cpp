#include "FloPoCo.hpp"

// works only with sollya
#ifdef HAVE_SOLLYA

#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
  There will be used in the emulate function to manipulate arbitraly large
  entries */
#include "gmp.h"
#include "mpfr.h"
#include <assert.h>
// for debug
#include <signal.h>

// include the header of the Operator
#include "FixSinCos.hpp"

using namespace std;
using namespace flopoco;


// personalized parameter
//string FixSinCos::operatorInfo = "FixSinCos w <options>";

#define SUBCYCLEPIPELINING 0

FixSinCos::FixSinCos(Target * target, int w_):Operator(target), w(w_)
{
/* constructor of the FixSinCos
  Target is the targeted FPGA : Stratix, Virtex ... (see Target.hpp for more informations)
  w and param1 are some parameters declared by this Operator developpers, 
  any number can be declared, you will have to modify 
      -> this function,  
      -> the prototype of this function (available in FixSinCos.hpp)
      -> the lines in main.cpp where the command line arguments are parsed in order to generate this FixSinCos
*/
	/* In this constructor we are going to generate an operator that takes as input three bit vectors X,Y,Z of lenght w, treats them as unsigned integers, sums them and then output the last param1 bit of the sum adding the first bit of the sum (most significant) in front of this output, all the vhdl code needed by this operator has to be generated in this function */

	// definition of the name of the operator
	ostringstream name;
	name << "FixSinCos_" << w ;
	setName(name.str());

	setCopyrightString("Guillaume Sergent (2012)");

	/* SET UP THE IO SIGNALS
	   Each IO signal is declared by addInput(name,n) or addOutput(name,n) 
	   where name is a string that stands for the name of the variable and 
	   n is an integer (int)   that stands for the length of the corresponding 
	   input/output */

	// declaring inputs
	addInput("X", w);
	//addFullComment(" addFullComment for a large comment ");
	//addComment("addComment for small left-aligned comment");

	// declaring outputs
	addOutput("S", w+1);
	addOutput("C", w+1);


	// the argument is reduced into (0,1/4) because one sin/cos
	// computation in this range can always compute the right sin/cos
	vhdl << tab << declare ("A",1) << " <= X" << of (w-1) << ";" << endl;
	vhdl << tab << declare ("B",1) << " <= X" << of (w-2) << ";" << endl;
	vhdl << tab << declare ("Y",w-2) << " <= X " << range (w-3,0) << ";" << endl;
	// now X -> A*.5 + B*.25 + Y where A,B \in {0,1} and Y \in {0,.25}

	// Y_prime = .25 - y
	// the VHDL is probably invalid
	// perhaps do a logic ~ at a cost of 1 ulp ?
	//vhdl << declare ("Y_prime",w-2) << " <= "
	//     << "2**" << (w-2) << " - Y;" << endl;
	// if we do an arithmetic 1's complement it will do **** since
	// 1-0 doesn't fit
	manageCriticalPath(target->localWireDelay(w-2) + target->lutDelay());
	vhdl << tab << declare ("Y_prime", w-2) << " <= " << "not Y;" << endl;

	// we need to know the number of guard bits _now_ to have a good Y_in
	const int g=4; //guard bits
	//const int g=3; //guard bits
	//we take 4 guard bits even if error < 8 ulp because rounding will take
	//another half final ulp
	int wIn = w-2+g;

	// Y_in = / Y_prime if B=1
	//        \ Y if B=0
	// and we extend the precision to make it look as if Y_prime was
	// 0.[1, wIn times] - Y: 1/2**g error reduction in Y_prime
	// (which should be arithmetic 1-Y)
	vhdl << tab << declare ("Y_in",wIn) << " <= Y_prime & "
	     << '"' << std::string (g, '1') << '"' << " when B='1' else Y & "
	     << '"' << std::string (g, '0') << '"' << ";"
	     << endl;

	// Exch = A ^ B
	vhdl << tab << declare ("Exch") << " <= A xor B;" << endl;

	// now we do manual polynomial computation to calculate sin (pi*y)
	// and cos (pi*y) using Taylor polynomials: with y'=pi*y
	// sin (4*pi*y) = y' - y'³/6
	// cos (4*pi*y) = 1 - y'²/2
	// this works if y' (or y) is small enough
	// to accomplish this we decompose x (==Y_in in the vhdl) to x = a + y
	// where a \in [0,1/4[ and {sin,cos} (pi*a) is tabulated
	// and y \in [0,1b-n[ is a small enough argument 
	// then we use the addition formulae (where Sin(x)=sin(pi*x)):
	// Sin (a+y) = Sin a Cos y + Cos a Sin y
	//           = Sin a - Sin a (1 - Cos y) + Cos a Sin y
	// Cos (a+y) = Cos a Cos y - Sin a Sin y
	//           = Cos a - Cos a (1 - Cos y) - Sin a Sin y
	// wA is the precision of A, beginning from the ,..[] bit
	// yes I know it's moronic to int->float->int
	// pi⁴/24=4.0587121 so (pi*y)⁴/24 is < (1+eps) ulp /this is not the thing to do actually/
	// iff 4 * (wA+2) - 2 >= w+g (2 as the log_2 of pi⁴/24)
	// the minimal a is therefore ceil((wIn)/4) - 2
	int wA = (int) ceil ((float) (w+g+2)/4.) - 2;
	if (wA <= 3)
		wA = 3;
	int wY = wIn-wA, wZ = wY+2;
	// vhdl:split (Y_in -> A_tbl & Y_red)
	vhdl << tab << declare ("A_tbl",wA) << " <= Y_in" << range (wIn-1,wIn-wA) << ";" << endl;
	vhdl << tab << declare ("Y_red",wY) << " <= Y_in" << range (wIn-wA-1,0) << ';' << endl;
	// vhdl:lut (A_tbl -> A_cos_pi_tbl, A_sin_pi_tbl)
	FunctionTable *sin_table, *cos_table;
	ostringstream omu; // calculates string of one minus (guardless) ulp
	omu << "(1 - 1b-" << w << ")";
	sin_table = new FunctionTable (target, omu.str() + " * sin(pi*x/4)",
	                               wA, -(w+g), -1);
	cos_table = new FunctionTable (target, omu.str() + " * cos(pi*x/4)",
	                               wA, -(w+g), -1);
	sin_table -> changeName(getName() + "_SinTable");
	cos_table -> changeName(getName() + "_CosTable");
	oplist.push_back (sin_table);
	oplist.push_back (cos_table);
	outPortMap (sin_table, "Y", "A_sin_pi_tbl");
	inPortMap (sin_table, "X", "A_tbl");
	outPortMap (cos_table, "Y", "A_cos_pi_tbl");
	inPortMap (cos_table, "X", "A_tbl");
	vhdl << instance (sin_table, "sin_table");
	vhdl << instance (cos_table, "cos_table");

	// the results have precision w+g
	// now, evaluate Sin Y_red and 1 - Cos Y_red
	// vhdl:cmul[pi] (Y_red -> Z)
	map<string, double> pi_mult_inputDelays;
	pi_mult_inputDelays["X"] = getCriticalPath();


#if 1 
	// For w=32: latency=18 (1 for the const mult), 781 + 1170	
	FixRealKCM *pi_mult;
	// the 2 extra bits to Z are added by the KCM multiplier
	pi_mult = new FixRealKCM (target, 0, wY-1, false, 0, "pi", 1.0, pi_mult_inputDelays); 
	oplist.push_back (pi_mult);
	inPortMap (pi_mult, "X", "Y_red");
	outPortMap (pi_mult, "R", "Z");
	vhdl << instance (pi_mult, "pi_mult");

	syncCycleFromSignal("Z", pi_mult->getOutputDelay("R"));
#else
	// For w=32: latency=17 (4 for the const mult), 863+1334
	// So this one seems to loose
	// TODO design a FixReal shift-and-add
	mpfr_t myPi;
	mpfr_init2 (myPi, wZ);
	mpfr_const_pi (myPi, GMP_RNDN);
	mpfr_mul_2si(myPi, myPi, wZ-2, GMP_RNDN);
	mpz_class mpzPi;
	mpfr_get_z(mpzPi.get_mpz_t(), myPi, GMP_RNDN);

	IntConstMult* pi_mult;
	pi_mult = new IntConstMult(target, wZ-2, mpzPi); 
	oplist.push_back (pi_mult);
	inPortMap (pi_mult, "X", "Y_red");
	outPortMap (pi_mult, "R", "Zfull");
	vhdl << instance (pi_mult, "pi_mult");
	int wZfull = getSignalByName("Zfull")->width();
	vhdl << tab << declare("Z",wZ) << " <= Zfull" << range (wZfull-1, wZfull-wZ) << ";" << endl; 
#endif


	map<string, double> sqr_z_inputDelays;
#if SUBCYCLEPIPELINING
	sqr_z_inputDelays["X"] = pi_mult->getOutputDelay("R");
	sqr_z_inputDelays["Y"] = pi_mult->getOutputDelay("R");
#else
	nextCycle();
#endif

	// vhdl:sqr (Z -> Z_2)
	// we have no truncated squarer as of now
	/*IntSquarer *sqr_z;
	sqr_z = new IntSquarer (target, wZ);
	oplist.push_back (sqr_z);
	inPortMap (sqr_z, "X", "Z");
	outPortMap (sqr_z, "R", "Z_2_ext");
	vhdl << instance (sqr_z, "sqr_z");
	// so now we truncate unnecessarily calculated bits of Z_2_ext
	int wZ_2 = 2*wZ - (w+g);
	vhdl << declare ("Z_2",wZ_2) << " <= Z_2_ext"
	     << range (wZ-1,wZ-wZ_2) << ";" << endl;*/
	// so we use a truncated multiplier instead
	IntTruncMultiplier *sqr_z;
	int wZ_2 = 2*wZ - (w+g);
#if 0
	sqr_z = new IntTruncMultiplier (target, wZ, wZ, wZ_2,
	                                1.f, 0, 0, false, false, true, 
	                                sqr_z_inputDelays);
	oplist.push_back (sqr_z);
	outPortMap (sqr_z, "R", "Z_2");
	inPortMap (sqr_z, "Y", "Z");
	inPortMap (sqr_z, "X", "Z");
	vhdl << instance (sqr_z, "sqr_z");
#else
	vhdl << tab << "-- First truncate the inputs of the multiplier to the precision of the output" << endl;
	vhdl << tab << declare("Z_truncToZ2", wZ_2) << " <= Z" << range(wZ-1, wZ-wZ_2) << ";" << endl;
	sqr_z = new IntTruncMultiplier (target, wZ_2, wZ_2, wZ_2,
	                                1.f, 0, 0, false, false, true, 
	                                sqr_z_inputDelays);
	oplist.push_back (sqr_z);
	outPortMap (sqr_z, "R", "Z_2");
	inPortMap (sqr_z, "Y", "Z_truncToZ2");
	inPortMap (sqr_z, "X", "Z_truncToZ2");
	vhdl << instance (sqr_z, "sqr_z");
#endif
	// remember the cycle and the critical path here, because we'll get back in time here
	map<string, double> z_3_inputDelays;
#if SUBCYCLEPIPELINING
	setSignalDelay("Z_2", sqr_z->getOutputDelay("R")) ; // TODO should be done automatically by outPortMap 
	syncCycleFromSignal("Z_2", sqr_z->getOutputDelay("R")); // TODO Then this would not need to pass a CP
	z_3_inputDelays["X"] = sqr_z->getOutputDelay("R");
	z_3_inputDelays["Y"] = sqr_z->getOutputDelay("R");
#else
	syncCycleFromSignal("Z_2"); // TODO Then this would not need to pass a CP
	nextCycle();
#endif

	// vhdl:mul (Z, Z_2 -> Z_3)

#if O
	IntTruncMultiplier *z_3;
	int wZ_3 = 3*wZ - 2*(w+g);
	z_3 = new IntTruncMultiplier (target, wZ, wZ_2, wZ_3-1,  // -1 for the div by 2
	                              1.f, 0, 0, false, false, true, 
	                              z_3_inputDelays); //last params wtf?
	oplist.push_back (z_3);
	outPortMap (z_3, "R", "Z_3_2");
	inPortMap (z_3, "Y", "Z_2");
	inPortMap (z_3, "X", "Z");
	vhdl << instance (z_3, "z_3_compute");
	syncCycleFromSignal("Z_3", z_3->getOutputDelay("R"));
#else
	int wZ_3 = 3*wZ - 2*(w+g) -1; // -1 for the div by 2
	vhdl << tab << "-- First truncate the inputs of the multiplier to the precision of the output" << endl;
	vhdl << tab << declare("Z_2_truncToZ3", wZ_3) << " <= Z_2" << range(wZ_2-1, wZ_2-wZ_3) << ";" << endl;
	vhdl << tab << declare("Z_truncToZ3", wZ_3) << " <= Z" << range(wZ-1, wZ-wZ_3) << ";" << endl;
	IntTruncMultiplier *z_3;
	z_3 = new IntTruncMultiplier (target, wZ_3, wZ_3, wZ_3,  
	                              1.f, 0, 0, false, false, true, 
	                              z_3_inputDelays); //last params wtf?
	oplist.push_back (z_3);
	outPortMap (z_3, "R", "Z_3_2");
	inPortMap (z_3, "Y", "Z_2_truncToZ3");
	inPortMap (z_3, "X", "Z_truncToZ3");
	vhdl << instance (z_3, "z_3_compute");
	syncCycleFromSignal("Z_3_2", z_3->getOutputDelay("R"));
#endif


	map<string, double> cdiv_3_inputDelays;
#if SUBCYCLEPIPELINING
	cdiv_3_inputDelays["X"] = z_3->getOutputDelay("R");
#else
	nextCycle();
#endif
	
	IntConstDiv *cdiv_3;
	cdiv_3 = new IntConstDiv (target, wZ_3, 3, -1, cdiv_3_inputDelays);
	oplist.push_back (cdiv_3);
	outPortMap (cdiv_3, "Q", "Z_3_6");
	inPortMap (cdiv_3, "X", "Z_3_2");
	vhdl << instance (cdiv_3, "cdiv_3");
	syncCycleFromSignal("Z_3_6", cdiv_3->getOutputDelay("Q"));

#if SUBCYCLEPIPELINING
#else
	nextCycle();
#endif

	// vhdl:sub (Z, Z_3_6 -> Z_sin)
	manageCriticalPath(target->adderDelay(wZ));
	vhdl << tab << declare ("Z_sin", wZ)
	     << " <= Z - Z_3_6;" << endl;
	setSignalDelay("Z_sin", getCriticalPath());



	// and now, evaluate Sin Y_in and Cos Y_in
	// Cos Y_in:
	// vhdl:slr (Z_2 -> Z_cos_red)
	// First get back to the cycle of Z2
#if SUBCYCLEPIPELINING
	setCycleFromSignal("Z_2", getSignalDelay("Z_2"));
#else
	setCycleFromSignal("Z_2");
	nextCycle();
#endif

	int wZ_cos_red = wZ_2 - 1;
	vhdl << tab << declare ("Z_cos_red", wZ_2-1) << " <= Z_2" << range (wZ_2-1,1) << ";" << endl;
	// // vhdl:id (A_cos_pi_tbl -> C_out_1)
	// vhdl:mul (Z_cos_red, A_cos_pi_tbl -> Cos_y_red_Cos_a)
#if 0
	IntTruncMultiplier *c_out_2;
	c_out_2 = new IntTruncMultiplier (target, wZ_cos_red, w+g, wZ_cos_red,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (c_out_2);
	outPortMap (c_out_2, "R", "Cos_y_red_Cos_a");
	inPortMap (c_out_2, "Y", "A_cos_pi_tbl");
	inPortMap (c_out_2, "X", "Z_cos_red");
	vhdl << instance (c_out_2, "c_out_2_compute");
#else
	vhdl << tab << "-- First truncate the larger input of the multiplier to the precision of the output" << endl;
	vhdl << tab << declare("A_cos_pi_tbl_truncToZ_cos_red", wZ_cos_red) << " <= A_cos_pi_tbl" << range(w+g-1, w+g-wZ_cos_red) << ";" << endl;
	IntTruncMultiplier *c_out_2;
	c_out_2 = new IntTruncMultiplier (target, wZ_cos_red, wZ_cos_red, wZ_cos_red,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (c_out_2);
	outPortMap (c_out_2, "R", "Cos_y_red_Cos_a");
	inPortMap (c_out_2, "Y", "A_cos_pi_tbl_truncToZ_cos_red");
	inPortMap (c_out_2, "X", "Z_cos_red");
	vhdl << instance (c_out_2, "c_out_2_compute");
#endif


#if SUBCYCLEPIPELINING
	syncCycleFromSignal("Cos_y_red_Cos_a", c_out_2->getOutputDelay("R"));
#else
	syncCycleFromSignal("Cos_y_red_Cos_a");
	nextCycle();
#endif

	// get back to the cycle of Z_sin, certainly later than the sinA
#if SUBCYCLEPIPELINING
	setCycleFromSignal("Z_sin", getSignalDelay("Z_sin")); 
#else
	setCycleFromSignal("Z_sin"); 
	nextCycle();
#endif

	// vhdl:mul (Z_sin, A_sin_pi_tbl -> Sin_y_Sin_a)
	IntTruncMultiplier *c_out_3;

#if 0
	c_out_3 = new IntTruncMultiplier (target, wZ, w+g, wZ,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (c_out_3);
	inPortMap (c_out_3, "Y", "A_sin_pi_tbl");
	inPortMap (c_out_3, "X", "Z_sin");
	outPortMap (c_out_3, "R", "Sin_y_Sin_a");
	vhdl << instance (c_out_3, "c_out_3_compute");

#else
	vhdl << tab << "-- First truncate the larger input of the multiplier to the precision of the output" << endl;
	vhdl << tab << declare("A_sin_pi_tbl_truncToZ", wZ) << " <= A_sin_pi_tbl" << range(w+g-1, w+g-wZ) << ";" << endl;
	c_out_3 = new IntTruncMultiplier (target, wZ, wZ, wZ,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (c_out_3);
	inPortMap (c_out_3, "Y", "A_sin_pi_tbl_truncToZ");
	inPortMap (c_out_3, "X", "Z_sin");
	outPortMap (c_out_3, "R", "Sin_y_Sin_a");
	vhdl << instance (c_out_3, "c_out_3_compute");
#endif

	// Synchronize the output of the two multipliers
#if SUBCYCLEPIPELINING
	syncCycleFromSignal("Cos_y_red_Cos_a", getSignalDelay("Cos_y_Cos_a"));
	syncCycleFromSignal("Sin_y_Sin_a", c_out_3->getOutputDelay("R"));
#else
	syncCycleFromSignal("Cos_y_red_Cos_a");
	syncCycleFromSignal("Sin_y_Sin_a");
	nextCycle();
#endif


	manageCriticalPath(target->localWireDelay() + target->adderDelay(wZ));
	// vhdl:add (Cos_y_red_Cos_a, Sin_y_Sin_a -> Cos_y_red_Cos_a_plus_Sin_y_Sin_a)
	vhdl << tab << declare ("Cos_y_red_Cos_a_plus_Sin_y_Sin_a", wZ)
	     << " <= Cos_y_red_Cos_a + Sin_y_Sin_a;" << endl;
	// vhdl:sub (A_cos_pi_tbl, Cos_y_red_Cos_a_plus_Sin_y_Sin_a -> C_out)
	// C_out has the entire precision; _g because it still has guards

	// TODO: This is suboptimal, 
	// the critical path does not two carry propagations if there is no register.
	manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
	vhdl << tab << declare ("C_out_g", w+g)
	     << " <= A_cos_pi_tbl - Cos_y_red_Cos_a_plus_Sin_y_Sin_a;" << endl;

	// now remove the guard bits
	// by rounding please

	// TODO for Guillaume:
	// 1/ The adder is too large, no need to add zeroes at the g LSBs
	// 2/ Try to fuse this addition and the previous one by adding the round bit to the sinA / cosA tables
	// And for F2D: again suboptimal evaluation of the critical path of a sequence of additions
	manageCriticalPath(target->localWireDelay() + target->adderDelay(w+g));
	vhdl << tab << declare ("C_out_rnd_aux", w+g)
	     << " <= C_out_g + " << '"' << std::string (w, '0')
	     << '1' << std::string (g-1, '0') << '"' << ';' << endl;


	// Sin Y_in:

	// First get back to the cycle of Z2 (same as cos_y_red):
	// it is certainly later than A_sin_pi_tbl

#if SUBCYCLEPIPELINING
	// TODO
#else
	setCycleFromSignal("Z_cos_red");
#endif

	// // vhdl:id (A_sin_pi_tbl -> S_out_1)
	// vhdl:mul (Z_cos_red, A_sin_pi_tbl -> Cos_y_red_Sin_a)
#if 0
	IntTruncMultiplier *s_out_2;
	s_out_2 = new IntTruncMultiplier (target, wZ_cos_red, w+g, wZ_cos_red,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (s_out_2);
	inPortMap (s_out_2, "X", "Z_cos_red");
	inPortMap (s_out_2, "Y", "A_sin_pi_tbl");
	outPortMap (s_out_2, "R", "Cos_y_red_Sin_a");
	vhdl << instance (s_out_2, "s_out_2_compute");
	syncCycleFromSignal("Cos_y_red_Sin_a");
#else
	vhdl << tab << "-- First truncate the larger input of the multiplier to the precision of the output" << endl;
	vhdl << tab << declare("A_sin_pi_tbl_truncToZ_cos_red", wZ_cos_red) << " <= A_sin_pi_tbl" << range(w+g-1, w+g-wZ_cos_red) << ";" << endl;
	IntTruncMultiplier *s_out_2;
	s_out_2 = new IntTruncMultiplier (target, wZ_cos_red, wZ_cos_red, wZ_cos_red,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (s_out_2);
	inPortMap (s_out_2, "X", "Z_cos_red");
	inPortMap (s_out_2, "Y", "A_sin_pi_tbl_truncToZ_cos_red");
	outPortMap (s_out_2, "R", "Cos_y_red_Sin_a");
	vhdl << instance (s_out_2, "s_out_2_compute");
	syncCycleFromSignal("Cos_y_red_Sin_a");
#endif


	// get back to the cycle of Z_sin, certainly later than the cosA
	setCycleFromSignal("Z_sin", getSignalDelay("Z_sin"));
	// vhdl:mul (Z_sin, A_cos_pi_tbl -> Sin_y_Cos_a)
	IntTruncMultiplier *s_out_3;
	s_out_3 = new IntTruncMultiplier (target, wZ, w+g, wZ,
	                                  1.f, 0, 0, false, false, true);
	oplist.push_back (s_out_3);
	inPortMap (s_out_3, "X", "Z_sin");
	inPortMap (s_out_3, "Y", "A_cos_pi_tbl");
	outPortMap (s_out_3, "R", "Sin_y_Cos_a");
	vhdl << instance (s_out_3, "s_out_3_compute");
	syncCycleFromSignal("Sin_y_Cos_a");

	nextCycle();
	manageCriticalPath(target->adderDelay(wZ));
	// vhdl:add (A_sin_pi_tbl, Sin_y_Cos_a -> Sin_y_Cos_a_plus_Sin_a)
	vhdl << tab << declare ("Sin_y_Cos_a_plus_Sin_a", w+g) //w+g necessary because of sin(pi*a)
	     << " <= A_sin_pi_tbl + Sin_y_Cos_a;" << endl;
	// vhdl:sub (Sin_y_Cos_a_plus_Sin_a, Cos_y_red_Sin_a -> S_out)
	// TODO: This is suboptimal, 
	// the critical path does not two carry propagations if there is no register.
	manageCriticalPath(target->adderDelay(w+g));
	vhdl << tab << declare ("S_out_g", w+g)
	     << " <= Sin_y_Cos_a_plus_Sin_a - Cos_y_red_Sin_a;" << endl;


	// now remove the guard bits
	// by rounding please
	manageCriticalPath(target->adderDelay(w+g));
	vhdl  << tab << declare ("S_out_rnd_aux", w+g)
	     << " <= S_out_g + " << '"' << std::string (w, '0')
	     << '1' << std::string (g-1, '0') << '"' << ';' << endl;

	//Final synchronization
	syncCycleFromSignal("C_out_g");

	vhdl << tab << declare ("C_out", w)
	     << " <= C_out_rnd_aux" << range (w+g-1, g) << ';' << endl;
	vhdl << tab << declare ("S_out", w)
	     << " <= S_out_rnd_aux" << range (w+g-1, g) << ';' << endl;

	// now just add the signs to the signals
	/* the output format is consequently
	 * struct output_format {
	 *	unsigned int sgn: 1; // sign is 1 for neg, as in IEEE754
	 *	unsigned int mantissa: w;
	 * };
	 */
	vhdl << tab << declare ("S_wo_sgn", w)
	     << " <= C_out when Exch = '1' else S_out;" << endl;
	vhdl << tab << declare ("C_wo_sgn", w)
	     << " <= S_out when Exch = '1' else C_out;" << endl;
	vhdl << tab << "S <= '0' & S_wo_sgn;" << endl;
	vhdl << tab << "C <= A & C_wo_sgn;" << endl;

};





void FixSinCos::emulate(TestCase * tc)
{
	mpz_class sx = tc->getInputValue ("X");
	mpfr_t x, sind, cosd, sinu, cosu, pixd, pixu, one_minus_ulp;
	mpz_t sind_z, cosd_z, sinu_z, cosu_z;
	mpfr_init2 (x, 1+w);
	mpz_init2 (sind_z, 1+w);
	mpz_init2 (cosd_z, 1+w);
	mpz_init2 (sinu_z, 1+w);
	mpz_init2 (cosu_z, 1+w);
	mpfr_inits (sind, cosd, sinu, cosu, pixd, pixu, (mpfr_ptr) 0);
	mpfr_init2 (one_minus_ulp, 1+w);
	//these 3 following roundings are exact
	mpfr_set_ui (one_minus_ulp, 1UL, GMP_RNDD);
	mpfr_div_2ui (one_minus_ulp, one_minus_ulp, w, GMP_RNDD);
	mpfr_ui_sub (one_minus_ulp, 1UL, one_minus_ulp, GMP_RNDD);

	mpfr_set_z (x, sx.get_mpz_t(), GMP_RNDD); // this rounding is exact
	mpfr_div_2si (x, x, w, GMP_RNDD); // this rounding is acually exact
	int i=0, ep; // ep: extra precision
	do {
		ep = 1 << i;
		mpfr_set_prec (sind, 1+w+ep);
		mpfr_set_prec (cosd, 1+w+ep);
		mpfr_set_prec (sinu, 1+w+ep);
		mpfr_set_prec (cosu, 1+w+ep);
		mpfr_set_prec (pixd, 2+w+ep);
		mpfr_set_prec (pixu, 2+w+ep);
		mpfr_const_pi (pixd, GMP_RNDD);
		mpfr_const_pi (pixu, GMP_RNDU);
		mpfr_mul (pixd, pixd, x, GMP_RNDD);
		mpfr_mul (pixu, pixu, x, GMP_RNDU);
		if (mpfr_cmp_ui_2exp (x, 1UL, -1) < 0) { // if (x < 0.5f)
			// then sin is increasing near x
			mpfr_sin (sind, pixd, GMP_RNDD);
			mpfr_sin (sinu, pixu, GMP_RNDU);
		} else {
			// then sin is decreasing near x
			mpfr_sin (sind, pixu, GMP_RNDD); // use the upper x for the lower sin
			mpfr_sin (sinu, pixd, GMP_RNDU);
		}
		mpfr_cos (cosd, pixu, GMP_RNDD); // cos decreases from 0 to pi
		mpfr_cos (cosu, pixd, GMP_RNDU);
		// multiply by the (1-ulp) factor now
		mpfr_mul (sind, sind, one_minus_ulp, GMP_RNDD);
		mpfr_mul (cosd, cosd, one_minus_ulp, GMP_RNDD);
		mpfr_mul (sinu, sinu, one_minus_ulp, GMP_RNDU);
		mpfr_mul (cosu, cosu, one_minus_ulp, GMP_RNDU);
		// if the cosine is <0 we must neg it then add 1
		if (mpfr_cmp_ui (cosd, 0) < 0) {
			mpfr_setsign (cosd, cosd, 0, GMP_RNDD); // exact rnd
			mpfr_add_ui (cosd, cosd, 1, GMP_RNDD); // exact rnd
		}
		// same as before
		if (mpfr_cmp_ui (cosu, 0) < 0) {
			mpfr_setsign (cosu, cosu, 0, GMP_RNDU);
			mpfr_add_ui (cosu, cosu, 1, GMP_RNDU);
		}
		mpfr_mul_2si (sind, sind, w, GMP_RNDD); // exact rnd here
		mpfr_mul_2si (cosd, cosd, w, GMP_RNDD);
		mpfr_mul_2si (sinu, sinu, w, GMP_RNDU);
		mpfr_mul_2si (cosu, cosu, w, GMP_RNDU);
		mpfr_get_z (sind_z, sind, GMP_RNDD); // there can be a real rounding here
		mpfr_get_z (cosd_z, cosd, GMP_RNDD);
		mpfr_get_z (sinu_z, sinu, GMP_RNDU); // there can be a real rounding here
		mpfr_get_z (cosu_z, cosu, GMP_RNDU);
		// now we test if the upper results are the lower ones + 1
		// as we want them to
		mpz_sub_ui (sinu_z, sinu_z, 1UL);
		mpz_sub_ui (cosu_z, cosu_z, 1UL);
		if (mpz_cmp (sind_z, sinu_z) == 0 &&
		    mpz_cmp (cosd_z, cosu_z) == 0) {
			// the rounding are really what we want
			break;
		} else {
			i++;
		}
	} while (i<16); // for sanity
	mpz_class sind_zc (sind_z), cosd_zc (cosd_z);
	// and since FunctionEvaluator does only faithful rounding
	// we add also as expected results the upper roundings
	tc->addExpectedOutput ("S", sind_zc);
	tc->addExpectedOutput ("C", cosd_zc);
	mpz_class ones (1); ones <<= w; ones -= 1;
	if ((sind_zc & ones) != ones)
		tc->addExpectedOutput ("S", sind_zc+1);
	if ((cosd_zc & ones) != ones)
		tc->addExpectedOutput ("C", cosd_zc+1);
	mpfr_clears (x, sind, cosd, sinu, cosu,
	             pixd, pixu, one_minus_ulp, NULL);
	mpz_clears (sind_z, cosd_z, sinu_z, cosu_z, NULL);
}


void FixSinCos::buildStandardTestCases(TestCaseList * tcl)
{
	TestCase* tc;
	tc = new TestCase (this);
	tc -> addInput ("X",mpz_class(0));
	emulate(tc);
	tcl->add(tc);
	tc = new TestCase (this);
	tc -> addInput ("X",mpz_class(1));
	emulate(tc);
	tcl->add(tc);
	mpz_class ones(1);
	ones <<= w;
	ones -= 1;
	tc = new TestCase (this);
	tc -> addInput ("X",ones);
	emulate(tc);
	tcl->add(tc);
}

void FixSinCos::buildRandomTestCases(TestCaseList * tcl, int n)
{
}

TestCase *FixSinCos::buildRandomTestCases(int i)
{
	TestCase *tc = new TestCase(this);
	return tc;
}

#endif // SOLLYA
