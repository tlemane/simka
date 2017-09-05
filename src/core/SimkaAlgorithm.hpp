/*****************************************************************************
 *   Simka: Fast kmer-based method for estimating the similarity between numerous metagenomic datasets
 *   A tool from the GATB (Genome Assembly Tool Box)
 *   Copyright (C) 2015  INRIA
 *   Authors: G.Benoit, C.Lemaitre, P.Peterlongo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_
#define TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_

#include <gatb/gatb_core.hpp>
#include <gatb/kmer/impl/RepartitionAlgorithm.hpp>
#include<stdio.h>
#include <gatb/kmer/impl/Model.hpp>

//#define PRINT_STATS
#define CHI2_TEST
//#define SIMKA_POTARA
//#define BOOTSTRAP
#define MAX_BOOTSTRAP 50
#define NB_BOOTSTRAP 45
//#define SIMKA_FUSION
//#define MULTI_PROCESSUS
//#define MULTI_DISK
//#define SIMKA_MIN
#include "SimkaDistance.hpp"

const string STR_SIMKA_SOLIDITY_PER_DATASET =           "-solidity-single";
const string STR_SIMKA_MAX_READS =                      "-max-reads";
const string STR_SIMKA_MIN_READ_SIZE =                  "-min-read-size";
const string STR_SIMKA_MIN_READ_SHANNON_INDEX =         "-read-shannon-index";
const string STR_SIMKA_MIN_KMER_SHANNON_INDEX =         "-kmer-shannon-index";
const string STR_KMER_PER_READ =                        "-kmer-per-read";
const string STR_SIMKA_COMPUTE_ALL_SIMPLE_DISTANCES=    "-simple-dist";
const string STR_SIMKA_COMPUTE_ALL_COMPLEX_DISTANCES =  "-complex-dist";
const string STR_SIMKA_KEEP_TMP_FILES =                 "-keep-tmp";
const string STR_SIMKA_COMPUTE_DATA_INFO =              "-data-info";

enum SIMKA_SOLID_KIND{
    RANGE,
    SUM,
};



typedef u_int16_t bankIdType;

























class SimkaCounterBuilder
{
public:
    
    /** Constructor.
     * \param[in] nbBanks : number of banks parsed during kmer counting.
     */
    SimkaCounterBuilder (size_t nbBanks=1)  :  _abundancePerBank(nbBanks)  {}
    
    /** Get the number of banks.
     * \return the number of banks. */
    size_t size() const  { return _abundancePerBank.size(); }
    
    /** Initialization of the counting for the current kmer. This method should be called
     * when a kmer is seen for the first time.
     * \param[in] idxBank : bank index where the new current kmer has been found. */
    void init (size_t idxBank=0)
    {
        for (size_t k=0; k<_abundancePerBank.size(); k++)  { _abundancePerBank[k]=0; }
        _abundancePerBank [idxBank]= 1;
    }
    
    /** Increase the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank */
    void increase (size_t idxBank=0)  {  _abundancePerBank [idxBank] ++;  }
    
    /** Set the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank */
    void set (CountNumber val, size_t idxBank=0)  {  _abundancePerBank [idxBank] = val;  }
    
    /** Get the abundance of the current kmer for the provided bank index.
     * \param[in] idxBank : index of the bank
     * \return the abundance of the current kmer for the given bank. */
    CountNumber operator[] (size_t idxBank) const  { return _abundancePerBank[idxBank]; }
    
    /** */
    const CountVector& get () const { return _abundancePerBank; }
    
private:
    CountVector _abundancePerBank;
};






/*********************************************************************
 * ** SimkaCountProcessor
 *********************************************************************/
template<size_t span>
class SimkaCountProcessorSimple{
    
private:
    
    size_t _nbBanks;
    size_t _kmerSize;
    //pair<CountNumber, CountNumber> _abundanceThreshold;
    //bool isAbundanceThreshold;
    
    SimkaStatistics* _stats;
    double _totalAbundance;
    
    u_int64_t _nbKmerCounted;
    double _minKmerShannonIndex;
    
    //vector<size_t> _banksOks;
    
    vector<u_int16_t> _sharedBanks;
    string _outputDir;
    size_t _partitionId;
    ofstream _outputPartitionFile;
    
#ifdef CHI2_TEST
    
    typedef std::pair<u_int64_t, CountVector> minimizer_Abundances;               //minimisers_abundances;
//    struct compare_those_minimisers { bool operator() (float l,float r) { return r < l; } } ;
    std::map< float, minimizer_Abundances>    ch2_to_minimisers_abundances;       //ch2 --> minimizer_Abundances
    std::unordered_map<u_int64_t, float>                                    stored_minimisers_xi2;              //minimiser --> xi2
//    typedef Kmer<span>::ModelMinimizer<Kmer<>::ModelCanonical>      ModelMinimizer;
//    ModelMinimizer mm;
    typedef typename Kmer<span>::ModelCanonical Model;
    typedef typename Kmer<span>::template ModelMinimizer<Model>  ModelMini;
    ModelMini modelMini;
    
#endif // CHI2_TEST
    size_t _maxChi2Values;
    
public:
    
    typedef typename Kmer<span>::Type  Type;
    //typedef typename Kmer<span>::Count Count;
    
