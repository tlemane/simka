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

#include "SimkaDistance.hpp"





SimkaStatistics::SimkaStatistics(size_t nbBanks, bool computeSimpleDistances, bool computeComplexDistances, const string& tmpDir, const vector<string>& datasetIds)
{

	_nbBanks = nbBanks;
	_symetricDistanceMatrixSize = (_nbBanks*(_nbBanks+1))/2;
	_computeSimpleDistances = computeSimpleDistances;
	_computeComplexDistances = computeComplexDistances;

	//_nbBanks = 10000;

	_nbKmers = 0;
	_nbDistinctKmers = 0;
	_nbSolidKmers = 0;
	_nbErroneousKmers = 0;
	_nbSharedKmers = 0;

	//_abundanceMin = abundanceMin;
	//_mutex = mutex;
	//_outputDir = outputDir;
	_datasetNbReads.resize(_nbBanks, 0);

	_nbSolidDistinctKmersPerBank.resize(_nbBanks, 0);
	_nbSolidKmersPerBank.resize(_nbBanks, 0);
	_nbKmersPerBank.resize(_nbBanks, 0);

	//_nbDistinctKmersSharedByBanksThreshold.resize(_nbBanks, 0);
	//_nbKmersSharedByBanksThreshold.resize(_nbBanks, 0);

	_matrixNbDistinctSharedKmers.resize(_symetricDistanceMatrixSize);
	_matrixNbSharedKmers.resize(_nbBanks);
	_brayCurtisNumerator.resize(_symetricDistanceMatrixSize);

	for(size_t i=0; i<_nbBanks; i++){
		//_matrixNbDistinctSharedKmers[i].resize(nbBanks, 0);
		_matrixNbSharedKmers[i].resize(nbBanks, 0);
		//_brayCurtisNumerator[i].resize(nbBanks, 0);
		//_kullbackLeibler[i].resize(nbBanks, 0);
	}



	if(_computeSimpleDistances){
		//_abundance_jaccard_intersection.resize(_nbBanks);
		//for(size_t i=0; i<_nbBanks; i++){
		//	_abundance_jaccard_intersection[i].resize(nbBanks, 0);
		//}

		_chord_NiNj.resize(_nbBanks);
		_chord_sqrt_N2.resize(_nbBanks);
		//_chord_N2j.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_chord_NiNj[i].resize(nbBanks, 0);
			//_chord_N2i[i].resize(nbBanks, 0);
			//_chord_N2j[i].resize(nbBanks, 0);
		}

		_hellinger_SqrtNiNj.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_hellinger_SqrtNiNj[i].resize(nbBanks, 0);
		}

		_kulczynski_minNiNj.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_kulczynski_minNiNj[i].resize(nbBanks, 0);
		}
	}

	if(_computeComplexDistances){
		_whittaker_minNiNj.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_whittaker_minNiNj[i].resize(nbBanks, 0);
		}

		_kullbackLeibler.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_kullbackLeibler[i].resize(nbBanks, 0);
		}

		_canberra.resize(_nbBanks);
		for(size_t i=0; i<_nbBanks; i++){
			_canberra[i].resize(nbBanks, 0);
		}
	}




	_totalReads = 0;

	for(size_t i=0; i<_nbBanks; i++){

		string name = datasetIds[i];
		string countFilename = tmpDir + "/count_synchro/" +  name + ".ok";

		string line;
		ifstream file(countFilename.c_str());
		vector<string> lines;
		while(getline(file, line)){
			if(line == "") continue;
			lines.push_back(line);
		}
		file.close();

		u_int64_t nbReads = strtoull(lines[0].c_str(), NULL, 10);


		_datasetNbReads[i] = nbReads;
		_nbSolidDistinctKmersPerBank[i] = strtoull(lines[1].c_str(), NULL, 10);
		_nbSolidKmersPerBank[i] = strtoull(lines[2].c_str(), NULL, 10);


		if(_computeSimpleDistances){
			_chord_sqrt_N2[i] = sqrt(strtoull(lines[3].c_str(), NULL, 10));
		}

		_totalReads += nbReads;
		
	}

}


