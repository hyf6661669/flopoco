#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <map>
#include <gmpxx.h>
#include <float.h>
#include <utility>

#include "Target.hpp"
#include "Signal.hpp"
#include "Instance.hpp"

#include "TestBenches/TestCase.hpp"

#include "sollya.h"

#include "FlopocoStream.hpp"
#include "utils.hpp"
#include "Tools/ResourceEstimationHelper.hpp"
#include "Tools/FloorplanningHelper.hpp"
#include "TestState.hpp"

using namespace std;

// variables set by the command-line interface in main.cpp


namespace flopoco {

	// global const variables
	static const map<string, double> emptyDelayMap;
	const std::string tab = "   ";

	// Reporting levels
#define LIST 0       // information necessary to the user of FloPoCo
#define INFO 1       // information useful to the user of FloPoCo
#define DETAILED 2   // information that shows how the algorithm works
#define DEBUG 3      // debug info, useful mostly to developers
#define FULL 4       // pure noise

#define INNER_SEPARATOR "................................................................................"
#define DEBUG_SEPARATOR "________________________________________________________________________________"
#define OUTER_SEPARATOR "################################################################################"
#define REPORT(level, stream) {if ((level)<=(verbose)){ cerr << "> " << srcFileName << ": " << stream << endl;}else{}}
#define THROWERROR(stream) {{ostringstream o; o << " ERROR in " << uniqueName_ << " (" << srcFileName << "): " << stream << endl; throw o.str();}}


//Floorplanning - direction of placement constraints
#define ABOVE						0
#define UNDER						1
#define TO_LEFT_OF					2
#define TO_RIGHT_OF					3

#define ABOVE_WITH_EXTRA				4
#define UNDER_WITH_EXTRA				5
#define TO_LEFT_OF_WITH_EXTRA			6
#define TO_RIGHT_OF_WITH_EXTRA			7

//Floorplanning - constraint type
#define PLACEMENT 					0
#define CONNECTIVITY				1


/**
 * This is a top-level class representing an Operator.
 * This class is inherited by all classes which will output a VHDL entity.
 */
class Operator
{

	static int uid;                  /**< The counter holding a unique id */

public:

	/**
	 * Add a sub-operator to this operator
	 */
	void addSubComponent(Operator* op);


	/**
	 * Add this operator to the global (first-level) list, which is stored in its Target (not really its place, sorry).
	 * This method should be called by
	 * 	1/ the main / top-level, or
	 * 	2/ for sub-components that are really basic operators,
	 * 	   expected to be used several times, *in a way that is independent of the context/timing*.
	 * Typical example is a table designed to fit in a LUT or parallel row of LUTs
	 * We assume all the operators added to GlobalOpList are un-pipelined.
	 */
	void addToGlobalOpList();

	/**
	 * Add operator @param op to the global (first-level) list, which is stored in its Target.
	 * C.f. version of the method with no parameters for usage and more explanations
	 * @param op the operator to add to the global operator list
	 */
	void addToGlobalOpList(Operator *op);


	/**
	 * Generates the code for a list of operators and all their subcomponents
	 */
	static void outputVHDLToFile(vector<Operator*> &oplist, ofstream& file);


#if 1
	/**
	 * Generates the code for this operator and all its subcomponents
	 */
	void outputVHDLToFile(ofstream& file);
#endif

	/**
	 * Operator Constructor.
	 * Creates an operator instance with an instantiated target for deployment.
	 * @param target_ The deployment target of the operator.
	 */
	Operator(Target* target, map<string, double> inputDelays = emptyDelayMap);


	/**
	 * Operator Destructor.
	 */
	virtual ~Operator() {}


 /*****************************************************************************/
 /*         Paperwork-related methods (for defining an operator entity)       */
 /*****************************************************************************/

	/**
	 * Adds an input signal to the operator.
	 * 	Adds a signal of type Signal::in to the the I/O signal list.
	 * @param name  the name of the signal
	 * @param width the number of bits of the signal.
	 * @param isBus describes if this signal is a bus, that is, an instance of std_logic_vector
	 */
	void addInput  (const std::string name, const int width=1, const bool isBus=true);

	/**
	 * Adds an input wire (of type std_logic) to the operator.
	 * 	Adds a signal of type Signal::in to the the I/O signal list.
	 * @param name  the name of the signal
	 */
	void addInput  (const std::string name) {
		addInput (name, 1, false);
	}

	void addInput (const char* name) {
		addInput (name, 1, false);
	}

	/**
	 * Adds  signal to the operator.
	 * 	Adds a signal of type Signal::out to the the I/O signal list.
	 * @param name  the name of the signal
	 * @param width the number of bits of the signal.
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 * @param isBus describes if this signal is a bus, that is, an instance of std_logic_vector
	 */
	void addOutput(const std::string name, const int width=1, const int numberOfPossibleOutputValues=1, const bool isBus=true);

	/**
	 * Adds an output wire (of type std_logic) with one possible value to the operator.
	 * 	Adds a signal of type Signal::out to the the I/O signal list.
	 * @param name  the name of the signal
	 */
	void addOutput(const std::string name);

	void addOutput(const char* name);

#if 1
	// Test:
	// One option is that fixed-point I/Os should always be plain std_logic_vectors.
	// It just makes the framework simpler, and anyway typing is managed internally
	// FP I/O need to be typed to manage the testbenches, e.g. FP equality does not resume to equality on the bit vectors.
	// This is not the case for fixed-point
	// (comment by F de Dinechin)

	/**
	 * Adds a fixed-point input signal to the operator.
	 * @param name  the name of the signal
	 * @param isSigned  is the signal signed/unsigned
	 * @param msb the most significant bit of the signal's format
	 * @param lsb the least significant bit of the signal's format
	 */
	void addFixInput(const std::string name, const bool isSigned, const int msb, const int lsb);


	/**
	 * Adds a fixed-point output signal to the operator.
	 * @param name  the name of the signal
	 * @param isSigned  is the signal signed/unsigned
	 * @param msb the most significant bit of the signal's format
	 * @param lsb the least significant bit of the signal's format
	 * @param numberOfPossibleOutputValues the number of possible values that the signal can take;
	 * 	useful for testing; related to rounding
	 */
	void addFixOutput(const std::string name, const bool isSigned, const int msb, const int lsb, const int numberOfPossibleOutputValues=1);
#endif

	/**
	 * Adds a floating point (FloPoCo format) input signal to the operator.
	 * 	Adds a signal of type Signal::in to the the I/O signal list,
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 3. (2 bits for exception, 1 for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 */
	void addFPInput(const std::string name, const int wE, const int wF);


	/**
	 * Adds a floating point (FloPoCo format) output signal to the operator.
	 * 	Adds a signal of type Signal::out to the the I/O signal list,
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 3. (2 bits for exception, 1 for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 */
	void addFPOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues=1);

	/**
	 * Adds a IEEE floating point input signal to the operator.
	 * 	Adds a signal of type Signal::in to the the I/O signal list,
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 1.  (1 bit for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 */
	void addIEEEInput(const std::string name, const int wE, const int wF);


