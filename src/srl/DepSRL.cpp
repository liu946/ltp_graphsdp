/*
 * File name    : DepSRL.cpp
 * Author       : msmouse
 * Create Time  : 2009-09-19
 * Remark       : feature selection, post-process, result generation
 *
 * Updates by   : jiangfeng
 *
 */


#include "DepSRL.h"
#include "ConstVar.h"
#include "vector"
#include "boost/bind.hpp"
#include "boost/algorithm/string.hpp"
#include "dynet/dynet.h"

#include "pp/config/PPConfig.h"
#include "srl/config/SRLConfig.h"

#include "structure/DataSentence.h"
#include "pp/extractor/ConverterSentenceToPPSampleGroup.h"
#include "srl/extractor/ConverterSentenceToSRLBiLSTMSampleGroup.h"
#include "structure/Prediction.h"

using namespace std;

// Load necessary resources into memory
int DepSRL::LoadResource(const string &ConfigDir)
{
    dynet::DynetParams params;
    params.mem_descriptor = "2000";
    dynet::initialize(params);

    ppConfig.init(ConfigDir + "/pp.config"); manageConfigPath(ppConfig, ConfigDir);
    srlConfig.init(ConfigDir + "/srl.config"); manageConfigPath(srlConfig, ConfigDir);
    cout << ppConfig.toString("\n", "->") << endl;
    cout << srlConfig.toString("\n", "->") << endl;

    pp_model = new PPBiLSTMModel(ppConfig, table1);
    pp_model->loadLookUpTable();
    table1.freeze();
    pp_model->init();

    srl_model = new SRLBiLSTMModel(srlConfig, table2);
    srl_model->loadLookUpTable();
    table2.freeze();
    srl_model->init();

    // load model
    if(!pp_model->load()) {
      std::cerr << "can't load model file " << ppConfig.model.c_str();
      exit(0);
    }
    if(!srl_model->load()) {
      std::cerr << "can't load model file " << srlConfig.model.c_str();
      exit(0);
    }

    m_resourceLoaded = true;

    return true;
}

// Release all resources
int DepSRL::ReleaseResource()
{
    delete srl_model;
    delete pp_model;

    m_resourceLoaded = false;

    return 1;
}