SimkaStatistics& SimkaStatistics::operator+=  (const SimkaStatistics& other){




	_nbKmers += other._nbKmers;
	_nbDistinctKmers += other._nbDistinctKmers;
	_nbSolidKmers += other._nbSolidKmers;
	_nbErroneousKmers += other._nbErroneousKmers;
	_nbSharedKmers += other._nbSharedKmers;

	for(size_t i=0; i<_nbBanks; i++){
		_nbKmersPerBank[i] += other._nbKmersPerBank[i];
	}

	for(size_t i=0; i<_symetricDistanceMatrixSize; i++){
		_brayCurtisNumerator[i] += other._brayCurtisNumerator[i];
		_matrixNbDistinctSharedKmers[i] += other._matrixNbDistinctSharedKmers[i];
	}

	for(size_t i=0; i<_nbBanks; i++){
		for(size_t j=0; j<_nbBanks; j++){
			_matrixNbSharedKmers[i][j] += other._matrixNbSharedKmers[i][j];
		}
	}


	if(_computeSimpleDistances){
		for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){
					_chord_NiNj[i][j] += other._chord_NiNj[i][j];
					_hellinger_SqrtNiNj[i][j] += other._hellinger_SqrtNiNj[i][j];
					_kulczynski_minNiNj[i][j] += other._kulczynski_minNiNj[i][j];
			}
		}
	}

	if(_computeComplexDistances){
		for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){
				_canberra[i][j] += other._canberra[i][j];
				_whittaker_minNiNj[i][j] += other._whittaker_minNiNj[i][j];
				_kullbackLeibler[i][j] += other._kullbackLeibler[i][j];
			}
		}
	}

	return *this;
}