	/**
	 * Adds a floating point output signal to the operator.
	 * 	Adds a signal of type Signal::out to the the I/O signal list,
	 * having the FP flag set on true. The total width of this signal will
	 * be wE + wF + 1. (1 bit for sign)
	 * @param name the name of the signal
	 * @param wE   the width of the exponent
	 * @param wF   the withh of the fraction
	 * @param numberOfPossibleOutputValues (optional, defaults to 1) set to 2 for a faithfully rounded operator for instance
	 */
	void addIEEEOutput(const std::string name, const int wE, const int wF, const int numberOfPossibleOutputValues=1);



	/**
	 * Sets the copyright string: should be authors + year
	 * @param authorsYears the names of the authors and the years of their contributions
	 */
	void setCopyrightString(std::string authorsYears);


	/**
	 * Use the Synopsys de-facto standard ieee.std_logic_unsigned for this entity
	 */
	void useStdLogicUnsigned();

	/**
	 * Use the Synopsys de-facto standard ieee.std_logic_signed for this entity
	 */
	void useStdLogicSigned();

	/**
	 * Use the real IEEE standard ieee.numeric_std for this entity
	 */
	void useNumericStd();

	/**
	 * Use the real IEEE standard ieee.numeric_std for this entity, also
	 * with support for signed operations on bit vectors
	 */
	void useNumericStd_Signed();

	/**
	 * Use the real IEEE standard ieee.numeric_std for this entity, also
	 * with support for unsigned operations on bit vectors
	 */
	void useNumericStd_Unsigned();

	/**
	 * Return the type of library used for this operator (ieee.xxx)
	 */
	int getStdLibType();

	/**
	 * Sets Operator name to given name, with either the frequency appended, or "comb" for combinatorial.
	 * @param operatorName new name of the operator
	*/
	void setNameWithFreq(std::string operatorName = "UnknownOperator");

	/**
	 * Sets the name of the operator to operatorName.
	 * @param operatorName new name of the operator
	*/
	void setName(std::string operatorName = "UnknownOperator");

	/**
	 * This method should be used by an operator to change the default name of a sub-component.
	 * The default name becomes the commented name.
	 * @param operatorName new name of the operator
	*/
	void changeName(std::string operatorName);

	/**
	 * Sets Operator name to prefix_(uniqueName_)_postfix
	 * @param prefix the prefix string which will be placed in front of the operator name
	 *               formed with the operator internal parameters
	 * @param postfix the postfix string which will be placed at the end of the operator name
	 *                formed with the operator internal parameters
	*/
	void setName(std::string prefix, std::string postfix);


	/**
	 * Adds a comment before the entity declaration, along with the copyright string etc.
	 * The "comment" string should include -- at the beginning of each line.
	*/
	void addHeaderComment(std::string comment);

	/**
	 * Returns a string value representing the name of the operator.
	 * @return operator name
	 */
	string getName() const;

	/**
	 * Produces a new unique identifier
	 */
	static int getNewUId();




 /*****************************************************************************/
 /*        VHDL-related methods (for defining an operator architecture)       */
 /*****************************************************************************/


	/**
	 * Functions related to pipeline management
	 */

	/**
	 * DEPRECATED
	 * Define the current cycle, and resets the critical path
	 * @param the new value of the current cycle
	 */
	void setCycle(int cycle, bool report=true) ;

	/**
	 * DEPRECATED
	 * Return the current cycle
	 * @return the current cycle
	 */
	int getCurrentCycle();

	/**
	 * DEPRECATED
	 * Advance the current cycle by 1, and resets the critical path
	 * @param the new value of the current cycle
	 */
	void nextCycle(bool report=true) ;

	/**
	 * DEPRECATED
	 * Define the current cycle, and reset the critical path
	 * @param the new value of the current cycle
	 */
	void previousCycle(bool report=true) ;

	/**
	 * DEPRECATED
	 * Return the critical path of the current cycle so far
	 */
	double getCriticalPath() ;

	/**
	 * DEPRECATED
	 * Set or reset the critical path of the current cycle
	 */
	void setCriticalPath(double delay) ;

	/**
	 * DEPRECATED
	 * Adds to the critical path of the current stage, and insert a pipeline stage if needed
	 * @param the delay to add to the critical path of current pipeline stage
	 */
	void addToCriticalPath(double delay) ;

	/**
	 * DEPRECATED
	 * Add @delay to the critical path, advancing the pipeline stages, if needed.
	 * This is the delay corresponding to the signals which follow in the operator constructor
	 * @param delay the delay to be added to the critical path
	 * @param report whether comments about pipelining operations are to be added as comments in the generated code
	 */
	bool manageCriticalPath(double delay=0.0, bool report=true);


	/**
	 * Return the critical path delay associated to a given output of the operator
	 * @param the name of the output
	 */
	double getOutputDelay(string s);

	/**
	 *@param[in] inputList the list of input signals
	 *@return the maximum delay of the inputs list
	 */
	//double getMaxInputDelays(vector<Signal*> inputList);

	/**
	 * DEPRECATED
	 * Set the current cycle to that of a signal and reset the critical path. It may increase or decrease current cycle.
	 * @param name is the signal name. It must have been defined before
	 * @param report is a boolean, if true it will report the cycle
	 */
	void setCycleFromSignal(string name, bool report=true) ;

	/**
	 * DEPRECATED
	 * Set the current cycle and the critical path. It may increase or decrease current cycle.
	 * @param name is the signal name. It must have been defined before.
	 * @param criticalPath is the critical path delay associated to this signal: typically getDelay(name)
	 * @param report is a boolean, if true it will report the cycle
	 */
	void setCycleFromSignal(string name, double criticalPath, bool report=true) ;
	// TODO: FIXME
	// param criticalPath is the critical path delay associated to this signal: typically getDelay(name)
	// Shouldn't this be the default behavior?
	// Check current use and fix.


	/**
	 * Return the cycle of the signal specified by @param name
	 * @param name the name of the signal
	 */
	int getCycleFromSignal(string name, bool report = false);

	/**
	 * Return the critical path of the signal specified by @param name
	 * @param name the name of the signal
	 */
	double getCPFromSignal(string name, bool report = false);

	/**
	 * Return the contribution to the critical path of the signal specified by @param name
	 * @param name the name of the signal
	 */
	double getCPContributionFromSignal(string name, bool report = false);


	/**
	 * DEPRECATED
	 * Advance the current cycle to that of a signal. It may only increase current cycle. To synchronize
	 * two or more signals, first call setCycleFromSignal() on the
	 * first, then syncCycleFromSignal() on the remaining ones. It
	 * will synchronize to the latest of these signals.
	 * @param name is the signal name. It must have been defined before
	 * @param report is a boolean, if true it will report the cycle
	 */
	bool syncCycleFromSignal(string name, bool report=true) ;

	/**
	 * DEPRECATED
	 * advance the current cycle to that of a signal, updating critical paths.
	 * @param name is the signal name. It must have been defined before
	 * @param criticalPath is a double, the critical path already consumed up to the signal passed as first argument.
	 *
	 * We have three cases:
	 * 	1/ currentCycle > name.cycle, then do nothing
	 * 	2/ currentCycle < name.cycle, then advance currentCycle to name.cycle, and set the current critical path to criticalPath
	 * 	3/ currentCycle = name.cycle: set critical path to the max of the two critical paths.
	 */
	bool syncCycleFromSignal(string name, double criticalPath, bool report=true) ;