int DepSRL::GetSRLResult(
        const vector<string> &words,
        const vector<string> &POSs,
        const vector<string> &NEs,
        const vector< pair<int, string> > &parse,
        vector< pair< int, vector< pair< string, pair< int, int > > > > > &vecSRLResult
        )
{
    vecSRLResult.clear();
    // todo construct a Sentence instance
    assert(words.size() == POSs.size());
    assert(words.size() == parse.size());
    DataSentence sentence;
    for (int j = 0; j < words.size(); ++j) {
        Word word(j, words[j], words[j], POSs[j], parse[j].first, NIL_LABEL, (j <= parse[j].first ? "before" : "after"), parse[j].second, NIL_LABEL);
        sentence.words.push_back(word);
    }
    // pp
    {
        // todo construct pp sample
        ConverterSentenceToPPSampleGroup conv_stnToPPSamp(table1);
        conv_stnToPPSamp.convert(sentence);
        PPBiLSTMSampleGroup& smp = conv_stnToPPSamp.getResult()[0];
        // todo pp prediction
        ComputationGraph hg;
        vector<Expression> adists = pp_model->label(hg, smp);
        vector<Prediction> results = pp_model->ExtractResults(hg, adists);
        // todo pp writeback
        assert(smp.samples.size() == results.size());
        for (int j = 0; j < smp.size(); ++j) {
            smp.samples[j].isPredicate = (unsigned int)(results[j][0].first || results[j][1].second > 0.1);
        }
        conv_stnToPPSamp.iconvOne(sentence, smp, 0);
    }
    if ( !sentence.predicateInnerIndex.size() ) {
        // skip all processing if no predicate
        return 1;
    }
    // srl
    vector< vector< pair<string, double> > > vecAllPairMaxArgs;
    vector< vector< pair<string, double> > > vecAllPairNextArgs;
    {
        // todo construct srl sample
        ConverterSentenceToSRLBiLSTMSampleGroup conv_stnToSRLSamp(table2);
        conv_stnToSRLSamp.convert(sentence);
        vector<SRLBiLSTMSampleGroup>& smps = conv_stnToSRLSamp.getResult();
        // todo srl prediction
        for (int k = 0; k < smps.size(); ++k) {
            ComputationGraph hg;
            vector<Expression> adists = srl_model->label(hg, smps[k]);
            vector<Prediction> results = srl_model->ExtractResults(hg, adists);
            // todo srl writeback
            assert(smps[k].samples.size() == results.size());
            vector< pair<string, double> > vecPairMaxArgs;
            vector< pair<string, double> > vecPairNextArgs;
            for (int j = 0; j < smps[k].samples.size(); ++j) {
                smps[k].samples[j].label = results[j][0].first;
                vecPairMaxArgs.push_back(make_pair(table2.label.getStringByIndex(results[j][0].first), results[j][0].second));
                vecPairNextArgs.push_back(make_pair(table2.label.getStringByIndex(results[j][1].first), results[j][1].second));
            }
            vecAllPairMaxArgs.push_back(vecPairMaxArgs);
            vecAllPairNextArgs.push_back(vecPairNextArgs);
        }
        //conv_stnToSRLSamp.iconv(&smps);
    }
    // todo formResult
    vector < vector < pair < int, int > > > vecAllPos;
    generateOldPosStructure(parse, sentence.predicateInnerIndex, vecAllPos, vecAllPairMaxArgs, vecAllPairNextArgs);

    if (!FormResult(words, POSs, sentence.predicateInnerIndex, vecAllPos,
            vecAllPairMaxArgs,vecAllPairNextArgs,
            vecSRLResult)) return 0;
    // todo post
    // rename arguments to short forms (ARGXYZ->AXYZ)
    if (!RenameArguments(vecSRLResult)) return 0;
    return 1;
}

void DepSRL::generateOldPosStructure(const vector< pair<int, string> > &parse,
                                     const vector<int> & predicate,
                                     vector< vector < pair < int, int > > > & vecAllPos,
                                     vector< vector< pair<string, double> > > & vecAllPairMaxArgs,
                                     vector< vector< pair<string, double> > > & vecAllPairNextArgs
    ) {
    vecAllPos.clear(); vecAllPos.resize(predicate.size());
    vector<pair<int, int>> allWordsSubTreePos;
    for (int j = 0; j < parse.size(); ++j) {
        allWordsSubTreePos.push_back(make_pair(subTreeBegin(j, parse), subTreeEnd(j, parse)));
    }
    for (int k = 0; k < predicate.size(); ++k) {
        vector< pair<string, double> > vecPredMaxArgs;
        vector< pair<string, double> > vecPredNextArgs;
        for (int j = 0; j < allWordsSubTreePos.size(); ++j) {
            if (!(allWordsSubTreePos[j].first <= predicate[k] && allWordsSubTreePos[j].second >= predicate[k])) {
                vecAllPos[k].push_back(allWordsSubTreePos[j]);
                vecPredMaxArgs.push_back(vecAllPairMaxArgs[k][j]);
                vecPredNextArgs.push_back(vecAllPairNextArgs[k][j]);
            }
        }
        vecAllPairMaxArgs[k] = vecPredMaxArgs;
        vecAllPairNextArgs[k] = vecPredNextArgs;
    }
}