void SimkaStatistics::print(){

	u_int64_t nbKmers = 0;
	u_int64_t nbDistinctKmersAfterMerging = _nbDistinctKmers;
	u_int64_t nbDistinctKmers = 0;
	u_int64_t nbSharedDistinctKmers = _nbSharedKmers;
	u_int64_t nbSharedKmers = 0;

	double meanCoverage = 0;

	for(size_t i=0; i<_nbBanks; i++){
		nbKmers += _nbSolidKmersPerBank[i];
		nbDistinctKmers += _nbSolidDistinctKmersPerBank[i];

		float coverage = (double)_nbSolidKmersPerBank[i] / (double)_nbSolidDistinctKmersPerBank[i];
		//cout << coverage << endl;
		meanCoverage += coverage;
		//nbDistinctKmers += _nbDistinctKmers;
		//for(size_t j=i+1; j<_nbBanks; j++){
		//	nbSharedDistinctKmers += _matrixNbDistinctSharedKmers[i][j];
		//	nbSharedKmers += _matrixNbSharedKmers[i][j];
		//}
	}

	meanCoverage /= _nbBanks;

	u_int64_t totalReads = 0;
	u_int64_t minReads = -1;
	u_int64_t maxReads = 0;
	for (size_t i=0; i<_nbBanks; i++){

		u_int64_t nbReads = _datasetNbReads[i];
		//nbReads /= _nbBankPerDataset[i];
		totalReads += nbReads;
		if(nbReads < minReads){
			minReads = nbReads;
			//_smallerBankId = _bankNames[i];
		}
		if(nbReads > maxReads){
			maxReads = nbReads;
		}

	}
	u_int64_t meanReads = totalReads / _nbBanks;

	cout << endl << "Stats" << endl;
	cout << "\tReads" << endl;
	cout << "\t\tTotal:    " << totalReads << "    " << totalReads/1000000 << "M" << "    " << totalReads/1000000000 << "G" << endl;
	cout << "\t\tMin:    " << minReads << "    " << minReads/1000000 << "M" << "    " << minReads/1000000000 << "G" << endl;
	cout << "\t\tMax:    " << maxReads << "    " << maxReads/1000000 << "M" << "    " << maxReads/1000000000 << "G" << endl;
	cout << "\t\tAverage:    " << meanReads << "    " << meanReads/1000000 << "M" << "    " << meanReads/1000000000 << "G" << endl;
	cout << "\tKmers" << endl;
	cout << "\t\tDistinct Kmers (before merging):    " << nbDistinctKmers << "    " << nbDistinctKmers/1000000 << "M" << "    " << nbDistinctKmers/1000000000 << "G" << endl;
	cout << "\t\tDistinct Kmers (after merging):    " << nbDistinctKmersAfterMerging << "    " << nbDistinctKmersAfterMerging/1000000 << "M" << "    " << nbDistinctKmersAfterMerging/1000000000 << "G" << endl;
	cout << "\t\tShared distinct Kmers:    " << nbSharedDistinctKmers << "    " << nbSharedDistinctKmers/1000000 << "M" << "    " << nbSharedDistinctKmers/1000000000 << "G" << endl;
	cout << "\t\tKmers:    " << nbKmers << "    " << nbKmers/1000000 << "M" << "    " << nbKmers/1000000000 << "G" << endl;
	cout << "\t\tMean k-mer coverage: " << meanCoverage << endl;

	cout << endl;



	return;
	//cout.precision(4);
    cout << endl << endl;

    //return;

    u_int64_t solidAbundance = 0;
    //for(int i=0; i<_nbSolidKmersPerBankAbundance.size(); i++)
    //	solidAbundance += _nbSolidKmersPerBankAbundance[i];
    for(size_t i=0; i<_nbKmersSharedByBanksThreshold.size(); i++)
    	solidAbundance += _nbKmersSharedByBanksThreshold[i];

    cout << "Statistics on kmer intersections:" << endl;
    cout << "\tNb kmers: " << _nbKmers << "    " << _nbKmers / 1000000 << " M" << "    " << _nbKmers / 1000000000 << " G" << endl;
    cout << endl;

    cout << "\tNb distinct kmers: " << _nbDistinctKmers << "    " << _nbDistinctKmers / 1000000 << " M" << "    " << _nbDistinctKmers / 1000000000 << " G" << "    " << (100*_nbDistinctKmers)/(float)_nbKmers << "%" << endl;
    cout << "\tNb solid kmers: " << _nbSolidKmers << "    " << _nbSolidKmers / 1000000 << " M" << "    " << _nbSolidKmers / 1000000000 << " G" << "    " << (100*_nbSolidKmers)/(float)_nbDistinctKmers << "% distinct" << "       " << (100*solidAbundance) / (double)_nbKmers << "% abundance" << endl;
    //for(int i=0; i<_nbBanks; i++){
	    //cout << "Nb kmers (M) " << i <<  ": " << _nbSolidKmersPerBank[i] << endl << endl;
    //}

    cout << endl;
    cout << "\tPotentially erroneous (Kmers appearing only one time in a single bank): " << endl;
    cout << "\t\t" << _nbErroneousKmers << "    " << _nbErroneousKmers / 1000000 << " M" << "    " << _nbErroneousKmers / 1000000000 << " G" << "    " << (100*_nbErroneousKmers)/(float)_nbDistinctKmers << "% distinct" << "      " << (100*_nbErroneousKmers)/(float)_nbKmers << "% abundance" << endl;

    cout << endl;
    cout << "\tKmer shared by T banks :" << endl;

    for(size_t i=0; i<_nbBanks; i++){
	    cout << "\t\tShared by " << i+1 <<  " banks:";

	    cout << endl;
	    cout << "\t\t\tDistinct:    " << _nbDistinctKmersSharedByBanksThreshold[i] << "    ";
	    if(_nbSolidKmers > 0){
		    cout << (_nbDistinctKmersSharedByBanksThreshold[i]*100) / (float)_nbSolidKmers << "%";
	    }
	    else{
	    	cout << "0%";
	    }

	    cout << endl;
	    cout << "\t\t\tAbundance:    " << _nbKmersSharedByBanksThreshold[i] << "    ";
	    if(solidAbundance > 0){
	    	cout << (_nbKmersSharedByBanksThreshold[i]*100) / (float)solidAbundance << "%";
	    }
	    else{
	    	cout << "0%";
	    }
	    if(_nbDistinctKmersSharedByBanksThreshold[i] > 0){
	    	cout << endl;
		    cout << "\t\t\tMean abundance per bank: " << _nbKmersSharedByBanksThreshold[i] / _nbDistinctKmersSharedByBanksThreshold[i] / (float) _nbBanks;
	    }

	    cout << endl;
    }

    //cout << endl;
    //cout << "Nb kmers in all banks (max/min > 10): " << _nbKmersInCoupleBankSupRatio << "    " << (_nbKmersInCoupleBankSupRatio*100) / (float)_nbSolidKmers << "%" <<  endl;


   cout << endl << endl;
}

