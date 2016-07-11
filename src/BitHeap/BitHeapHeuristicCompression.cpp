//#ifdef HAVE_SCIP

#include "BitHeapHeuristicCompression.hpp"
#include <algorithm>

namespace flopoco
{


    BitHeapHeuristicCompression::BitHeapHeuristicCompression(BitHeap* bh, std::string mode)
       : bh_(bh), mode(mode)
#ifdef HAVE_SCIP
       , bitHeapILPCompression(bh)
#endif //HAVE_SCIP
    {
        //standard values
        lowerBound = 1.5;
        useHoles = true;
        generateSolution = false;
        passHeuristicSolution = false;
        passMultipleSolutions = false;
        usePreReduction = false;
        reduceILPStageCount = false;
        useMaxHeuristicStageCount = false;
        optimalAndPreReduction = false;
        differentStages = false;
        minSliceCount = 10000000;
        minFixedStage = 3;
        maxFixedStage = 6;
        paSorting = false;
        buildSingleStages = false;
        numberOfBuildStages = 0;
        useSmootherCompressorPlacing = false;
        dontUsePA = false;
        useVariableCompressors = false;
        useCompleteHeuristic = false;
        getLowerBoundsFromBitHeap = false;

        //modified values

        //lowerBound = 0.0;
        //useHoles = false;
        generateSolution = true;
        passHeuristicSolution = true;
        //passMultipleSolutions = true;
        //usePreReduction = true;
        reduceILPStageCount = true;
        //useMaxHeuristicStageCount = true;
        //optimalAndPreReduction = true;
        differentStages = true;
        dontUsePA = false;
        minFixedStage = 0;
        maxFixedStage = 8;
        //paSorting = true;
        //buildSingleStages = true;
        //numberOfBuildStages = 8;
        //useSmootherCompressorPlacing = true;
        //useVariableCompressors = true;
        //useCompleteHeuristic = true;
        getLowerBoundsFromBitHeap = true;



        //now some dependencies

        if(reduceILPStageCount && !passHeuristicSolution){
            reduceILPStageCount = false;
        }
        cout << "mode is " << mode << endl;
        //get the compressiontype from mode
#ifdef HAVE_SCIP
        if(mode.compare("heuristic_parandeh-afshar_modified") == 0){
            bitHeapILPCompression.compressionType = 6;
        }
        else if(mode.compare("heuristic_pa") == 0){
            bitHeapILPCompression.compressionType = 7;
        }
        else if(mode.compare("optimalMinStages") == 0){
            bitHeapILPCompression.compressionType = 5;
        }
#endif //HAVE_SCIP
    }



    BitHeapHeuristicCompression::~BitHeapHeuristicCompression(){
        //cleanUp();
    }

    int BitHeapHeuristicCompression::getMaxStageCount(int maxHeight){
        int height=2;
        int stageCount=0;

        //the following loop computes the Dadda sequence [Dadda 1965] which is [used] as max. no of stages
        //(typically less are selected by the optimization using higher order compressors)
        while(height < maxHeight)
        {
            //REPORT(DEBUG, "stageCount=" << stageCount << ", height=" << height);
            height = floor(height*3.0/2.0);
            stageCount++;
        }
        return stageCount;
    }