    SimkaCountProcessorSimple(SimkaStatistics* stats, size_t nbBanks, size_t kmerSize, const pair<size_t, size_t>& abundanceThreshold, SIMKA_SOLID_KIND solidKind, bool soliditySingle, double minKmerShannonIndex, const string& outputDir, size_t partitionId) :
    _stats(stats),
    #ifdef CHI2_TEST 
    modelMini(kmerSize,8)
    #endif
    {
    	_outputDir = outputDir;
    	_partitionId = partitionId;
    	string outputFilename = _outputDir + "/select_kmers_out_" + Stringify::format("%i", _partitionId) + ".txt";
    	_outputPartitionFile.open(outputFilename.c_str());

        _maxChi2Values = 1000;
        // We configure the vector for the N.(N+1)/2 possible pairs
        //_countTotal.resize (_nbBanks*(_nbBanks+1)/2);
        
        _nbBanks = nbBanks;
        _kmerSize = kmerSize;
        //_abundanceThreshold = abundanceThreshold;
        _minKmerShannonIndex = minKmerShannonIndex;
        
        //_localStats = new SimkaStatistics(_nbBanks, _stats._distanceParams);
        
        _nbKmerCounted = 0;
        //isAbundanceThreshold = _abundanceThreshold.first > 1 || _abundanceThreshold.second < 1000000;
        
    }
    
    
    void end(){
#ifdef CHI2_TEST
        

//        size_t nbValues = ch2_to_minimisers_abundances.size();
        std::map< float, minimizer_Abundances>::iterator it; // std::map< float, minimizer_Abundances, ch2_to_minimisers_abundancesFunction>
        for (it = ch2_to_minimisers_abundances.begin(); it!=ch2_to_minimisers_abundances.end(); ++it){

        	CountVector& counts = it->second.second;
            updateDistance(counts);

            string outLine = "";
            for(size_t i=0; i<counts.size(); i++){
            	CountNumber count = counts[i];
            	string countStr = Stringify::format("%i", count);
            	outLine += countStr + " ";
            }

            outLine.erase(outLine.size()-1); //remove last space
            outLine += "\n";

        	_outputPartitionFile.write(outLine.c_str(), outLine.size());
        }

        _outputPartitionFile.close();
//        for(size_t i=0; i<nbValues; i++){
//            double val = ch2_to_minimisers_abundances.top().first;
//            CountVector counts = ch2_to_minimisers_abundances.top().second;
//            
//            
//            //cout << val << endl;
//            //for(size_t j=0; j<counts.size(); j++){
//            //	cout << counts[j] << " ";
//            //}
//            //cout << endl;
//            
//            updateDistance(counts);
//            
//            ch2_to_minimisers_abundances.pop();
//        }
#endif // CHI2_TEST
    }
    
