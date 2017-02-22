#define HAVE_SCIP //!!!

#ifndef BITHEAPILPCOMPRESSION_H
#define BITHEAPILPCOMPRESSION_H

#ifdef HAVE_SCIP

#include "BitHeap.hpp"

#include <scip/scip.h>
#include <scip/scipdefplugins.h>

#include <vector>
#include <list>

//#define LARGE_NUMBER 10000; //number which has to be larger than any column in the bit heap (used in ILP)

namespace flopoco
{
    //class BitHeapHeuristicCompression;
    //struct variableBasicCompressor;

	class BitHeapILPCompression
	{
	public:

        struct variableBasicCompressor {
            vector<int> height;
            vector<int> outputs;
            double areaCost;
        };


        BitHeapILPCompression(BitHeap* bh);
		~BitHeapILPCompression();
		int generateProblem();
		int writeProblem(std::string filename = "");
		bool solve();
        int passHeuristicSolutions();
		void plotSolution();
        int cleanUp();

        vector<list<pair<int,int> > > solution; //index is stage s, list contains pairs of compression elements (1st) and column (2nd), note that there may be several compression elements for the same column
        vector<vector<list<pair<int, int> > > > heuristicSolutions; //contains solutions, which are generated by heuristics
        //variables to implement heuristic approach
        bool useHeuristic;
        bool useFixedStageCount;
        bool getExternalStageCount;
        double preReductionAreaCost;
        bool dontAddFlipFlop;
        vector<vector<int> > newBits;  //thats the bits which are added by the heuristic in each Cycle
        double costOfCurrentSolution;
        bool infeasible;
        unsigned zeroStages;
        int noOfStages_;
        bool useVariableCompressors;
        int compressionType;

        void buildVariableCompressors();

	protected:
		BitHeap* bh_;
		SCIP* scip;


		int noOfColumnsMax;//max. number of columns in the bit heap
		vector<BasicCompressor *>* possibleCompressors_; //vector containing all possible compressors, (index = compressior id)
        vector<variableBasicCompressor> variableBCompressors;
		int getMaxStageCount(int maxHeight);


		BasicCompressor *flipflop; //pointer to the basic compressor acting as sinple flip flop
	private:
		//containers for ILP variables:
		vector<vector<vector<SCIP_VAR*> > > compCountVars; //k_s_e_c: counts the number of compression elements for stage (1st index), compressing element (2nd index) and column (3rd index)
		vector<vector<SCIP_VAR*> > columnBitCountVars; //N_s_c: counts the number of bits in stage s (1st index) and column (2nd index)
		vector<SCIP_VAR*> stageVars; //D_s: true if stage is output stage

        vector<vector<SCIP_VAR*> > newBitsCountVars; //U_s_c: counts the number of bits which are added in stage s (1st index) and column (2nd index)

		SCIP_SOL* sol;
		int noOfStagesUsed;

		string srcFileName; //for debug outputs

        vector<vector<vector<int > > > compressorCount;
        vector<vector<int > > heuristicN;

        void computeCompressorCount(int pos);
        void computeHeuristicN();
        void printNewBits();
        
		string uniqueName_; /**< useful only to enable same kind of reporting as for FloPoCo operators. */        

	};
}

#endif // HAVE_SCIP

#endif // BITHEAPILPCOMPRESSION_H