    int BitHeapHeuristicCompression::generateProblem(){

        if(passHeuristicSolution){
            lowerBound = 0.0;
        }
#ifdef HAVE_SCIP
        bitHeapILPCompression.useVariableCompressors = useVariableCompressors;
        bitHeapILPCompression.buildVariableCompressors();//only done if useVariableCompressors = true
#endif //HAVE_SCIP

        noOfStages_ = getMaxStageCount(bh_->getMaxHeight());
        cout << "noOfStages: " << noOfStages_ << endl;
        //fill solution with empty lists
        solution.resize(noOfStages_);

        if(!getLowerBoundsFromBitHeap){
            for(unsigned i = 0; i < (sizeof(lowerBounds) / sizeof(lowerBounds[0])); i++){
                lowerBounds[i] = 10.0;
                if(useCompleteHeuristic || passMultipleSolutions){
                    lowerBounds[i] = 0;     //use PA in every stage
                }
            }
        }





        //now fill lowerBounds with real values
        //the preset value is infinity (a.k.a. 10.0)

        //lowerBounds[0] = 0;
        //lowerBounds[1] = 1;
        //lowerBounds[2] = 1.75;
        //lowerBounds[3] = 1.75;
        //lowerBounds[4] = 0;
        //lowerBounds[5] = 0;
        //lowerBounds[6] = 0;
        //lowerBounds[7] = 0;

        printLowerBounds();

        //set the minFixedStage to at least the number of stages which are solved by the heuristic
        unsigned lowerBoundZeros = 0;
        for(unsigned i = 0; i < (sizeof(lowerBounds) / sizeof(lowerBounds[0])); i++){
            if(fabs(lowerBounds[i]) < 0.0001){
                lowerBoundZeros++;
            }
            else{
                break;
            }
        }
        if(lowerBoundZeros > minFixedStage){
            minFixedStage = lowerBoundZeros;
            if(maxFixedStage < minFixedStage){
                maxFixedStage = minFixedStage;
                cout << "maxFixedStage set to minFixedStage" << endl;
            }
        }
        cout << "minFixedStage = " << minFixedStage << " maxFixedStage = " << maxFixedStage << endl;

        //generate newBits
        vector<int> firstCycle(bh_->bits.size());
        for(unsigned w = 0; w < bh_->bits.size(); w++){
            firstCycle[w] = bh_->bits[w].size();
        }
        newBits.push_back(firstCycle);
        originalNewBits.push_back(firstCycle);

        //now fill the rest with zeros
        //in every stage, the size increases by two, because the (6,0,6;5) compressor can increase the size by two

        for(unsigned v = 1; v < (unsigned)noOfStages_; v++){
            vector<int> tempZeroVector(2 * v + bh_->bits.size());
            for(unsigned w = 0; w < tempZeroVector.size(); w++){
                tempZeroVector[w] = 0;
            }
            newBits.push_back(tempZeroVector);
            originalNewBits.push_back(tempZeroVector);
        }

        //cout << mode << endl;

        //debug output:: newBits
        unsigned int sumOfBits = 0;
        cout << "newbits before heuristic:" << endl;
        for(unsigned i = 0; i < newBits.size(); i++){
            for(unsigned j = 0; j < newBits[i].size(); j++){
                cout << newBits[i][j] << " ";
                sumOfBits += newBits[i][j];
            }
            cout << endl;
        }
        cout << "The total amount of initial Bits in the Bitheap is " << sumOfBits << endl;




        if(mode.compare("heuristic_parandeh-afshar_modified") == 0 || mode.compare("optimalMinStages") == 0){
            //this is an improved algorithm of the heuristic descriped by parandeh-afshar

            if(mode.compare("optimalMinStages") == 0){
                dontUsePA = true;
            }
            vector<BasicCompressor *> * compUnsorted = bh_->getPossibleCompressors();

/*
            //now add flipflop so that a lowerBound of 0 creates a valid solution
            BasicCompressor *flipflop;
            vector<int> col(1);
            col[0]=1;
            flipflop = new BasicCompressor(bh_->getOp()->getTarget(),col);
            flipflop->areaCost = 0.5;
            compUnsorted->push_back(flipflop);
*/
            if(paSorting){
                cout << "we sort the compressors according to the pa-efficiency: inputBits / outputBits" << endl;
            }
            else{
                cout << "we sort the compressors according to the efficiency : (inputBits - outputBits)/area" << endl;
            }
            //now sort them
            bool used[compUnsorted->size()];
            for(unsigned i = 0; i < compUnsorted->size(); i++){
                used[i] = false;
            }
            unsigned count = 0;
            unsigned coveringDots = 0;
            double efficiency = 0;
            while(compUnsorted->size() > count){
                efficiency = 0;
                coveringDots = 0;
                BasicCompressor * bc;
                unsigned pos;
                for(unsigned i = 0; i < compUnsorted->size(); i++){
                    if(!paSorting){ //our sorting - efficiency                        
                        if(computeEfficiency(compUnsorted->at(i)) > efficiency && !used[i]){
                            bc = compUnsorted->at(i);
                            efficiency = computeEfficiency(compUnsorted->at(i));
                            pos = i;
                        }
                    }
                    else{
                        unsigned tempCoveringDots = 0;
                        for(unsigned j = 0; j < compUnsorted->at(i)->height.size(); j++){
                            tempCoveringDots += compUnsorted->at(i)->height[j];
                        }

                        if(computeCompressionRatioPA(compUnsorted->at(i)) - efficiency > 0.001 && !used[i]){
                            //compression ratio is bigger
                            bc = compUnsorted->at(i);
                            efficiency = computeCompressionRatioPA(compUnsorted->at(i));
                            coveringDots = tempCoveringDots;
                            pos = i;
                        }
                        else if(fabs(computeCompressionRatioPA(compUnsorted->at(i)) - efficiency) < 0.001 && !used[i]){
                            //has compression ratio is the same, has it more covering dots?
                            if(tempCoveringDots > coveringDots){
                                bc = compUnsorted->at(i);
                                efficiency = computeCompressionRatioPA(compUnsorted->at(i));
                                coveringDots = tempCoveringDots;
                                pos = i;
                            }
                        }

                    }
                }

                bhCompressor tempCompressor;
                tempCompressor.pointer = bc;
                tempCompressor.originalPosition = pos;
                tempCompressor.maxEfficiency = efficiency;

                cout << tempCompressor.originalPosition << " has the efficiency of ";
                cout << tempCompressor.maxEfficiency << "  and outputsize " << tempCompressor.pointer->getOutputSize() << endl;




                //now put it at the back of the compressors-vector
                compressors.push_back(tempCompressor);
                used[pos] = true;
                count++;
            }

            //now add flipflop so that a lowerBound of 0 creates a valid solution
            BasicCompressor *flipflop;
            vector<int> col(1);
            col[0]=1;
            flipflop = new BasicCompressor(bh_->getOp()->getTarget(),col);
            if(bh_->getOp()->getTarget()->isPipelined()){
                std::string targetID = bh_->getOp()->getTarget()->getID();
                if((targetID == "Virtex6") || (targetID == "Virtex7") || (targetID == "Spartan6")){
                    flipflop->areaCost = 0.5; //there are two flip-flops per LUT for Virtex 6/7 and Spartan6
                }
                else{
                    flipflop->areaCost = 1; //assume 1 for unknown device //!!!!
                }
            }
            else{
                flipflop->areaCost = 0.01; //nearly 0 for unpipelined designs
            }
            compUnsorted->push_back(flipflop);

            bhCompressor tempCompressor;
            tempCompressor.pointer = flipflop;
            tempCompressor.originalPosition = compressors.size();
            tempCompressor.maxEfficiency = 0.0;
            compressors.push_back(tempCompressor);


                  //debug compressors
            for(unsigned i = 0; i < compressors.size(); i++){
                cout << "(";
                for(unsigned j = 0; j < compressors[i].pointer->getNumberOfColumns(); j++){
                    cout << compressors[i].pointer->getColumnSize(j) << ",";
                }

                cout << ";";
                cout << compressors[i].pointer->getOutputSize() << ") with efficiency " << compressors[i].maxEfficiency << endl;
            }


            if(usePreReduction){
                preReduction(-100, 1.75);
                cout << "prereduction" << endl;
            }

            if((!passHeuristicSolution && !dontUsePA) || !dontUsePA){
                cout << "using algorithm" << endl;

                if(passMultipleSolutions){
#ifdef HAVE_SCIP
                    cout << "solutions in heuristicSolutions before first round are " << bitHeapILPCompression.heuristicSolutions.size() << endl;

                    generateAllHeu(false, 1.5);

                    bhCompressor tempCompressor = compressors[0];
                    compressors.erase(compressors.begin());
                    tempCompressor.maxEfficiency = -1.0;
                    compressors.push_back(tempCompressor);
                    cout << "solutions in heuristicSolutions are " << bitHeapILPCompression.heuristicSolutions.size() << endl;
                    generateAllHeu(false, 1.5);
#endif //HAVE_SCIP
                }
                else{
                    cout << "dontUsePA should be 0, equals " << dontUsePA << endl;
                    algorithm();            //dontUsePA == false
                }




            }
            else if(!dontUsePA || passMultipleSolutions){
#ifdef HAVE_SCIP
                cout << "!dontUsePA or passMultipleSolutions" << endl << endl;
                if(passMultipleSolutions){
                    cout << "solutions in heuristicSolutions before first round are " << bitHeapILPCompression.heuristicSolutions.size() << endl;

                    generateAllHeu(false, 1.5);

                    bhCompressor tempCompressor = compressors[0];
                    compressors.erase(compressors.begin());
                    tempCompressor.maxEfficiency = -1.0;
                    compressors.push_back(tempCompressor);
                    cout << "solutions in heuristicSolutions are " << bitHeapILPCompression.heuristicSolutions.size() << endl;
                    generateAllHeu(false, 1.5);

                }
                else if(!optimalAndPreReduction){
                    algorithm();
                    addHeuristicSolutionToILPSolver();
                }
                else{
                    //optimal, preReduction is done
                    cout << "preReduction done. We now use the optimal method" << endl;
                    printBitHeap();
                }
#endif //HAVE_SCIP

            }

            if(preSolution.size() > 0){
                //no merging just replacing because previous solution should be empty?
                solution = preSolution;
            }



        }
        else if(mode.compare("heuristic_pa") == 0){
            //real parandeh-afshar

            //setting the flags so that the ilp-solver does not "work"
            passHeuristicSolution = false;
            passMultipleSolutions = false;
            reduceILPStageCount = false;


            cout << "in heuristic_pa" << endl;

            if(paSorting){
                cout << "we sort the compressors according to the pa-efficiency: inputBits / outputBits" << endl;
            }
            else{
                cout << "we sort the compressors according to the efficiency : (inputBits - outputBits)/area" << endl;
            }

            vector<BasicCompressor *> * compUnsorted = bh_->getPossibleCompressors();
            //now sort them
            bool used[compUnsorted->size()];
            for(unsigned i = 0; i < compUnsorted->size(); i++){
                used[i] = false;
            }
            unsigned count = 0;
            double compressionRatio = 0;
            while(compUnsorted->size() > count){
                compressionRatio = 0;
                BasicCompressor * bc;
                unsigned pos = 0;
                for(unsigned i = 0; i < compUnsorted->size(); i++){
                    if(paSorting){
                        if(computeCompressionRatioPA(compUnsorted->at(i)) > (compressionRatio + 0.0001) && !used[i]){
                            bc = compUnsorted->at(i);
                            compressionRatio = computeCompressionRatioPA(compUnsorted->at(i));
                            pos = i;
                        }
                        else if(fabs(computeCompressionRatioPA(compUnsorted->at(i)) - compressionRatio) < 0.0001){
                            //now check which one has the bigger covering bits:

                            int oldCoveringDots = 0;
                            int newCoveringDots = 0;
                            for(unsigned l = 0; l < compUnsorted->at(pos)->height.size(); l++){
                                oldCoveringDots += compUnsorted->at(pos)->height[l];
                            }
                            for(unsigned l = 0; l < compUnsorted->at(i)->height.size(); l++){
                                newCoveringDots += compUnsorted->at(i)->height[l];
                            }

                            if(newCoveringDots > oldCoveringDots){
                                bc = compUnsorted->at(i);
                                compressionRatio = computeCompressionRatioPA(compUnsorted->at(i));
                                pos = i;
                            }

                        }
                    }
                    else{
                        if(computeEfficiency(compUnsorted->at(i)) > (compressionRatio + 0.0001) && !used[i]){
                            bc = compUnsorted->at(i);
                            compressionRatio = computeEfficiency(compUnsorted->at(i));
                            pos = i;
                        }
                    }

                }

                bhCompressor tempCompressor;
                tempCompressor.pointer = bc;
                tempCompressor.originalPosition = pos;
                tempCompressor.maxEfficiency = compressionRatio;

                //cout << tempCompressor.originalPosition << " has the efficiency of ";
                //cout << tempCompressor.maxEfficiency << "  and outputsize " << tempCompressor.pointer->getOutputSize() << endl;

                //now put it at the back of the compressors-vector
                compressors.push_back(tempCompressor);
                used[pos] = true;
                count++;


            }

            //now add flipflop so that a lowerBound of 0 creates a valid solution
            BasicCompressor *flipflop;
            vector<int> col(1);
            col[0]=1;
            flipflop = new BasicCompressor(bh_->getOp()->getTarget(),col);
            if(bh_->getOp()->getTarget()->isPipelined()){
                std::string targetID = bh_->getOp()->getTarget()->getID();
                if((targetID == "Virtex6") || (targetID == "Virtex7") || (targetID == "Spartan6")){
                    flipflop->areaCost = 0.5; //there are two flip-flops per LUT for Virtex 6/7 and Spartan6
                }
                else{
                    flipflop->areaCost = 1; //assume 1 for unknown device //!!!!
                }
            }
            else{
                flipflop->areaCost = 0.01; //nearly 0 for unpipelined designs
            }
            compUnsorted->push_back(flipflop);

            bhCompressor tempCompressor;
            tempCompressor.pointer = flipflop;
            tempCompressor.originalPosition = compressors.size();
            tempCompressor.maxEfficiency = 0.0;
            compressors.push_back(tempCompressor);
#ifdef HAVE_SCIP
            bitHeapILPCompression.dontAddFlipFlop = true;
#endif //HAVE_SCIP
            //sorting is the same for the moment TODO: change sorting to parandeh-afshar (sort for bitreduction)

            //debug compressors
            //debug compressors
            for(unsigned i = 0; i < compressors.size(); i++){
                cout << "(";
                for(unsigned j = 0; j < compressors[i].pointer->getNumberOfColumns(); j++){
                    cout << compressors[i].pointer->getColumnSize(j) << ",";
                }

                cout << ";";
                cout << compressors[i].pointer->getOutputSize() << ") with efficiency " << compressors[i].maxEfficiency << endl;
            }



            parandehAfshar();


        }

        //debug output:: newBits
        cout << "newbits:" << endl;
        for(unsigned i = 0; i < newBits.size(); i++){
            for(unsigned j = 0; j < newBits[i].size(); j++){

                cout << newBits[i][j] << " ";
            }
            cout << endl;
        }

        if((mode.compare("heuristic_parandeh-afshar_modified") == 0) || (mode.compare("heuristic_pa") == 0)){
            cout << "size of solution is " << solution.size() << endl;
            return 0;
        }

        //debug output:: compressors:
        /*
        for(unsigned s=0; s < solution.size(); s++)
        {
            list<pair<int,int> >::iterator it;
            for(it = solution[s].begin(); it != solution[s].end(); ++it)
            {
                cout << "applying compressor " << (*it).first << " to column " << (*it).second << " in stage " << s << endl;

            }
        }
        */

#ifdef HAVE_SCIP

        //now initialize BitHeapILPCompression
        bitHeapILPCompression.solution = solution;
        bitHeapILPCompression.newBits = newBits;        
        bitHeapILPCompression.useHeuristic = true;
        bitHeapILPCompression.zeroStages = 0;               //generally assume that there are none presolved stages
        if(reduceILPStageCount){
            setUpNewStageCountOfILP();
        }
        if(solution.size() > 0){
            //compute the area size of the already chosen compressors for better comparability
            double area = computeAreaofSolution();
            bitHeapILPCompression.preReductionAreaCost = area;
        }

        if(differentStages){
            preSolution.clear();
            preSolution = solution;         //saving the solution, maybe we did a reduction (e.g. buildSingleStages)
            solution.clear();
            cout << "noOfStages_: " << noOfStages_ << endl;
            solution.resize(noOfStages_);

            if(!useCompleteHeuristic){
                //check if the heuristic generated a solution already
                //we assume a finaladderheight of 2
                bool bitFound = false;
                bool finished = true;
                bool zerosSet = false;
  //              unsigned zeros;     //not needed because it doesn't make it faster
                for(unsigned s = 0; s < newBits.size() && finished; s++){
                    //if bitFound == true : a previous stage has unfinished bits
                    //if we find now bits as well, we are not ready

                    //if we found bits in the previous stage as well in the current Stage, we are not finished
                    bool previousStageBitsFound = bitFound;

                    for(unsigned c = 0; c < newBits[s].size(); c++){
                        if(newBits[s][c] > 2){
                            //break
                            minFixedStage = s;
                            finished = false;
                            break;
                        }
                        else if(newBits[s][c] > 0 && newBits[s][c] < 3){
                            if(previousStageBitsFound){
                                minFixedStage = s;
                                finished = false;
                                break;
                            }
                            bitFound = true;
                        }
                    }

                    if(!zerosSet && bitFound){  //set zeros at previous stage and lock that value
                        if(s > 0){
//                            zeros = s - 1;
                            zerosSet = true;
                        }
                    }
                    if(!zerosSet && !bitFound && !finished){
                        //we found a stage with more than 2 bits, but no bits in previous stage
                        if(s > 0){
//                            zeros = s - 1;
                            zerosSet = true;
                        }
                    }
                }

                if(finished){
                    //heuristic found complete solution therefore set flag useCompleteHeuristic
                    cout << "useCompleteHeuristic set to TRUE" << endl;
                    useCompleteHeuristic = true;
                }

            }

            if(useCompleteHeuristic){
                int stagesUsedByHeuristic = 0;
                //check the size of the preSolution:
                for(unsigned i = 0; i < preSolution.size(); i++){
                    if(preSolution[i].size() > 0){
                        stagesUsedByHeuristic++;
                    }
                    else{
                        break;
                    }
                }

                minFixedStage = stagesUsedByHeuristic;
                maxFixedStage = stagesUsedByHeuristic;
                cout << "set minFixedStage and maxFixedStage equal" << endl;
                bitHeapILPCompression.zeroStages = 0; //trivial - therefore problemreduction is not necessary (1 second)

                setUpILPForMoreStages(minFixedStage, true);
            }

            setUpILPForMoreStages(minFixedStage, true);


            /*
            if(buildSingleStages){
                bitHeapILPCompression.zeroStages = numberOfBuildStages;
            }
            else{
                bitHeapILPCompression.zeroStages = 0;
            }
            bitHeapILPCompression.getExternalStageCount = true;
            bitHeapILPCompression.noOfStages_ = minFixedStage;
            bitHeapILPCompression.useFixedStageCount = true;
            bitHeapILPCompression.solution.clear();
            bitHeapILPCompression.solution.resize(noOfStages_);

            */
        }


        cout << "initialisation of bitHeapILPCompression finished" << endl;
        bitHeapILPCompression.generateProblem();

        if(passHeuristicSolution){
            int count = bitHeapILPCompression.passHeuristicSolutions();

            cout << "there were " << count << " heuristic solutions passed" << endl;
        }
#endif //HAVE_SCIP
        return 0;
    }