    void process (size_t partId, const typename Kmer<span>::Type& kmer, const CountVector& counts){
        
        //cout << kmer.toString(_kmerSize) << endl;
        //for(size_t i=0; i<counts.size(); i++){
        //	cout << counts[i] << " ";
        //}
        //cout << endl;
        
#ifdef PRINT_STATS
        _totalAbundance = 0;
        _stats->_nbDistinctKmers += 1;
        
        
        for(size_t i=0; i<counts.size(); i++){
            
            CountNumber abundance = counts[i];
            //_nbKmerCounted += abundance;
            //_stats._speciesAbundancePerDataset[i].push_back(abundance);
            
            //cout << counts[i] << " ";
            _stats->_nbKmers += abundance;
            _stats->_nbKmersPerBank[i] += abundance;
            _totalAbundance += abundance;
        }
#endif
        
        /* A DEPLACER PENDANT LE COMPTAGE DES KMERS
         if(_minKmerShannonIndex != 0){
         double shannonIndex = getShannonIndex(kmer);
         if(shannonIndex < _minKmerShannonIndex){
         return;
         }
         }*/
        
#ifdef CHI2_TEST // CF NOTE https://www.evernote.com/l/ARU0wJUAfI1Ky5MRVADaNLbjYZ4o1joXJOo
        //        std::cout << "minimizer value is         : " << kmer.minimizer().value()     << std::endl;
        //// Xi2 computation ////
        _totalAbundance = 0;
        for(size_t i=0; i<counts.size(); i++){
            _totalAbundance += counts[i];
        }
        float X2j = 0;
        for(size_t i=0; i<counts.size(); i++){
            X2j += pow((counts[i]/_totalAbundance - _stats->_datasetNbReads[i]/_stats->_totalReads), 2) / (_stats->_datasetNbReads[i] / (_stats->_totalReads*_totalAbundance));
        }
        //cout << X2j << endl;
        
        //u_int64_t thisminimiser = 0;//kmer.minimizer().value(); /// TODO HERE: GET THE KMER MINIMISER
        u_int64_t thisminimiser = modelMini.getMinimizerValue(kmer);
        
        std::unordered_map<u_int64_t, float>::iterator storedM = stored_minimisers_xi2.find(thisminimiser);
        if (storedM != stored_minimisers_xi2.end()){                                                                            // this minimiser exists - only one occurrence of a minimiser
            if (storedM->second <X2j){                                                                                          // the stored minimizer has lower Xi2 value, we must replace it
                std::map< float, minimizer_Abundances>::const_iterator storedkmerIt = ch2_to_minimisers_abundances.find(X2j);   // get all minimisers with this Xi2 (there me be more than one)
                while(storedkmerIt != ch2_to_minimisers_abundances.end() && storedkmerIt->first == X2j){                        // find the stored minimiser with value kmer.minimizer().value()
                    if(storedkmerIt->second.first == thisminimiser){                                                            // found it
                        ch2_to_minimisers_abundances.erase(storedkmerIt);                                                       // remove this previous element (in order to maintain the map ordered)
                        ch2_to_minimisers_abundances.insert({X2j, minimizer_Abundances(thisminimiser,counts)});                 // add the new element X2J --> <minimiser, counts>
                        storedM->second=X2j;                                                                                    // update the stored X2J in the map minimisers --> map
                        break;                                                                                                  // we made the modification for this minimiser, we can stop searching the list
                        // TODO : we must pass exactly once here, this should be asserted
                    }
                    storedkmerIt++;                                                                                             // try next equal minimiser or leave -- TODO : we could check here that we found a least one value (assert)
                }
            }
            
        }
        
        else { // this minimiser does not exist - if the set ch2_to_minimisers_abundances is full (ch2_to_minimisers_abundances.size() == _maxChi2Values) then we may replace the lowest minimiser value
            if(ch2_to_minimisers_abundances.size() > _maxChi2Values){                                                           // ch2_to_minimisers_abundances is full.
                if(X2j > ch2_to_minimisers_abundances[0].first){                                                                // Smallest xi is smaller than X2J. We replace it.
                    u_int64_t toremove_minimiser = ch2_to_minimisers_abundances[0].first;                                       // shortcut
                    stored_minimisers_xi2.erase(stored_minimisers_xi2.find(toremove_minimiser));                                // Remove in the stored_minimisers_xi2 the minimiser having this smaller xi2 value
                    stored_minimisers_xi2.insert({thisminimiser,X2j});                                                          // add the correct minimiser.
                    ch2_to_minimisers_abundances.erase(ch2_to_minimisers_abundances.begin());                                   // remove this smallest element
                    ch2_to_minimisers_abundances.insert({X2j, minimizer_Abundances(thisminimiser,counts)});                     // add the new element X2J --> <minimiser, counts>
                }
            }
            else {                                                                                                              // just add the new minimiser
                stored_minimisers_xi2.insert({thisminimiser,X2j});                                                              // add the correct minimiser
                ch2_to_minimisers_abundances.insert({X2j, minimizer_Abundances(thisminimiser,counts)});                         // add the new element X2J --> <minimiser, counts>
            }
        }
        
        
        
        
        //cout << ch2_to_minimisers_abundances.size() << "    " << X2j << "   " << ch2_to_minimisers_abundances.top() << endl;
        //cout <<  X2j << "    " << pvalue << endl;
        
        return;
        
#endif // CHI2_TEST
        /*
         //for(size_t i=0; i<_datasetNbReads.size(); i++)
         //	cout << i << " " << _datasetNbReads[i] << endl;
         
         //cout << _totalReads << " " << _totalAbundance << endl;
         //float Ri = 500000;
         //float Rtotal = Ri * _nbBanks;
         //float Ntotal = _totalAbundance;
         float X2j = 0;
         for(size_t i=0; i<counts.size(); i++){
         
         float Ni = counts[i];
         
         X2j += pow((Ni/_totalAbundance - _datasetNbReads[i]/_totalReads), 2) / (_datasetNbReads[i] / (_totalReads*_totalAbundance));
         }
         
         //cout << X2j << endl;
         //if(_totalAbundance == 1){
         //	cout << X2j << endl;
         //}
         if(X2j <= (_nbBanks-1)*1.7) return false;
         */
        
        //cout << X2j << endl;
        
        
        //cout << kmer.toString(31) << endl;
        
        //cout << endl;
        
        //if(_progress){ //Simka_min
        //	_stats->_nbSolidKmers += 1;
        //	computeStats(counts);
        //}
        //else{
        
        updateDistance(counts);
        
        //else
        //	computeStats(counts);
        
        
        //_stats->_nbSolidKmers += 1;
    }
    
    void updateDistance(const CountVector& counts){
        _sharedBanks.clear();
        
        for(size_t i=0; i<counts.size(); i++)
            if(counts[i]) _sharedBanks.push_back(i);
        
        updateDistanceDefault(counts);
        
        if(_stats->_computeSimpleDistances)
            updateDistanceSimple(counts);
        
        if(_stats->_computeComplexDistances)
            updateDistanceComplex(counts);
    }
    
    void updateDistanceDefault(const CountVector& counts){
        
        
        for(size_t ii=0; ii<_sharedBanks.size(); ii++){
            for(size_t jj=ii+1; jj<_sharedBanks.size(); jj++){
                
                u_int16_t i = _sharedBanks[ii];
                u_int16_t j = _sharedBanks[jj];
                size_t symetricIndex = j + ((_nbBanks-1)*i) - (i*(i-1)/2);
                
                u_int64_t abundanceI = counts[i];
                u_int64_t abundanceJ = counts[j];
                
                _stats->_matrixNbSharedKmers[i][j] += counts[i];
                _stats->_matrixNbSharedKmers[j][i] += counts[j];
                _stats->_matrixNbDistinctSharedKmers[symetricIndex] += 1;
                
                //cout << i << " " << j << "    " << (j + ((_nbBanks-1)*i) - (i*(i-1)/2)) << endl;
                _stats->_brayCurtisNumerator[symetricIndex] += min(abundanceI, abundanceJ);
            }
        }
        
    }
    
    
    void updateDistanceSimple(const CountVector& counts){
        
        
        for(size_t ii=0; ii<_sharedBanks.size(); ii++){
            for(size_t jj=ii+1; jj<_sharedBanks.size(); jj++){
                
                u_int16_t i = _sharedBanks[ii];
                u_int16_t j = _sharedBanks[jj];
                
                u_int64_t abundanceI = counts[i];
                u_int64_t abundanceJ = counts[j];
                
                
                //cout << _stats->_chord_sqrt_N2[i] << endl;
                //_stats->_chord_NiNj[i][j] += abundanceI * abundanceJ;
                _stats->_chord_NiNj[i][j] += abundanceI * abundanceJ;
                _stats->_hellinger_SqrtNiNj[i][j] += sqrt(abundanceI * abundanceJ);
                _stats->_kulczynski_minNiNj[i][j] += min(abundanceI, abundanceJ);
            }
        }
        
    }
    
