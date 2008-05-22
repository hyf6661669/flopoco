#include <iostream>
#include <fstream>
#include <sstream>

#include "FPExp.hpp"
#include "FloFP.hpp"
#include "utils.hpp"

#include "fpexp/stdfragment.h"
#include "fpexp/explore.h"

using namespace std;

FPExp::FPExp(Target* target, int wE, int wF)
	: wE(wE), wF(wF)
{
	/* Generate unique name */
	{
		std::ostringstream o;
		o << "FPExp_" << wE << "_" << wF;
		unique_name = o.str();
	}

	add_FP_input("x", wE, wF);
	add_FP_output("r", wE, wF);

	int explore_size = wF;
	int exponent_size = wE;

	f = explore(explore_size);
	if (!f) throw std::string("FPExp::FPExp(): No fragment");

	result_length = f->prepare(area, max_error);

	g = intlog2(max_error) + 2;
	cout
		<< "    Estimated area (for fixed-point part): " << area << endl
		<< "    Maximum error: " << max_error << endl
		<< "    Internal precision: " << result_length << endl
		<< "    Precision: " << result_length - g << endl;
}	

FPExp::~FPExp()
{
}

// Overloading the virtual functions of Operator
void FPExp::output_vhdl(std::ostream& o, std::string name)
{
	Licence(o, "Cristian KLEIN (2008)");
	stringstream fp_exp, fixp_exp, fixp_exp_tbl;

	f->generate(unique_name, fixp_exp, fixp_exp_tbl);

	std::string cstInvLog2, cstLog2;
	{
		mpz_class z;

		mpfr_t mp2, mp1, mp;
		mpfr_init2(mp, 2*(wE+wF+g));	// XXX: way too much precision
		mpfr_inits(mp1, mp2, 0);
		mpfr_set_si(mp1, 1, GMP_RNDN);
		mpfr_set_si(mp2, 2, GMP_RNDN);

		mpfr_log(mp, mp2, GMP_RNDN);
		mpfr_mul_2si(mp, mp, wE-1+wF+g, GMP_RNDN);
		mpfr_get_z(z.get_mpz_t(), mp, GMP_RNDN);
		{
			std::ostringstream o;
			o.fill('0');
			o.width(wE-1+wF+g);
			o << z.get_str(2);
			cstLog2 = o.str();
		}

		mpfr_mul_2si(mp, mp, -(wE-1+wF+g), GMP_RNDN);
		mpfr_div(mp, mp1, mp, GMP_RNDN);
		mpfr_mul_2si(mp, mp, wE+1, GMP_RNDN);
		mpfr_get_z(z.get_mpz_t(), mp, GMP_RNDN);
		{
			std::ostringstream o;
			o.fill('0');
			o.width(wE+1);
			o << z.get_str(2);
			cstInvLog2 = o.str();
		}

		mpfr_clears(mp1, mp2, mp, 0);
	}

	fp_exp <<
		"library ieee;\n"
		"use ieee.std_logic_1164.all;\n"
		"use ieee.std_logic_arith.all;\n"
		"use ieee.std_logic_unsigned.all;\n"
		"\n"
		"package pkg_" << unique_name << " is\n"
		"  function min ( x, y : integer ) return integer;\n"
		"  function max ( x, y : integer ) return integer;\n"
		"  function fp_exp_shift_wx ( wE, wF, g : positive ) return positive;\n"
		"  function log2 ( x : positive ) return integer;\n"
		"  \n"
		"  component " << unique_name << "_shift is\n"
		"    generic ( wE : positive;\n"
		"              wF : positive;\n"
		"              g : positive );\n"
		"    port ( fpX : in  std_logic_vector(wE+wF downto 0);\n"
		"           nX  : out std_logic_vector(wE+wF+g-1 downto 0);\n"
		"           ofl : out std_logic;\n"
		"           ufl : out std_logic );\n"
		"  end component;\n"
		"  \n";

	fp_exp <<
		"  component " << unique_name << " is" << endl <<
		"    generic ( wE : positive := " << wE << ";" << endl <<
		"              wF : positive := " << result_length - g << ";" << endl <<
		"              g : positive := " << g << " );" << endl;

	fp_exp <<
		"    port ( x : in  std_logic_vector(2+wE+wF downto 0);\n"
		"           r : out std_logic_vector(2+wE+wF downto 0) );\n"
		"  end component;\n"
		"end package;\n"
		"\n"
		"package body pkg_" << unique_name << " is\n"
		"  function min ( x, y : integer ) return integer is\n"
		"  begin\n"
		"    if x <= y then\n"
		"      return x;\n"
		"    else\n"
		"      return y;\n"
		"    end if;\n"
		"  end function;\n"
		"\n"
		"  function max ( x, y : integer ) return integer is\n"
		"  begin\n"
		"    if x >= y then\n"
		"      return x;\n"
		"    else\n"
		"      return y;\n"
		"    end if;\n"
		"  end function;\n"
		"  \n"
		"  function fp_exp_shift_wx ( wE, wF, g : positive ) return positive is\n"
		"  begin\n"
		"    return min(wF+g, 2**(wE-1)-1);\n"
		"  end function;\n"
		"  \n"
		"  function log2 ( x : positive ) return integer is\n"
		"    variable n : natural := 0;\n"
		"  begin\n"
		"    while 2**(n+1) <= x loop\n"
		"      n := n+1;\n"
		"    end loop;\n"
		"    return n;\n"
		"  end function;\n"
		"\n"
		"end package body;\n"
		"\n"
		"-- Conversion de l'entrée en virgule fixe\n"
		"-- ======================================\n"
		"\n"
		"library ieee;\n"
		"use ieee.std_logic_1164.all;\n"
		"use ieee.std_logic_arith.all;\n"
		"use ieee.std_logic_unsigned.all;\n"
		"library work;\n"
		"use work.pkg_" << unique_name << ".all;\n"
		"\n"
		"entity " << unique_name << "_shift is\n"
		"  generic ( wE : positive;\n"
		"            wF : positive;\n"
		"            g : positive );\n"
		"  port ( fpX : in  std_logic_vector(wE+wF downto 0);\n"
		"         nX  : out std_logic_vector(wE+wF+g-1 downto 0);\n"
		"         ofl : out std_logic;\n"
		"         ufl : out std_logic );\n"
		"end entity;\n"
		"\n"
		"architecture arch of " << unique_name << "_shift is\n"
		"  -- longueur de la partie fractionnaire de x\n"
		"  constant wX : integer  := fp_exp_shift_wx(wE, wF, g);\n"
		"  -- nombre d'étapes du décalage \n"
		"  -- (de l'ordre de log2(taille du nombre en virgule fixe))\n"
		"  constant n  : positive := log2(wX+wE-2)+1;\n"
		"\n"
		"  signal e0 : std_logic_vector(wE+1 downto 0);\n"
		"  signal eX : std_logic_vector(wE+1 downto 0);\n"
		"\n"
		"  signal mXu : std_logic_vector(wF downto 0);\n"
		"  signal mXs : std_logic_vector(wF+1 downto 0);\n"
		"\n"
		"  signal buf : std_logic_vector((n+1)*(wF+2**n+1)-1 downto 0);\n"
		"begin\n"
		"  -- évalue le décalage à effectuer\n"
		"  e0 <= conv_std_logic_vector(2**(wE-1)-1 - wX, wE+2);\n"
		"  eX <= (\"00\" & fpX(wE+wF-1 downto wF)) - e0;\n"
		"\n"
		"  -- underflow quand l'entrée est arrondie à zéro (donc au final exp(entrée) = 1) (?)\n"
		"  ufl <= eX(wE+1);\n"
		"  -- overflow (détection partielle en se basant uniquement sur l'exposant de l'entrée)\n"
		"  ofl <= not eX(wE+1) when eX(wE downto 0) > conv_std_logic_vector(wX+wE-2, wE+1) else\n"
		"         '0';\n"
		"\n"
		"  -- mantisse de l'entrée (rajoute le 1 implicite)\n"
		"  mXu <= \"1\" & fpX(wF-1 downto 0);\n"
		"  -- représentation signée de la mantisse\n"
		"  mXs <= (wF+1 downto 0 => '0') - (\"0\" & mXu) when fpX(wE+wF) = '1' else (\"0\" & mXu);\n"
		"\n"
		"  -- ajoute eX zéros à droite de la mantisse\n"
		"  buf(wF+1 downto 0) <= mXs;\n"
		"  shift : for i in 0 to n-1 generate\n"
		"    buf( (i+1)*(wF+2**n+1) + wF+2**(i+1) downto\n"
		"         (i+1)*(wF+2**n+1) )\n"
		"      <=   -- pas de décalage si eX(i) = 0\n"
		"           ( 2**i-1 downto 0 => buf(i*(wF+2**n+1) + wF+2**i) ) &\n"
		"           buf( i*(wF+2**n+1) + wF+2**i downto\n"
		"                i*(wF+2**n+1) )\n"
		"        when eX(i) = '0' else\n"
		"           -- décalage de 2 ^ i bits si eX(i) = 1\n"
		"           buf( i*(wF+2**n+1) + wF+2**i downto\n"
		"                i*(wF+2**n+1) ) &\n"
		"           ( 2**i-1 downto 0 => '0' );\n"
		"  end generate;\n"
		"\n"
		"  no_padding : if wX >= g generate\n"
		"    nX <= buf(n*(wF+2**n+1)+wF+wE+wX-1 downto n*(wF+2**n+1)+wX-g);\n"
		"  end generate;\n"
		"\n"
		"  padding : if wX < g generate\n"
		"    nX <= buf(n*(wF+2**n+1)+wF+wE+wX-1 downto n*(wF+2**n+1)) & (g-wX-1 downto 0 => '0');\n"
		"  end generate;\n"
		"\n"
		"end architecture;\n"
		"\n"
		"-- Exponentielle en virgule flottante\n"
		"-- ==================================\n"
		"\n"
		"library ieee;\n"
		"use ieee.std_logic_1164.all;\n"
		"use ieee.std_logic_arith.all;\n"
		"use ieee.std_logic_unsigned.all;\n"
		"library work;\n"
		"use work.pkg_" << unique_name << "_exp.all;\n"
		"use work.pkg_" << unique_name << ".all;\n"
		"\n";

	fp_exp <<
		"entity " << unique_name << " is" << endl <<
		"    generic ( wE : positive := " << wE << ";" << endl <<
		"              wF : positive := " << result_length - g << ";" << endl <<
		"              g : positive := " << g << " );" << endl <<
		"    port ( x : in  std_logic_vector(2+wE+wF downto 0);" << endl <<
		"           r : out std_logic_vector(2+wE+wF downto 0));" << endl <<
		"end entity;" << endl << endl <<
		"architecture arch of " << unique_name << " is" << endl <<
		"  constant cstInvLog2 : std_logic_vector(wE+1 downto 0) := \"" << cstInvLog2 << "\";" << endl <<
		"  constant cstLog2 : std_logic_vector(wE-1+wF+g-1 downto 0) := \"" << cstLog2 << "\";" << endl;

	fp_exp <<
		"\n"
		"  signal nX : std_logic_vector(wE+wF+g-1 downto 0);\n"
		"  signal nK0 : std_logic_vector(wE+4+wE downto 0);\n"
		"  signal nK1 : std_logic_vector(wE+4+wE+1 downto 0);\n"
		"  signal nK  : std_logic_vector(wE downto 0);\n"
		"  \n"
		"  signal nKLog20 : std_logic_vector(wE+wE-1+wF+g-1 downto 0);\n"
		"  signal nKLog2  : std_logic_vector(wE+wE-1+wF+g downto 0);\n"
		"  signal nY  : std_logic_vector(wE+wF+g-1 downto 0);\n"
		"  signal sign : std_logic;\n"
		"  signal unsigned_input : std_logic_vector(wF+g-2 downto 0);\n"
		"  \n"
		"  signal nZ : std_logic_vector(wF+g-1 downto 0);\n"
		"  signal significand : std_logic_vector(wF+g-1 downto g);\n"
		"  signal exponent : std_logic_vector(wE downto 0);\n"
		"\n"
		"  signal sticky : std_logic;\n"
		"  signal round  : std_logic;\n"
		"\n"
		"  signal fR0 : std_logic_vector(wF+1 downto 0);\n"
		"  signal fR1 : std_logic_vector(wF downto 0);\n"
		"  signal fR  : std_logic_vector(wF-1 downto 0);\n"
		"\n"
		"  signal eR : std_logic_vector(wE downto 0);\n"
		"  \n"
		"  signal ofl0 : std_logic;\n"
		"  signal ofl1 : std_logic;\n"
		"  signal ofl2 : std_logic;\n"
		"  signal ufl0 : std_logic;\n"
		"  signal ufl1 : std_logic;\n"
		"  \n"
		"begin\n"
		"  shift : " << unique_name << "_shift\n"
		"    generic map ( wE => wE,\n"
		"                  wF => wF,\n"
		"                  g => g )\n"
		"    port map ( fpX => X(wE+wF downto 0),\n"
		"               nX  => nX,\n"
		"               ofl => ofl0,\n"
		"               ufl => ufl0 );\n"
		"\n"
		"  nK0 <= nX(wE+wF+g-2 downto wF+g-4) * cstInvLog2;\n"
		"  nK1 <= (\"0\" & nK0) - (\"0\" & cstInvLog2 & (wE+4-2 downto 0 => '0')) when nX(wE+wF+g-1) = '1' else\n"
		"         \"0\" & nK0;\n"
		"\n"
		"  nK <= nK1(wE+4+wE+1 downto 4+wE+1) + ((wE downto 1 => '0') & nK1(4+wE));\n"
		"\n"
		"  nKLog20 <= nK(wE-1 downto 0) * cstLog2;\n"
		"  nKLog2  <= (\"0\" & nKLog20) - (\"0\" & cstLog2 & (wE-1 downto 0 => '0')) when nK(wE) = '1' else\n"
		"             \"0\" & nKLog20;\n"
		"\n"
		"  nY <= nX - nKLog2(wE+wE-1+wF+g-1 downto wE-1);\n"
		"  sign <= nY(wF+g-1);\n"
		"  unsigned_input <= nY(wF+g-2 downto 0) when sign = '0' else (wF+g-2 downto 0 => '0') - nY(wF+g-2 downto 0);\n"
		"  \n";

	fp_exp << "  label2 : " << unique_name << "_exp_" << result_length - 1 << endl;

	fp_exp <<
		"    port map (x    => unsigned_input,\n"
		"              y    => nZ,\n"
		"              sign => sign);\n"
		"\n"
		"  significand <= nZ(wF+g-1 downto g)   + ((wF-g-1 downto g+1 => '0') & nZ(g-1)) when sign = '0' else\n"
		"                 nZ(wF+g-2 downto g-1) + ((wF-g-2 downto g   => '0') & nZ(g-2));\n"
		"  exponent <= nK + (\"00\" & (wE-2 downto 1 => '1') & (not sign));\n"
		"\n"
		"  ofl1 <= '1' when exponent(wE-1 downto 0) = (wE-1 downto 0 => '0') else\n"
		"          '1' when exponent(wE-1 downto 0) = (wE-1 downto 0 => '1') else\n"
		"          ofl0 or exponent(wE);\n"
		"\n"
		"  ufl1 <= '1' when X(wE+wF+2 downto wE+wF+1) = \"00\" else\n"
		"          ufl0;\n"
		"\n"
		"  ofl2 <= '1' when X(wE+wF+2 downto wE+wF+1) = \"10\" else\n"
		"          ofl1 and (not ufl1);\n"
		"  \n"
		"  R(wE+wF+2 downto wE+wF+1) <= \"11\"                   when X(wE+wF+2 downto wE+wF+1) = \"11\" else\n"
		"                                 (not X(wE+wF)) & \"0\" when ofl2 = '1'                         else\n"
		"                                 \"01\";\n"
		"\n"
		"  R(wE+wF downto 0) <= \"00\" & (wE-2 downto 0 => '1') & (wF-1 downto 0 => '0') when ufl1 = '1' else\n"
		"                         \"0\" & exponent(wE-1 downto 0) & significand;\n"
		"end architecture;\n";

	o << fixp_exp_tbl.str() << fixp_exp.str() << fp_exp.str();
}