    void BitHeapHeuristicCompression::algorithm(){
        cout << "in algorithm__________________" << endl;
        cout << "compresssors.size() = " << compressors.size() << endl;
        //if debugLoop == true, there are only debugMax steps
        bool debugLoop = false;
        int debugMax = 100;
        int debugCount = 0;

        unsigned numberOfStages = newBits.size() - 1;

        if(buildSingleStages && (numberOfBuildStages > 0) ){
            numberOfStages = numberOfBuildStages;
        }

        //find a compressor which fits the best and use it.
        for(unsigned s = 0; s < numberOfStages && !(debugLoop && debugCount >= debugMax); s++){
            debugCount++;
            bool found = true;
            while(found == true && !(debugLoop && debugCount >= debugMax)){
                debugCount++;

                if(useSmootherCompressorPlacing){
                    bool smootherUsed;
                    smootherUsed = smootherCompressorPlacing(s, 1.75);

                    if(smootherUsed){   //show bitheap only if it is used to see changes
                        cout << "BitHeap after using a smoother compressor placing in stage " << s << endl;
                        printBitHeap();
                    }
                }

                found = false;
                double achievedEfficiencyBest = -1.0;
                unsigned compressor = 0;
                unsigned column = 0;
                for(unsigned i = 0; i < compressors.size()  && !(debugLoop && debugCount >= debugMax); i++){
                    debugCount++;
                    //first get maximal efficiency of this compressor.
                    //if its lower than the efficiency already found, then stop
                    double maxEfficiency = compressors[i].maxEfficiency;
                    if((maxEfficiency - 0.0001) < achievedEfficiencyBest || (maxEfficiency + 0.0001) < lowerBounds[s]){    //accuracy
                        break;
                    }

                    unsigned maxWidth = newBits[s].size() - (compressors[i].pointer->getOutputSize() - 1);
                    bool used[maxWidth];        //used to check whether we already tried the compressor in this column
                    for(unsigned k = 0; k < maxWidth; k++){
                        used[k] = false;
                    }

                    unsigned columnsAlreadyChecked = 0;
                    bool foundCompressor = false;
                    while((columnsAlreadyChecked < maxWidth) && !(foundCompressor && (fabs(maxEfficiency - achievedEfficiencyBest) < 0.0001)) ){ //maxEfficiency accuracy
                        //get max stage
                        unsigned currentMaxColumn = 0;
                        int currentSize = 0;
                        for(unsigned c = 0; c < maxWidth; c++){
                            //cout << "s = " << s << " c = " << c << " and newBits = " << newBits[s][c] << "  ---old- maxWidth = " << currentSize << " currentMaxColumn = " << currentMaxColumn<< endl;
                            if(!used[c] && newBits[s][c] > currentSize){
                                currentMaxColumn = c;
                                currentSize = newBits[s][c];
                            }
                        }
                        used[currentMaxColumn] = true;
                        //cout << "final maxColumn = " << currentMaxColumn <<" with width " << currentSize << " for compressor " << i << endl;

                        double achievedEfficiencyCurrent = compEffBitHeap(s, currentMaxColumn, i);
                        //cout << "achieved efficiency is " << achievedEfficiencyCurrent << endl;

                        //check if it is necessary
                        bool necessary = true;
                        //BUG: if we build a solution, then the last "2-x" bits are allways covered with flipflops
                        //if(i != compressors.size() - 1){        //if i is the last one than it is a flipflop and we need it to build a solution
                        if(!fabs(lowerBounds[s]) < 0.001){
                            necessary = false;
                            for(unsigned k = 0; k < currentMaxColumn + compressors[i].pointer->getNumberOfColumns(); k++){
                                unsigned sum = 0;
                                for(unsigned l = s; l < newBits.size(); l++){
                                    sum += newBits[l][k];
                                }
                                if(sum > 2){
                                    necessary = true;
                                    break;
                                }
                            }

                        }

                        if(achievedEfficiencyCurrent > (achievedEfficiencyBest + 0.0001) && achievedEfficiencyCurrent > (lowerBounds[s] - 0.0001) && necessary){       //accuracy !!
                            achievedEfficiencyBest = achievedEfficiencyCurrent;
                            compressor = i;
                            column = currentMaxColumn;
                            foundCompressor = true;
                            found = true;
                        }
                        columnsAlreadyChecked++;
                    }
                }

                if(found){
                    //we found a compressor and use him now.

                    //cout << "using compressor " <<  compressor  << " in column ";
                    //cout << column << " with efficiency " << achievedEfficiencyBest << endl;

                    useCompressor(s, column, compressor);
                    //printBitHeap();

                }

            }


            //now we are done with this stage. if we need to generate a solution, then we
            //have to put the remaining bits into the next stage.
            //if we reached the stage were we can use the two-bit-adder, we are done
            if(fabs(lowerBounds[s]) < 0.0001 ){
                /*
                cout << "generate solution - therefore put remaining bits into the next stage" << endl;
                for(unsigned j = 0; j < newBits[s].size(); j++){
                    if(newBits[s][j] > 0){
                        int temp = newBits[s][j];
                        newBits[s + 1][j] += newBits[s][j];
                        newBits[s][j] = 0;

                        //assuming: flip-flop is not in compressors, but after that at the back of bh_->possibleCompressors
                        //which is compressors.size
                        for(int k = 0; k < temp; k++){
                            solution[s].push_back(pair<int,int>((compressors.size()),j));

                        }

                    }
                }

                */
                //check exit-condition

                bool twoBitAdderReached = true;
                for(unsigned a = 0; a < newBits.size(); a++){
                    if(a != s + 1){
                        //check if those stages are empty
                        for(unsigned b = 0; b < newBits[a].size(); b++){
                            if(newBits[a][b] > 0){
                                twoBitAdderReached = false;
                                break;
                            }
                        }
                    }
                    else{
                        //check if s + 1 stage is the final one (only has a maximum bitcount of 2
                        for(unsigned b = 0; b < newBits[a].size(); b++){
                            if(newBits[a][b] > 2){
                                twoBitAdderReached = false;
                                break;
                            }
                        }


                    }
                }

                if(twoBitAdderReached){
                    //cout << endl << "twoBitAdderStage reached" << endl;
                    //printBitHeap();
                    break;
                }
            }

        }

        /*
        //workaround
        if(passHeuristicSolution){
            bitHeapILPCompression.heuristicSolutions.push_back(solution);
            solution.clear();
            solution.resize(noOfStages_);
            cout << endl << "bitHeap number " << (bitHeapILPCompression.heuristicSolutions.size() - 1 ) << endl;
            printBitHeap();

            newBits = originalNewBits;
        }

        */



        if(useHoles){
            //now delete the holes in the first stages, because they can't be used there
            for(unsigned i = 0; i < newBits[0].size(); i++){
                if(newBits[0][i] < 0){
                    newBits[0][i] = 0;
                }
            }

            //cout << "newBits after deleting holes in stage 0" << endl;
            //printBitHeap();
        }





    }