	/**
	 * DEPRECATED
	 * Sets the delay of the signal with the name given by first argument
	 * @param name the name of the signal
	 * @param delay the delay to be associated with the name
	 */
	void setSignalDelay(string name, double delay);

	/**
	 * DEPRECATED
	 * Returns the delay on the signal with the name denoted by the argument
	 * @param name signal Name
	 * @return delay of this signal
	 */
	double getSignalDelay(string name);


	/**
	 * Functions modifying Signals
	 * These methods belong to the Signal class. Unfortunately, having them in the Signal class
	 * creates a circular dependency, so they are now in Operators, seeing as this is the only place
	 * where they are used.
	 */

	/**
	 * Reset the list of predecessors of the signal targetSignal
	 */
	void resetPredecessors(Signal* targetSignal);

	/**
	 * Add a new predecessor for the signal targetSignal;
	 * a predecessor is a signal that appears in the right-hand side of an assignment
	 * that has this signal on the left-hand side.
	 * @param predecessor a direct predecessor of the current signal
	 * @param delayCycles the extra delay (in clock cycles) between the two signals
	 * 		by default equal to zero
	 */
	void addPredecessor(Signal* targetSignal, Signal* predecessor, int delayCycles = 0);

	/**
	 * Add new predecessors for the signal targetSignal;
	 * @param predecessors a list of direct predecessors of the current signal
	 */
	void addPredecessors(Signal* targetSignal, vector<pair<Signal*, int>> predecessorList);

	/**
	 * Remove an existing predecessor of the signal targetSignal;
	 * @param predecessor a direct predecessor of the current signal
	 * @param delayCycles the extra delay (in clock cycles) between the two signals
	 * 		by default equal to -1, meaning it is not taken into account
	 * @return true if the signal could be removed from the predecessors, false if it doesn't exist as a predecessor
	 */
	void removePredecessor(Signal* targetSignal, Signal* predecessor, int delayCycles = 0);

	/**
	 * Reset the list of successors of the signal targetSignal
	 */
	void resetSuccessors(Signal* targetSignal);

	/**
	 * Add a new successor of the signal targetSignal;
	 * a successor is a signal that appears in the left-hand side of an assignment
	 * that has this signal on the right-hand side.
	 * @param successor a direct successor of the current signal
	 * @param delayCycles the extra delay (in clock cycles) between the two signals
	 * 		by default equal to zero
	 * @return true if the signal could be added as a successor, false if it already existed
	 */
	void addSuccessor(Signal* targetSignal, Signal* successor, int delayCycles = 0);

	/**
	 * Add new successors for the signal targetSignal;
	 * @param successors a list of direct successors of the current signal
	 */
	void addSuccessors(Signal* targetSignal, vector<pair<Signal*, int>> successorList);

	/**
	 * Remove an existing successor of the signal targetSignal;
	 * @param successor a direct successor of the current signal
	 * @param delayCycles the extra delay (in clock cycles) between the two signals
	 * 		by default equal to -1, meaning it is not taken into account
	 * @return true if the signal could be removed from the successors, false if it doesn't exist as a successor
	 */
	void removeSuccessor(Signal* targetSignal, Signal* successor, int delayCycles = 0);

	/**
	 * Set the parent operator of signal
	 */
	void setSignalParentOp(Signal* signal, Operator* newParentOp);

	/**
	 * Declares a signal appearing on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param width is the width of the signal (optional, default 1)
	 * @param isbus: a signal of width 1 is declared as std_logic when false, as std_logic_vector when true (optional, default false)
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @return name
	 */
	string declare(string name, const int width, bool isbus=true, Signal::SignalType regType = Signal::wire);

	/**
	 * Declares a signal appearing on the Left Hand Side of a VHDL assignment
	 * @param criticalPathContribution is the delay that the signal being declared adds to the critical path
	 * @param name is the name of the signal
	 * @param width is the width of the signal (optional, default 1)
	 * @param isbus: a signal of width 1 is declared as std_logic when false, as std_logic_vector when true (optional, default false)
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @param criticalPathContribution: the delay that the signal adds to the critical path of the circuit
	 * @return name
	 */
	string declare(double criticalPathContribution, string name, const int width, bool isbus=true, Signal::SignalType regType = Signal::wire);

	/**
	 * Declares a signal of length 1 as in the previous declare() function, but as std_logic by default
	 * @param name is the name of the signal
	 * @param isbus: if true, declares the signal as std_logic_vector; else declares the signal as std_logic
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @return name
	 */
	string declare(string name, Signal::SignalType regType = Signal::wire);

	/**
	 * Declares a signal of length 1 as in the previous declare() function, but as std_logic by default
	 * @param criticalPathContribution is the delay that the signal being declared adds to the critical path
	 * @param name is the name of the signal
	 * @param isbus: if true, declares the signal as std_logic_vector; else declares the signal as std_logic
	 * @param regType: the registring type of this signal. See also the Signal Class for mor info
	 * @param criticalPathContribution: the delay that the signal adds to the critical path of the circuit
	 * @return name
	 */
	string declare(double criticalPathContribution, string name, Signal::SignalType regType = Signal::wire);


	/**
	 * Declares a fixed-point signal on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param isSigned whether the signal is signed, or not
	 * @param MSB the weight of the MSB of the signal
	 * @param LSB the weight of the LSB of the signal
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @return name
	 */
	string declareFixPoint(string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType = Signal::wire);

	/**
	 * Declares a fixed-point signal on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param isSigned whether the signal is signed, or not
	 * @param MSB the weight of the MSB of the signal
	 * @param LSB the weight of the LSB of the signal
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @param criticalPathContribution: the delay that the signal adds to the critical path of the circuit
	 * @return name
	 */
	string declareFixPoint(double criticalPathContribution, string name, const bool isSigned, const int MSB, const int LSB, Signal::SignalType regType = Signal::wire);


	/**
	 * Declares a floating-point signal on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param wE the weight of the exponent of the signal
	 * @param wF the weight of the mantisa of the signal
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @return name
	 */
	string declareFloatingPoint(string name, const int wE, const int wF, Signal::SignalType regType = Signal::wire, const bool ieeeFormat=false);

	/**
	 * Declares a floating-point signal on the Left Hand Side of a VHDL assignment
	 * @param name is the name of the signal
	 * @param wE the weight of the exponent of the signal
	 * @param wF the weight of the mantisa of the signal
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 * @param criticalPathContribution: the delay that the signal adds to the critical path of the circuit
	 * @return name
	 */
	string declareFloatingPoint(double criticalPathContribution, string name, const int wE, const int wF, Signal::SignalType regType = Signal::wire, const bool ieeeFormat=false);


	/**
	 * Initialize a newly declared signal.
	 * Method used to share code between the declare functions
	 * @param s the newly declared signal
	 * @param criticalPathContribution: the delay that the signal adds to the critical path of the circuit
	 * @param regType: the registring type of this signal. See also the Signal Class for more info
	 */
	void initNewSignal(Signal *s, double criticalPathContribution, Signal::SignalType regType);

