/* Each Operator declared within the flopoco framework has 
to inherit the class Operator and overload some functions listed below*/
#ifndef random_table_lookup_transform_hpp
#define random_table_lookup_transform_hpp
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include "gmp.h"
#include "mpfr.h"
#include <math.h>
#include <assert.h>
#include <iomanip>
#include <time.h>

#include "../RngTransformOperator.hpp"

/* This file contains a lot of useful functions to manipulate vhdl */
#include "utils.hpp"



/*  All flopoco operators and utility functions are declared within
the flopoco namespace.
*/
namespace flopoco{
namespace random{

/* Given an operator, it will create an array of them and apply a hadamard transform */
class HadamardTransform : public RngTransformOperator {
private:
	unsigned m_log2n, m_n, m_baseWidth;
	RngTransformOperator *m_base;

	unsigned m_uniformInputBits;
	std::string m_uniformInputName;

	unsigned m_nonUniformOutputCount;
	unsigned m_nonUniformOutputWidth;
	std::string m_nonUniformOutputNameBase; 

	void Connect(std::string dstName, int dstIdx, std::string srcName, int srcL, int srcR, int dir, int srcW);
	void HadamardStage(std::string dstName, std::string srcName, int totalSize, int size, int srcW);
	void Hadamard(std::string name, int log2size, int srcW);

public:
	HadamardTransform(Target* target, int log2n, RngTransformOperator *base);

	~HadamardTransform();

	void emulate(TestCase * tc);

	void buildRandomTestCases(TestCaseList* tcl, int n);

	TestCase* buildRandomTestCase(int i);

	virtual unsigned uniformInputBits() const
	{ return m_uniformInputBits; }
	
	virtual std::string uniformInputName() const
	{ return m_uniformInputName; }

	virtual unsigned nonUniformOutputCount() const
	{ return m_nonUniformOutputCount; }
	
	virtual bool nonUniformOutputsAreHomogenous() const
	{ return m_base->nonUniformOutputsAreHomogenous(); }
	
	virtual unsigned nonUniformOutputWidth(int) const
	{ return m_nonUniformOutputWidth; }
	
	virtual std::string nonUniformOutputName(int i) const
	{ return join(m_nonUniformOutputNameBase,i); }
};

}; // random
}; // flopoco
#endif
