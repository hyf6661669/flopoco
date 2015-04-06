#ifndef __lut_sr_urng_HPP
#define __lut_sr_urng_HPP
#include <vector>
#include <sstream>
#include <set>

#include "Operator.hpp"
#include "../transforms/RngTransformOperator.hpp"

/* This file contains a lot of useful functions to manipulate vhdl */
#include "utils.hpp"

#include "../transforms/RngTransformOperator.hpp"

/*  All flopoco operators and utility functions are declared within
  the flopoco namespace.
*/
namespace flopoco{
namespace random{


// new operator class declaration
class LutSrRng : public Operator {
  public:
    /* operatorInfo is a user defined parameter (not a part of Operator class) for
      stocking information about the operator. The user is able to defined any number of parameter in this class, as soon as it does not affect Operator parameters undeliberatly*/
    static string operatorInfo;
    std::vector<int> perm;		//output permutation
    int seedtap;
  
    int want_r; // Number of actual outputs

    int n,r;
    int t;		// XOR input count
    int k;
    uint32_t s;

	std::vector<set<int> > taps;	//XOR connections
	std::vector<int> cycle;		//initial seed cycle

  std::vector<int> cs, ns;

  public:

    // constructor, defined there with two parameters (default value 0 for each)
    LutSrRng(Target* target,int want_r, int want_t, int want_k, int want_n=0);

    // destructor
    ~LutSrRng();


    // Below all the functions needed to test the operator
    /* the emulate function is used to simulate in software the operator
      in order to compare this result with those outputed by the vhdl opertator */
    void emulate(TestCase * tc);

    /* function used to create Standard testCase defined by the developper */
  //  void buildStandardTestCases(TestCaseList* tcl);


	void buildStandardTestCases(TestCaseList* tcl);


//    void buildRandomTestCases(TestCaseList* tcl, int n);
//    TestCase* buildRandomTestCase(int i);

  /** Take the given transform, and supply it with uniform bits. The inputs of the
    * resulting operator will be the LUT-SR inputs, while the outputs will be those
    * of the transform operator
   */
  static Operator *DriveTransform(std::string name, RngTransformOperator *base);
};

}; // random
}; // flopoco
#endif