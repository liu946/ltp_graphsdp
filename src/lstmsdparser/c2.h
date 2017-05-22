#ifndef __LTP_LSTMSDPARSER_CPYPDICT_H__
#define __LTP_LSTMSDPARSER_CPYPDICT_H__

#include <string>
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <unordered_map>
#include <functional>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "lstmsdparser/listbased.h"

namespace cpyp {

inline void split(const std::string& source, const char& sep, 
    std::vector<std::string>& ret, int maxsplit=-1) {
  std::string str(source);
  int numsplit = 0;
  int len = str.size();
  size_t pos;
  for (pos = 0; pos < str.size() && (str[pos] == sep); ++ pos);
  str = str.substr(pos);

  ret.clear();
  while (str.size() > 0) {
    pos = std::string::npos;

    for (pos = 0; pos < str.size() && (str[pos] != sep); ++ pos);

    if (pos == str.size()) {
      pos = std::string::npos;
    }

    if (maxsplit >= 0 && numsplit < maxsplit) {
      ret.push_back(str.substr(0, pos));
      ++ numsplit;
    } else if (maxsplit >= 0 && numsplit == maxsplit) {
      ret.push_back(str);
      ++ numsplit;
    } else if (maxsplit == -1) {
      ret.push_back(str.substr(0, pos));
      ++ numsplit;
    }

    if (pos == std::string::npos) {
      str = "";
    } else {
      for (; pos < str.size() && (str[pos] == sep); ++ pos);
      str = str.substr(pos);
    }
  }
}

/**
 * Return a list of words of string str, the word are separated by
 * separator.
 *
 *  @param  str         std::string     the string
 *  @param  sep         char            the separator
 *  @param  maxsplit    std::string     the sep upperbound
 *  @return             std::vector<std::string> the words
 */
inline std::vector<std::string> split(const std::string& source, const char& sep, int maxsplit = -1) {
  std::vector<std::string> ret;
  split(source, sep, ret, maxsplit);
  return ret;
}

class Corpus {
 //typedef std::unordered_map<std::string, unsigned, std::hash<std::string> > Map;
// typedef std::unordered_map<unsigned,std::string, std::hash<std::string> > ReverseMap;
public: 
   bool DEBUG = false;
   bool USE_SPELLING=false; 

   std::map<int,std::vector<unsigned>> correct_act_sent;
   std::map<int,std::vector<unsigned>> sentences;
   std::map<int,std::vector<unsigned>> sentencesPos;

   std::map<int,std::vector<unsigned>> correct_act_sentDev;
   std::map<int,std::vector<unsigned>> sentencesDev;
   std::map<int,std::vector<unsigned>> sentencesPosDev;
   std::map<int,std::vector<std::string>> sentencesStrDev;
   unsigned nsentencesDev;

   std::map<int,std::vector<unsigned>> sentencesTest;
   std::map<int,std::vector<unsigned>> sentencesPosTest;
   std::map<int,std::vector<std::string>> sentencesStrTest;
   unsigned nsentencesTest;

   unsigned nsentences;
   unsigned nwords;
   unsigned nactions;
   unsigned npos;

   unsigned nsentencestest;
   unsigned nsentencesdev;
   int max;
   int maxPos;

   std::map<std::string, unsigned> wordsToInt;
   std::map<unsigned, std::string> intToWords;
   std::vector<std::string> actions;

   std::map<std::string, unsigned> posToInt;
   std::map<unsigned, std::string> intToPos;