int DepSRL::FormResult(
        const vector<string> &words,
        const vector<string> &POSs,
        const vector<int>          &VecAllPredicates,
        VecPosForSent        &vecAllPos,
        vector< vector< pair<string, double> > > &vecAllPairMaxArgs,
        vector< vector< pair<string, double> > > &vecAllPairNextArgs,
        vector< pair< int, vector< pair< string, pair< int, int > > > > > &vecSRLResult
        )
{
    vecSRLResult.clear();
    vector< pair< string, pair< int, int > > > vecResultForOnePredicate;

    for (size_t idx=0; idx<VecAllPredicates.size(); ++idx)
    {
        int predicate_position = VecAllPredicates[idx];

        vecResultForOnePredicate.clear();

        ProcessOnePredicate(
                words, POSs, predicate_position, vecAllPos[idx], 
                vecAllPairMaxArgs[idx], vecAllPairNextArgs[idx], 
                vecResultForOnePredicate
                );

        if ( vecResultForOnePredicate.size() > 1 ) {
            vecResultForOnePredicate.pop_back(); // pop the "V" arg
            vecSRLResult.push_back(make_pair(predicate_position,vecResultForOnePredicate));
        }
        //vecResultForOnePredicate.pop_back(); // pop the "V" arg
        //vecSRLResult.push_back(make_pair(predicate_position,vecResultForOnePredicate));
    }

    return 1;
}

// result forming form one predicate, based on hjliu's original function
void DepSRL::ProcessOnePredicate(
        const vector<string>& vecWords,
        const vector<string>& vecPos,
        int intPredicates, 
        const vector< pair<int, int> >& vecPairPS,
        const vector< pair<string, double> >& vecPairMaxArgs, 
        const vector< pair<string, double> >& vecPairNextArgs,
        vector< pair< string, pair< int, int > > > &vecResultForOnePredicate
        )
{
    vector< pair<int, int> > vecPairPSBuf;
    vector< pair<string, double> > vecPairMaxArgBuf;
    vector< pair<string, double> > vecPairNextArgBuf;

    //step1. remove the null label
    vector< pair<string, double> > vecPairNNLMax;
    vector< pair<string, double> > vecPairNNLNext; 
    vector< pair<int, int> > vecPairNNLPS;
    RemoveNullLabel(vecPairMaxArgs, vecPairNextArgs, vecPairPS, vecPairNNLMax, vecPairNNLNext, vecPairNNLPS);

    // step 2. insert the args
    vector<int> vecItem;
    for (int index = 0; index < vecPairNNLPS.size(); index++)
    {
        InsertOneArg( vecPairNNLPS.at(index), vecPairNNLMax.at(index), vecPairNNLNext.at(index), vecPairPSBuf, vecPairMaxArgBuf, vecPairNextArgBuf ) ;
    }

    // step 3. insert predicate node
    if ( IsInsertPredicate(intPredicates, vecPairMaxArgBuf, vecPairPSBuf) )
    {
        pair<int, int> prPdPS;
        pair<string, double> prPdArg;
        prPdPS.first   = intPredicates;
        prPdPS.second  = intPredicates;
        prPdArg.first  = S_PD_ARG;
        prPdArg.second = 1;

        vecPairPSBuf.push_back(prPdPS);
        vecPairMaxArgBuf.push_back(prPdArg);
        vecPairNextArgBuf.push_back(prPdArg);
    }

    // step 4. post process
    PostProcess(vecPos, vecPairPS, vecPairMaxArgs, vecPairNextArgs, vecPairPSBuf, vecPairMaxArgBuf, vecPairNextArgBuf);

    // put into output vector
    for (int index = 0; index < vecPairPSBuf.size(); index++)
    {
        vecResultForOnePredicate.push_back(make_pair(vecPairMaxArgBuf[index].first, vecPairPSBuf[index]));
    }
}

void DepSRL::RemoveNullLabel(const vector< pair<string, double> >& vecPairMaxArgs, 
        const vector< pair<string, double> >& vecPairNextArgs, 
        const vector< pair<int, int> >& vecPairPS, 
        vector< pair<string, double> >& vecPairNNLMax, 
        vector< pair<string, double> >& vecPairNNLNext, 
        vector< pair<int, int> >& vecPairNNLPS) const
{
    vecPairNNLMax.clear();
    vecPairNNLNext.clear();
    vecPairNNLPS.clear();
    for (int index = 0; index < vecPairMaxArgs.size(); index++)
    {
        if ( vecPairMaxArgs.at(index).first.compare(NIL_LABEL) )
        {
            vecPairNNLMax.push_back(vecPairMaxArgs.at(index));
            vecPairNNLNext.push_back(vecPairNextArgs.at(index));
            vecPairNNLPS.push_back(vecPairPS.at(index));
        }
    }
}

