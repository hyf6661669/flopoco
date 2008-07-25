#include "HOTBM.hpp"
#include "HOTBM/Function.hh"
#include "HOTBM/Param.hh"
#include "HOTBM/HOTBMInstance.hh"
#include "HOTBM/Exhaustive.hh"
#include "utils.hpp"

HOTBM::HOTBM(Target* target, string func, int wI, int wO, int n)
	: inst(0), f(*new Function(func)), wI(wI), wO(wO)
{
	try {
		Param p(wI, wO, n);
		Exhaustive ex(f, p);

		inst = 0;
		inst = ex.getInstance();
		inst->roundTables();
	} catch (const char *s) {
		throw std::string(s);
	}
	if (!inst)
		throw std::string("HOTBM cound not be generated.");

	// TODO: Better unique name
	{
		std::ostringstream o;
		o << "hotbm_" << wI << "_" << wO << "_" << n;
		uniqueName_ = o.str();
	}
	setCombinatorial();
	setPipelineDepth(0);
	addInput("x", wI);
	addOutput("r", wO+1);
}

HOTBM::~HOTBM()
{
	if (inst)
		delete inst;
}

// Overloading the virtual functions of Operator
void HOTBM::outputVHDL(std::ostream& o, std::string name)
{
	if (!inst)
		return;

	inst->genVHDL(o, name);
}

TestIOMap HOTBM::getTestIOMap()
{
	TestIOMap tim;
	tim.add(*getSignalByName("x"));
	tim.add(*getSignalByName("r"), 2); // Faithful rounding
	return tim;
}

void HOTBM::fillTestCase(mpz_class a[])
{
	/* Get inputs / outputs */
	mpz_class &x  = a[0];
	mpz_class &rd = a[1]; // rounded down
	mpz_class &ru = a[2]; // rounded up

	int outSign = 0;

	mpfr_t mpX, mpR;
	mpfr_inits(mpX, mpR, 0);

	/* Convert a random signal to an mpfr_t in [0,1[ */
	mpfr_set_z(mpX, x.get_mpz_t(), GMP_RNDN);
	mpfr_div_2si(mpX, mpX, wI, GMP_RNDN);

	/* Compute the function */
	f.eval(mpR, mpX);

	/* Compute the signal value */
	if (mpfr_signbit(mpR))
	{
		outSign = 1;
		mpfr_abs(mpR, mpR, GMP_RNDN);
	}
	mpfr_mul_2si(mpR, mpR, wO, GMP_RNDN);

	/* NOT A TYPO. HOTBM only guarantees faithful
	 * rounding, so we will round down here,
	 * add both the upper and lower neighbor.
	 */
	mpfr_get_z(rd.get_mpz_t(), mpR, GMP_RNDD);
	ru = rd + 1;
}