   // String literals
   static constexpr const char* UNK = "UNK";
   static constexpr const char* BAD0 = "<BAD0>";
   static constexpr const char* ROOT = "ROOT";

/*  std::map<unsigned,unsigned>* headsTraining;
  std::map<unsigned,std::string>* labelsTraining;

  std::map<unsigned,unsigned>*  headsParsing;
  std::map<unsigned,std::string>* labelsParsing;*/


 
 public:
  Corpus() {
    max = 0;
    maxPos = 0;
  }


inline unsigned UTF8Len(unsigned char x) {
  if (x < 0x80) return 1;
  else if ((x >> 5) == 0x06) return 2;
  else if ((x >> 4) == 0x0e) return 3;
  else if ((x >> 3) == 0x1e) return 4;
  else if ((x >> 2) == 0x3e) return 5;
  else if ((x >> 1) == 0x7e) return 6;
  else return 0;
}

inline void load_conll_file(std::string file){
  std::ifstream actionsFile(file);
  //correct_act_sent=new vector<vector<unsigned>>();
  if (!actionsFile){
    std::cerr << "### File does not exist! ###" << std::endl;
  }
  std::string lineS;

  int sentence=0;
  wordsToInt[Corpus::BAD0] = 0;
  intToWords[0] = Corpus::BAD0;
  wordsToInt[Corpus::UNK] = 1; // unknown symbol
  intToWords[1] = Corpus::UNK;
  wordsToInt["ROOT"] = 2; // root
  intToWords[2] = "ROOT";
  posToInt["ROOT"] = 1; // root
  intToPos[1] = "ROOT";
  assert(max == 0);
  assert(maxPos == 0);
  max=3;
  maxPos=2;
  
  std::vector<unsigned> current_sent;
  std::vector<unsigned> current_sent_pos;
  std::map<int, std::vector<std::pair<int, std::string>>> graph;
  TransitionSystem* system;
  system = new ListBased();
  while (getline(actionsFile, lineS)){
    //istringstream iss(line);
    //string lineS;
    //iss>>lineS;
    ReplaceStringInPlace(lineS, "-RRB-", "_RRB_");
    ReplaceStringInPlace(lineS, "-LRB-", "_LRB_");
    if (lineS.empty()) {
        /*for (int j = 0; j < graph.size(); ++j){
        std::cerr << j << "\t" << intToWords[current_sent[j]] 
        << "\t" << intToPos[current_sent_pos[j]]
        << "\t" << graph[j].back().first << "\t" << graph[j].back().second << std::endl;
      }*/
      std::vector<std::string> gold_acts;
      system->get_actions(graph, gold_acts);
      bool found=false;
      //std::cerr << std::endl;
      for (auto g: gold_acts){
        //std::cerr << g << std::endl;
        int i = 0;
        found=false;
        for (auto a: actions) {
          if (a==g) {
            std::vector<unsigned> a=correct_act_sent[sentence];
            a.push_back(i);
            correct_act_sent[sentence]=a;
            found=true;
            break;
          }
          ++i;
        }
        if (!found) {
          actions.push_back(g);
          std::vector<unsigned> a=correct_act_sent[sentence];
          a.push_back(actions.size()-1);
          correct_act_sent[sentence]=a;
        }
      }
      current_sent.push_back(wordsToInt[Corpus::ROOT]);
      current_sent_pos.push_back(posToInt[Corpus::ROOT]);
      sentences[sentence] = current_sent;
      sentencesPos[sentence] = current_sent_pos;
      sentence++;
      nsentences = sentence;
      
      current_sent.clear();
      current_sent_pos.clear();
      graph.clear();
    } else {
      //stack and buffer, for now, leave it like this.
      // one line in each sentence may look like:
      // 5  American  american  ADJ JJ  Degree=Pos  6 amod  _ _
      // read the every line
      if (lineS[0] == '#') continue;
      std::vector<std::string> items = split(lineS, '\t');
      if (items[0].find('-') != std::string::npos) continue;
      if (items[0].find('.') != std::string::npos) continue;
      unsigned id = std::atoi(items[0].c_str()) - 1;
      std::string word = items[1];
      //std::string word = StrToLower(items[1]);
      std::string pos = items[3];
      unsigned head = std::atoi(items[6].c_str()) - 1;
      std::string rel = items[7];
      graph[id].push_back(std::make_pair(head, rel));
      if (graph[id].size() > 1) continue;
      // new POS tag
      if (posToInt[pos] == 0) {
        posToInt[pos] = maxPos;
        intToPos[maxPos] = pos;
        npos = maxPos;
        maxPos++;
      }
      // new word
      if (wordsToInt[word] == 0) {
        wordsToInt[word] = max;
        intToWords[max] = word;
        nwords = max;
        max++;
      }
      current_sent.push_back(wordsToInt[word]);
      current_sent_pos.push_back(posToInt[pos]);
    }
  }


  // Add the last sentence.
  if (current_sent.size() > 0) {
    std::vector<std::string> gold_acts;
    system->get_actions(graph, gold_acts);
    bool found=false;
    for (auto g: gold_acts){
      //std::cerr << g << std::endl;
      int i = 0;
      found=false;
      for (auto a: actions) {
        if (a==g) {
          std::vector<unsigned> a=correct_act_sent[sentence];
          a.push_back(i);
          correct_act_sent[sentence]=a;
          found=true;
          break;
        }
        ++i;
      }
      if (!found) {
        actions.push_back(g);
        std::vector<unsigned> a=correct_act_sent[sentence];
        a.push_back(actions.size()-1);
        correct_act_sent[sentence]=a;
      }
    }
    current_sent.push_back(wordsToInt[Corpus::ROOT]);
    current_sent_pos.push_back(posToInt[Corpus::ROOT]);
    sentences[sentence] = current_sent;
    sentencesPos[sentence] = current_sent_pos;
    sentence++;
    nsentences = sentence;
  }
      
  actionsFile.close();
  if (DEBUG){
    std::cerr<<"done"<<"\n";
    for (auto a: actions)
      std::cerr<<a<<"\n";
  }
  nactions=actions.size();
  if (DEBUG){
    std::cerr<<"nactions:"<<nactions<<"\n";
    std::cerr<<"nwords:"<<nwords<<"\n";
    for (unsigned i=0;i<npos;i++)
      std::cerr<<i<<":"<<intToPos[i]<<"\n";
  }
  //nactions=actions.size();
}

inline void load_conll_fileDev(std::string file){
  std::ifstream actionsFile(file);
  //correct_act_sent=new vector<vector<unsigned>>();
  if (!actionsFile){
    std::cerr << "### File does not exist! ###" << std::endl;
  }
  std::string lineS;

  assert(maxPos > 1);
  assert(max > 3);
  int sentence=0;
  std::vector<unsigned> current_sent;
  std::vector<unsigned> current_sent_pos;
  std::vector<std::string> current_sent_str;
  std::map<int, std::vector<std::pair<int, std::string>>> graph;
  TransitionSystem* system;
  system = new ListBased();
  while (getline(actionsFile, lineS)){
    //istringstream iss(line);
    //string lineS;
    //iss>>lineS;
    //std::cerr << lineS << std::endl;
    ReplaceStringInPlace(lineS, "-RRB-", "_RRB_");
    ReplaceStringInPlace(lineS, "-LRB-", "_LRB_");
    if (lineS.empty()) {
      std::vector<std::string> gold_acts;
      system->get_actions(graph, gold_acts);
      bool found=false;
      for (auto g: gold_acts){
        //std::cerr << g << std::endl;
        auto actionIter = std::find(actions.begin(), actions.end(), g);
        if (actionIter != actions.end()) {
          unsigned actionIndex = std::distance(actions.begin(), actionIter);
          correct_act_sentDev[sentence].push_back(actionIndex);
        } else {
          // new action
          actions.push_back(g);
          unsigned actionIndex = actions.size() - 1;
          correct_act_sentDev[sentence].push_back(actionIndex);
        }
      }
      current_sent.push_back(wordsToInt[Corpus::ROOT]);
      current_sent_pos.push_back(posToInt[Corpus::ROOT]);
      current_sent_str.push_back("");
      sentencesDev[sentence] = current_sent;
      sentencesPosDev[sentence] = current_sent_pos;
      sentencesStrDev[sentence] = current_sent_str; 
      sentence++;
      nsentencesDev = sentence;

      /*for (int j = 0; j < graph.size(); ++j){
        std::cerr << j << "\t" << intToWords[current_sent[j]] << "\t" << intToPos[current_sent_pos[j]]
        << "\t" << graph[j].back().first << "\t" << graph[j].back().second << std::endl;
      }*/
      
      current_sent.clear();
      current_sent_pos.clear();
      current_sent_str.clear();
      graph.clear();
    } else {
      //stack and buffer, for now, leave it like this.
      // one line in each sentence may look like:
      // 5  American  american  ADJ JJ  Degree=Pos  6 amod  _ _
      // read the every line
      if (lineS[0] == '#') continue;
      std::vector<std::string> items = split(lineS,'\t');
      if (items[0].find('-') != std::string::npos) continue;
      if (items[0].find('.') != std::string::npos) continue;
      unsigned id = std::atoi(items[0].c_str()) - 1;
      std::string word = items[1];
      //std::string word = StrToLower(items[1]);
      std::string pos = items[3];
      unsigned head = std::atoi(items[6].c_str()) - 1;
      std::string rel = items[7];
      graph[id].push_back(std::make_pair(head, rel));
      if (graph[id].size() > 1) continue;
      // new POS tag
      if (posToInt[pos] == 0) {
        posToInt[pos] = maxPos;
        intToPos[maxPos] = pos;
        npos = maxPos;
        maxPos++;
      }
      // add an empty string for any token except OOVs (it is easy to 
      // recover the surface form of non-OOV using intToWords(id)).
      current_sent_str.push_back("");
      // OOV word
      if (wordsToInt[word] == 0) {
          // save the surface form of this OOV before overwriting it.
          current_sent_str[current_sent_str.size()-1] = word;
          word = Corpus::UNK;
      }
      //current_sent_str[current_sent_str.size()-1] = items[1]; // saving lower word
      current_sent.push_back(wordsToInt[word]);
      current_sent_pos.push_back(posToInt[pos]);
    }
  }

  // Add the last sentence.
  if (current_sent.size() > 0) {
    std::vector<std::string> gold_acts;
    system->get_actions(graph, gold_acts);
    bool found=false;
    for (auto g: gold_acts){
      //std::cerr << g << std::endl;
      auto actionIter = std::find(actions.begin(), actions.end(), g);
      if (actionIter != actions.end()) {
        unsigned actionIndex = std::distance(actions.begin(), actionIter);
        correct_act_sentDev[sentence].push_back(actionIndex);
      } else {
        // new action
        actions.push_back(g);
        unsigned actionIndex = actions.size() - 1;
        correct_act_sentDev[sentence].push_back(actionIndex);
      }
    }
    current_sent.push_back(wordsToInt[Corpus::ROOT]);
    current_sent_pos.push_back(posToInt[Corpus::ROOT]);
    current_sent_str.push_back("");

    sentencesDev[sentence] = current_sent;
    sentencesPosDev[sentence] = current_sent_pos;
    sentencesStrDev[sentence] = current_sent_str;     
    sentence++;
    nsentencesDev = sentence;
  }
      
  actionsFile.close();
  if (DEBUG){
    std::cerr<<"done"<<"\n";
    for (auto a: actions)
      std::cerr<<a<<"\n";
  }
  nactions=actions.size();
  if (DEBUG){
    std::cerr<<"nactions:"<<nactions<<"\n";
    std::cerr<<"nwords:"<<nwords<<"\n";
    for (unsigned i=0;i<npos;i++)
      std::cerr<<i<<":"<<intToPos[i]<<"\n";
  }
  //nactions=actions.size();
}

inline void load_conll_fileTest(std::string file){
  std::ifstream actionsFile(file);
  //correct_act_sent=new vector<vector<unsigned>>();
  if (!actionsFile){
    std::cerr << "### File does not exist! ###" << std::endl;
  }
  std::string lineS;

  assert(maxPos > 1);
  assert(max > 3);
  //assert(maxExtraWord == 1);
  int sentence = 0;
  std::vector<unsigned> current_sent;
  std::vector<unsigned> current_sent_pos;
  std::vector<std::string> current_sent_str;
  while (getline(actionsFile, lineS)){
    //istringstream iss(line);
    //string lineS;
    //iss>>lineS;
    ReplaceStringInPlace(lineS, "-RRB-", "_RRB_");
    ReplaceStringInPlace(lineS, "-LRB-", "_LRB_");
    if (lineS.empty()) {
      current_sent.push_back(wordsToInt[Corpus::ROOT]);
      current_sent_pos.push_back(posToInt[Corpus::ROOT]);
      current_sent_str.push_back("");
      sentencesTest[sentence] = current_sent;
      sentencesPosTest[sentence] = current_sent_pos;
      sentencesStrTest[sentence] = current_sent_str;
      sentence++;
      nsentencesTest = sentence;

      /*for (int j = 0; j < current_sent.size(); ++j){
        std::cerr << j << "\t" << intToWords[current_sent[j]] << "\t" << intToPos[current_sent_pos[j]]
        << "\t" << intToCluster[current_sent_cls[j]] << std::endl;
      }*/
      
      current_sent.clear();
      current_sent_pos.clear();
      current_sent_str.clear();
    } else {
      //stack and buffer, for now, leave it like this.
      // one line in each sentence may look like:
      // 5  American  american  ADJ JJ  Degree=Pos  6 amod  _ _
      // read the every line
      if (lineS[0] == '#') continue;
      std::vector<std::string> items = split(lineS,'\t');
      if (items[0].find('.') != std::string::npos) continue;
      unsigned id = std::atoi(items[0].c_str()) - 1;
      std::string word = items[1];
      //std::string word = StrToLower(items[1]);
      std::string pos = items[3];
      //unsigned head = std::atoi(items[6].c_str()) - 1;
      //std::string rel = items[7];
      // new POS tag
      if (posToInt[pos] == 0) {
        posToInt[pos] = maxPos;
        intToPos[maxPos] = pos;
        npos = maxPos;
        maxPos++;
      }
      // add an empty string for any token except OOVs (it is easy to 
      // recover the surface form of non-OOV using intToWords(id)).
      current_sent_str.push_back("");
      // OOV word
      if (wordsToInt[word] == 0) {
        // save the surface form of this OOV before overwriting it.
        current_sent_str[current_sent_str.size()-1] = word;
        word = Corpus::UNK;
      }
      //current_sent_str[current_sent_str.size()-1] = items[1]; // saving lower word
      // cluster
      current_sent.push_back(wordsToInt[word]);
      current_sent_pos.push_back(posToInt[pos]);
    }
  }

  // Add the last sentence.
  if (current_sent.size() > 0) {
    current_sent.push_back(wordsToInt[Corpus::ROOT]);
    current_sent_pos.push_back(posToInt[Corpus::ROOT]);
    current_sent_str.push_back("");
    sentencesTest[sentence] = current_sent;
    sentencesPosTest[sentence] = current_sent_pos;
    sentencesStrTest[sentence] = current_sent_str; 
    sentence++;
    nsentencesTest = sentence;
  }
      
  actionsFile.close();
  if (DEBUG){
    std::cerr << "test sentence = " << nsentencesTest << std::endl;
    std::cerr<<"done"<<"\n";
    for (auto a: actions)
      std::cerr<<a<<"\n";
  }
  nactions=actions.size();
  if (DEBUG){
    std::cerr<<"nactions:"<<nactions<<"\n";
    std::cerr<<"nwords:"<<nwords<<"\n";
    for (unsigned i=0;i<npos;i++)
      std::cerr<<i<<":"<<intToPos[i]<<"\n";
  }
}

inline void load_correct_actions(std::string file){
	
  std::ifstream actionsFile(file);
  //correct_act_sent=new vector<vector<unsigned>>();
  std::string lineS;
	
  int count=-1;
  int sentence=-1;
  bool initial=false;
  bool first=true;
  wordsToInt[Corpus::BAD0] = 0;
  intToWords[0] = Corpus::BAD0;
  wordsToInt[Corpus::UNK] = 1; // unknown symbol
  intToWords[1] = Corpus::UNK;
  assert(max == 0);
  assert(maxPos == 0);
  max=2;
  maxPos=1;
  
	std::vector<unsigned> current_sent;
  std::vector<unsigned> current_sent_pos;
  while (getline(actionsFile, lineS)){
    //istringstream iss(line);
    //string lineS;
 		//iss>>lineS;
    ReplaceStringInPlace(lineS, "-RRB-", "_RRB_");
    ReplaceStringInPlace(lineS, "-LRB-", "_LRB_");
		if (lineS.empty()) {
			count = 0;
			if (!first) {
				sentences[sentence] = current_sent;
				sentencesPos[sentence] = current_sent_pos;
      }
      
			sentence++;
			nsentences = sentence;
      
			initial = true;
                   current_sent.clear();
			current_sent_pos.clear();
		} else if (count == 0) {
			first = false;
			//stack and buffer, for now, leave it like this.
			count = 1;
			if (initial) {
        // the initial line in each sentence may look like:
        // [][the-det, cat-noun, is-verb, on-adp, the-det, mat-noun, ,-punct, ROOT-ROOT]
        // first, get rid of the square brackets.
        lineS = lineS.substr(5, lineS.size() - 6);
        // read the initial line, token by token "the-det," "cat-noun," ...
        std::istringstream iss(lineS);
        do {
          std::string word;
          iss >> word;
          if (word.size() == 0) { continue; }
          // remove the trailing comma if need be.
          if (word[word.size() - 1] == ',') { 
            word = word.substr(0, word.size() - 1);
          }
          // split the string (at '-') into word and POS tag.
          size_t posIndex = word.rfind('-');
          if (posIndex == std::string::npos) {
            std::cerr << "cant find the dash in '" << word << "'" << std::endl;
          }
          assert(posIndex != std::string::npos);
          std::string pos = word.substr(posIndex + 1);
          word = word.substr(0, posIndex);
          // new POS tag
          if (posToInt[pos] == 0) {
            posToInt[pos] = maxPos;
            intToPos[maxPos] = pos;
            npos = maxPos;
            maxPos++;
          }

          // new word
          if (wordsToInt[word] == 0) {
            wordsToInt[word] = max;
            intToWords[max] = word;
            nwords = max;
            max++;
          }
        
          current_sent.push_back(wordsToInt[word]);
          current_sent_pos.push_back(posToInt[pos]);
        } while(iss);
			}
			initial=false;
		}
		else if (count==1){
			int i=0;
			bool found=false;
			for (auto a: actions) {
				if (a==lineS) {
					std::vector<unsigned> a=correct_act_sent[sentence];
	                                a.push_back(i);
        	                        correct_act_sent[sentence]=a;
					found=true;
				}
				i++;
			}
			if (!found) {
				actions.push_back(lineS);
				std::vector<unsigned> a=correct_act_sent[sentence];
				a.push_back(actions.size()-1);
				correct_act_sent[sentence]=a;
			}
			count=0;
		}
	}

  // Add the last sentence.
  if (current_sent.size() > 0) {
    sentences[sentence] = current_sent;
    sentencesPos[sentence] = current_sent_pos;
    sentence++;
    nsentences = sentence;
  }
      
  actionsFile.close();
/*	std::string oov="oov";
	posToInt[oov]=maxPos;
        intToPos[maxPos]=oov;
        npos=maxPos;
        maxPos++;
        wordsToInt[oov]=max;
        intToWords[max]=oov;
        nwords=max;
        max++;*/
  if (DEBUG){
	  std::cerr<<"done"<<"\n";
	  for (auto a: actions) {
		  std::cerr<<a<<"\n";
	  }
  }
	nactions=actions.size();
  if (DEBUG){
	std::cerr<<"nactions:"<<nactions<<"\n";
        std::cerr<<"nwords:"<<nwords<<"\n";
	for (unsigned i=0;i<npos;i++){
                std::cerr<<i<<":"<<intToPos[i]<<"\n";
        }
  }
	nactions=actions.size();
	
}

inline unsigned get_or_add_word(const std::string& word) {
  unsigned& id = wordsToInt[word];
  if (id == 0) {
    id = max;
    ++max;
    intToWords[id] = word;
    nwords = max;
  }
  return id;
}

inline void load_correct_actionsDev(std::string file) {
  std::ifstream actionsFile(file);
  std::string lineS;

  assert(maxPos > 1);
  assert(max > 3);
  int count = -1;
  int sentence = -1;
  bool initial = false;
  bool first = true;
  std::vector<unsigned> current_sent;
  std::vector<unsigned> current_sent_pos;
  std::vector<std::string> current_sent_str;
  while (getline(actionsFile, lineS)) {
    ReplaceStringInPlace(lineS, "-RRB-", "_RRB_");
    ReplaceStringInPlace(lineS, "-LRB-", "_LRB_");
    if (lineS.empty()) {
      // an empty line marks the end of a sentence.
      count = 0;
      if (!first) {
        sentencesDev[sentence] = current_sent;
        sentencesPosDev[sentence] = current_sent_pos;
        sentencesStrDev[sentence] = current_sent_str;
      }
      
      sentence++;
      nsentencesDev = sentence;
      
      initial = true;
      current_sent.clear();
      current_sent_pos.clear();
      current_sent_str.clear();
    } else if (count == 0) {
      first = false;
      //stack and buffer, for now, leave it like this.
      count = 1;
      if (initial) {
        // the initial line in each sentence may look like:
        // [][the-det, cat-noun, is-verb, on-adp, the-det, mat-noun, ,-punct, ROOT-ROOT]
        // first, get rid of the square brackets.
        lineS = lineS.substr(5, lineS.size() - 6);
        // read the initial line, token by token "the-det," "cat-noun," ...
        std::istringstream iss(lineS);
        do {
          std::string word;
          iss >> word;
          if (word.size() == 0) { continue; }
          // remove the trailing comma if need be.
          if (word[word.size() - 1] == ',') { 
            word = word.substr(0, word.size() - 1);
          }
          // split the string (at '-') into word and POS tag.
          size_t posIndex = word.rfind('-');
          assert(posIndex != std::string::npos);
          std::string pos = word.substr(posIndex + 1);
          word = word.substr(0, posIndex);
          // new POS tag
          if (posToInt[pos] == 0) {
            posToInt[pos] = maxPos;
            intToPos[maxPos] = pos;
            npos = maxPos;
            maxPos++;
          }
          // add an empty string for any token except OOVs (it is easy to 
          // recover the surface form of non-OOV using intToWords(id)).
          current_sent_str.push_back("");
          // OOV word
          if (wordsToInt[word] == 0) {
            if (USE_SPELLING) {
              max = nwords + 1;
              //std::cerr<< "max:" << max << "\n";
              wordsToInt[word] = max;
              intToWords[max] = word;
              nwords = max;
            } else {
              // save the surface form of this OOV before overwriting it.
              current_sent_str[current_sent_str.size()-1] = word;
              word = Corpus::UNK;
            }
          }
          current_sent.push_back(wordsToInt[word]);
          current_sent_pos.push_back(posToInt[pos]);
        } while(iss);
      }
      initial = false;
    } else if (count == 1) {
      auto actionIter = std::find(actions.begin(), actions.end(), lineS);
      if (actionIter != actions.end()) {
        unsigned actionIndex = std::distance(actions.begin(), actionIter);
        correct_act_sentDev[sentence].push_back(actionIndex);
      } else {
        // TODO: right now, new actions which haven't been observed in training
        // are not added to correct_act_sentDev. This may be a problem if the
        // training data is little.
          // new action
          actions.push_back(lineS);
          unsigned actionIndex = actions.size() - 1;
          correct_act_sentDev[sentence].push_back(actionIndex);
      }
      count=0;
    }
  }

  // Add the last sentence.
  if (current_sent.size() > 0) {
    sentencesDev[sentence] = current_sent;
    sentencesPosDev[sentence] = current_sent_pos;
    sentencesStrDev[sentence] = current_sent_str;
    sentence++;
    nsentencesDev = sentence;
  }
  
  actionsFile.close();
}

void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}


/*  inline unsigned max() const { return words_.size(); }
  inline unsigned size() const { return words_.size(); }
  inline unsigned count(const std::string& word) const { return d_.count(word); }*/

/*  static bool is_ws(char x) {
    return (x == ' ' || x == '\t');
  }

  inline void ConvertWhitespaceDelimitedLine(const std::string& line, std::vector<unsigned>* out) {
    size_t cur = 0;
    size_t last = 0;
    int state = 0;
    out->clear();
    while(cur < line.size()) {
      if (is_ws(line[cur++])) {
        if (state == 0) continue;
        out->push_back(Convert(line.substr(last, cur - last - 1)));
        state = 0;
      } else {
        if (state == 1) continue;
        last = cur - 1;
        state = 1;
      }
    }
    if (state == 1)
      out->push_back(Convert(line.substr(last, cur - last)));
  }

  inline unsigned Convert(const std::string& word, bool frozen = false) {
    Map::iterator i = d_.find(word);
    if (i == d_.end()) {
      if (frozen)
        return 0;
      words_.push_back(word);
      d_[word] = words_.size();
      return words_.size();
    } else {
      return i->second;
    }
  }

  inline const std::string& Convert(const unsigned id) const {
    if (id == 0) return b0_;
    return words_[id-1];
  }
  template<class Archive> void serialize(Archive& ar, const unsigned int version) {
    ar & b0_;
    ar & words_;
    ar & d_;
  }
 private:
  std::string b0_;
  std::vector<std::string> words_;
  Map d_;*/
};