	/**
	 * Resizes a fixed-point signal and assigns it to a new declared signal.
	 * 	May zero-extend, sign-extend, or truncate.
	 * 	Warns at debug level when truncating LSBs, and warns a bit louder when truncating MSBs.
	 * @param lhsName is the name of the new (resized) signal
	 * @param rhsName is the name of the old (to be resized) signal
	 * @return name
	 */
	void resizeFixPoint(string lhsName, string rhsName, const int MSB, const int LSB, const int indentLevel=1);


	/**
	 * Delay a signal for a given amount of of cycles.
	 * The name of the signal will be temporarily modified to signal_name^nbDelayCycles,
	 * until the second parsing stage.
	 * The use of the '^' symbol should be kept coherent with the notation in
	 * FlopocoStream
	 * @param signalName the signal to delay
	 * @param nbDelayCycles the number of cycles the signal will be delayed
	 */
	string delay(string signalName, int nbDelayCycles = 1);


	// TODO: add methods that allow for signals with reset (when rewriting FPLargeAcc for the new framework)


#if 1
	/**
	 * Use a signal on the Right
	 * @param name is the name of the signal
	 * @return name
	 */
	string use(string name);

	string use(string name, int delay);
#endif

	/**
	 * Declare an output mapping for an instance of a sub-component
	 * Also declares the local signal implicitly, with width taken from the component
	 * @param op is a pointer to the subcomponent
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignalName is the name of the signal in This mapped to this port
	 * @param newSignal_ (by default true), defined whether or not actualSignalName has to be declared as a new signal by outPortMap
	 * @return name
	 */
	void outPortMap(Operator* op, string componentPortName, string actualSignalName, bool newSignal = true);


	/**
	 * Use a signal as input of a subcomponent
	 * @param op is a pointer to the subcomponent
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignalName is the name of the signal (of this) mapped to this port
	 */
	void inPortMap(Operator* op, string componentPortName, string actualSignalName);

	/**
	 * Use a constant signal as input of a subcomponent.
	 * @param componentPortName is the name of the port on the component
	 * @param actualSignal is the constant signal to be mapped to this port
	 */
	void inPortMapCst(Operator* op, string componentPortName, string actualSignal);

	/**
	 * Returns the VHDL for an instance of a sub-component.
	 * @param op represents the operator to be port mapped
	 * @param instanceName is the name of the instance as a label
	 * @param isGlobalOperator if true, the call to instance will add the operator to the globalOpLits as well
	 * @return name
	 */
	string instance(Operator* op, string instanceName, bool isGlobalOperator = false);




	/**
	 * Adds attributes to the generated VHDL so that the tools use embedded RAM blocks for an instance
	 * @param t a pointer to this instance
	 */
	void useHardRAM(Operator* t);

	/**
	 * Adds attributes to the generated VHDL so that the tools use LUT-based RAM blocks for an instance
	 * @param t a pointer to this instance
	 */
	void useSoftRAM(Operator* t);



	/**
	 * Define the architecture name for this operator (by default : arch)
	 *	@param[in] 	architectureName		- new name for the operator architecture
	 */
	void setArchitectureName(string architectureName);

	/**
	 * A new architecture inline function
	 * @param[in,out] o 	- the stream to which the new architecture line will be added
	 * @param[in]     name	- the name of the entity corresponding to this architecture
	 **/
	void newArchitecture(std::ostream& o, std::string name);

	/**
	 * A begin architecture inline function
	 * @param[in,out] o 	- the stream to which the begin line will be added
	 **/
	void beginArchitecture(std::ostream& o);

	/**
	 * A end architecture inline function
	 * @param[in,out] o 	- the stream to which the begin line will be added
	 **/
	void endArchitecture(std::ostream& o);





 /*****************************************************************************/
 /*        Testing-related methods (for defining an operator testbench)       */
 /*****************************************************************************/

	/**
	 * Gets the correct value associated to one or more inputs.
	 * @param tc the test case, filled with the input values, to be filled with the output values.
	 * @see FPAdd for an example implementation
	 */
	virtual void emulate(TestCase * tc);

	/**
	 * Append standard test cases to a test case list. Standard test
	 * cases are operator-dependent and should include any specific
	 * corner cases you may think of. Never mind removing a standard test case because you think it is no longer useful!
	 * @param tcl a TestCaseList
	 */
	virtual void buildStandardTestCases(TestCaseList* tcl);


	/**
	 * Generate Random Test case identified by an integer . There is a default
	 * implementation using a uniform random generator, but most
	 * operators are not exercised efficiently using such a
	 * generator. For instance, in FPAdd, the random number generator
	 * should be biased to favor exponents which are relatively close
	 * so that an effective addition takes place.
	 * This function create a new TestCase (to be free after use)
	 * See FPExp.cpp for an example of overloading this method.
	 * @param i the identifier of the test case to be generated
	 * @return TestCase*
	 */
	virtual TestCase* buildRandomTestCase(int i);





 /*****************************************************************************/
 /*     From this point, we have methods that are not needed in normal use    */
 /*****************************************************************************/




	/**
	 * Append random test cases to a test case list. There is a default
	 * implementation using a uniform random generator, but most
	 * operators are not exercised efficiently using such a
	 * generator. For instance, in FPAdd, the random number generator
	 * should be biased to favor exponents which are relatively close
	 * so that an effective addition takes place.
	 * In most cases you do need to overload this method,
	 * but simply overload  buildRandomTestCase(int i)
	 * which is called by the default implementation of buildRandomTestCaseList
	 * @param tcl a TestCaseList
	 * @param n the number of random test cases to add
	 */
	virtual void buildRandomTestCaseList(TestCaseList* tcl, int n);





	/**
	 * Build all the signal declarations from signals implicitly declared by declare().
	 */
	string buildVHDLSignalDeclarations();

	/**
	 * Build all the component declarations from the list of components built by instance().
	 */
	string buildVHDLComponentDeclarations();

	/**
	 * Build all the registers from signals implicitly delayed by declare()
	 *	This is the 2.0 equivalent of outputVHDLSignalRegisters
	 */
	string buildVHDLRegisters();

	/**
	 * Build all the type declarations.
	 */
	string buildVHDLTypeDeclarations();

	/**
	 * Output the VHDL constants.
	 */
	string buildVHDLConstantDeclarations();

	/**
	 * Output the VHDL constants.
	 */
	string buildVHDLAttributes();





	/**
	 * The main function that outputs the VHDL for the operator.
	 * If you use the modern (post-0.10) framework you no longer need to overload this method,
	 * the default will do.
	 * @param o the stream where the entity will be output
	 * @param name the name of the architecture
	 */
	virtual void outputVHDL(std::ostream& o, std::string name);

	/**
	 * The main function outputs the VHDL for the operator.
	 * Calls the two parameter version, with name = uniqueName
	 * @param o the stream where the entity will be output
	 */
	void outputVHDL(std::ostream& o);





	/**
	 * Returns true if the operator needs a clock signal;
	 * It will also get a rst but doesn't need to use it.
	 */
	bool isSequential();


	/**
	 * Returns true if the operator needs a recirculation signal
	 *  TODO : change name
	 */
	bool isRecirculatory();

	/**
	 * Set the operator to sequential.
	 * You shouldn't need to use this method for standard operators
	 * (Operator::Operator()  calls it according to Target)
	 */
	void setSequential();