    void BitHeapHeuristicCompression::parandehAfshar(){

        cout << "in parandeh-Afshar" << endl;

        //if debugLoop == true, there are only debugMax steps
        bool debugLoop = false;
        bool exit = false;
        int debugMax = 100;
        int debugCount = 0;


        for(unsigned s = 0; s < (newBits.size() - 1) && !exit && !(debugLoop && debugCount >= debugMax); s++){
            //cout << "stage = " << s << endl;
            debugCount++;
            bool found = true;
            while(found && !(debugLoop && debugCount >= debugMax)){
                debugCount++;
                found = false;
                double achievedEfficiencyBest = -1.0;
                unsigned compressor = 0;
                unsigned column = 0;

                bool used[newBits[s].size()];        //used to check whether we already tried the compressor in this column
                for(unsigned k = 0; k < newBits[s].size(); k++){
                    used[k] = false;
                }

                unsigned columnsAlreadyChecked = 0;

                while((columnsAlreadyChecked < newBits[s].size()) && !found ){ //if we found a compressor in the current column, then we dont search the next column

                    //cout << "now find compressor" << endl;
                    unsigned currentMaxColumn = 0;
                    int currentSize = 0;
                    for(unsigned c = 0; c < newBits[s].size(); c++){
                        //cout << "s = " << s << " c = " << c << " and newBits = " << newBits[s][c] << "  ---old- maxWidth = " << currentSize << " currentMaxColumn = " << currentMaxColumn<< endl;
                        if(!used[c] && newBits[s][c] > currentSize){
                            currentMaxColumn = c;
                            currentSize = newBits[s][c];
                        }
                    }
                    used[currentMaxColumn] = true;
                    columnsAlreadyChecked++;

                    for(unsigned i = 0; i < compressors.size() && !(debugLoop && debugCount >= debugMax); i++){
                        double achievedEfficiencyCurrentLeft = compEffBitHeap(s, currentMaxColumn, i);

                        double achievedEfficiencyCurrentRight = -1;
                        int rightStartPoint = currentMaxColumn - (compressors[i].pointer->getNumberOfColumns() - 1);

                        if(rightStartPoint > 0){
                            achievedEfficiencyCurrentRight = compEffBitHeap(s, (unsigned) rightStartPoint, i);
                        }
                        //cout << "column is " << currentMaxColumn << " and compressor is " << i << endl;
                        //cout << " right Eff = " << achievedEfficiencyCurrentRight << " and left Eff = " << achievedEfficiencyCurrentLeft << endl;

                        if(achievedEfficiencyCurrentLeft > 0.0001 && achievedEfficiencyCurrentRight <= achievedEfficiencyCurrentLeft + 0.0001){
                            //normal (left) search is successful - prefer it if left and right search has equal efficiency

                            if(achievedEfficiencyBest + 0.0001 < achievedEfficiencyCurrentLeft){
                                achievedEfficiencyBest = achievedEfficiencyCurrentLeft;
                                compressor = i;
                                column = currentMaxColumn;
                            }
                            found = true;
                        }
                        else if(achievedEfficiencyCurrentRight > 0.0001 && achievedEfficiencyCurrentRight > achievedEfficiencyCurrentLeft + 0.0001){
                            //right search is successful

                            if(achievedEfficiencyBest + 0.0001 < achievedEfficiencyCurrentRight){
                                achievedEfficiencyBest = achievedEfficiencyCurrentRight;
                                compressor = i;
                                column = rightStartPoint;
                            }
                            found = true;
                        }
                        //cout << "current best compressor = " << compressor << " and column  = " << column <<  " with efficiency " << achievedEfficiencyBest << endl;
                    }

                }

                //now use the compressor

                if(found){
                    useCompressor(s, column, compressor);
                    //cout << "used compressor "<< compressor<< " in column "<<column<< " and stage " << s << endl << endl;
                    //printBitHeap();
                    //cout << endl;
                }
                //check if we got the exit condition
                exit = true;
                for(unsigned k = 0; k < newBits[newBits.size() - 1].size(); k++){       //go through all columns
                    unsigned sum = 0;
                    for(unsigned t = s; t < newBits.size(); t++){                       //go through all stages starting with s
                        if(newBits[t].size() > k){
                            //cout << "checking " << t << " and " << k << " with value " << newBits[t][k] << endl;
                            if(newBits[t][k] > 0){
                               sum += newBits[t][k];
                            }
                        }
                    }

                    if(sum > 2){
                        exit = false;
                        break;
                    }
                }

                if(exit){
                    //cout << endl << "we reached the end___________________________________" << endl;
                    break;
                }


            }


            //now we are done with this stage. if we need to generate a solution, then we
            //have to put the remaining bits into the next stage.
            //if we reached the stage were we can use the two-bit-adder, we are done
            if(generateSolution){
                cout << "generate solution - therefore put remaining bits into the next stage" << endl;
                for(unsigned j = 0; j < newBits[s].size(); j++){
                    if(newBits[s][j] > 0){
                        int temp = newBits[s][j];
                        newBits[s + 1][j] += newBits[s][j];
                        newBits[s][j] = 0;

                        //assuming: flip-flop is not in compressors, but after that at the back of bh_->possibleCompressors
                        //which is compressors.size
                        for(int k = 0; k < temp; k++){
                            solution[s].push_back(pair<int,int>((compressors.size() - 1),j));

                        }

                    }
                }

                //check exit-condition

                bool twoBitAdderReached = true;
                for(unsigned a = 0; a < newBits.size(); a++){
                    if(a != s + 1){
                        //check if those stages are empty
                        for(unsigned b = 0; b < newBits[a].size(); b++){
                            if(newBits[a][b] > 0){
                                twoBitAdderReached = false;
                                break;
                            }
                        }
                    }
                    else{
                        //check if s + 1 stage is the final one (only has a maximum bitcount of 2
                        for(unsigned b = 0; b < newBits[a].size(); b++){
                            if(newBits[a][b] > 2){
                                twoBitAdderReached = false;
                                break;
                            }
                        }


                    }
                }

                if(twoBitAdderReached){
                    //cout << endl << "twoBitAdderStage reached" << endl;
                    //printBitHeap();
                    break;
                }

                if(!twoBitAdderReached && !found && s == newBits.size() - 1){
                    //add new stage;
                    vector<int> zeroVector;
                    zeroVector.resize(newBits[s].size() + 2);
                    newBits.push_back(zeroVector);

                    cout << "added a new stage because the parandeh-afshar algorithm wasn't able to find a solution" << endl;
                }


            }

            cout << "after stage " << s << " the bitheap looks like" << endl;
            printBitHeap();

            //if we reached the last stage, and there are still more than 3 bits in any column of the last stage, add an additional stage
            if(s == newBits.size() - 2){
                cout << "last stage" << endl;
                bool addNewStage = false;
                for(unsigned k = 0; k < newBits[s].size(); k++){
                    if(newBits[s + 1][k] > 2){
                        addNewStage = true;
                        break;
                    }
                }

                if(addNewStage){
                    cout << "we weren't able to compute a solution within " << s << " stages. Therefore we need an additional stage." << endl;
                    vector<int> tempList;
                    tempList.resize(newBits[s].size() + 2);
                    for(unsigned k = 0; k < tempList.size(); k++){
                        tempList[k] = 0;
                    }
                    cout << "old newBitsSize  = " << newBits.size();
                    cout << "; noOfStages_ is << " << noOfStages_ << " old" << endl;
                    newBits.push_back(tempList);
                    solution.resize(newBits.size());
                    noOfStages_++;
#ifdef HAVE_SCIP
                    bitHeapILPCompression.solution.resize(newBits.size());                    
                    bitHeapILPCompression.noOfStages_ = noOfStages_;
                    bitHeapILPCompression.getExternalStageCount = true;
#endif //HAVE_SCIP
                    cout << "NEW newBitsSize is " << newBits.size() << " and noOfStages_ is " << noOfStages_ << endl;
                }
            }




        }



    }