    void updateDistanceComplex(const CountVector& counts){
        
        
        //_sharedBanks.clear();
        
        //for(size_t i=0; i<counts.size(); i++)
        //	if(counts[i]) _sharedBanks.push_back(i);
        
        double xi = 0;
        double xj = 0;
        double d1 = 0;
        double d2 = 0;
        
        for(size_t i=0; i<counts.size(); i++){
            if(counts[i]){
                for(size_t j=i+1; j<counts.size(); j++){
                    
                    //In this loop we know that (abundanceI > 0)
                    double abundanceI = counts[i];
                    double abundanceJ = counts[j];
                    
                    if(abundanceJ){
                        
                        
                        //_stats->_matrixNbSharedKmers[i][j] += abundanceI;
                        //_stats->_matrixNbSharedKmers[j][i] += abundanceJ;
                        //_stats->_matrixNbDistinctSharedKmers[i][j] += 1;
                        //_stats->_chord_NiNj[i][j] += abundanceI * abundanceJ;
                        //_stats->_chord_NiNj[i][j] += (abundanceI * abundanceJ) / (_stats->_chord_sqrt_N2[i]*_stats->_chord_sqrt_N2[j]);
                        //_stats->_hellinger_SqrtNiNj[i][j] += sqrt(abundanceI * abundanceJ);
                        //_stats->_kulczynski_minNiNj[i][j] += min(abundanceI, abundanceJ);
                        
                        
                        double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                        double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                        xi = (double)abundanceI / _stats->_nbSolidKmersPerBank[i];
                        d1 = xi * log((2*xY) / (xY + yX));
                        
                        
                        //xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                        //yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                        xj = (double)abundanceJ / _stats->_nbSolidKmersPerBank[j];
                        d2 = xj * log((2*yX) / (xY + yX));
                    }
                    else{
                        d2 = 0;
                        double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                        double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                        xi = (double)abundanceI / _stats->_nbSolidKmersPerBank[i];
                        d1 = xi * log((2*xY) / (xY + yX));
                    }
                    
                    /*
                     if(abundanceI){
                     double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                     double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                     xi = (double)abundanceI / _stats->_nbSolidKmersPerBank[i];
                     d1 = xi * log((2*xY) / (xY + yX));
                     }
                     else{
                     d1 = 0;
                     }
                     
                     if(abundanceJ){
                     double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                     double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                     xj = (double)abundanceJ / _stats->_nbSolidKmersPerBank[j];
                     d2 = xj * log((2*yX) / (xY + yX));
                     }
                     else{
                     d2 = 0;
                     }*/
                    
                    _stats->_kullbackLeibler[i][j] += d1 + d2;
                    
                    _stats->_canberra[i][j] += abs(abundanceI - abundanceJ) / (abundanceI + abundanceJ);
                    //_stats->_brayCurtisNumerator[i][j] += abs(abundanceI - abundanceJ);
                    _stats->_whittaker_minNiNj[i][j] += abs((int)((u_int64_t)(abundanceI*_stats->_nbSolidKmersPerBank[j]) - (u_int64_t)(abundanceJ*_stats->_nbSolidKmersPerBank[i])));
                    
                    //cout << _stats->_nbSolidKmersPerBank[i] << endl;
                    
                    
                }
            }
            else{
                
                //Here, we know that (abundanceI == 0)
                
                for(size_t jj=0; jj<_sharedBanks.size(); jj++){
                    
                    u_int16_t j = _sharedBanks[jj];
                    if(i > j) continue;
                    
                    double abundanceI = counts[i];
                    double abundanceJ = counts[j];
                    
                    d1 = 0;
                    double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
                    double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
                    xj = (double)abundanceJ / _stats->_nbSolidKmersPerBank[j];
                    d2 = xj * log((2*yX) / (xY + yX));
                    
                    _stats->_kullbackLeibler[i][j] += d1 + d2;
                    
                    _stats->_canberra[i][j] += abs(abundanceI - abundanceJ) / (abundanceI + abundanceJ);
                    //_stats->_brayCurtisNumerator[i][j] += abs(abundanceI - abundanceJ);
                    //cout << _stats->_nbSolidKmersPerBank[i] << endl;
                    
                    _stats->_whittaker_minNiNj[i][j] += abs((int)((u_int64_t)(abundanceI*_stats->_nbSolidKmersPerBank[j]) - (u_int64_t)(abundanceJ*_stats->_nbSolidKmersPerBank[i])));
                    
                }
            }
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        /*
         return;
         
         double xi = 0;
         double xj = 0;
         double d1 = 0;
         double d2 = 0;
         
         #ifdef PRINT_STATS
         int nbBanksThatHaveKmer = 0;
         #endif
         //u_int64_t totalAbundance = 0;
         
         
         
         for(size_t i=0; i<counts.size(); i++){
         
         
         #ifdef PRINT_STATS
         if(abundanceI){
         nbBanksThatHaveKmer += 1;
         //_stats->_nbSolidDistinctKmersPerBank[i] += 1;
         //_stats->_nbSolidKmersPerBank[i] += abundanceI;
         //_stats->_chord_N2[i] += pow(abundanceI, 2);
         }
         #endif
         
         for(size_t j=i+1; j<counts.size(); j++){
         
         
         if(counts[i] || counts[j]){
         
         double abundanceI = counts[i];
         double abundanceJ = counts[j];
         
         if(abundanceI && abundanceJ){
         
         
         _stats->_matrixNbSharedKmers[i][j] += abundanceI;
         _stats->_matrixNbSharedKmers[j][i] += abundanceJ;
         _stats->_matrixNbDistinctSharedKmers[i][j] += 1;
         
         //_stats->_chord_NiNj[i][j] += abundanceI * abundanceJ;
         _stats->_chord_NiNj[i][j] += (abundanceI * abundanceJ) / (_stats->_chord_sqrt_N2[i]*_stats->_chord_sqrt_N2[j]);
         _stats->_hellinger_SqrtNiNj[i][j] += sqrt(abundanceI * abundanceJ);
         _stats->_kulczynski_minNiNj[i][j] += min(abundanceI, abundanceJ);
         
         
         
         _stats->_whittaker_minNiNj[i][j] += abs((int)((u_int64_t)(abundanceI*_stats->_nbSolidKmersPerBank[j]) - (u_int64_t)(abundanceJ*_stats->_nbSolidKmersPerBank[i])));
         
         
         }
         
         if(abundanceI){
         double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
         double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
         xi = (double)abundanceI / _stats->_nbSolidKmersPerBank[i];
         d1 = xi * log((2*xY) / (xY + yX));
         }
         else{
         d1 = 0;
         }
         
         if(abundanceJ){
         double xY = abundanceI * _stats->_nbSolidKmersPerBank[j];
         double yX = abundanceJ * _stats->_nbSolidKmersPerBank[i];
         xj = (double)abundanceJ / _stats->_nbSolidKmersPerBank[j];
         d2 = xj * log((2*yX) / (xY + yX));
         }
         else{
         d2 = 0;
         }
         
         _stats->_kullbackLeibler[i][j] += d1 + d2;
         
         _stats->_canberra[i][j] += abs(abundanceI - abundanceJ) / (abundanceI + abundanceJ);
         _stats->_brayCurtisNumerator[i][j] += abs(abundanceI - abundanceJ);
         //cout << _stats->_nbSolidKmersPerBank[i] << endl;
         
         _stats->_whittaker_minNiNj[i][j] += abs((int)((u_int64_t)(abundanceI*_stats->_nbSolidKmersPerBank[j]) - (u_int64_t)(abundanceJ*_stats->_nbSolidKmersPerBank[i])));
         }
         
         
         }
         
         }*/
        
#ifdef PRINT_STATS
        _stats->_nbDistinctKmersSharedByBanksThreshold[nbBanksThatHaveKmer-1] += 1;
        _stats->_nbKmersSharedByBanksThreshold[nbBanksThatHaveKmer-1] += _totalAbundance;
        
        if(_totalAbundance == 1){
            //if( == 1){
            _stats->_nbErroneousKmers += 1;
            //}
        }
        //else if(nbBanksThatHaveKmer == counter.size()){
        //}
#endif
        
    }
    
    //inline bool isSolidVector(const CountVector& counts);
    double getShannonIndex(const Type&  kmer){
        float index = 0;
        //float freq [5];
        
        vector<float> _freqs(4, 0);
        
        //char* seqStr = seq.getDataBuffer();
        
        for (size_t i=0; i<_kmerSize; i++){
            _freqs[kmer[i]] += 1.0;
            //seq[sizeKmer-i-1] = bin2NT [(*this)[i]];
        }
        
        // Frequency of each letter (A, C, G, T or N)
        //for(size_t i=0; i < seq.size(); i++)
        //	_freqs[nt2binTab[(unsigned char)seq[i]]] += 1.0;
        
        // Shannon index calculation
        for (size_t i=0; i<_freqs.size(); i++){
            _freqs[i] /= (float) _kmerSize;
            if (_freqs[i] != 0)
                index += _freqs[i] * log (_freqs[i]) / log(2);
        }
        return abs(index);
    }
    
    double approx_gamma(double Z)
    {
        const double RECIP_E = 0.36787944117144232159552377016147;  // RECIP_E = (E^-1) = (1.0 / E)
        const double TWOPI = 6.283185307179586476925286766559;  // TWOPI = 2.0 * PI
        
        double D = 1.0 / (10.0 * Z);
        D = 1.0 / ((12 * Z) - D);
        D = (D + Z) * RECIP_E;
        D = pow(D, Z);
        D *= sqrt(TWOPI / Z);
        
        return D;
    }
    
    static double igf(double S, double Z)
    {
        if(Z < 0.0)
        {
            return 0.0;
        }
        double Sc = (1.0 / S);
        Sc *= pow(Z, S);
        Sc *= exp(-Z);
        
        double Sum = 1.0;
        double Nom = 1.0;
        double Denom = 1.0;
        
        for(int I = 0; I < 200; I++)
        {
            Nom *= Z;
            S++;
            Denom *= S;
            Sum += (Nom / Denom);
        }
        
        return Sum * Sc;
    }
    
    double chisqr(int Dof, double Cv)
    {
        if(Cv < 0 || Dof < 1)
        {
            return 0.0;
        }
        double K = ((double)Dof) * 0.5;
        double X = Cv * 0.5;
        if(Dof == 2)
        {
            return exp(-1.0 * X);
        }
        
        double PValue = igf(K, X);
        //if(isnan(PValue) || isinf(PValue) || PValue <= 1e-8)
        //{
        //    return 1e-14;
        //}
        
        PValue /= approx_gamma(K);
        //PValue /= tgamma(K);
        
        return PValue;
        //return (1.0 - PValue);
    }
    
    
};













/********************************************************************************/
/**
 *
 */
template <class Item, typename Filter> class SimkaInputIterator : public Iterator<Item>
{
public:
    
    /** Constructor.
     * \param[in] ref : the referred iterator
     * \param[in] initRef : will call 'first' on the reference if true
     */
    SimkaInputIterator(Iterator<Item>* refs, size_t nbBanks, u_int64_t maxReads, Filter filter)
    :  _filter(filter), _mainref(0) {
        
        setMainref(refs);
        _ref = _mainref->getComposition()[0];
        _isDone = false;
        _nbDatasets = nbBanks;
        _nbBanks = _mainref->getComposition().size() / _nbDatasets;
        _maxReads = maxReads;
        _nbReadProcessed = 0;
        _currentBank = 0;
        _currentInternalBank = 0;
        _currentDataset = 0;
        
    }
    
    
    bool isFinished(){
        if(_currentDataset == _nbDatasets){
            _isDone = true;
            return true;
        }
        return false;
    }
    
    void nextDataset(){
        _currentDataset += 1;
        
        if(isFinished()) return;
        
        _currentBank = _currentDataset * _nbBanks;
        
        _currentInternalBank = 0;
        _nbReadProcessed = 0;
        
        if(isFinished()) return;
        
        _ref = _mainref->getComposition()[_currentBank];
        _isDone = false;
        first();
        //nextBank();
    }
    
    void nextBank(){
        //cout << "next bank" << endl;
        //cout << "next bank "<< endl;
        _currentInternalBank += 1;
        if(_currentInternalBank == _nbBanks){
            nextDataset();
        }
        else{
            _isDone = false;
            _currentBank += 1;
            _ref = _mainref->getComposition()[_currentBank];
            first();
        }
    }
    
    void first()
    {
        
        _ref->first();
        
        while (!_ref->isDone() && _filter(_ref->item())==false)
            _ref->next();
        
        _isDone = _ref->isDone();
        
        if(!_isDone) *(this->_item) = _ref->item();
        
    }
    
    void next(){
        
        
        if(isFinished()){
            _isDone = true;
            return;
        }
        
        //cout << "haha" << endl;
        
        _ref->next();
        while (!_ref->isDone() && _filter(_ref->item())==false) _ref->next();
        
        _isDone = _ref->isDone();
        
        //cout << "haha" << endl;
        //if(!_isDone){
        //cout << _currentBank << "  " << _isDone << endl;
        
        //}
        
        //cout << _nbReadProcessed << "  " << _currentBank << "    " << _nbBanks << "   " << _maxReads << endl;
        
        
        if(_isDone){
            if(isFinished()){
                //cout << _nbReadProcessed << endl;
                return;
            }
            else{
                //cout << _nbReadProcessed << endl;
                nextBank();
                if(isFinished()){
                    //cout << _nbReadProcessed << endl;
                    return;
                }
            }
        }
        else{
            *(this->_item) = _ref->item();
            _nbReadProcessed += 1;
        }
        
        if(_maxReads && _nbReadProcessed >= _maxReads){
            if(isFinished())
                return;
            else
                nextDataset();
        }
        
    }
    
    /** \copydoc  Iterator::isDone */
    bool isDone()  {  return _isDone;  }
    
    /** \copydoc  Iterator::item */
    Item& item ()  {  return *(this->_item);  }
    
    
private:
    
    bool            _isDone;
    size_t _currentBank;
    //vector<Iterator<Item>* > _refs;
    Iterator<Item>* _ref;
    size_t _nbBanks;
    u_int64_t _maxReads;
    Filter _filter;
    u_int64_t _nbReadProcessed;
    size_t _currentInternalBank;
    size_t _currentDataset;
    size_t _nbDatasets;
    
    Iterator<Item>* _mainref;
    void setMainref (Iterator<Item>* mainref)  { SP_SETATTR(mainref); }
};


struct SimkaSequenceFilter
{
    //u_int64_t _maxNbReads;
    //u_int64_t _maxNbReadsPerBank;
    //u_int64_t _nbReadProcessed;
    //CancellableIterator<Sequence>* _it;
    //int* _bankIndex;
    //int* _datasetIndex;
    
    
    SimkaSequenceFilter(size_t minReadSize, double minShannonIndex){
        //_maxNbReads = 0;
        //_nbReadProcessed = 0;
        _minReadSize = minReadSize;
        _minShannonIndex = minShannonIndex;
    }
    
#ifdef BOOTSTRAP
    vector<bool> _bootstraps;
    
    
    void setBootstrap(vector<bool>& bootstraps){
        _bootstraps = bootstraps;
        //for(size_t i=0; i<_bootstraps.size(); i++)
        //	cout << _bootstraps[i];
        //cout << endl << endl;
    }
    
#endif
    