void SimkaStatistics::load(const string& filename){


	IterableGzFile<long double>* file = new IterableGzFile<long double>(filename);
	Iterator<long double>* it = file->iterator();
	LOCAL(it);
	it->first();

	//_nbBanks = it->item(); it->next();
	_computeSimpleDistances = it->item(); it->next();
	_computeComplexDistances = it->item(); it->next();
	//cout << _computeSimpleDistances << "   " << _computeComplexDistances << endl;
	_nbKmers = it->item(); it->next();
	_nbErroneousKmers = it->item(); it->next();
	_nbDistinctKmers = it->item(); it->next();
	_nbSolidKmers = it->item(); it->next();
	_nbSharedKmers = it->item(); it->next();

    for(size_t i=0; i<_nbBanks; i++){ _nbSolidDistinctKmersPerBank[i] = it->item(); it->next();}
    for(size_t i=0; i<_nbBanks; i++){ _nbKmersPerBank[i] = it->item(); it->next();}
    for(size_t i=0; i<_nbBanks; i++){ _nbSolidKmersPerBank[i] = it->item(); it->next();}
    //for(size_t i=0; i<_nbBanks; i++){ _nbDistinctKmersSharedByBanksThreshold[i] = it->item(); it->next();}
    //for(size_t i=0; i<_nbBanks; i++){ _nbKmersSharedByBanksThreshold[i] = it->item(); it->next();}


    for(size_t i=0; i<_nbBanks; i++){
    	//cout << i << endl;
    	//cout << _nbBanks << endl;
    	//cout << _matrixNbDistinctSharedKmers[i].size() << endl;
            for(size_t j=0; j<_nbBanks; j++){ _matrixNbSharedKmers[i][j] = it->item(); it->next();}

            //for(size_t j=0; j<_nbBanks; j++){ _abundance_jaccard_intersection[i][j] = it->item(); it->next();}
    }

    for(size_t i=0; i<_symetricDistanceMatrixSize; i++){
        _matrixNbDistinctSharedKmers[i] = it->item(); it->next();
        _brayCurtisNumerator[i] = it->item(); it->next();
    }

	if(_computeSimpleDistances){
	    for(size_t i=0; i<_nbBanks; i++){ _chord_sqrt_N2[i] = it->item(); it->next();}
	    for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){ _chord_NiNj[i][j] = it->item(); it->next();}
			for(size_t j=0; j<_nbBanks; j++){ _hellinger_SqrtNiNj[i][j] = it->item(); it->next();}
			for(size_t j=0; j<_nbBanks; j++){ _kulczynski_minNiNj[i][j] = it->item(); it->next();}
	    }
	}


	if(_computeComplexDistances){
	    for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){ _canberra[i][j] = it->item(); it->next();}
			for(size_t j=0; j<_nbBanks; j++){ _whittaker_minNiNj[i][j] = it->item(); it->next();}
			for(size_t j=0; j<_nbBanks; j++){ _kullbackLeibler[i][j] = it->item(); it->next();}
	    }
	}

	delete file;

	
}