    //this reduces the problem so that a smaller problem is passed to the ilp-solver
    void BitHeapHeuristicCompression::preReduction(unsigned minHeight, double minEff){
        minimumHeightReduction = minHeight;
        //minimumEffReduction = minEff;
        lowerBound = minEff;
        cout << "in prereduction " << endl << endl << endl;

        //disabling generate solution by not putting bits into the next stage with ffs
        bool rememberGenerateSolution = false;
        if(generateSolution){
            generateSolution = false;
            rememberGenerateSolution = true;

        }

        //the real checking if the conditions are met is done in compEffBitHeap()        
        algorithm();

        cout << "preReduction done. The problem is now reduced to the following bitheap:" << endl;
        printBitHeap();

        //enabling generateSolution
        if(rememberGenerateSolution){
            generateSolution = true;
        }

        originalNewBits = newBits;
        preSolution = solution;
        solution.clear();
        solution.resize(noOfStages_);
        usePreReduction = false;



    }


    //this function places and (6,0,6;5) compressor in every column starting in column 0.
    //this leads to a more smoother outputBits distribution of 5 in every column (except the start & end). On the other hand the inputbits are 6,12,12,12,12,.... 12,6
    //if it reaches the maximum bits of the bitheap and there is at least one compressor placed, it will start at column 0 again.
    //it will be only placed, if the efficiency is reached.
    bool BitHeapHeuristicCompression::smootherCompressorPlacing(unsigned s, double efficiency){
        //assumption: (6,0,6:5) is first compressor
        bool atLeastOnceUsed = false;

        bool found = true;

        while(found){
            found = false;
            for(unsigned c = 0; c < newBits[s].size() - 2; c++){
                if((compEffBitHeap(s, c, 0) + 0.00001) >= efficiency){
                    found = true;
                    atLeastOnceUsed = true;
                    useCompressor(s, c, 0);
                }
            }
        }

        return atLeastOnceUsed;
    }