	/**
	 * Set the operator to combinatorial
	 * You shouldn't need to use this method for standard operators
	 * (Operator::Operator()  calls it according to Target)
	 */
	void setCombinatorial();



	/**
	 * Set the operator to need a recirculation signal in order to
	 * trigger the pipeline work
	 */
	void setRecirculationSignal();

	/**
	 * Indicates that it is not a warning if there is feedback of one cycle, but it
	 * is an error if a feedback of more than one cycle happens.
	 */
	void setHasDelay1Feedbacks();


	/**
	 * Indicates that it is not a warning if there is feedback of one cycle, but it
	 * is an error if a feedback of more than one cycle happens.
	 */
	bool hasDelay1Feedbacks();







	/**
	 * Returns a pointer to the signal having the name as @param s.
	 * Throws an exception if the signal is not yet declared.
	 * @param s then name of the signal we want to return
	 * @return the pointer to the signal having name s
	 */
	Signal* getSignalByName(string s);

	/**
	 * Same as getSignalByName() but will strip the ^xx added to the signal
	 * name to signify that the signal is delayed by xx cycles.
	 * Mostly for internal use.
	*/
	Signal* getDelayedSignalByName(string s);

	/**
	 * Return the list of signals declared in this operator
	 */
	vector<Signal*> getSignalList();

	bool isSignalDeclared(string name);


	/**
	 * Return the list of component instances declared in this operator
	 */
	vector<Instance*> getInstances();


	/** DEPRECATED
	 * Outputs component declaration
	 * @param o the stream where the component is outputed
	 * @param name the name of the VHDL component we want to output to o
	 */
	virtual void outputVHDLComponent(std::ostream& o, std::string name);

	/** DEPRECATED
	 * Outputs the VHDL component code of the current operator
	 * @param o the stream where the component is outputed
	 */
	void outputVHDLComponent(std::ostream& o);



	/**
	 * Return the number of input+output signals
	 * @return the size of the IO list. The total number of input and output signals
	 *         of the architecture.
	 */
	int getIOListSize() const;

	/**
	 * Returns a pointer to the list containing the IO signals.
	 * @return pointer to ioList
	 */
	vector<Signal*> * getIOList();

	/**
	 * Passes the IOList by value.
	 * @return the ioList
	 */
	vector<Signal*> getIOListV(){
		return ioList_;
	}


	/**
	 * Returns a pointer a signal from the ioList.
	 * @param the index of the signal in the list
	 * @return pointer to the i'th signal of ioList
	 */
	Signal * getIOListSignal(int i);





	/** DEPRECATED, better use setCopyrightString
	 * Output the licence
	 * @param o the stream where the licence is going to be outputted
	 * @param authorsYears the names of the authors and the years of their contributions
	 */
	void licence(std::ostream& o, std::string authorsYears);


	/**
	 * Output the licence, using copyrightString_
	 * @param o the stream where the licence is going to be outputted
	 */
	void licence(std::ostream& o);


	void pipelineInfo(std::ostream& o, std::string authorsYears);


	void pipelineInfo(std::ostream& o);

	/**
	 * Output the standard library paperwork
	 * @param o the stream where the libraries will be written to
	 */
	void stdLibs(std::ostream& o);


	/** DEPRECATED
	 * Output the VHDL entity of the current operator.
	 * @param o the stream where the entity will be outputted
	 */
	void outputVHDLEntity(std::ostream& o);

	/** DEPRECATED
	 * Output all the signal declarations
	 * @param o the stream where the signal deca
	 */
	void outputVHDLSignalDeclarations(std::ostream& o);


	/**
	 * Add a VHDL type declaration.
	 */
 	void addType(std::string name, std::string def);

	/**
	 * Add a VHDL constant. This may make the code easier to read, but more difficult to debug.
	 */
	void addConstant(std::string name, std::string ctype, int cvalue);

	void addConstant(std::string name, std::string ctype, mpz_class cvalue);

	void addConstant(std::string name, std::string ctype, string cvalue);


	/**
	 * Add an attribute, declaring the attribute's name if it is not done already.
	 */
	void addAttribute(std::string attributeName,  std::string attributeType,  std::string object, std::string value );

	/**
	 * A new line inline function
	 * @param[in,out] o the stream to which the new line will be added
	 **/
	inline void newLine(std::ostream& o) {	o<<endl; }



	/**
	 * Final report function, prints to the terminal.
	 * By default, reports the pipeline depth, but feel free to overload
	 * it if you have anything useful to tell to the end user
	*/
	virtual void outputFinalReport(int level);


	/**
	 * Returns the pipeline depth of this operator
	 * @return the pipeline depth of the operator
	*/
	int getPipelineDepth();

	/**
	 * Set the pipeline depth
	 * Should not be used for operators without memory
	 * @param d the pipeline depth
	 */
	void setPipelineDepth(int d);

	/**
	 * Set the pipeline depth, automatically, as the maximum cycle of the outputs
	 */
	void setPipelineDepth();

	/**
	 * Return the input delay map
	 * @return the input map containing the signal -> delay associations
	 */
	map<string, double> getInputDelayMap();

	/**
	 * Return the output delay map
	 * @return the output map containing the signal -> delay associations
	 */
	map<string, double> getOutDelayMap();

	/**
	 * Return the declare table
	 * @return the map containing the signal -> declaration cycle
	 */
	map<string, int> getDeclareTable();

	/**
	 * Return the target member
	 */
	Target* getTarget();

	/**
	 * Return the operator's unique name
	 */
	string getUniqueName();

	/**
	 * Return the architecture name
	 */
	string getArchitectureName();

	vector<Signal*> getTestCaseSignals();

	map<string, string> getPortMap();

	map<string, Operator*> getSubComponents();

	string getSrcFileName();

	int getOperatorCost();

	int getNumberOfInputs();

	int getNumberOfOutputs();

	map<string, Signal*> getSignalMap();

	map<string, pair<string, string> > getConstants();

	map<string, string> getAttributes();

	map<string, string> getTypes();

	map<pair<string,string>, string> getAttributesValues();

	bool getHasRegistersWithoutReset();

	bool getHasRegistersWithAsyncReset();

	bool getHasRegistersWithSyncReset();

	bool hasReset();

	bool hasClockEnable();

	void setClockEnable(bool val);

	string getCopyrightString();

	bool getNeedRecirculationSignal();

	Operator* getIndirectOperator();

	void setIndirectOperator(Operator* op);

	vector<Operator*> getOpList();

	vector<Operator*>& getOpListR();


	bool hasComponent(string s);

	void cleanup(vector<Operator*> *ol, Operator* op);

	FlopocoStream* getFlopocoVHDLStream();

	/**
	 * Second level parsing of the VHDL code
	 * This function should not be called before the signals are scheduled
	 * WARNING: this function has as a precondition that the signals should be scheduled
	 */
	void parse2();


	/**
	 * Extract the timing dependences between signals.
	 * The raw data is stored in the vhdl FlopocoStream object, in the form of
	 * triplets, storing ("lhs_signal_name", "rhs_signal_name", delay).
	 * Warning: This function should only be called after the vhdl code
	 * 			has been parsed (the first parse)
	 */
	void extractSignalDependences();