void DepSRL::InsertOneArg(const pair<int, int>& pArgPS, 
        const pair<string, double>& pMaxArg, 
        const pair<string, double>& pNextArg,                                    
        vector< pair<int, int> >& vecPairPSBuf, 
        vector< pair<string, double> >& vecPairMaxArgBuf, 
        vector< pair<string, double> >& vecPairNextArgBuf) const
{
    // 2.1. process the collision
    vector<int> vctCol;
    FindCollisionCand(vecPairPSBuf, pArgPS, vctCol);
    if ( !IsInsertNColLabel(vctCol, pMaxArg, vecPairMaxArgBuf, vecPairNextArgBuf, vecPairPSBuf) ) 
    {
        // insert current node
        // vecPairMaxArgBuf.push_back(pMaxArg);
        // vecPairNextArgBuf.push_back(pNextArg);
        // vecPairPSBuf.push_back(pArgPS);

        // process next arg
        return;
    }

    // 2.2. process the same args
    vector<int> vctSame;
    vector<int> vctSameDel;
    FindSameLabelCand(vecPairMaxArgBuf, pMaxArg, vctSame);
    if ( !IsInsertSameLabel(vctSame, pMaxArg, vecPairMaxArgBuf, vecPairNextArgBuf, vecPairPSBuf, vctSameDel) )
    {
        // insert current node
        // vecPairMaxArgBuf.push_back(pMaxArg);
        // vecPairNextArgBuf.push_back(pNextArg);
        // vecPairPSBuf.push_back(pArgPS);

        // process next arg
        return; 
    }

    // 2.3 insert current node
    // remove collisions and same-args
    // BOOST_FOREACH (int id, vctCol) {
    for(int id = 0; id < vctCol.size(); id++) {
        vecPairMaxArgBuf[id].second  = -1;
        vecPairNextArgBuf[id].second = -1;
        vecPairPSBuf[id].second      = -1;
    }
    // BOOST_FOREACH (int id, vctSameDel) {
    for(int id = 0; id < vctSameDel.size(); id++) {
        vecPairMaxArgBuf[id].second  = -1;
        vecPairNextArgBuf[id].second = -1;
        vecPairPSBuf[id].second      = -1;
    }
    vecPairMaxArgBuf.erase(
            remove_if(
                vecPairMaxArgBuf.begin(),
                vecPairMaxArgBuf.end(),
                boost::bind(
                    less<double>(),
                    boost::bind(
                        &pair<string,double>::second,
                        _1
                        ),
                    0
                    )
                ),
            vecPairMaxArgBuf.end()
            );
    vecPairNextArgBuf.erase(
            remove_if(
                vecPairNextArgBuf.begin(),
                vecPairNextArgBuf.end(),
                boost::bind(
                    less<double>(),
                    boost::bind(
                        &pair<string,double>::second,
                        _1
                        ),
                    0
                    )
                ),
            vecPairNextArgBuf.end()
            );
    vecPairPSBuf.erase(
            remove_if(
                vecPairPSBuf.begin(),
                vecPairPSBuf.end(),
                boost::bind(
                    less<int>(),
                    boost::bind(
                        &pair<int,int>::second,
                        _1
                        ),
                    0
                    )
                ),
            vecPairPSBuf.end()
            );
    vecPairMaxArgBuf.push_back(pMaxArg);
    vecPairNextArgBuf.push_back(pNextArg);
    vecPairPSBuf.push_back(pArgPS);
}