    //this function generates all Heuristic solutions by switching the priority of compressors with the same efficiency
    //if allCombinations == true, then it will compute all possible rankings and generate solutions
    //otherwise it will only switch the compressors with the given parameter efficiency
    int BitHeapHeuristicCompression::generateAllHeu(bool allCombinations, double efficiency){

        if(!allCombinations){
            unsigned count = 0;
            unsigned firstOccurance = 0;
            bool foundCompressor = false;
            for(unsigned i = 0; i < compressors.size(); i++){
                if(fabs(compressors[i].maxEfficiency - efficiency) < 0.001){
                    count++;
                    if(!foundCompressor){
                        firstOccurance = i;
                    }
                    foundCompressor = true;

                }



            }
            cout << count << " found for efficiency " << efficiency << endl;
            cout << "first occurance is " << firstOccurance << endl;

            //storing compressors and originalCompressorPosition;
            vector<bhCompressor> unmodifiedCompressors;
            //vector<unsigned> unmodifiedPositions;


            for(unsigned i = 0; i < compressors.size(); i++){
                unmodifiedCompressors.push_back(compressors[i]);
            }


            int combinations[count];
            for(unsigned i = 0; i < count; i++){
                combinations[i] = firstOccurance + i;
            }
            int times = 0;

            do{
                algorithm();
                addHeuristicSolutionToILPSolver();
                times++;

                for(unsigned i = 0; i < count; i++){
                    compressors[firstOccurance + i] = unmodifiedCompressors[combinations[i]];
                    //originalCompressorPosition[firstOccurance + i] = unmodifiedPositions[combinations[i]];
                }

            } while (std::next_permutation(combinations, combinations + count));


            //cout << "we were " << times <<" times in the do-while loop" << endl;
        }
        else{
            algorithm();
            addHeuristicSolutionToILPSolver();
        }




        return 0;
    }

    //this adds the solution to the heuristicSolutions in the bitHeapILPCompression
    //and resets the problem
    void BitHeapHeuristicCompression::addHeuristicSolutionToILPSolver(){
#ifdef HAVE_SCIP
        bitHeapILPCompression.heuristicSolutions.push_back(solution);
        solution.clear();
        solution.resize(noOfStages_);
        cout << endl << "bitHeap number " << (bitHeapILPCompression.heuristicSolutions.size() - 1 ) << endl;
        printBitHeap();

        newBits = originalNewBits;
#endif //HAVE_SCIP
    }



    //this is the standard efficiency E = delta / k; delta is Bits which are reduced and k is the size of the compressor in Luts
    double BitHeapHeuristicCompression::computeEfficiency(BasicCompressor* comp){
        int inputBits = 0;
        int outputBits = 0;

        for(unsigned i = 0; i < comp->height.size(); i++){
            inputBits += comp->height[i];
        }
        for(unsigned i = 0; i < comp->outputs.size(); i++){
            outputBits += comp->outputs[i];
        }

        int reducedBits = inputBits - outputBits;

        return  (double) reducedBits / comp->areaCost;
    }



    //ranking of Parandeh-Afshar descriped in his paper "Efficient Synthesis of Compressor Trees 2008"
    double BitHeapHeuristicCompression::computeCompressionRatioPA(BasicCompressor* comp){
        int inputBits = 0;
        int outputBits = 0;

        for(unsigned i = 0; i < comp->height.size(); i++){
            inputBits += comp->height[i];
        }
        for(unsigned i = 0; i < comp->outputs.size(); i++){
            outputBits += comp->outputs[i];
        }


        return (double) inputBits / outputBits;
    }


    //conditions met disabled!!

    //this is the standardEfficiency E you would achieve if the compressor is used in stage s column c
    //the conditions for compressors from parandeh-afshar are checked as well
    double BitHeapHeuristicCompression::compEffBitHeap(unsigned s, unsigned c, unsigned compPos){
        int sum = 0;
        bool conditionsMet = true;
//        bool preReductionConditionsMet = true;

        //make sure the width of the current stage is large enough
        if(newBits[s].size() - 1 < c + (compressors[compPos].pointer->getNumberOfColumns() - 1)){
            //cout << "we got over the edge of the current stage " << s << " at column " << c << " and original compPos " << compressors[compPos].originalPosition << endl;
        }

        for(unsigned i = 0; i < compressors[compPos].pointer->getNumberOfColumns(); i++){

            if(i == 0 && newBits[s][c] == 1){
                //the input of the last bit is only connected to the lowest output
                conditionsMet = true;           //TODO: check whether we need conditionsMet
            }

            if(i + c < newBits[s].size()){  //we only add something to "sum" if there is a column

                if(newBits[s][c + i] > 0){
                    //we do not count "holes"
                    if((unsigned) newBits[s][c + i] >= compressors[compPos].pointer->getColumnSize(i)){
                        sum += compressors[compPos].pointer->getColumnSize(i);
                    }
                    else{
                        sum += newBits[s][c + i];
                    }
                }

/*
                if(((unsigned) newBits[s][c + i]) - compressors[compPos].pointer->getColumnSize(i) < (int) minimumHeightReduction){
                    preReductionConditionsMet = false;      //deactivated
                }
*/
            }


        }

        //now in sum are the bits which are covered with this compressor
        sum -= compressors[compPos].pointer->getOutputSize();


        double eff = ((double) sum) / compressors[compPos].pointer->areaCost;


        if(eff <= 0.0001 && !generateSolution){   //not shure about accuracy - normally <= 0.0
            //conditionsMet = false;                //TODO: clean up
        }
/*
        if(usePreReduction && (!preReductionConditionsMet || eff < minimumEffReduction - 0.0001)){
            return -1.0;
        }
*/
        if(!conditionsMet){
            return -1.0;
        }
        else{
            return eff;
        }
    }


    void BitHeapHeuristicCompression::useCompressor(unsigned s, unsigned column, unsigned newCompPos){

        //make changes to newBits
        for(unsigned i = 0; i < compressors[newCompPos].pointer->getNumberOfColumns(); i++){
            newBits[s][column + i] -= compressors[newCompPos].pointer->getColumnSize(i);
            if(!useHoles){
                if(newBits[s][column + i] < 0){
                    newBits[s][column + i] = 0;
                }
            }

        }
        for(unsigned i = 0; i < (unsigned) compressors[newCompPos].pointer->getOutputSize(); i++){
            newBits[s + 1][column + i]++;
        }

        //add compressor to solution
        solution[s].push_back(pair<int,int>(compressors[newCompPos].originalPosition,column));
    }

    int BitHeapHeuristicCompression::printBitHeap(){
        for(unsigned i = 0; i < newBits.size(); i++){
            for(unsigned j = 0; j < newBits[i].size(); j++){
                cout << newBits[i][j] << " ";
            }
            cout << endl;
        }

        return 0;
    }

    //this function reduces the stages s of the solution. this is only possible if at least one presolution is passed
    //if useMaxHeuristicStageCount is false, the smallest s of all heuristic solutions is the s for the ILP-problem and
    //all presolutions with more stages are being deleted
    //otherwise the s in the ILP-problem is the s of the presolution, which needs the most stages
    void BitHeapHeuristicCompression::setUpNewStageCountOfILP(){
#if HAVE_SCIP
        unsigned maxS = 0;

        if(!useMaxHeuristicStageCount){
            maxS = 1000;
        }

        unsigned sOfSolution[bitHeapILPCompression.heuristicSolutions.size()];

        for(unsigned i = 0; i < bitHeapILPCompression.heuristicSolutions.size(); i++){
            unsigned tempS = 0;

            for(unsigned s = 0; s < bitHeapILPCompression.heuristicSolutions[i].size(); s++){
                if(!bitHeapILPCompression.heuristicSolutions[i][s].empty()){
                    tempS = s;
                }
            }




            sOfSolution[i] = tempS;

            if(useMaxHeuristicStageCount){      //find maximal stage
                if(tempS > maxS){
                    maxS = tempS;
                }
            }
            else{                               //find minimal stage
                if(tempS < maxS){
                    maxS = tempS;
                }
            }
        }

        //now delete all solutions, which are bigger than maxS and resize all others
        // but only if !useMaxHeuristicStageCount
        if(!useMaxHeuristicStageCount){
            for(int i = (int) (bitHeapILPCompression.heuristicSolutions.size() - 1); i >= 0; i--){
                if(sOfSolution[i] != maxS){
                    bitHeapILPCompression.heuristicSolutions.erase(bitHeapILPCompression.heuristicSolutions.begin() + i);
                }
            }
        }

        //setting up noOfStages in ILP

        bitHeapILPCompression.getExternalStageCount = true;
        bitHeapILPCompression.noOfStages_ = (maxS + 2); //two - 1 because inthe next stage(the last stage) is no compressor and one because we start at counting with 0


        cout << "new numberOfStages_ is " << (maxS + 2) << endl;
        if(useMaxHeuristicStageCount){
            cout << "numberOfStages_ was reduced so that all heuristic solutions are still valid" << endl;
        }
        else{
            cout << "numberOfstages_ was reduced as much as possible. heuristic solutions with more stages could have been deleted" << endl;
        }
#endif //HAVE_SCP
    }