	/**
	 * SCHEDULING
	 *
	 * Flow: start with the inputs of the circuit. The inputs are all synchronized
	 * 		at the same cycle, and their critical path might vary. Each input
	 * 		computes its timing and then starts the timing of its children.
	 * 		When the timing is called on an internal node (i.e. not an input),
	 * 		the node first checks if the node has already been scheduled. If yes,
	 * 		then stop the timing procedures, as there is nothing else to do (this
	 * 		might also be a backward loop).
	 * 		Then, check if all of the predecessors have been scheduled.
	 * 		If yes, then the node schedules itself, according to the timing of
	 * 		its parents and to its own constraints, and then launches the timing
	 * 		of its own children.
	 * 		If not, then the timing procedures are stopped. This can be done
	 * 		because it means that the node depends on another node that hasn't
	 * 		yet been scheduled. When the predecessor will finally be scheduled,
	 * 		the launch of the timing for the respective node will also be triggered.
	 *
	 * Backward loops: When dealing with a loop, the timing procedures will come
	 * 		to a halt inside the loop, as they will detect that the node which has
	 * 		data coming from the backward edge has already been scheduled, so there
	 * 		is nothing else left to do.
	 *
	 * Sub-components: start by launching the scheduling procedures on the signals
	 * 		of the operator.
	 * 		We first schedule the signal, and then detect whether the signal and
	 * 		its parent belong to the same operator (meaning the signal belongs to
	 * 		a sub-component of the parent operator of the respective signal's parent).
	 * 		If this is not an input signal, we start scheduling the signal's children.
	 * 		If this is an input signal, we then check if all of the other inputs of the
	 * 		respective sub-component have also been scheduled. If not, we can just
	 * 		stop the current call. If yes, then we synchronize all of the inputs to
	 * 		the same cycle, and then launch the scheduling procedure for the
	 * 		sub-component.
	 * 		When encountering an output port, the scheduling procedures should
	 * 		treat it as a regular signal: set its timing and launch the scheduling
	 * 		of its children.
	 */


	/**
	 * Start the scheduling for this operator.
	 * Try to schedule all the inputs, and then launch the scheduling for the
	 * rest of the internal signals of the operator.
	 */
	void startScheduling();

	/**
	 * Try to schedule the signal targetSignal. The signal can be schedules only
	 * if all of its predecessors have been already scheduled. The function will
	 * also trigger the scheduling of its children, if targetSignal has any.
	 * @param targetSignal the signal to be scheduled
	 */
	void scheduleSignal(Signal *targetSignal);

	/**
	 * Set the timing of a signal.
	 * Used also to share code between the different timing methods.
	 * @param targetSignal the signal to be scheduled
	 */
	void setSignalTiming(Signal* targetSignal);


	void setuid(int mm);

	int getuid();



	string signExtend(string name, int w);

	string zeroExtend(string name, int w);

	int level; //printing issues




	/**
	 * Add a comment line in to vhdl stream
	 */
	void addComment(string comment, string align = tab);

	/**
	 * Add a full line of '-' with comment centered within
	 */
	void addFullComment(string comment, int lineLength = 80);


	/**
	 * Completely replace "this" with a copy of another operator.
	 */
	void cloneOperator(Operator *op);

	/**
	 * Create a deep copy of the operator op, changing also the corresponding
	 * internal references.
	 */
	void deepCloneOperator(Operator *op);

	/**
	 * Method returning a random number depending on a fixed limit, the mean and
	 * the standard deviation
	 **/
	static float pickRandomNum ( float limit = 0, int fp = 8, int sp = 4 );

	/**
	 * Once the valid TestState parameters is created with pickRandomNum, this method checks
	 * if parameters already exist or no for the operator selected opName
	 * Tests are realized with the multimap testMemory
	 **/
	static bool checkExistence ( TestState parameters, string opName );


	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for resource estimations

	//--Logging functions

	/**
	 * Add @count flip-flops to the total estimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addFF(int count = 1);

	/**
	 * Add @count registers to increase the total flip-flop estimate
	 * from the register characteristics
	 * @param count (by default 1) the number of registers to add
	 * @param width the width of each register
	 * @return the string describing the performed operation
	 */
	std::string addReg(int width, int count = 1);

	/**
	 * Add @count function generators to the total estimate
	 * Suggest Look-Up Table type (based on number of inputs), in order
	 * to obtain more accurate predictions
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs number of inputs of the LUT (0 for default option
	 * of target technology)
	 * @return the string describing the performed operation
	 */
	std::string addLUT(int nrInputs = 0, int count = 1);

	/**
	 * Add @count multipliers to the total estimate
	 * NOTE: also increases the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addMultiplier(int count = 1);

	/**
	 * Add @count multipliers each having inputs of bitwidths @widthX and
	 * @widthY, respectively
	 * The user can also chose to what degree the multipliers are
	 * implemented in logic (a number between 0 and 1)
	 * NOTE: also increases the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the multipliers
	 * @param ratio (by default 1) the ratio to which the multipliers
	 * are implemented in logic (0 for 0%, 1 for 100%)
	 * @return the string describing the performed operation
	 */
	std::string addMultiplier(int widthX, int widthY, double ratio = 1, int count = 1);

	/**
	 * Add @count adders/subtracters each having inputs of bitwidths @widthX and
	 * @widthY, respectively
	 * The user can also chose to what degree the adders/subtracters are
	 * implemented in logic (a number between 0 and 1)
	 * NOTE: can also increase the DSP count
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the multipliers
	 * @param ratio (by default 0) the ratio to which the multipliers
	 * are implemented in logic (0 for 0%, 1 for 100%)
	 * @return the string describing the performed operation
	 */
	std::string addAdderSubtracter(int widthX, int widthY, double ratio = 0, int count = 1);

	/**
	 * Add @count memories to the total estimate, each having @size
	 * words of @width bits
	 * The memories can be either RAM or ROM, depending on the value of
	 * the @type parameter
	 * NOTE: Defaults to adding RAM memories
	 * @param count (by default 1) the number of elements to add
	 * @param size the number of words of the memory
	 * @param width the bitwidth of each of the memory's word
	 * @param type (by default 0) the type of the memory
	 * (0 for RAM, 1 for ROM)
	 * @return the string describing the performed operation
	 */
	std::string addMemory(int size, int width, int type = 0, int count = 1);

	//---More particular resource logging
	/**
	 * Add @count DSP(s) to the total estimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addDSP(int count = 1);

	/**
	 * Add @count RAM(s) to the total estimate
	 * NOTE: For a more precise description of the memory being added, use the
	 * @addMemory() function with the corresponding parameters
	 * NOTE: adds memories with the default widths and sizes
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addRAM(int count = 1);

	/**
	 * Add @count ROM(s) to the total estimate
	 * NOTE: For a more precise description of the memory being added, use the
	 * @addMemory() function with the corresponding parameters
	 * NOTE: adds memories with the default widths and sizes
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addROM(int count = 1);

	/**
	 * Add @count Shift Registers to the total estimate, each having a
	 * bitwidth of @width bits
	 * NOTE: this function also modifies the total number of LUTs and FFs
	 * in the design; this aspect should be considered so as not to result
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the registers
	 * @param depth the depth of the shift register
	 * @return the string describing the performed operation
	 */
	std::string addSRL(int width, int depth, int count = 1);