    //void setMaxReads(u_int64_t maxReads){
    //	_maxNbReads = maxReads;
    //}
    
    //void setIt(CancellableIterator<Sequence>* it){
    //	_it = it;
    //}
    
    bool operator() (Sequence& seq){
        
        //cout << seq.toString() << endl;
        //cout << _nbReadProcessed << endl;
        //if(_maxNbReads != 0){
        //	if(_nbReadProcessed >= _maxNbReads){
        //		_it->_cancel = true;
        //		return false;
        //	}
        //}
        
        //cout << seq.getIndex() << " " <<  _nbReadProcessed << endl;
        
#ifdef BOOTSTRAP
        int readPerBootstrap = _maxNbReads / MAX_BOOTSTRAP;
        int bootstrapIndex = seq.getIndex() / readPerBootstrap;
        if(!_bootstraps[bootstrapIndex]) return false;
        //cout << bootstrapIndex << endl;
#endif
        
        if(!isReadSizeValid(seq))
            return false;
        
        if(!isShannonIndexValid(seq))
            return false;
        
        
        //cout << _nbReadProcessed << endl;
        //_nbReadProcessed += 1;
        
        return true;
    }
    
    bool isReadSizeValid(Sequence& seq){
        if(_minReadSize == 0) return true;
        return seq.getDataSize() >= _minReadSize;
    }
    