    double BitHeapHeuristicCompression::computeAreaofSolution(){

        vector<BasicCompressor *> * comps = bh_->getPossibleCompressors();
        double areaSize = 0.0;
        double flipFlopCost = 0.0;
        if(bh_->getOp()->getTarget()->isPipelined())        {
            std::string targetID = bh_->getOp()->getTarget()->getID();
            if((targetID == "Virtex6") || (targetID == "Virtex7") || (targetID == "Spartan6")){
                flipFlopCost = 0.5; //there are two flip-flops per LUT for Virtex 6/7 and Spartan6
            }
            else{
                flipFlopCost = 1.0; //assume 1 for unknown device //!!!!
            }
        }
        else{
            flipFlopCost = 0.01; //nearly 0 for unpipelined designs
        }
        for(unsigned i = 0; i < solution.size(); i++){
            cout << "i = " << i << endl;
            list<pair<int, int> >:: iterator it;

            for(it = solution[i].begin(); it != solution[i].end(); it++){
                if((*it).first == (int) compressors.size()){
                    areaSize += flipFlopCost; //cost of flipflop
                }
                else{
                    areaSize += comps->at((*it).first)->areaCost;
                }

                //areaSize += compressors[(*it).first].areaCost;
            }
        }
        return areaSize;
    }

    //now just functions for the ilp module

    int BitHeapHeuristicCompression::writeProblem(std::string filename){
#ifdef HAVE_SCIP
        bitHeapILPCompression.writeProblem(filename);
#endif //HAVE_SCIP
        return 0;
    }

    void BitHeapHeuristicCompression::setUpILPForMoreStages(unsigned stages, bool firstOne){
#ifdef HAVE_SCIP
        cout << "in setupILPForMoreStages with stages = " << stages << endl;
        bitHeapILPCompression.solution.clear();
        bitHeapILPCompression.solution.resize(stages);
        if(!firstOne){
            bitHeapILPCompression.cleanUp();
        }
        unsigned zeros = 0;
        for(unsigned i = 0; i < sizeof(lowerBounds) / sizeof(lowerBounds[0]); i++){
           if(fabs(lowerBounds[i]) < 0.001){
               zeros++;
           }
        }

        //zeros does not result in a much faster ilp-solving process
        if(!useCompleteHeuristic ){
/*            if(!getExternalZero){
                bitHeapILPCompression.zeroStages = zeros;
                cout << "zeros added" << endl;
            }
            else{
                bitHeapILPCompression.zeroStages = exZero;
                cout << "external zeros added" << endl;
            }
*/
        }


        bitHeapILPCompression.getExternalStageCount = true;
        bitHeapILPCompression.useFixedStageCount = true;
        bitHeapILPCompression.useHeuristic = true;
        bitHeapILPCompression.dontAddFlipFlop = true; //!firstOne;  //first one: add flipflop otherwise not
        bitHeapILPCompression.noOfStages_ = stages;

        cout << "setting up done" << endl;
#endif //HAVE_SCIP
    }

    int BitHeapHeuristicCompression::solve(){
        cout << "in solve()" << endl;
        if((mode.compare("heuristic_parandeh-afshar_modified") == 0) || (mode.compare("heuristic_pa") == 0)) {
            cout << "we shouldn't use scip therefore we should be done" << endl;
            return 0;
        }
#ifdef HAVE_SCIP
        if(differentStages){
            cout << "differentStages == true" << endl;
        }
        else{
            cout << "differentStages == false" << endl;
        }
        if(differentStages){
            printBitHeap();
            cout << "minFixedStage = " << minFixedStage << " and maxFixedStage = " << maxFixedStage << endl;
            bool takeFirstSolution = false; //we also check the next stages for a better solution
            for(unsigned s = minFixedStage; s <= maxFixedStage && !takeFirstSolution; s++){

                if(s != minFixedStage){
                    //setting up new bitHeapILPCompressioin
                    //bitHeapILPCompression = BitHeapILPCompression(bh_);
  //                  vector<list<pair<int,int> > > emptySolution;
    //                bitHeapILPCompression.solution = emptySolution;
                    cout << "calling cleanUp" << endl;
/*
                    bitHeapILPCompression.solution.clear();
                    bitHeapILPCompression.solution.resize(noOfStages_);
                    bitHeapILPCompression.cleanUp();
                    if(buildSingleStages){
                        bitHeapILPCompression.zeroStages = numberOfBuildStages;
                    }
                    else{
                        bitHeapILPCompression.zeroStages = 0;
                    }
                    bitHeapILPCompression.newBits = newBits;
                    bitHeapILPCompression.useHeuristic = true;
                    bitHeapILPCompression.dontAddFlipFlop = true;
                    bitHeapILPCompression.noOfStages_ = s;
                    bitHeapILPCompression.getExternalStageCount = true;
                    bitHeapILPCompression.useFixedStageCount = true;
                    bitHeapILPCompression.generateProblem();

*/
                    setUpILPForMoreStages(s, false);
                    bitHeapILPCompression.generateProblem();
                }

                bitHeapILPCompression.solve();
                cout << "solving done" << endl;
                if(!bitHeapILPCompression.infeasible){
                    cout << "solution is not infeasible" << endl;
                    if(bitHeapILPCompression.costOfCurrentSolution < minSliceCount){
                        minSliceCount = bitHeapILPCompression.costOfCurrentSolution;
                        solution.clear();
                        solution = bitHeapILPCompression.solution;
                        takeFirstSolution = true;      //uncomment, if you want to take the first solution, ilp finds (no restarts with a higher stagecount)
                    }
                }

                //bitHeapILPCompression.cleanUp();
            }

            if(bitHeapILPCompression.infeasible){       //if we have no solution after we went all the stages, exit
                exit(-1);
            }
            /*
            cout << endl << "before merging, solution" << endl;
            for(int j = 0; j < solution.size(); j++){
                list<pair<int,int> >:: iterator it;
                for(it = solution[j].begin(); it != solution[j].end(); it++){
                    cout << "applying compressor " << (*it).first << " to column " << (*it).second << " in stage " << j << endl;
                }

            }

            cout << endl << endl;

            cout << endl << "before merging, presolution" << endl;
            for(int j = 0; j < preSolution.size(); j++){
                list<pair<int,int> >:: iterator it;
                for(it = preSolution[j].begin(); it != preSolution[j].end(); it++){
                    cout << "applying compressor " << (*it).first << " to column " << (*it).second << " in stage " << j << endl;
                }

            }

            cout << endl << endl;
            */

            if(preSolution.size() > solution.size()){
                solution.resize(preSolution.size());
            }
            for(unsigned s = 0; s < preSolution.size(); s++){   //merge solution and presolution at the end

                solution[s].splice(solution[s].end(), preSolution[s]);
            }


            /*
            cout << "after SCIP" << endl;
            for(int j = 0; j < solution.size(); j++){
                list<pair<int,int> >:: iterator it;
                for(it = solution[j].begin(); it != solution[j].end(); it++){
                    cout << "applying compressor " << (*it).first << " to column " << (*it).second << " in stage " << j << endl;
                }

            }
            */
        }
        else{
            bitHeapILPCompression.solve();
            //copying solution from ILPCompression
            solution = bitHeapILPCompression.solution;
        }

        buildVariableCompressorSolution();

        //to fix the stagecount: delete zero stages

        unsigned tempMaxStage = solution.size();
        for(unsigned i = 0; i < solution.size(); i++){
            if(!solution[i].empty()){
                tempMaxStage = i;
            }
        }
        if(tempMaxStage + 1 < solution.size()){
            //first check if stages are really empty:
            cout << "we are deleting stages. initial stagesize is " << solution.size() << endl;
            unsigned leastEmptyStage = solution.size();
            for(unsigned i = solution.size() - 1; i >= tempMaxStage + 1; i--){
                if(solution[i].size() == 0){
                    leastEmptyStage = i;
                }
                else{
                    break;
                }
            }
            cout << "leastEmptyStage " << leastEmptyStage << endl;
            solution.resize(leastEmptyStage);          //cut empty stages off
            cout << "cut off of unused stages" << endl;
        }



        cout << "done" << endl;
#endif //HAVE_SCIP
        return 0;
    }