bool DepSRL::IsInsertPredicate(int intPredicate, 
        vector< pair<string, double> >& vecPairMaxArgBuf, 
        vector< pair<int, int> >& vecPairPSBuf) const
{
    for(int index = 0; index < vecPairPSBuf.size(); index++)
    {
        if ( (vecPairPSBuf.at(index).first <= intPredicate) &&
                (vecPairPSBuf.at(index).second >= intPredicate) )
        {
            vecPairPSBuf.at(index).first = intPredicate;
            vecPairPSBuf.at(index).second = intPredicate;
            vecPairMaxArgBuf.at(index).first = S_PD_ARG;
            vecPairMaxArgBuf.at(index).second = 1;

            return 0;
        }
    }

    return 1;
}

/*
void DepSRL::TransVector(const vector<const char*>& vecInStr, 
                               vector<string>& vecOutStr) const
{
    vector<const char*>::const_iterator itInStr;
    itInStr = vecInStr.begin();
    while (itInStr != vecInStr.end()) 
    {
        vecOutStr.push_back(*itInStr);
        itInStr++;
    }
}
*/


void DepSRL::PostProcess(const vector<string>& vecPos,
        const vector< pair<int, int> >& vecPairPS,
        const vector< pair<string, double> >& vecPairMaxArgs,
        const vector< pair<string, double> >& vecPairNextArgs,
        vector< pair<int, int> >& vecPairPSBuf,
        vector< pair<string, double> >& vecPairMaxArgsBuf,
        vector< pair<string, double> >& vecPairNextArgsBuf) const
{
    // step 1. process QTY args
    QTYArgsProcess(vecPos, vecPairPSBuf, vecPairMaxArgsBuf, vecPairNextArgsBuf);

    // step 2. process PSR-PSE arg
    PSERArgsProcess(S_ARG0_TYPE, vecPos, vecPairPS, vecPairMaxArgs, vecPairNextArgs, vecPairPSBuf, vecPairMaxArgsBuf, vecPairNextArgsBuf);
}


void DepSRL::FindCollisionCand(const vector< pair<int, int> >& vecPairPSCands, 
        const pair<int, int>& pairCurPSCand, 
        vector<int>& vecPairColPSCands) const
{
    vecPairColPSCands.clear();
    for (int index = 0; index < vecPairPSCands.size(); index++)
    {
        if ( ((pairCurPSCand.first >= vecPairPSCands.at(index).first) && (pairCurPSCand.first <= vecPairPSCands.at(index).second)) ||
                ((pairCurPSCand.second >= vecPairPSCands.at(index).first) && (pairCurPSCand.second <= vecPairPSCands.at(index).second)) || 
                ((pairCurPSCand.first <= vecPairPSCands.at(index).first) && (pairCurPSCand.second >= vecPairPSCands.at(index).second)) )
        {
            vecPairColPSCands.push_back(index);
        }
    }
}

// format: (argType, argProp)
void DepSRL::FindSameLabelCand(
        const vector< pair<string, double> >& vecPairArgCands,
        const pair<string, double>& pairCurArgCand,
        vector<int>& vecPairSameArgCands) const
{
    vecPairSameArgCands.clear();
    for (int index = 0; index < vecPairArgCands.size(); index++)
    {
        if ( !pairCurArgCand.first.compare(vecPairArgCands.at(index).first) )
        {
            vecPairSameArgCands.push_back(index);
        }
    }
}

void DepSRL::QTYArgsProcess(
        const vector<string>& vecPos,
        vector< pair<int, int> >& vecPairPSBuf,
        vector< pair<string, double> >& vecPairMaxArgsBuf,
        vector< pair<string, double> >& vecPairNextArgsBuf) const
{
    vector< pair<int, int> > vecPairPSTemp(vecPairPSBuf);
    vector< pair<string, double> > vecPairMaxArgsTemp(vecPairMaxArgsBuf);
    vector< pair<string, double> > vecPairNextArgsTemp(vecPairNextArgsBuf);

    vecPairPSBuf.clear();
    vecPairMaxArgsBuf.clear();
    vecPairNextArgsBuf.clear();
    // process rule : if (arg_type is "*-QTY") then the pos_pattern must (AD|CD|M)+
    // else must process: if next arg_type is "NULL" then drop this candidate
    // else replace with the next arg_type
    for (int index = 0; index < vecPairPSTemp.size(); index++)
    {
        if ( (vecPairMaxArgsTemp.at(index).first.find(S_QTY_ARG) != string::npos) &&
                !IsPosPattern(vecPairPSTemp.at(index).first, vecPairPSTemp.at(index).second, vecPos, S_QTY_POS_PAT) )
        {
            if ( !vecPairNextArgsTemp.at(index).first.compare(S_NULL_ARG) )
            {
                continue;
            }
            else
            {
                vecPairMaxArgsTemp.at(index) = vecPairNextArgsTemp.at(index);
            }
        }

        // add to candidate
        vecPairPSBuf.push_back(vecPairPSTemp.at(index));
        vecPairMaxArgsBuf.push_back(vecPairMaxArgsTemp.at(index));
        vecPairNextArgsBuf.push_back(vecPairNextArgsTemp.at(index));
    }
}