    bool isShannonIndexValid(Sequence& seq){
        if(_minShannonIndex == 0) return true;
        return getShannonIndex(seq) >= _minShannonIndex;
    }
    
    float getShannonIndex(Sequence& seq){
        
        static char nt2binTab[128] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 1, 0, 0, //69
            0, 3, 0, 0, 0, 0, 0, 0, 4, 0, //79
            0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        
        float index = 0;
        //float freq [5];
        
        vector<float> _freqs(5, 0);
        
        char* seqStr = seq.getDataBuffer();
        
        // Frequency of each letter (A, C, G, T or N)
        for(size_t i=0; i < seq.getDataSize(); i++)
            _freqs[nt2binTab[(unsigned char)seqStr[i]]] += 1.0;
        
        // Shannon index calculation
        for (size_t i=0; i<_freqs.size(); i++){
            _freqs[i] /= (float) seq.getDataSize();
            if (_freqs[i] != 0)
                index += _freqs[i] * log (_freqs[i]) / log(2);
        }
        return abs(index);
        
    }
    
    size_t _minReadSize;
    double _minShannonIndex;
};

/*
 template <class Item> class SimkaTruncateIterator : public TruncateIterator<Item>
 {
 public:
 
	SimkaTruncateIterator (Iterator<Item>* ref, u_int64_t limit, bool initRef=true)
 : TruncateIterator<Item>(*ref, limit, initRef), _ref2(0){ setRef(ref); }
 
 private:
 
 Iterator<Item>* _ref2;
 void setRef (Iterator<Item>* ref2)  { SP_SETATTR(ref2); }
 
 };*/

template<typename Filter> class SimkaBankFiltered : public BankDelegate
{
public:
    
    u_int64_t _refNbReads;
    u_int64_t _refTotalSeqSize;
    u_int64_t _refMaxReadSize;
    
    /** Constructor.
     * \param[in] ref : referred bank.
     * \param[in] filter : functor that filters sequence.
     */
    SimkaBankFiltered (IBank* ref, const Filter& filter, const vector<size_t>& nbPaireds, u_int64_t maxReads) : BankDelegate (ref), _filter(filter)  {
        
        _nbPaireds = nbPaireds;
        _maxReads = maxReads;
        _nbBanks = ref->getCompositionNb();
        ref->estimate(_refNbReads, _refTotalSeqSize, _refMaxReadSize);
        
        
        //cout << _refNbReads << endl;
        //cout << _refTotalSeqSize << endl;
        //cout << _refMaxReadSize << endl;
    }
    
    
    void estimate (u_int64_t& number, u_int64_t& totalSize, u_int64_t& maxSize){
        
        
        if(_maxReads == 0){
            number = _refNbReads;
            totalSize = _refTotalSeqSize;
            maxSize = _refMaxReadSize;
        }
        else{
            
            u_int64_t maxReads = 0;
            for(size_t i=0; i<_nbBanks; i++){
                maxReads += _maxReads * _nbPaireds[i];
            }
            //cout << _refNbReads << endl;
            //cout << _maxReads*_nbBanks << endl;
            maxReads = min (maxReads, _refNbReads);
            //cout << "ha " <<  maxReads << endl;
            
            if(maxReads == _refNbReads){
                number = _refNbReads;
                totalSize = _refTotalSeqSize;
                maxSize = _refMaxReadSize;
            }
            else{
                number = maxReads;
                double factor =  (double)maxReads / (double)_refNbReads;
                totalSize = _refTotalSeqSize * factor;
                maxSize = _refMaxReadSize;
            }
        }
        
        //number = _maxReads;
        //totalSize = (_totalSizeRef*_nbReadToProcess)/_numberRef;
        //maxSize = _maxSizeRef;
        
        //cout << number2 << endl;
        
        //u_int64_t readSize = totalSize2 / number2;
        //cout << "lal:" << number2 << endl;
        //number = _maxReads;
        
        //number = _nbReadToProcess;
        //totalSize = _nbReadToProcess*readSize;
        //maxSize = readSize;
        
        cout << number << endl;
        //cout << totalSize << endl;
        //cout << maxSize << endl;
    }
    
    /** \copydoc tools::collections::Iterable::iterator */
    Iterator<Sequence>* iterator ()
    {
        
        //cout << endl << "---" << endl;
        //cout << "lala" << endl;
        // We create one iterator from the reference
        Iterator<Sequence>* it = _ref->iterator ();
        
        // We get the composition for this iterator
        std::vector<Iterator<Sequence>*> iterators = it->getComposition();
        
        //if (iterators.size() == 1)  { return new FilterIterator<Sequence,Filter> (it, _filter); }
        //else
        //{
        // We are going to create a new CompositeIterator, we won't need the one we just got from the reference
        LOCAL(it);
        
        // We may have to encapsulate each sub iterator with the filter.
        for (size_t i=0; i<iterators.size(); i++)  {
            
            /*
             //cout << "\t\t" << _nbReadsPerDataset[i] << endl;
             
             //cout << _nbReadsPerDataset[i] << endl;
             //Depending on the parameter -max-reads we truncate or not the reads iterator
             if(_nbReadsPerDataset[i] == 0){
             
             //Max nb reads parameter is set to 0. All the reads of each dataset are processed
             iterators[i] = new FilterIterator<Sequence,Filter> (iterators[i], _filter);
             
             }
             else{
             
             //We create a truncated iterator that stop processing reads when _nbReadsPerDataset[i] is reached
             //cout << _nbReadsPerDataset[i] << endl;
             
             //CancellableIterator<Sequence>* truncIt = new CancellableIterator<Sequence>(*iterators[i]);
             Filter filter(_filter);
             //filter.setMaxReads(_nbReadsPerDataset[i]);
             //filter.setIt(truncIt);
             
             #ifdef BOOTSTRAP
             
             srand (time(NULL));
             size_t nbBootstrap = 0;
             vector<bool> iSBoostrap(MAX_BOOTSTRAP);
             
             while(nbBootstrap != NB_BOOTSTRAP){
             int index = rand() % iSBoostrap.size();
             
             if(!iSBoostrap[index]){
             iSBoostrap[index] = true;
             nbBootstrap += 1;
             }
             }
             filter.setBootstrap(iSBoostrap);
             
             #endif
             FilterIterator<Sequence,Filter>* filterIt = new FilterIterator<Sequence,Filter> (iterators[i], filter);
             iterators[i] = filterIt;
             
             }*/
            
            
            //Iterator<Sequence>* it = iterators[i];
            //std::vector<Iterator<Sequence>*> iterators_ = it->getComposition();
            iterators[i] = new SimkaInputIterator<Sequence, Filter> (iterators[i], _nbPaireds[i], _maxReads, _filter);
            
        }
        
        return new CompositeIterator<Sequence> (iterators);
    }
    
private:
    
    vector<size_t> _nbPaireds;
    Filter _filter;
    u_int64_t _maxReads;
    size_t _nbBanks;
};





/*********************************************************************
 * ** SimkaAlgorithm
 *********************************************************************/
template<size_t span=KMER_DEFAULT_SPAN>
class SimkaAlgorithm : public Algorithm
{
    
public:
    
    
    typedef typename Kmer<span>::Type                                       Type;
    typedef typename Kmer<span>::Count                                      Count;
    typedef typename Kmer<span>::ModelCanonical                             ModelCanonical;
    typedef typename ModelCanonical::Kmer                                   KmerType;
    