void SimkaStatistics::save (const string& filename){


	BagGzFile<long double>* file = new BagGzFile<long double>(filename);


	//file->insert(_nbBanks);
	file->insert((long double)_computeSimpleDistances);
	file->insert((long double)_computeComplexDistances);
	file->insert((long double)_nbKmers);
	file->insert((long double)_nbErroneousKmers);
	file->insert((long double)_nbDistinctKmers);
	file->insert((long double)_nbSolidKmers);
	file->insert((long double)_nbSharedKmers);

    for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_nbSolidDistinctKmersPerBank[i]);}
    for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_nbKmersPerBank[i]);}
    for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_nbSolidKmersPerBank[i]);}
    //for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_nbDistinctKmersSharedByBanksThreshold[i]);}
    //for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_nbKmersSharedByBanksThreshold[i]);}


    for(size_t i=0; i<_nbBanks; i++){
    	//cout << i << endl;
    	//cout << _nbBanks << endl;
    	//cout << _matrixNbDistinctSharedKmers[i].size() << endl;
            for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_matrixNbSharedKmers[i][j]);}

            //for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_abundance_jaccard_intersection[i][j]);}
    }


    for(size_t i=0; i<_symetricDistanceMatrixSize; i++){
        file->insert((long double)_matrixNbDistinctSharedKmers[i]);
    	file->insert((long double)_brayCurtisNumerator[i]);
    }

	if(_computeSimpleDistances){
	    for(size_t i=0; i<_nbBanks; i++){ file->insert((long double)_chord_sqrt_N2[i]);}
	    for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_chord_NiNj[i][j]);}
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_hellinger_SqrtNiNj[i][j]);}
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_kulczynski_minNiNj[i][j]);}
	    }
	}


	if(_computeComplexDistances){
	    for(size_t i=0; i<_nbBanks; i++){
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_canberra[i][j]);}
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_whittaker_minNiNj[i][j]);}
			for(size_t j=0; j<_nbBanks; j++){ file->insert((long double)_kullbackLeibler[i][j]);}
	    }
	}
	
	file->flush();

	delete file;


	}

void SimkaStatistics::outputMatrix(const string& outputDir, const vector<string>& bankNames){


	SimkaDistance _simkaDistance(*this);

	_outputFilenameSuffix = "";

	char buffer[200];

	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_chord", _simkaDistance._matrix_presenceAbsence_chordHellinger());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_whittaker", _simkaDistance._matrix_presenceAbsence_Whittaker());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_kulczynski", _simkaDistance._matrix_presenceAbsence_kulczynski());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_braycurtis", _simkaDistance._matrix_presenceAbsence_sorensenBrayCurtis());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_jaccard", _simkaDistance._matrix_presenceAbsence_jaccardCanberra());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_simka-jaccard", _simkaDistance._matrix_presenceAbsence_jaccard_simka());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_simka-jaccard_asym", _simkaDistance._matrix_presenceAbsence_jaccard_simka_asym());
	dumpMatrix(outputDir, bankNames, "mat_presenceAbsence_ochiai", _simkaDistance._matrix_presenceAbsence_ochiai());


	dumpMatrix(outputDir, bankNames, "mat_abundance_simka-jaccard", _simkaDistance._matrixSymJaccardAbundance());
	dumpMatrix(outputDir, bankNames, "mat_abundance_simka-jaccard_asym", _simkaDistance._matrixAsymJaccardAbundance());
	dumpMatrix(outputDir, bankNames, "mat_abundance_ab-ochiai", _simkaDistance._matrixOchiai());
	dumpMatrix(outputDir, bankNames, "mat_abundance_ab-sorensen", _simkaDistance._matrixSorensen());
	dumpMatrix(outputDir, bankNames, "mat_abundance_ab-jaccard", _simkaDistance._matrixJaccardAbundance());

	const vector<vector<float> >& matrix = _simkaDistance._matrixBrayCurtis();
	dumpMatrix(outputDir, bankNames, "mat_abundance_braycurtis", matrix);
	dumpMatrix(outputDir, bankNames, "mat_abundance_jaccard", _simkaDistance.computeJaccardDistanceFromBrayCurtis(matrix));

	if(_computeSimpleDistances){
		dumpMatrix(outputDir, bankNames, "mat_abundance_chord", _simkaDistance._matrixChord());
		dumpMatrix(outputDir, bankNames, "mat_abundance_hellinger", _simkaDistance._matrixHellinger());
		dumpMatrix(outputDir, bankNames, "mat_abundance_kulczynski", _simkaDistance._matrixKulczynski());
	}

	if(_computeComplexDistances){
		dumpMatrix(outputDir, bankNames, "mat_abundance_whittaker", _simkaDistance._matrixWhittaker());
		dumpMatrix(outputDir, bankNames, "mat_abundance_jensenshannon", _simkaDistance._matrixKullbackLeibler());
		dumpMatrix(outputDir, bankNames, "mat_abundance_canberra", _simkaDistance._matrixCanberra());
	}
}



