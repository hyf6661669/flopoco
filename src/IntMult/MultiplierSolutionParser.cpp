#include "MultiplierSolutionParser.hpp"

namespace flopoco {


MultiplierSolutionParser::MultiplierSolutionParser(std::string fileName){

    srcFileName = "MultiplierSolution:";
    uniqueName_ = "MultiplierSolutionParser for " + fileName;

    solFileName = fileName;


}

bool MultiplierSolutionParser::MultiplierSolutionParser::readSolution(){

    std::ifstream solFile(solFileName);

    if(solFile.is_open()){
        std::string line;
        while(getline(solFile, line)){
            if(variableIsTrue(line)){
				addVariableToSolution(line);
			}
        }
        solFile.close();
        return true;
    }
    else{
        //THROWERROR("IntMultiplier: unable to open file << fileName");
        return false;
    }
}


list<pair< unsigned int, pair<unsigned int, unsigned int> > > MultiplierSolutionParser::getSolution(){
    return solution;
}

bool MultiplierSolutionParser::variableIsTrue(string line){
	
	std::size_t found = line.find("m_");
	if(found == std::string::npos){
		return false;	
	}
	found = line.find(" 1");
	if(found == std::string::npos){
		return false;
	}
	return true;
}

void MultiplierSolutionParser::addVariableToSolution(string line){
	
	//example line: m_27_19_15 1
	//first value is x, second is y, third is type
	
	std::size_t firstUnderscorePos;
	std::size_t secondUnderscorePos;
	std::size_t thirdUnderscorePos;
	std::size_t whitespacePos;
	
	firstUnderscorePos = line.find("_");
	secondUnderscorePos = line.find("_", firstUnderscorePos + 1);
	thirdUnderscorePos = line.find("_", secondUnderscorePos + 1);
	whitespacePos = line.find(" ");
	
	string xString = line.substr(firstUnderscorePos + 1, secondUnderscorePos - firstUnderscorePos);
	string yString = line.substr(secondUnderscorePos + 1, thirdUnderscorePos - secondUnderscorePos);
	string typeString = line.substr(thirdUnderscorePos + 1, whitespacePos - thirdUnderscorePos);
	
	unsigned int xValue = atoi(xString.c_str());
	unsigned int yValue = atoi(yString.c_str());
	unsigned int typeValue = atoi(typeString.c_str());
	
	std::pair <unsigned int, unsigned int> coordinates (xValue, yValue);
	std::pair <unsigned int, std::pair<unsigned int, unsigned int> > entry (typeValue, coordinates);
	
	solution.push_back(entry);
}

	
}   //end namespace flopoco