	/**
	 * Add @count wire elements to the total estimate
	 * The estimation can be done in conjunction with the declaration of a
	 * certain signal, in which specify the signal's name is specified
	 * through the @signalName parameter
	 * NOTE: it is not advised to use the function without specifying
	 * the signal's name, as it results in duplication of resource count
	 * NOTE: if @signalName is provided, @count can be omitted, as it
	 * serves no purpose
	 * @param count (by default 1) the number of elements to add
	 * @param signalName (by default the empty string) the name of the
	 * corresponding signal
	 * @return the string describing the performed operation
	 */
	std::string addWire(int count = 1, std::string signalName = "");

	/**
	 * Add @count I/O ports to the total estimate
	 * The estimation can be done in conjunction with the declaration
	 * of a certain port, in which specify the port's name is specified
	 * through the @portName parameter
	 * NOTE: it is not advised to use the function without specifying
	 * the port's name, as it results in duplication of resource count
	 * NOTE: if @portName is provided, @count can be omitted, as it
	 * serves no purpose
	 * @param count (by default 1) the number of elements to add
	 * @param portName (by default the empty string) the name of the
	 * corresponding port
	 * @return the string describing the performed operation
	 */
	std::string addIOB(int count = 1, std::string portName = "");

	//---Even more particular resource logging
	/**
	 * Add @count multiplexers to the total estimate, each having
	 * @nrInputs inputs of @width bitwidths
	 * NOTE: this function also modifies the total number of LUTs in
	 * the design; this aspect should be considered so as not to result
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs (by default 2) the number of inputs to the MUX
	 * @param width the bitwidth of the inputs and the output
	 * @return the string describing the performed operation
	 */
	std::string addMux(int width, int nrInputs = 2, int count = 1);

	/**
	 * Add @count counters to the total estimate, each having
	 * @width bitwidth
	 * NOTE: this function also modifies the total number of LUTs and
	 * FFs in the design; this aspect should be considered so as not to
	 * result in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the counter
	 * @return the string describing the performed operation
	 */
	std::string addCounter(int width, int count = 1);

	/**
	 * Add @count accumulators to the total estimate, each having
	 * @width bitwidth
	 * NOTE: this function also modifies the total number of LUTs and
	 * FFs and DSPs in the design; this aspect should be considered so
	 * as not to result in counting the resources multiple times and
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param width the bitwidth of the accumulator
	 * @param useDSP (by default false) whether the use of DSPs is allowed
	 * @return the string describing the performed operation
	 */
	std::string addAccumulator(int width, bool useDSP = false, int count = 1);

	/**
	 * Add @count decoder to the total estimate, each decoding an input
	 * signal of wIn bits to an output signal of wOut bits
	 * NOTE: this function also modifies the total number of LUTs and
	 * FFs and RAMs in the design; this aspect should be considered so
	 * as not to result in counting the resources multiple times and
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @return the string describing the performed operation
	 */
	std::string addDecoder(int wIn, int wOut, int count = 1);

	/**
	 * Add @count arithmetic operator to the total estimate, each having
	 * @nrInputs of @width bitwidths
	 * NOTE: this function also modifies the total number of LUTs in
	 * the design; this aspect should be considered so as not to result
	 * in counting the resources multiple times and overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrInputs (by default 2) the number of inputs of the gate
	 * @param width the bitwidth of the inputs
	 * @return the string describing the performed operation
	 */
	std::string addArithOp(int width, int nrInputs = 2, int count = 1);

	/**
	 * Add @count Finite State Machine to the total estimate, each
	 * having @nrStates states, @nrTransitions transitions
	 * NOTE: this function also modifies the total number of LUTs and
	 * FFs and ROMs in the design; this aspect should be considered so
	 * as not to result in counting the resources multiple times and
	 * overestimate
	 * @param count (by default 1) the number of elements to add
	 * @param nrStates the number of states of the FSM
	 * @param nrTransitions (by default 0) the number of transitions of
	 * the FSM
	 * @return the string describing the performed operation
	 */
	std::string addFSM(int nrStates, int nrTransitions = 0, int count = 1);

	//--Resource usage statistics
	/**
	 * Generate statistics regarding resource utilization in the design,
	 * based on the user's approximations
	 * @param detailLevel (by default 0, basic resource estimations)
	 * the level of detail to which the resource utilizations are
	 * reported (0 - basic report; 1 - include the more specific
	 * resources; 2 - include all statistics)
	 * @return a formatted string containing the statistics
	 */
	std::string generateStatistics(int detailLevel = 0);

	//--Utility functions related to the generation of resource usage statistics
	/**
	 * Count registers that are due to design pipelining
	 * @return the string describing the performed operation
	 */
	std::string addPipelineFF();

	/**
	 * Count wires from declared signals
	 * @return the string describing the performed operation
	 */
	std::string addWireCount();

	/**
	 * Count I/O ports from declared inputs and outputs
	 * @return the string describing the performed operation
	 */
	std::string addPortCount();

	/**
	 * Count resources added from components
	 * @return the string describing the performed operation
	 */
	std::string addComponentResourceCount();

	/**
	 * Perform automatic operations related to resource estimation; this includes:
	 * 		- count registers added due to pipelining framework
	 * 		- count input/output ports
	 * 		- count resources in subcomponents
	 * Should not be used together with the manual estimation functions addWireCount, addPortCount, addComponentResourceCount!
	 * @return the string describing the performed operation
	 */
	void addAutomaticResourceEstimations();
	/////////////////////////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Functions used for floorplanning
	/**
	 * NOTE: Floorplanning should be used only is resource estimation is
	 * also used. The floorplanning tools rely on the data provided by
	 * the resource estimation.
	 */


	/**
	 * Count the resources that have been added (as glue logic), since
	 * the last module has been instantiated. It will create a virtual
	 * module that is placed between the real modules, and that accounts
	 * for the space needed for the glue logic.
	 * Possibly to be integrated in the instance() method, as the
	 * process can be done without the intervention of the user.
	 * Uses and updates the pastEstimation... set of variables.
	 * @return the string summarizing the operation
	 */
	std::string manageFloorplan();

	/**
	 * Add a new placement constraint between the @source and @sink
	 * modules. The constraint should be read as: "@sink is @type of @source".
	 * The type of the constraint should be one of the following
	 * predefined constants: TO_LEFT_OF, TO_RIGHT_OF, ABOVE, UNDER.
	 * NOTE: @source and @sink are the operators' names, NOT
	 * the instances' names
	 * @param source the source sub-component
	 * @param sink the sink sub-component
	 * @param type the constraint type (has as value predefined constant)
	 * @return the string summarizing the operation
	 */
	std::string addPlacementConstraint(std::string source, std::string sink, int type);

	/**
	 * Add a new connectivity constraint between the @source and @sink
	 * modules. The constraint should be read as: "@sink is connected
	 * to @source by @nrWires wires".
	 * NOTE: @source and @sink are the operators' names, NOT
	 * the instances' names
	 * @param source the source sub-component
	 * @param sink the sink sub-component
	 * @param nrWires the number of wires that connect the two modules
	 * @return the string summarizing the operation
	 */
	std::string addConnectivityConstraint(std::string source, std::string sink, int nrWires);