    SimkaAlgorithm(IProperties* options);
    ~SimkaAlgorithm();
    void execute();
    void print();
    
    //void executeSimkamin();
    
    
    static string toString(u_int64_t value){
        char buffer[40];
        snprintf(buffer, 30, "%llu", value);
        return string(buffer);
    }
    
protected:
    
    
    bool setup();
    bool isInputValid();
    void parseArgs();
    bool createDirs();
    void computeMaxReads();
    void layoutInputFilename();
    void createBank();
    void count();
    
    void outputMatrix();
    
    //void dumpMatrix(const string& outputFilename, const vector<vector<float> >& matrix);
    //void outputHeatmap();
    //void __outputHeatmap(const string& outputFilenamePrefix, const string& matrixPercFilename, const string& matrixNormFilename);
    
    void clear();
    
    u_int64_t _maxMemory;
    size_t _nbCores;
    string _outputDir;
    string _outputDirTemp;
    size_t _nbBanks;
    string _inputFilename;
    size_t _kmerSize;
    pair<CountNumber, CountNumber> _abundanceThreshold;
    SIMKA_SOLID_KIND _solidKind;
    bool _soliditySingle;
    int64_t _maxNbReads;
    size_t _minReadSize;
    double _minReadShannonIndex;
    double _minKmerShannonIndex;
    size_t _nbMinimizers;
    //size_t _nbCores;
    