    //this function goes through the solution, finds variableBasicCompressors, deletes them and
    //fills the variable compressor solution
    void BitHeapHeuristicCompression::buildVariableCompressorSolution(){
        int offset = compressors.size();
        //right now only RCA
        int rcaLow = 0;			//change these three if necessary
        int rcaMid = 1;			//value is the position in ilpCompressions variableBCompressors
        int rcaHigh = 2;
        rcaLow += offset;
        rcaMid += offset;
        rcaHigh += offset;

        varCompSolution.resize(solution.size());

        for(unsigned s = 0; s < solution.size(); s++){

            bool found = true;
            while(found){
                found = false;
                variableCompressor tVarComp;
                tVarComp.type = 0;			//type 0 == RCA
                tVarComp.startCompressorWidth = 1;
                tVarComp.middleCompressorWidth = 0;
                tVarComp.endCompressorWidth = 1;

                //now search for rca low basicVarCompressor
                bool lowFound = false;
                int currentColumn = 0;
                std::list<pair<int, int> >::iterator it;
                for(it = solution[s].begin(); it != solution[s].end(); it++){
                    if((*it).first == rcaLow){
                        //we found the start of a RCA
                        tVarComp.column = (*it).second;
                        currentColumn = (*it).second + 1;		//search for middle or high varBasicComp starting with next column
                        solution[s].erase(it);
                        lowFound = true;
                        break;
                    }
                }

                if(lowFound){
                    bool highFound = false;
                    while(!highFound){
                        for(it = solution[s].begin(); it != solution[s].end(); it++){
                            if((*it).second == currentColumn){
                                if((*it).first == rcaMid){		//we found a suitable middle Compressor
                                    tVarComp.middleCompressorWidth++;
                                    currentColumn++;
                                    solution[s].erase(it);
                                    break;				//start again to search
                                }
                                else if((*it).first == rcaHigh){	//we found the high compressor
                                    highFound = true;
                                    solution[s].erase(it);
                                    break;
                                }
                            }
                        }
                    }

                    //if we found a RCA compressor, add it to the varCompSolution
                    if(highFound){
                        varCompSolution[s].push_back(tVarComp);
                        found = true;
                    }
                    else{
                        cout << "=============" << endl;
                        cout << "something went horrible wrong. a RCA compressor is not closed with a high compressor" << endl;
                        cout << "=============" << endl;
                    }
                }

            }

            //place here the search for other compressors with variable width

        }




        //debug
        for(unsigned s = 0; s < varCompSolution.size(); s++){
            for(list<variableCompressor>::iterator it = varCompSolution[s].begin(); it != varCompSolution[s].end(); it++){
                cout << "variable Compressor no. " << (*it).type << " starting at column " << (*it).column << " with middleSize of " << (*it).middleCompressorWidth << endl;
            }

        }
    }

    void BitHeapHeuristicCompression::printLowerBounds(){
        cout << "PA-vector is (";
        for(unsigned i = 0; i < (sizeof(lowerBounds) / sizeof(lowerBounds[0])); i++){
            if(lowerBounds[i] > 4.1){
                cout << "inf";
                break;
            }
            else{
                cout << lowerBounds[i];
            }
            if(i + 1 != sizeof(lowerBounds) / sizeof(lowerBounds[0])){
                cout << "; ";
            }
        }
        cout << ")" << endl;
    }

    int BitHeapHeuristicCompression::plotSolution(){
#ifdef HAVE_SCIP
        bitHeapILPCompression.plotSolution();
#endif //HAVE_SCIP
        return 0;
    }

    void BitHeapHeuristicCompression::setLowerBounds(string s){
        //parses the string efficiencyPerStage and writes the values to lowerBounds
        unsigned int sizeOfLowerBounds = (sizeof(lowerBounds) / sizeof(lowerBounds[0]));
        if(getLowerBoundsFromBitHeap){

            if(s.compare("") == 0){
                //efficiencyPerStage wasn't specified. therefore use complete heuristic ( = (0,0,0,...))
                for(unsigned j = 0; j < sizeOfLowerBounds; j++){
                    lowerBounds[j] = 0.0;
                }
                return;
            }


            unsigned int pos = 0;
            bool fraction = false;
            bool valueRead = false;
            double currentValue = 0.0;
            unsigned int fractionDigitCount = 0;
            bool foundInf = false;
            for(unsigned i = 0; i < s.size(); i++){
                if(valueRead && (s.at(i) == ' ' || s.at(i) == ',' || s.at(i) == ';')){    //new value
                    if(pos < sizeOfLowerBounds){
                        lowerBounds[pos] = currentValue;
                    }
                    else{
                        return;
                    }
                    pos++;
                    fraction = false;
                    currentValue = 0;
                    valueRead = false;
                    fractionDigitCount = 0;
                }
                else if(valueRead && s.at(i) == '.'){                   //fraction start
                    fraction = true;
                    valueRead = true;
                }
                else if(s.at(i) >= 48 && s.at(i) <= 57){                //digit
                    valueRead = true;
                    int tempValue = s.at(i) - 48;
                    if(!fraction){
                        currentValue *= 10;
                        currentValue += tempValue;
                    }
                    else{
                        double divider = 1.0;
                        fractionDigitCount++;
                        for(unsigned int j = 0; j < fractionDigitCount; j++){
                            divider *= 10;
                        }
                        double tempFraction = 1.0 / divider;
                        tempFraction *= tempValue;
                        currentValue += tempFraction;
                    }
                }
                                                                        //infinity
                else if((i + 2 < s.size()) && s.at(i) == 'i' && s.at(i + 1) == 'n' && s.at(i + 2) == 'f'){
                    if(pos < sizeOfLowerBounds){
                        lowerBounds[pos] = 10.0;
                    }
                    else{
                        return;
                    }
                    pos++;
                    foundInf = true;
                    break;      //we don't need to look further because there will be no bits in the next stage
                }

            }
            if(!foundInf && valueRead){
                //write last value
                if(pos < sizeOfLowerBounds){
                    lowerBounds[pos] = currentValue;
                }
                else{
                    return;
                }
                pos++;
            }




            //fill rest with infinity
            for(unsigned int j = pos; j < sizeOfLowerBounds; j++){
                lowerBounds[j] = 10.0;
            }
        }
    }

    int BitHeapHeuristicCompression::cleanUp(){
        if((mode.compare("heuristic_parandeh-afshar_modified") == 0) || (mode.compare("heuristic_pa") == 0)){
            return 0;
        }
#ifdef HAVE_SCIP
        return bitHeapILPCompression.cleanUp();
#else
        return 0;
#endif  //HAVE_SCIP
    }
}


//#endif