void SimkaStatistics::dumpMatrix(const string& outputDir, const vector<string>& bankNames, const string& outputFilename, const vector<vector<float> >& matrix){


	string filename = outputDir + "/" + outputFilename + ".csv";
	gzFile out = gzopen((filename + ".gz").c_str(),"wb");

	//char buffer[200];
	string str;

	for(size_t i=0; i<matrix.size(); i++){
		str += ";" + bankNames[i];
		//str += ";" + datasetInfos[i]._name;
	}
	str += '\n';
	gzwrite(out, str.c_str(), str.size());

	for(size_t i=0; i<matrix.size(); i++){

		str = "";
		str += bankNames[i];
		//str += datasetInfos[i]._name + ";";
		for(size_t j=0; j<matrix.size(); j++){

			str += ";" + Stringify::format("%f", matrix[i][j]);
			//snprintf(buffer,200,"%.2f", matrix[i][j]);
			//snprintf(buffer,200,"%f", matrix[i][j]);
			//str += string(buffer) + ";";

			//str += to_string(matrix[i][j]) + ";";
		}

		//matrixNormalizedStr.erase(matrixNormalizedStr.end()-1);
		//str.erase(str.size()-1);
		//str.pop_back(); //remove ; at the end of the line
		str += '\n';

		gzwrite(out, str.c_str(), str.size());
	}

	gzclose(out);

	//gatb::core::system::IFile* file = gatb::core::system::impl::System::file().newFile(outputDir + "/" + outputFilename + _outputFilenameSuffix + ".csv", "wb");
	//file->fwrite(str.c_str(), str.size(), 1);
	//file->flush();
	//delete file;

}




SimkaDistance::SimkaDistance(SimkaStatistics& stats) : _stats(stats){
	_nbBanks = _stats._nbBanks;
}



vector<vector<float> > SimkaDistance::createSquaredMatrix(size_t n){
    vector<vector<float> > matrix;

    matrix.resize(n);
    for(size_t i=0; i<n; i++)
    	matrix[i].resize(n, 0);

    return matrix;

}


void SimkaDistance::get_abc(size_t i, size_t j, size_t symetricIndex, u_int64_t& a, u_int64_t& b, u_int64_t& c){

	a = _stats._matrixNbDistinctSharedKmers[symetricIndex];
	b = (_stats._nbSolidDistinctKmersPerBank[i] - a);
	c = (_stats._nbSolidDistinctKmersPerBank[j] - a);

}

double SimkaDistance::distance_abundance_brayCurtis(size_t i, size_t j, size_t symetricIndex){

	//double intersection = _stats._abundance_jaccard_intersection[i][j];
	double union_ = _stats._nbSolidKmersPerBank[i] + _stats._nbSolidKmersPerBank[j];
	if(union_ == 0) return 1;

	double intersection = 2 * _stats._brayCurtisNumerator[symetricIndex];

	double jaccard = 1 - intersection / union_;

	return jaccard;
}

//Abundance Chord
double SimkaDistance::distance_abundance_chord(size_t i, size_t j){

	double den = _stats._chord_sqrt_N2[i]*_stats._chord_sqrt_N2[j];
	if(den == 0) return sqrt(2);

	long double chordDistance =  sqrtl(2 - 2*_stats._chord_NiNj[i][j] / den);
	/*
	long double intersection = 2*_stats._chord_NiNj[i][j];
	if(intersection == 0) return sqrt(2);

	long double unionSize = sqrtl(_stats._chord_N2[i]) * sqrtl(_stats._chord_N2[j]);
	if(unionSize == 0)  return sqrt(2);

	double chordDistance = sqrtl(2 - (intersection / unionSize));*/
	//chordDistance -= 1;
	//return chordDistance;
	return chordDistance;
}

