#include "Corpus.hpp"

#include <fstream>
#include <iostream>
#include <algorithm>

#define keyAdjust 10000

using namespace std;
const bool compareVectorSize(const std::vector<int> &i, const std::vector<int> &j){
	return i.size() < j.size();
}

const bool compareIntMap(const std::map<int, int>::value_type &i1, const std::map<int, int>::value_type &i2){
	return i1.second<i2.second;
}

void Corpus::loadCorpusFromFile(string filename){
	numberOfWords = 0;
	numberOfUniqueWords = 0;
	numOfSymbols =0;

	symbols.clear();
	invertSymbols.clear();
	allWords.clear();
	uniqueWords.clear();
	wordCount.clear();

	//Read File line by line
	ifstream symbolFile;
	symbolFile.open(filename.c_str());
	string line;
	vector<int> currentWord;
	map<char,int>::iterator itSymbols;
	cout << "Loading Corpus from file: " << filename << endl;
	while(getline(symbolFile, line)){

		currentWord.clear();
		for(string::iterator it = line.begin(); it != line.end(); it++){
			if(*it == ' '){
				continue;
			}

			itSymbols = symbols.find(*it);
			if(itSymbols == symbols.end()){
				symbols[*it] = numOfSymbols;
				numOfSymbols++;
				itSymbols = symbols.find(*it);
			}
			currentWord.push_back(itSymbols->second);
		}
		allWords.push_back(currentWord);
	}
	numberOfWords = allWords.size();
	symbolFile.close();
	
	std::sort(allWords.begin(), allWords.end(), compareVectorSize);
	reduceToUniqueWords();
}

void Corpus::reduceToUniqueWords(){
	for(unsigned int iter = 0; iter < allWords.size(); iter++){
		bool found = false;
		for(unsigned int iter2 = 0; iter2 < uniqueWords.size(); iter2++){
			if(uniqueWords[iter2].size() != allWords[iter].size()){
				continue;
			}
			found = true;
			for(unsigned int i=0; i<allWords[iter].size(); i++){
				if(allWords[iter][i] != uniqueWords[iter2][i]){
					found = false;
					break;
				}
			}	
			if(found==true){
				wordCount[iter2]++;
				break;
			}
		}
		if(found == false){
			wordCount.push_back(1);
			uniqueWords.push_back(allWords[iter]);
		}
	}
	for(vector<double>::iterator iter = wordCount.begin(); iter!=wordCount.end(); iter++){
		*iter = *iter/numberOfWords;
	}
	numberOfUniqueWords = uniqueWords.size();
}


bestChunk Corpus::findMaxChunk(){
	calculateBiwordsCount();
	map<int,int>::iterator max = std::max_element(biwordsCount.begin(), biwordsCount.end(), compareIntMap);
	bestChunk maxchunk;
	maxchunk.leftSymbol = max->first/10000;
	maxchunk.rightSymbol = max->first%10000;
	maxchunk.conflict = biwordsConflict[max->first];
	maxchunk.prob = biwordsProb[max->first];
	maxchunk.count = max->second;
	if(maxchunk.conflict == true){
		bestChunk temp = maxchunk;
		do{
			biwordsCount.erase(max->first);
			max = std::max_element(biwordsCount.begin(), biwordsCount.end(), compareIntMap);
			maxchunk.leftSymbol = max->first/10000;
			maxchunk.rightSymbol = max->first%10000;
			maxchunk.conflict = biwordsConflict[max->first];
			maxchunk.prob = biwordsProb[max->first];
			maxchunk.count = max->second;
			
		}while(maxchunk.conflict==true && biwordsCount.size()!=0);
		if(maxchunk.conflict==true){
			maxchunk = temp;
		}
	}
	return maxchunk;
}

void Corpus::calculateBiwordsCount(){
	biwordsCount.clear();
	biwordsProb.clear();
	biwordsConflict.clear();
	int currSymbol;
	int prevSymbol;
	int previusKey;
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		previusKey = -1;
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			prevSymbol = currSymbol;
			currSymbol = uniqueWords[i][j];
			if(j==0){
				continue;
			}
			int key = prevSymbol*keyAdjust + currSymbol;
			map<int,int>::iterator iter = biwordsCount.find(key);
			if(iter == biwordsCount.end()){
				biwordsCount[key] = 1;//*wordCount[i];
				biwordsProb[key] = wordCount[i];
				biwordsConflict[key] = false;
			}else{
				if(biwordsConflict[key] == true || key == previusKey){
					biwordsConflict[key] = true;
				}
				biwordsCount[key]++; //+= wordCount[i];
				biwordsProb[key]+= wordCount[i];
			}
			previusKey = key;
		}
	}

	//for(map<int,double>::iterator iter = biwordsCount.begin(); iter != biwordsCount.end(); iter++){
	//	cout << iter->first << " " << iter->second << " " << biwordsConflict[iter->first] << endl;
	//}
}

void Corpus::initReduceForPCFG(PCFG *pcfg){
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			uniqueWords[i][j] = pcfg->rulesForTermSymbol[uniqueWords[i][j]]->id;
		}
	}
}

void Corpus::reduceCorpusForRule(Rule *rule){
	for(unsigned int i=0; i<uniqueWords.size(); i++){
		std::vector<int> newWord;
		bool reduce = true;
		int prevSymbol;
		int currSymbol = -1;
		for(unsigned int j=0; j<uniqueWords[i].size(); j++){
			prevSymbol = currSymbol;
			currSymbol = uniqueWords[i][j];
			if(reduce){
				reduce = false;
				continue;
			}
			for(unsigned int i=0; i<rule->numberOfNTProductions(); i++){
				if(rule->getLeftNTProduction(i)->id == prevSymbol && rule->getRightNTProduction(i)->id == currSymbol){
					reduce = true;
					break;
				}
			}

			if(reduce){
				newWord.push_back(rule->id);
				prevSymbol = -1;
				currSymbol = -1;
			}else{
				newWord.push_back(prevSymbol);
			}
		}
		if(currSymbol != -1){
			newWord.push_back(currSymbol);
		}
		uniqueWords[i] = newWord;
	}
}

void Corpus::printCorpus(bool unique){
	int count = 1;
	cout << endl;
	cout << "-----Symbols and the number associated to them-----" << endl;
	for(map<char,int>::iterator it = symbols.begin(); it != symbols.end(); it++){
		cout << count << ") " << it->first << " " << it->second << endl;
		count++;
	}
	cout << endl;

	if(!unique){
		count = 1;
		cout << endl;
		cout << "-----Words in Corpus-----" << endl;
		for(vector<vector<int> >::iterator it1 = allWords.begin(); it1 != allWords.end(); it1++){
			cout << count << ") ";
			for(vector<int>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
				cout << *it2 << " ";
			}
			cout << endl;
			count++;
		}
		cout << endl;
	}else{
		count = 1;
		cout << endl;
		cout << "-----Unique Words in Corpus-----" << endl;
		for(vector<vector<int> >::iterator it1 = uniqueWords.begin(); it1 != uniqueWords.end(); it1++){
			cout << count << ") ";
			for(vector<int>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
				cout << *it2 << " ";
			}
			cout << " count = " << wordCount[count-1];
			cout << endl;
			count++;
		}
		cout << endl;
	}
}
