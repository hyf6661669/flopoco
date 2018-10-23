// general c++ library for manipulating streams
#include <iostream>
#include <sstream>

/* header of libraries to manipulate multiprecision numbers
   There will be used in the emulate function to manipulate arbitraly large
   entries */
#include "gmp.h"
#include "mpfr.h"

// include the header of the Operator
#include "PoolingCore.hpp"

// include int comparator class
#include "Max2Input.hpp"

using namespace std;
namespace flopoco {




    PoolingCore::PoolingCore(Target* target, unsigned int wordSize_, unsigned int windowSize_) :
        Operator(target), wordSize(wordSize_), windowSize(windowSize_), inputNames(){
        // throw error
        if(windowSize<2)
        {
            stringstream e;
            e << "Only window sizes of 2 or more are supported";
            THROWERROR(e.str());
        }

        // author
        setCopyrightString("Nicolai Fiege, 2017");

        // definition of the source file name, used for info and error reporting using REPORT
        srcFileName="PoolingCore";

        // definition of the name of the operator
        ostringstream name;
        name << "PoolingCore_wIn_" << wordSize << "_windowSize_" << windowSize;
        setName(name.str());
        this->useNumericStd();

        // add inputs and output
        unsigned int numberOfInputs = windowSize*windowSize;
        for(unsigned int i=0; i<numberOfInputs; i++)
        {
            inputNames.push_back("X"+to_string(i));
            addInput(inputNames[i], wordSize);
        }
        addOutput("R",wordSize);
		
        // vector for comparator tree
        vector < vector < Max2Input* > > comparators;
        vector < Max2Input* > firstStage;
        comparators.push_back(firstStage);

        // fill vector and declare an output signal for every comparator
        unsigned int stageCounter=0;
        for(unsigned int i=1; i<=numberOfInputs-1; i++)
        {
            if(i>=((unsigned int)1<<(stageCounter+1)))
            {
                stageCounter++;
                vector < Max2Input* > tmp;
                comparators.push_back(tmp);
            }
            Max2Input* op = new Max2Input(target, wordSize, true);
            comparators[stageCounter].push_back(op);
            this->addSubComponent(op);
        }

        // connect comparators
        unsigned int inputCounter=0;
        unsigned int comparatorCounter=0;
        map < Max2Input*,string > outputSignalNames;
        map < Max2Input*,string > indexSignalNames;
        for(unsigned int i=0; i<comparators.size(); i++)
        {
            unsigned int i0=comparators.size()-1-i;
            for(unsigned int j=0; j<comparators[i0].size(); j++)
            {
                unsigned int j0 = j;
                //outportmap
                outputSignalNames[comparators[i0][j0]]=declare("s"+to_string(comparatorCounter),this->wordSize);
                this->outPortMap(comparators[i0][j0],"R","s"+to_string(comparatorCounter),false);

                //inportmap
                if(comparators.size()>i0+1)
                {
                    //connect 1 or 2 comparators of a higher stage
                    if(comparators[i0+1].size()>=(2*(j0+1)))
                    {
                        //connect 2 comparators of a higher stage
                        this->inPortMap(comparators[i0][j0],"X0",outputSignalNames[comparators[i0+1][2*j0]]);
                        this->inPortMap(comparators[i0][j0],"X1",outputSignalNames[comparators[i0+1][2*j0+1]]);
                    }
                    else if(comparators[i0+1].size()==(2*(j0+1)-1))
                    {
                        //connect 1 comparator of a higher stage and 1 input
                        this->inPortMap(comparators[i0][j0],"X0",outputSignalNames[comparators[i0+1][2*j0]]);
                        this->inPortMap(comparators[i0][j0],"X1",inputNames[inputCounter]);
                        inputCounter++;
                    }
                    else
                    {
                        //connect 2 inputs
                        this->inPortMap(comparators[i0][j0],"X0",inputNames[inputCounter]);
                        inputCounter++;
                        this->inPortMap(comparators[i0][j0],"X1",inputNames[inputCounter]);
                        inputCounter++;
                    }
                }
                else
                {
                    //connect 2 inputs
                    this->inPortMap(comparators[i0][j0],"X0",inputNames[inputCounter]);
                    inputCounter++;
                    this->inPortMap(comparators[i0][j0],"X1",inputNames[inputCounter]);
                    inputCounter++;
                }

                this->vhdl << this->instance(comparators[i0][j0], "comparator_instance"+to_string(comparatorCounter));
                comparatorCounter++;
            }
        }

        nextCycle();
        vhdl << "R <= " << outputSignalNames[comparators[0][0]] << ";" << endl;
        this->setCycle(comparators.size()+1);
    }
}//namespace flopoco