//Abundance Hellinger
double SimkaDistance::distance_abundance_hellinger(size_t i, size_t j){

	double union_ = sqrt(_stats._nbSolidKmersPerBank[i]) * sqrt(_stats._nbSolidKmersPerBank[j]);
	if(union_ == 0) return sqrt(2);

	double intersection = 2*_stats._hellinger_SqrtNiNj[i][j];

	double hellingerDistance = sqrt(2 - (intersection / union_));

	return hellingerDistance;
}

//Abundance Jaccard Intersection - Bray curtis with min()
//double SimkaDistance::distance_abundance_jaccard_intersection(size_t i, size_t j){

	//double intersection = _stats._abundance_jaccard_intersection[i][j];
//	double union_ = _stats._nbSolidKmersPerBank[i] + _stats._nbSolidKmersPerBank[j];

//double intersection = 2 * _stats._abundance_jaccard_intersection[i][j];

//double jaccard = 1 - intersection / union_;

//return jaccard;
//}

//Abundance Whittaker
double SimkaDistance::distance_abundance_whittaker(size_t i, size_t j){

	long double union_ = _stats._nbSolidKmersPerBank[i] * _stats._nbSolidKmersPerBank[j];
	if(union_ == 0) return 1;

	long double intersection = _stats._whittaker_minNiNj[i][j];

	double whittakerDistance = 0.5 * (intersection / union_);

	return whittakerDistance;
}

//Abundance Kullback Leibler
double SimkaDistance::distance_abundance_kullbackLeibler(size_t i, size_t j){

	if(_stats._kullbackLeibler[i][j] == 0) return 1;

	return sqrt(0.5 * _stats._kullbackLeibler[i][j]);
	//return _stats._kullbackLeibler[i][j];
}

//Abundance Canberra
double SimkaDistance::distance_abundance_canberra(size_t i, size_t j, u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	if((a+b+c) == 0) return 1;

	double canberraDistance = (1 / (a+b+c)) * _stats._canberra[i][j];

	return canberraDistance;
}

//Abundance Kulczynski
double SimkaDistance::distance_abundance_kulczynski(size_t i, size_t j){

	if(_stats._nbSolidKmersPerBank[i] == 0 || _stats._nbSolidKmersPerBank[j] == 0) return 1;

	long double n1 = (double) _stats._kulczynski_minNiNj[i][j] / (double) _stats._nbSolidKmersPerBank[i];
	long double n2 = (double) _stats._kulczynski_minNiNj[j][i] / (double) _stats._nbSolidKmersPerBank[j];

	//long double numerator = (_stats._nbSolidKmersPerBank[i] + _stats._nbSolidKmersPerBank[j]) * _stats._kulczynski_minNiNj[i][j];
	//long double denominator = _stats._nbSolidKmersPerBank[i] * _stats._nbSolidKmersPerBank[j];
	//double kulczynskiDistance = 1 - 0.5 * (numerator / denominator);

	double kulczynskiDistance = 1 - 0.5 * (n1 + n2);

	return kulczynskiDistance;
}

//abundance jaccard simka
double SimkaDistance::distance_abundance_jaccard_simka(size_t i, size_t j, SIMKA_MATRIX_TYPE type){

	double numerator = 0;
	double denominator = 0;

	double A1 = _stats._matrixNbSharedKmers[i][j];
	double B1 = _stats._matrixNbSharedKmers[j][i];
	double A0 = _stats._nbSolidKmersPerBank[i];
	double B0 = _stats._nbSolidKmersPerBank[j];

	if(type == SYMETRICAL){
		//numerator = A1 * B1;
		//denominator = A0*B1 + A1*B0 - A1*B1;
		numerator = A1 + B1;
		denominator = A0 + B0;
	}
	else if(type == ASYMETRICAL){
		numerator = A1;
		denominator = A0;
	}

	if(denominator == 0) return 1;
	return 1 - numerator / denominator;

}