TestCaseList FPExp::generateStandardTestCases(int n)
{
	// TODO
	return TestCaseList();
}

TestCaseList FPExp::generateRandomTestCases(int n)
{
	Signal& sx = *get_signal_by_name("x");
	Signal& sr = *get_signal_by_name("r");
	Signal  sr_exc = (*get_signal_by_name("r")).getException();
	Signal  sr_sgn = (*get_signal_by_name("r")).getSign();
	Signal  sr_exp = (*get_signal_by_name("r")).getExponent();
	Signal  sr_man = (*get_signal_by_name("r")).getMantissa();

	TestCaseList tcl;	/* XXX: Just like Lyon's Transportion Company. :D */
	FloFP x(wE, wF), r(wE, wF);

	for (int i = 0; i < n; i++)
	{
		x = getLargeRandom(sx.width()-2) + (mpz_class(1) << (wE + wF + 1));
		r = x.exp();

		TestCase tc;
		tc.addInput(sx, x.getSignalValue());
		tc.addExpectedOutput(sr_exc, r.getExceptionSignalValue());
		tc.addExpectedOutput(sr_sgn, r.getSignSignalValue());
		if (r.getExceptionSignalValue() == 1)
		{
			/* Exp only returns faithful rounding */
			tc.addExpectedOutput(sr, r.getRoundedDownSignalValue());
			tc.addExpectedOutput(sr, r.getRoundedUpSignalValue());
		}
		tcl.add(tc);
	}


	return tcl;
}

