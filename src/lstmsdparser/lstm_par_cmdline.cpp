#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "lstm_sdparser_dll.h"

#define EXECUTABLE "lstm_par_cmdline"
#define DESCRIPTION "The console application for dependency parsing."

void split(const std::string& src, const std::string& separator, std::vector<std::string>& dest)
{
     std::string str = src;
     std::string substring;
     std::string::size_type start = 0, index;
 
     do
     {
         index = str.find_first_of(separator,start);
         if (index != std::string::npos)
         {    
             substring = str.substr(start,index-start);
             dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
             if (start == std::string::npos) return;
         }
     }while(index != std::string::npos);
      
     //the last token
     substring = str.substr(start);
     dest.push_back(substring);
}

void output_conll(std::vector<std::string> words, std::vector<std::string> postags,
                    std::vector<std::vector<std::string>> hyp){
    std::string str = "";
    std::string full = "";
    for(int i = 0; i < words.size(); i++){
      str = "";
      str += std::to_string(i+1) + "\t" + words[i] + "\t" + words[i] + "\t" + postags[i] + "\t" + postags[i] + "\t_\t";
      for (int j = 0; j < hyp[i].size(); j++){
        if (hyp[i][j] != "-NULL-"){
          full = str + std::to_string(j+1) + "\t" + hyp[i][j] + "\t_\t_";
          std::cerr << full << std::endl;
        }
      }
    }
    std::cerr << std::endl;
}

int main(int argc, char * argv[]) {
  if (argc < 3) {
    std::cerr << "usage: ./lstm_par [data directory] [input file]" << std::endl;
    return -1;
  }

  void * engine = lstmsdparser_create_parser(argv[1]);
  if (!engine) {
    std::cerr << "fail to init parser" << std::endl;
    return -1;
  }

  std::ifstream ifs(argv[2]);

  std::string sentence = "";
  while (!ifs.eof()) {
      std::getline(ifs, sentence,'\n');
      if (!sentence.length()) break;
      std::vector<std::string> sent;
      split(sentence, "\t", sent);
      std::vector<std::string> words;
      std::vector<std::string> postags;
      for (int i = 0; i < sent.size(); i++){
        int idx = sent[i].find_first_of('_', 0);
        words.push_back(sent[i].substr(0, idx));
        postags.push_back(sent[i].substr(idx + 1));
      }
      std::vector<std::vector<std::string>> hyp;
      lstmsdparser_parse(engine, words, postags, hyp);
      output_conll(words, postags, hyp);
  }

/*
  std::vector<std::vector<std::string>> hyp;
  std::string word[]={"我","是","中国","学生"}; // id : 22 146 296 114 21
  size_t w_count=sizeof(word)/sizeof(std::string);
  std::string pos[]={"r","v","ns","n"};
  size_t p_count=sizeof(word)/sizeof(std::string);
  std::vector<std::string> words(word,word+w_count);
  std::vector<std::string> postags(pos,pos+p_count);
  */

  //lstmsdparser_parse(engine, words, postags, hyp);

  lstmsdparser_release_parser(engine);
  return 0;
}

