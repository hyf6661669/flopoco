/*
An operator factory to enable parsing and doc to be delegated to the operators

Author : David Thomas, Florent de Dinechin

Initial software.
Copyright © INSA-Lyon, ENS-Lyon, INRIA, CNRS, UCBL,  
2015-.
  All rights reserved.

*/
#ifndef flopoco_operator_factory_hpp
#define flopoco_operator_factory_hpp

#include "Operator.hpp"
#include <boost/smart_ptr.hpp>

// Operator Factory, based on the one by David Thomas, with a bit of clean up.
// For typical use, see src/ShiftersEtc/Shifter.*

namespace flopoco
{
	
		// Note: not using boost::function here, as it's likely to scare people, and also drags in quite a few header dependencies
	typedef void (*usage_func_t)(std::ostream &);
	typedef Operator * (*parser_func_t)(Target *,const std::vector<std::string> &,int &);	
	class OperatorFactory;
	typedef boost::shared_ptr<OperatorFactory> OperatorFactoryPtr;



	
	/** This is the class that manages a list of OperatorFactories, and the overall command line and documentation. Each OperatorFactory is responsible for the command line and parsing for one Operator sub-class. */
	class OperatorFactoryHolding
	{
	public:

		static void registerFactory(OperatorFactoryPtr factory);
		static void add(
										std::string name,			
										std::string categories,	// semi-colon seperated list of categories
										usage_func_t usage,
										parser_func_t parser
										);

		static unsigned getFactoryCount();
		static OperatorFactoryPtr getFactoryByIndex(unsigned i);
		static OperatorFactoryPtr findFactory(std::string operatorName);


		
	private:
		static std::vector<OperatorFactoryPtr> sm_factoriesByIndex;
		static std::map<std::string,OperatorFactoryPtr> sm_factoriesByName;
	};






	

	
	/** This is the abstract class that each operator factory will inherit. 
			Each OperatorFactory is responsible for the command line and parsing for one Operator sub-class.  */
	class OperatorFactory
	{

	private:
		std::string m_name;
		std::vector<std::string> m_categories;
		usage_func_t m_usage;
		parser_func_t m_parser;
	public:
		
		/** Implements a no-frills factory, given a usage function and a parser function
				\param name Name for the operator. The factory will only respond for this name (case sensitive)
				\param categories A semi-colon seperated list of categories that the operator belongs to. Used to sort the doc.
				\param usage A function that can print the usage for the function to a std::ostream
				\param parser A function that can parse a vector of string arguments into an Operator instance
				\param testParameters Zero or more sets of arguments that the operator can be tested with.
		**/
		OperatorFactory(
										std::string name,			
										std::string categories,
										usage_func_t usage,
										parser_func_t parser	);
		
		virtual const std::string &name() const
		{ return m_name; }
		
		virtual const std::vector<std::string> &categories() const
		{ return m_categories; }
		
		virtual void usage(std::ostream &dst) const
		{ m_usage(dst); }
		
		/*! Consumes zero or more string arguments, and creates an operator
			\param args The offered arguments start at index 0 of the vector, and it is up to the
			factory to check the types and whether there are enough.
			\param consumed On exit, the factory indicates how many of the arguments are used up.
			\note I would prefer this to be shared_ptr<Operator>, but this is more in the
			flopoco style. If desired, the creator can immediately wrap it in shared_ptr, as it would
			be a unique reference.
		*/
		virtual Operator *parseCommandLine(
																			 Target *target,
																			 const std::vector<std::string> &args,
																			 int &consumed
																			 )const
		{
			return m_parser(target, args, consumed);
		}



		static void add(
										std::string name,			
										std::string categories,	// semi-colon seperated list of categories
										usage_func_t usage,
										parser_func_t parser
										);

	public:	
		static std::vector<std::string> Parameters(std::string a)
		{ return std::vector<std::string>(1, a); }
	
		static std::vector<std::string> Parameters(std::string a, std::string b)
		{ std::vector<std::string> res; res.push_back(a); res.push_back(b); return res; }
	
		static std::vector<std::string> Parameters(std::string a, std::string b, std::string c)
		{ std::vector<std::string> res; res.push_back(a); res.push_back(b); res.push_back(c); return res; }
	
		static std::vector<std::string> Parameters(std::string a, std::string b, std::string c, std::string d)
		{ std::vector<std::string> res; res.push_back(a); res.push_back(b); res.push_back(c); res.push_back(d); return res; }
	
		static std::vector<std::string> Parameters(std::string a, std::string b, std::string c, std::string d, std::string e)
		{ std::vector<std::string> res; res.push_back(a); res.push_back(b); res.push_back(c); res.push_back(d); res.push_back(e); return res; }
	
	
		static std::vector<std::vector<std::string> > ParameterList(const std::vector<std::string> &a)
		{ std::vector<std::vector<std::string> > res; res.push_back(a); return res; }

		static std::vector<std::vector<std::string> > ParameterList(const std::vector<std::string> &a, const std::vector<std::string> &b)
		{ std::vector<std::vector<std::string> > res; res.push_back(a); res.push_back(b); return res; }
	
		static std::vector<std::vector<std::string> > ParameterList(const std::vector<std::string> &a, const std::vector<std::string> &b, const std::vector<std::string> &c)
		{ std::vector<std::vector<std::string> > res; res.push_back(a); res.push_back(b); res.push_back(c); return res; }
	
	};
}; // namespace flopoco

#endif