void DepSRL::PSERArgsProcess(
        const string& strArgPrefix,
        const vector<string>& vecPos, 
        const vector< pair<int, int> >& vecPairPS,
        const vector< pair<string, double> >& vecPairMaxArgs,
        const vector< pair<string, double> >& vecPairNextArgs,
        vector< pair<int, int> >& vecPairPSBuf, 
        vector< pair<string, double> >& vecPairMaxArgsBuf, 
        vector< pair<string, double> >& vecPairNextArgsBuf) const
{
    vector<int> vecPSRIndex;
    vector<int> vecPSEIndex;
    pair<int, int> pArgPS;
    pair<string, double> pMaxArg;
    pair<string, double> pNextArg;

    string psrArgType = strArgPrefix + S_HYPHEN_TAG + S_PSR_ARG;
    string pseArgType = strArgPrefix + S_HYPHEN_TAG + S_PSE_ARG;
    // step 1. find the PSR and PSE args index
    for (int index = 0; index < vecPairPSBuf.size(); index++)
    {
        if (vecPairMaxArgsBuf.at(index).first.find(psrArgType) != string::npos)
        {
            vecPSRIndex.push_back(index);
        }

        if (vecPairMaxArgsBuf.at(index).first.find(pseArgType) != string::npos)
        {
            vecPSEIndex.push_back(index);
        }
    }

    // step 2. check if matched
    if ( vecPSRIndex.empty() &&
            !vecPSEIndex.empty() )
    {
        // process the PSE args
        if ( IsMaxPropGreaterThreshold(I_ARG_THRESHOLD_VAL, vecPSEIndex, vecPairMaxArgsBuf) &&
                FindArgFromDropCand(psrArgType, vecPairPS, vecPairMaxArgs, vecPairNextArgs, pArgPS, pMaxArg, pNextArg) )
        {
            //find the matched arg-type
            InsertOneArg( pArgPS, pMaxArg, pNextArg, vecPairPSBuf, vecPairMaxArgsBuf, vecPairNextArgsBuf );
        }
    }
    else if ( !vecPSRIndex.empty() &&
            vecPSEIndex.empty() )
    {
        // process the PSR args
        // process the PSE args
        if ( IsMaxPropGreaterThreshold(I_ARG_THRESHOLD_VAL, vecPSRIndex, vecPairMaxArgsBuf) &&
                FindArgFromDropCand(pseArgType, vecPairPS, vecPairMaxArgs, vecPairNextArgs, pArgPS, pMaxArg, pNextArg) )
        {
            //find the matched arg-type
            InsertOneArg( pArgPS, pMaxArg, pNextArg, vecPairPSBuf, vecPairMaxArgsBuf, vecPairNextArgsBuf );
        }
    }

}

