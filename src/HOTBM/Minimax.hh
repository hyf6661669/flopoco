#ifndef _MINIMAX_HH_
#define _MINIMAX_HH_

#include "Function.hh"
#include "MPPolynomial.hh"

using namespace std;



class Minimax {
public:
  Minimax(Function &f, double ia, double ib, int d);
  ~Minimax();

  MPPolynomial &getMPP() const;
  void getMPErr(mpfr_t mpErr_) const;

private:
  MPPolynomial *mpP;
  mpfr_t mpErr;
};

#endif // _MINIMAX_HH_
