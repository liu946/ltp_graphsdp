//
// Created by liu on 2017-05-12.
//

#ifndef Srl_Pi_CONFIG_H
#define Srl_Pi_CONFIG_H

#include "config/ModelConf.h"

class SrlPiBaseConfig : public virtual ModelConf {
public:
  unsigned word_dim;
  unsigned emb_dim;
  unsigned pos_dim;
  unsigned rel_dim;
  unsigned lstm_input_dim;
  unsigned lstm_hidden_dim;
  unsigned layers;

  string embeding;

  SrlPiBaseConfig(string confName = "Configuration"): ModelConf(confName) {
    registerConf<unsigned>("word_dim"       , UNSIGNED, word_dim         , "word_dim"       , 100);
    registerConf<unsigned>("emb_dim"        , UNSIGNED, emb_dim          , "emb_dim"        , 50);
    registerConf<unsigned>("pos_dim"        , UNSIGNED, pos_dim          , "pos_dim"        , 12);
    registerConf<unsigned>("rel_dim"        , UNSIGNED, rel_dim          , "rel_dim"        , 50);
    registerConf<unsigned>("lstm_input_dim" , UNSIGNED, lstm_input_dim   , "lstm_input_dim" , 100);
    registerConf<unsigned>("lstm_hidden_dim", UNSIGNED, lstm_hidden_dim  , "lstm_hidden_dim", 100);
    registerConf<unsigned>("layers"         , UNSIGNED, layers           , "dynetRnnBuilder layers"    , 1);

    registerConf<string>  ("embeding" , STRING,   embeding , "embeding", "");
  }
};

class SrlPiTrainConfig : public virtual SrlPiBaseConfig, public virtual LabelModelTrainerConf {
public:

  SrlPiTrainConfig(string confName = "Configuration"):
          SrlPiBaseConfig(confName),
          LabelModelTrainerConf(confName)
  {

  }
};

class SrlPiPredConfig : public virtual SrlPiBaseConfig, public virtual LabelModelPredictorConf {
public:
  SrlPiPredConfig(string confName = "Configuration"):
          SrlPiBaseConfig(confName),
          LabelModelPredictorConf(confName)
  { }
};

#endif //Srl_Pi_CONFIG_H