bool DepSRL::FindArgFromDropCand(
        const string& strArgPat,
        const vector< pair<int, int> >& vecPairPS,
        const vector< pair<string, double> >& vecPairMaxArgs,
        const vector< pair<string, double> >& vecPairNextArgs,
        pair<int, int>& pArgPS,
        pair<string, double>& pMaxArg,
        pair<string, double>& pNextArg) const
{
    int maxIndex = -1;
    int flag = -1;
    double maxProp = 0;

    for (int index = 0; index < vecPairPS.size(); index++)
    {
        if ( (vecPairMaxArgs.at(index).first.find(strArgPat) != string::npos) &&
                (vecPairMaxArgs.at(index).second > maxProp) )
        {
            maxIndex = index;
            maxProp = vecPairMaxArgs.at(index).second;
            flag = 1;
        }
        else if ( (vecPairNextArgs.at(index).first.find(strArgPat) != string::npos) &&
                (vecPairNextArgs.at(index).second > maxProp) )
        {
            maxIndex = index;
            maxProp = vecPairNextArgs.at(index).second;
            flag = 0;
        }
    }

    if ( (flag == -1) || (maxProp < 0.01) )
    {
        return 0;
    }
    else if (flag == 1)
    {
        pMaxArg = vecPairMaxArgs.at(maxIndex);
    }
    else
    {
        pMaxArg = vecPairNextArgs.at(maxIndex);
    }

    pArgPS = vecPairPS.at(maxIndex);
    pNextArg = vecPairNextArgs.at(maxIndex);
    return 1;
}

void DepSRL::ReplaceArgFromNextProp(
        const vector<int>& vecIndex,
        vector< pair<int, int> >& vecPairPSBuf, 
        vector< pair<string, double> >& vecPairMaxArgsBuf, 
        vector< pair<string, double> >& vecPairNextArgsBuf) const
{
    int delIndex = 0;
    // if next arg_type is "NULL" then drop this candidate
    // else replace with the next arg_type
    for (int index = 0; index < vecIndex.size(); index++)
    {
        if ( !vecPairNextArgsBuf.at(vecIndex.at(index)).first.compare(S_NULL_ARG) )
        {
            vecPairPSBuf.erase( vecPairPSBuf.begin() + vecIndex.at(index) - delIndex );
            vecPairMaxArgsBuf.erase( vecPairMaxArgsBuf.begin() + vecIndex.at(index) - delIndex );
            vecPairNextArgsBuf.erase( vecPairNextArgsBuf.begin() + vecIndex.at(index) - delIndex );

            delIndex++;
        }
        else
        {
            vecPairMaxArgsBuf.at(vecIndex.at(index)) = vecPairNextArgsBuf.at(vecIndex.at(index));
        }
    }
}

bool DepSRL::IsPosPattern(
        int intBegin,
        int intEnd,
        const vector<string>& vecPos,
        const string& strPattern) const
{
    vector<string> vecItem;
    boost::algorithm::split(vecItem, strPattern, boost::is_any_of("|"));

    for (int index = intBegin; index < intEnd; index++)
    {
        if ( find(vecItem.begin(), vecItem.end(), vecPos.at(index)) == vecItem.end() )
        {
            return 0;
        }
    }

    return 1;
}

bool DepSRL::IsMaxPropGreaterThreshold(
        double dThreSholdVal,
        const vector<int>& vecIndex,
        const vector< pair<string, double> >& vecPairMaxArgsBuf) const
{
    vector<int>::const_iterator itIndex;

    itIndex = vecIndex.begin();
    while (itIndex != vecIndex.end())
    {
        if (vecPairMaxArgsBuf.at(*itIndex).second >= dThreSholdVal)
        {
            return 1;
        }

        ++ itIndex;
    }

    return 0;
}