    SimkaStatistics* _stats;
    //SimkaDistance* _simkaDistance;
    
    string _banksInputFilename;
    vector<string> _tempFilenamesToDelete;
    IBank* _banks;
    IProperties* _options;
    
    vector<string> _bankNames;
    //vector<u_int64_t> _nbReadsPerDataset;
    
    string _outputFilenameSuffix;
    
    u_int64_t _totalKmers;
    vector<size_t> _nbBankPerDataset;
    
    string _largerBankId;
    bool _computeSimpleDistances;
    bool _computeComplexDistances;
    bool _keepTmpFiles;
    //string _matDksNormFilename;
    //string _matDksPercFilename;
    //string _matAksNormFilename;
    //string _matAksPercFilename;
    //string _heatmapDksFilename;
    //string _heatmapAksFilename;
    
    /*
     gatb::core::tools::dp::IteratorListener* _progress;
     void setProgress (gatb::core::tools::dp::IteratorListener* progress)  { SP_SETATTR(progress); }
     
     
     size_t _nbPartitions;
     std::vector <std::vector<size_t> > _nbKmersPerPartitionPerBank;
     vector<vector<u_int64_t> > _nbk_per_radix_per_part;//number of kxmer per parti per rad
     
     
     Storage* _tmpPartitionsStorage;
     void setPartitionsStorage (Storage* tmpPartitionsStorage)  {  SP_SETATTR(tmpPartitionsStorage);  }
     
     Partition<Type>* _tmpPartitions;
     void setPartitions (Partition<Type>* tmpPartitions)  {  SP_SETATTR(tmpPartitions);  }
     
     vector<u_int64_t> _nbKmerPerPartitions;
     int getSizeofPerItem () const { return Type::getSize()/8 + sizeof(bankIdType); }
     std::vector<size_t> getNbCoresList();
     
     //this->_local_pInfo.incKmer_and_rad (p, radix_kxmer.getVal(), kx_size); //nb of superkmer per x per parti per radix
     
     //vector<SpeciesAbundanceVectorType > _speciesAbundancePerDataset;
     
     //MultiDiskStorage<Type>* _multiStorage;
     //u_int64_t _maxDisk;
     */
    
    
};







#endif /* TOOLS_SIMKA_SRC_SIMKAALGORITHM_HPP_ */