/*void ReadFromFile(const std::string& filename,
                  Corpus* d,
                  std::vector<std::vector<unsigned> >* src,
                  std::set<unsigned>* src_vocab) {
  std::cerr << "Reading from " << filename << std::endl;
  std::ifstream in(filename);
  assert(in);
  std::string line;
  int lc = 0;
  while(getline(in, line)) {
    ++lc;
    src->push_back(std::vector<unsigned>());
    d->ConvertWhitespaceDelimitedLine(line, &src->back());
    for (unsigned i = 0; i < src->back().size(); ++i) src_vocab->insert(src->back()[i]);
  }
}

void ReadParallelCorpusFromFile(const std::string& filename,
                                Corpus* d,
                                std::vector<std::vector<unsigned> >* src,
                                std::vector<std::vector<unsigned> >* trg,
                                std::set<unsigned>* src_vocab,
                                std::set<unsigned>* trg_vocab) {
  src->clear();
  trg->clear();
  std::cerr << "Reading from " << filename << std::endl;
  std::ifstream in(filename);
  assert(in);
  std::string line;
  int lc = 0;
  std::vector<unsigned> v;
  const unsigned kDELIM = d->Convert("|||");
  while(getline(in, line)) {
    ++lc;
    src->push_back(std::vector<unsigned>());
    trg->push_back(std::vector<unsigned>());
    d->ConvertWhitespaceDelimitedLine(line, &v);
    unsigned j = 0;
    while(j < v.size() && v[j] != kDELIM) {
      src->back().push_back(v[j]);
      src_vocab->insert(v[j]);
      ++j;
    }
    if (j >= v.size()) {
      std::cerr << "Malformed input in parallel corpus: " << filename << ":" << lc << std::endl;
      abort();
    }
    ++j;
    while(j < v.size()) {
      trg->back().push_back(v[j]);
      trg_vocab->insert(v[j]);
      ++j;
    }
  }
}*/

} // namespace

#endif // __LTP_LSTMSDPARSER_CPYPDICT_H__