//abundance Ochiai
double SimkaDistance::distance_abundance_ochiai(size_t i, size_t j){

	double A1 = _stats._matrixNbSharedKmers[i][j];
	double B1 = _stats._matrixNbSharedKmers[j][i];
	double A0 = _stats._nbSolidKmersPerBank[i];
	double B0 = _stats._nbSolidKmersPerBank[j];

	if(A0 == 0 || B0 == 0) return 1;

	return 1 - sqrt(A1/A0) * sqrt(B1/B0);
}

//abundance Sorensen
double SimkaDistance::distance_abundance_sorensen(size_t i, size_t j){

	double numerator = 0;
	double denominator = 0;

	double A1 = _stats._matrixNbSharedKmers[i][j];
	double B1 = _stats._matrixNbSharedKmers[j][i];
	double A0 = _stats._nbSolidKmersPerBank[i];
	double B0 = _stats._nbSolidKmersPerBank[j];

	numerator = 2*A1*B1;
	denominator = A0*B1 + A1*B0;

	if(denominator == 0) return 1;
	return 1 - numerator / denominator;
}

//abundance jaccard
double SimkaDistance::distance_abundance_jaccard(size_t i, size_t j){

	double numerator = 0;
	double denominator = 0;

	double A1 = _stats._matrixNbSharedKmers[i][j];
	double B1 = _stats._matrixNbSharedKmers[j][i];
	double A0 = _stats._nbSolidKmersPerBank[i];
	double B0 = _stats._nbSolidKmersPerBank[j];

	numerator = A1 * B1;
	denominator = A0*B1 + A1*B0 - A1*B1;

	if(denominator == 0) return 1;
	return 1 - numerator / denominator;

}

double SimkaDistance::distance_presenceAbsence_chordHellinger(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	double p1 = sqrt((a+b)*(a+c));

	if(p1 == 0) return sqrt(2);
	return sqrt(2*(1-a/p1));
}

double SimkaDistance::distance_presenceAbsence_whittaker(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	if(a+b == 0 || a+c == 0) return 1;
	double p1 = b / (a + b);
	double p2 = c / (a + c);

	double p3 = a / (a + b);
	double p4 = a / (a + c);

	double distance = 0.5 * (p1 + p2 + abs(p3-p4));

	return distance;
}

double SimkaDistance::distance_presenceAbsence_canberra(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	return 0;
}

double SimkaDistance::distance_presenceAbsence_kulczynski(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	if(a+b == 0 || a+c == 0) return 1;

	double p1 = a / (a + b);
	double p2 = a / (a + c);

	double distance = 1 - 0.5*(p1 + p2);

	return distance;
}

double SimkaDistance::distance_presenceAbsence_sorensenBrayCurtis(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	if((2*a + b + c) == 0) return 1;

	double distance = (b+c) / (2*a + b + c);

	return distance;
}




double SimkaDistance::distance_presenceAbsence_ochiai(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	float val = sqrt((a+b)*(a+c));
	if(val == 0) return 1;

	return 1 - (a / val);
}

double SimkaDistance::distance_presenceAbsence_jaccardCanberra(u_int64_t& ua, u_int64_t& ub, u_int64_t& uc){

	double a = (double) ua;
	double b = (double) ub;
	double c = (double) uc;

	if((a+b+c) == 0) return 1;
	return (b+c) / (a+b+c);
}

double SimkaDistance::distance_presenceAbsence_jaccard_simka(size_t i, size_t j, size_t symetricIndex, SIMKA_MATRIX_TYPE type){

	double numerator = 0;
	double denominator = 0;

    if(type == SYMETRICAL){
    	numerator = 2*_stats._matrixNbDistinctSharedKmers[symetricIndex];
    	denominator = _stats._nbSolidDistinctKmersPerBank[i] + _stats._nbSolidDistinctKmersPerBank[j];
    }
    else if(type == ASYMETRICAL){
    	numerator = _stats._matrixNbDistinctSharedKmers[symetricIndex];
    	denominator = _stats._nbSolidDistinctKmersPerBank[i];
    }

    if(denominator == 0) return 1;
    return 1 - numerator/denominator;
}