bool DepSRL::IsInsertNColLabel(
        const vector<int>& vecCol,
        const pair<string, double>& pArgCand,
        vector< pair<string, double> >& vecPairMaxArgBuf,
        vector< pair<string, double> >& vecPairNextArgBuf,
        vector< pair<int, int> >& vecPairPSBuf) const
{
    int id;
    int isPSColInsert = 1;
    if ( !vecCol.empty() )
    {
        for (id = 0; id < vecCol.size(); id++)
        {
            // P(Ci) > P(A), no insert
            if ( vecPairMaxArgBuf.at(vecCol.at(id)).second > pArgCand.second)
            {
                // isPSColInsert = 0;
                // break;
                return 0;
            }
        }

        /*
        // delete the collision nodes
        if (isPSColInsert)
        {
            for (id = 0; id < vecCol.size(); id++)
            {
                vecPairMaxArgBuf.erase(vecPairMaxArgBuf.begin() + vecCol.at(id) - id);
                vecPairNextArgBuf.erase(vecPairNextArgBuf.begin() + vecCol.at(id) - id );
                vecPairPSBuf.erase(vecPairPSBuf.begin() + vecCol.at(id) - id);
            }

            return 1;
        }

        return 0;
        */

    }

    return 1;
}

bool DepSRL::IsInsertSameLabel(
        const vector<int>& vecSame, 
        const pair<string, double>& pArgCand,
        vector< pair<string, double> >& vecPairMaxArgBuf, 
        vector< pair<string, double> >& vecPairNextArgBuf,
        vector< pair<int, int> >& vecPairPSBuf,
        vector<int>& vecSameDel) const
{
    int id;
    int isArgSameInsert = 1;

    // P(A) <  0.4
    if (pArgCand.second < 0.4)
    {
        isArgSameInsert = 0;
    }

    if ( !vecSame.empty() )
    {
        for (id = 0; id < vecSame.size(); id++)
        {
            // P(Ei) < P(A) < 0.5, insert
            if ( (vecPairMaxArgBuf.at(vecSame.at(id)).second < 0.5) &&
                    (vecPairMaxArgBuf.at(vecSame.at(id)).second < pArgCand.second) )
            {
                vecSameDel.push_back(vecSame.at(id));
                isArgSameInsert = 1;
            }
        }

        //delete the  small prob nodes
        if (isArgSameInsert)
        {
            // for (id = 0; id < vecArgDel.size(); id++)
            // {
            //     vecPairMaxArgBuf.erase(vecPairMaxArgBuf.begin() + vecArgDel.at(id) - id);
            //     vecPairNextArgBuf.erase(vecPairNextArgBuf.begin() + vecArgDel.at(id) - id);
            //     vecPairPSBuf.erase(vecPairPSBuf.begin() + vecArgDel.at(id) - id);
            // }

            return 1;
        }

        return 0;
    }
    else
    {
        return 1;
    }

}

int DepSRL::RenameArguments(
        vector< pair< int, vector< pair< string, pair< int, int > > > > > &vecSRLResult
        )
{
    for (vector< pair< int, vector< pair< string, pair< int, int > > > > >::iterator 
            predicate_iter = vecSRLResult.begin();
            predicate_iter != vecSRLResult.end();
            ++predicate_iter
        )
    {
        for(vector< pair< string, pair< int, int > > >::iterator
                argument_iter = predicate_iter->second.begin();
                argument_iter != predicate_iter->second.end();
                ++argument_iter
           )
        {
            if (argument_iter->first.substr(0,3) == "ARG") {
                argument_iter->first = "A" + argument_iter->first.substr(3);
            }
        }
    }

    return 1;
}

void DepSRL::manageConfigPath(ModelConf &config, const string &dirPath) {
    config.model = dirPath + '/' + config.model;
    config.indexPath = dirPath + '/' + config.indexPath;
}

inline int DepSRL::subTreeBegin(int index, const vector<pair<int, string> > &parse) {
    int minSubTreeIndex = index;
    while (isOnTree(minSubTreeIndex - 1, index, parse)) minSubTreeIndex--;
    return minSubTreeIndex;
}

inline int DepSRL::subTreeEnd(int index, const vector<pair<int, string> > &parse) {
    int maxSubTreeIndex = index;
    while (isOnTree(maxSubTreeIndex + 1, index, parse)) maxSubTreeIndex++;
    return maxSubTreeIndex;
}

inline bool DepSRL::isOnTree(int index, int root, const vector<pair<int, string> > &parse) {
    for (; 0 <= index && index < parse.size(); index = parse[index].first) {
        if (index == root) return true;
    }

    return false;
}