	/**
	 * Add a new aspect constraint for @source module. The constraint
	 * should be read as: "@source's width is @ratio times larger than
	 * its width".
	 * @param source the source sub-component
	 * @param ratio the aspect ratio
	 * @return the string summarizing the operation
	 */
	std::string addAspectConstraint(std::string source, double ratio);

	/**
	 * Add a new constraint for @source module, regarding the contents
	 * of the module. The constraint gives an indication on the possible
	 * size/shape constraints, depending what the module contains.
	 * @param source the source sub-component
	 * @param value the type of content constraint
	 * @param length the length, if needed, of the component (for
	 * example for adders or multipliers)
	 * @return the string summarizing the operation
	 */
	std::string addContentConstraint(std::string source, int value, int length);

	/**
	 * Process the placement and connectivity constraints that the
	 * user has input using the corresponding functions.
	 * Start by processing the placement constraints and then, when
	 * needed, process the connectivity constraints
	 * @return the string summarizing the operation
	 */
	std::string processConstraints();

	/**
	 * Create the virtual grid for the sub-components.
	 * @return the string summarizing the operation
	 */
	std::string createVirtualGrid();

	/**
	 * Transform the virtual placement grid into the actual placement on
	 * the device, ready to generate the actual constraints file.
	 * @return the string summarizing the operation
	 */
	std::string createPlacementGrid();

	/**
	 * Create the file that will contain the floorplanning constraints.
	 * @return the string summarizing the operation
	 */
	std::string createConstraintsFile();

	/**
	 * Generate the placement for a given module.
	 * @param moduleName the name of the module
	 * @return the string summarizing the operation
	 */
	std::string createPlacementForComponent(std::string moduleName);

	/**
	 * Create the floorplan, according the flow described in each
	 * function and according to the user placed constraints.
	 */
	std::string createFloorplan();

	/////////////////////////////////////////////////////////////////////////////////////////////////



	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////BEWARE: don't add anything below without adding it to cloneOperator, too

	//disabled during the overhaul
	//map<string, Operator*> subComponents_;					/**< The list of instantiated sub-components */
	vector<Instance*>      instances_;                      /**< The list of instances (with the corresponding port maps) */
	vector<Signal*>        signalList_;      				/**< The list of internal signals of the operator */
	vector<Signal*>        ioList_;                         /**< The list of I/O signals of the operator */

	FlopocoStream          vhdl;                            /**< The internal stream to which the constructor will build the VHDL code */
	int                    numberOfTests;                   /**< The number of tests, set by TestBench before this operator is tested */


	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Variables used for resource estimations
	std::ostringstream 	resourceEstimate;                   /**< The log of resource estimations made by the user */
	std::ostringstream 	resourceEstimateReport;             /**< The final report of resource estimations made by the user */

	ResourceEstimationHelper* reHelper;                     /**< Performs all the necessary operations for resource estimation */

	bool reActive;                                          /**< Shows if any resource estimation operations have been performed */
	/////////////////////////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////////Variables used for floorplanning
	std::ostringstream 			floorplan;                  /**< Stream containing the floorplanning operations */

	FloorplanningHelper*		flpHelper;                  /**< Tools for floorplanning */
	/////////////////////////////////////////////////////////////////////////////////////////////////

	static multimap < string, TestState > testMemory;       /**< multimap which will be used to test if the selected operator already had been tested */
protected:
	string              uniqueName_;                        /**< By default, a name derived from the operator class and the parameters */
	string 				architectureName_;                  /**< Name of the operator architecture */
	vector<Signal*>     testCaseSignals_;                   /**< The list of pointers to the signals in a test case entry. Its size also gives the dimension of a test case */

	//disabled during the overhaul
	//map<string, map<string, string>>  portMaps_;          /**< Port maps for the instances of this operator */
	map<string, Signal*>  tmpInPortMap_;                    /**< Input port map for the instance of this operator currently being built. Temporary variable, that will be pushed into portMaps_ */
	map<string, Signal*>  tmpOutPortMap_;                   /**< Output port map for the instance of this operator currently being built. Temporary variable, that will be pushed into portMaps_ */
	//disabled during the overhaul
	//map<string, double>  outDelayMap;                       /**< Slack delays on the outputs */
	//map<string, double>  inputDelayMap;                     /**< Slack delays on the inputs */
	string               srcFileName;                       /**< Used to debug and report.  */
	//disabled during the overhaul
	//map<string, int>     declareTable;                      /**< Table containing the name and declaration cycle of the signal */
	int                  myuid;                             /**< Unique id>*/
	int                  cost;                              /**< The cost of the operator depending on different metrics */
	vector<Operator*>    oplist;                            /**< A list of all the sub-operators */


private:
	Target*                target_;                         /**< The target on which the operator will be deployed */
	int                    stdLibType_;                     /**< 0 will use the Synopsys ieee.std_logic_unsigned, -1 uses std_logic_signed, 1 uses ieee numeric_std  (preferred) */
	int                    numberOfInputs_;                 /**< The number of inputs of the operator */
	int                    numberOfOutputs_;                /**< The number of outputs of the operator */
	bool                   isSequential_;                   /**< True if the operator needs a clock signal*/
	int                    pipelineDepth_;                  /**< The pipeline depth of the operator. 0 for combinatorial circuits */
	map<string, Signal*>   signalMap_;                      /**< A container of tuples for recovering the signal based on it's name */
	map<string, pair<string, string>> constants_;           /**< The list of constants of the operator: name, <type, value> */
	map<string, string>    attributes_;                     /**< The list of attribute declarations (name, type) */
	map<string, string>    types_;                          /**< The list of type declarations (name, type) */
	map<pair<string,string>, string >  attributesValues_;   /**< attribute values <attribute name, object (component, signal, etc)> ,  value> */
	bool                   hasRegistersWithoutReset_;       /**< True if the operator has registers without a reset */
	bool                   hasRegistersWithAsyncReset_;     /**< True if the operator has registers having an async reset */
	bool                   hasRegistersWithSyncReset_;      /**< True if the operator has registers having a synch reset */
	string                 commentedName_;                  /**< Usually is the default name of the architecture.  */
	string                 headerComment_;                  /**< Optional comment that gets added to the header. Possibly multiline.  */
	string                 copyrightString_;                /**< Authors and years.  */
	// TODO move the two following to outputVHDL

	//disabled during the overhaul
	//int                    currentCycle_;                 /**< The current cycle, when building a pipeline */
	//double                 criticalPath_;               	/**< The current delay of the current pipeline stage */

	bool                   needRecirculationSignal_;        /**< True if the operator has registers having a recirculation signal  */
	bool                   hasClockEnable_;    	            /**< True if the operator has a clock enable signal  */
	int					   hasDelay1Feedbacks_;             /**< True if this operator has feedbacks of one cycle, and no more than one cycle (i.e. an error if the distance is more). False gives warnings */
	Operator*              indirectOperator_;               /**< NULL if this operator is just an interface operator to several possible implementations, otherwise points to the instance*/

};

	// global variables used through most of FloPoCo,
	// to be encapsulated in something, someday?

	extern int verbose;

} //namespace
#endif
