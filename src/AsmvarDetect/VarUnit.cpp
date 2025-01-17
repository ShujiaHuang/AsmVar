#include "VarUnit.h"

VarUnit::VarUnit() {

	target.id = "-"; 
	query.id  = "-"; 
	tarSeq    = "."; 
	qrySeq    = "."; 
	strand    = '.'; 
	type      = "."; 
	isHete    = false;
	score     = 0;   
	mismap    = 1.0;
	isClear   = false;
	
	homoRun = 0;
	isSuccessAlign = false;
	isGoodReAlign  = false;
	cipos = make_pair(0, 0);
	ciend = make_pair(0, 0);

	return;
}

VarUnit::VarUnit(const VarUnit& V) {

	target  = V.target; query   = V.query; tarSeq = V.tarSeq; 
	qrySeq  = V.qrySeq; strand  = V.strand;
	type    = V.type  ; isHete  = V.isHete; isClear = V.isClear; 

	score  = V.score;  
	mismap = V.mismap;
	homoRun= V.homoRun;
	cipos  = V.cipos;
	ciend  = V.ciend;
	identity       = V.identity;
	isSuccessAlign = V.isSuccessAlign;
	isGoodReAlign  = V.isGoodReAlign;

	exp_target = V.exp_target;
	exp_tarSeq = V.exp_tarSeq;

	return;
}

void VarUnit::ConvQryCoordinate(long int qrySeqLen) {

	// This funtion just conversion the coordinate of Axt/MAF format creat 
	// by 'lastz'/'last ', which mapped to the '-' strand
	if ( strand != '-' ) return;

	long int itemp = query.start;
	query.start = qrySeqLen - query.end + 1;
	query.end   = qrySeqLen - itemp + 1;

	return;
}

void VarUnit::Swap() { // Swap the target and query region. Ignore 'exp_target' 

	Region tmp = target; target = query;  query  = tmp;
	string str = tarSeq; tarSeq = qrySeq; qrySeq = str;

	return;
}

vector<VarUnit> VarUnit::ReAlignAndReCallVar(string &targetSeq,
											 string &querySeq, 
											 AgeOption opt) {
// Return new VarUnit after AGE Realignment
// This is a design strategy, I'm not going to simply update the raw VarUnit!

	vector<VarUnit> vus;
	AgeAlignment alignment(*(this), opt);
	if (alignment.Align(targetSeq, querySeq)) {
	// Successful align!
		isSuccessAlign = true;
		vus = alignment.VarReCall();
	} else {
		isSuccessAlign = false;
	} 
	return vus;
}

void VarUnit::OutStd(long int tarSeqLen, long int qrySeqLen, ofstream &O) { 
// Output

	string reAlignStat = (isSuccessAlign) ? "CanAGE" : "CanotAGE";
	O << target.id << "\t" << target.start << "\t" << target.end << "\t" 
	  << target.end - target.start + 1     << "\t" << tarSeqLen  << "\t"
	  << query.id  << "\t" << query.start  << "\t" << query.end  << "\t" 
	  << query.end  - query.start  + 1
	  << "\t" << qrySeqLen <<"\t"<< strand << "\t" << score << "\t" << mismap    
	  << "\t" << type      <<"\t"<< reAlignStat    << "\n";
	return;
}

void VarUnit::OutErr() {
// Output the alignment to STDERR

	string reAlignStat = (isSuccessAlign) ? "CanAGE": "CanotAGE";
    cerr << target.id << "\t" << target.start << "\t" << target.end << "\t"
      << target.end - target.start + 1
	  << "\t" << cipos.first  << ","  << cipos.second << "\t" << ciend.first 
	  << ","  << ciend.second << "\t" << query.id     << "\t" << query.start   
	  << "\t" << query.end    << "\t" << query.end  - query.start  + 1
      << "\t" << strand << "\t" << homoRun << "\t" << isGoodReAlign 
	  << "\t" << score  << "\t" << mismap  << "\t" << type << "\t"
	  << reAlignStat    << "\n";
    return;
}

void VarUnit::OutStd(long int tarSeqLen, long int exp_tarSeqLen, 
					 long int qrySeqLen, ofstream &O) {

	if (exp_target.isEmpty()) { 
		cerr << "[ERROR]exp_target is empty!\n"; 
		exit(1); 
	}

	OutStd(tarSeqLen, qrySeqLen, O);
	O << exp_target.id << "\t" << exp_target.start << "\t" << exp_target.end 
	  << "\t" << exp_target.end - exp_target.start + 1 
	  << "\t" << exp_tarSeqLen << "\t" << query.id << "\t" << query.start   
	  << "\t" << query.end     << "\t" << query.end - query.start + 1 
	  << "\t" << qrySeqLen     << "\t" << strand << "\t" << score 
	  << "\t" << mismap << "\t" << type + "-E"   << endl;
	return;
}

void VarUnit::PrintStd() {
// Output the alignment to STDERR

    if (tarSeq.empty() || qrySeq.empty()){
        std::cerr << "tarSeq.empty() || qrySeq.empty()" << endl; exit(1);
    }
	cout << "# " << type   << ":" << strand       << ":" << score 
		 << ":"  << mismap << ":" << target.id    << "-" << target.start
		 << "-"  << target.end << ":" << query.id << "-" << query.start 
		 << "-"  << query.end  << "\n";
    return;
}

vector<VarUnit> MergeVarUnit(vector<VarUnit> &VarUnitVector, int distDelta = 1) {
// CAUTION : Merge vector<VarUnit>, but all new merge result just 
// using the same strand of first element: VarUnitVector[0]!!
// And I'm not going to keep all the query infomation after merging!!

    bool flag(false);

    VarUnit varunit;
    vector<VarUnit> newVector;
    map<string, long int> tarPrePos, qryPrePos;
    map<string, string> id2seq;
    for (size_t i(0); i < VarUnitVector.size(); ++i) {

        string tarId = VarUnitVector[i].target.id;
        string qryId = VarUnitVector[i].query.id ;
        string id  = VarUnitVector[i].target.id + ":" + VarUnitVector[i].query.id;
        string seq = VarUnitVector[i].tarSeq + "-" + VarUnitVector[i].qrySeq;

        if (!tarPrePos.count(tarId) || !qryPrePos.count(qryId) || !id2seq.count(id)) {

            // The light is on => Get the region!
            if (flag) newVector.push_back(varunit);
            varunit    = VarUnitVector[i];
            id2seq[id] = seq;
            flag       = true; // first time
        } else {

            if (tarPrePos[tarId] > VarUnitVector[i].target.start) {
                std::cerr << "[ERROR]Your target hasn't been sorted.\n";
                VarUnitVector[i].target.OutErrReg();
                exit(1);
            }
			
            if (qryPrePos[qryId] > VarUnitVector[i].query.start) {
                std::cerr << "[ERROR]Your query hasn't been sorted.\n";
                VarUnitVector[i].query.OutErrReg();
                exit(1);
            }
			
            if (varunit.target.end + distDelta >= VarUnitVector[i].target.start
                && varunit.query.end + distDelta >= VarUnitVector[i].query.start
                && id2seq[id] == seq) {

                if (VarUnitVector[i].target.end > varunit.target.end)
                    varunit.target.end = VarUnitVector[i].target.end;
				
                if (VarUnitVector[i].query.end > varunit.query.end)
                    varunit.query.end = VarUnitVector[i].query.end;
				
            } else {

                newVector.push_back(varunit);
                varunit = VarUnitVector[i];
            }
        }
        tarPrePos[tarId] = VarUnitVector[i].target.start;
        qryPrePos[qryId] = VarUnitVector[i].query.start;
        id2seq[id]       = seq;
    }
    if (flag) newVector.push_back(varunit);

    return newVector;
}

/**********************************
 * Author : Shujia Huang
 * Date   : 2014-08-05 22:49:56
 *
 * Class  AgeAlignment
 **********************************/

void AgeAlignment::Init(VarUnit &v, AgeOption opt) {

	vu_     = v;
	rvu_    = v; // No change
	para_   = opt;
	isInit_ = true;
}

bool AgeAlignment::Align(string &tarFa, string &qryFa) {

	if (!isInit_) { 
		cerr << "[ERROR] You should init AgeAlignment before calling Align()\n";
		exit(1);
	}

	int flag = 0;
    if (para_.indel) flag |= AGEaligner::INDEL_FLAG;
    if (para_.inv  ) flag |= AGEaligner::INVERSION_FLAG;
    if (para_.invl ) flag |= AGEaligner::INVL_FLAG;
    if (para_.invr ) flag |= AGEaligner::INVR_FLAG;
    if (para_.tdup ) flag |= AGEaligner::TDUPLICATION_FLAG;
    if (flag == 0) flag  = AGEaligner::INDEL_FLAG; // Default

    int n_modes = 0;
    if (flag & AGEaligner::INDEL_FLAG)        n_modes++;
    if (flag & AGEaligner::TDUPLICATION_FLAG) n_modes++;
    if (flag & AGEaligner::INVR_FLAG)         n_modes++;
    if (flag & AGEaligner::INVL_FLAG)         n_modes++;
    if (flag & AGEaligner::INVERSION_FLAG)    n_modes++;

    if (n_modes != 1) {
        cerr << "# [Error] In mode specification. ";
        if ( n_modes == 0 ) cerr << "No mode is specified.\n";
        if ( n_modes  > 1 ) cerr << "More than one mode is specified.\n";
        exit(1);
    }
	//////////////////////////////////////////////////////////////////////
	
	ExtendVU(tarFa.length(), qryFa.length(), para_.extendVarFlankSzie);
    // Do not AGE if the memory cost is bigger than 10G.
    long int varTarSize = vu_.target.end - vu_.target.start;
	long int varQrySize = vu_.query.end  - vu_.query.start;
    if (IsHugeMemory(varTarSize, varQrySize)) return false;

#ifdef AGE_TIME
timeval ali_s,ali_e;
gettimeofday(&ali_s, NULL);
#endif

	Sequence *tarSeq = new Sequence(tarFa, vu_.target.id, 1, false);
	Sequence *qrySeq = new Sequence(qryFa, vu_.query.id , 1, false);
	Sequence *tar = tarSeq->substr(vu_.target.start, vu_.target.end);
	Sequence *qry = qrySeq->substr(vu_.query.start , vu_.query.end );
	Scorer scr(para_.match, para_.mismatch, para_.gapOpen, para_.gapExtend);

	isalign_ = true;
	if (para_.both) {
            
        Sequence *qryClone = qry->clone(); 
        qryClone->revcom();
        AGEaligner aligner1(*tar, *qry);
        AGEaligner aligner2(*tar, *qryClone);                                         
        bool res1 = aligner1.align(scr, flag);
		bool res2 = aligner2.align(scr, flag);

		if (!res1 && !res2) {
			cerr<<"No alignment made.\n";
			isalign_ = false;
		} else if (aligner1.score() >= aligner2.score()) {
			aligner1.SetAlignResult(); 
			alignResult_ = aligner1.align_result();

			rvu_.PrintStd();
			aligner1.printAlignment();
			cout << "\n";
        } else {
			aligner2.SetAlignResult();
			alignResult_ = aligner2.align_result();

			rvu_.PrintStd();
			aligner2.printAlignment();
			cout << "\n";
        } 
        delete qryClone;

    } else {

        AGEaligner aligner(*tar, *qry);
        if (aligner.align(scr, flag)){
			aligner.SetAlignResult();
			alignResult_ = aligner.align_result();

			rvu_.PrintStd();
			aligner.printAlignment();
			cout << "\n";
        } else {
            cerr << "No alignment made.\n";
			isalign_ = false; 
        }
    }
/*
if (isalign_) {
cerr << "\n";
for (size_t i(0); i < alignResult_._map.size(); ++i) {
	cerr << alignResult_._map[i].first._sequence << "\t" << alignResult_._map[i].first._id << " " << alignResult_._map[i].first._start << "\t" << alignResult_._map[i].first._end << "\n";
	cerr << alignResult_._map_info[i] << "\n";
	cerr << alignResult_._map[i].second._sequence << "\t" << alignResult_._map[i].second._id << " " << alignResult_._map[i].second._start << "\t" << alignResult_._map[i].second._end << "\n";
}
cerr << "\n";
}
*/

#ifdef AGE_TIME
gettimeofday(&ali_e, NULL);
cerr << "\nAlignment time is " << ali_e.tv_sec - ali_s.tv_sec
    + (ali_e.tv_usec - ali_s.tv_usec)/1e+6 <<" s\n\n";
#endif

	Sequence::deleteSequences(tarSeq);
	Sequence::deleteSequences(qrySeq);

	return isalign_;
}

vector<VarUnit> AgeAlignment::VarReCall() {
// debug here carefully

	const double MISMAP_VALE = 0.01;
	vector<VarUnit> vus;
	if (isalign_) {

		isgoodAlign_ = false;
		if (alignResult_._identity.size() >= 3 &&
			alignResult_._identity[0].second > 98 && // Average identity
			alignResult_._identity[1].first  > 30 && // left side size
			alignResult_._identity[1].second > 95 && // left identity
			alignResult_._identity[2].first  > 30 && // right side size
			alignResult_._identity[2].second > 95)   // right identity 
			isgoodAlign_ = true;

		if (alignResult_._identity.size() > 3) {
			cerr << "[WARNING] alignResult_._identity.size() > 3!! It may be";
			cerr << "a bug, please contact the author to confirm!\n";
		}
			
		// Call the variant in the excise region
		pair<MapData, MapData> pre_map = alignResult_._map[0];
		for (size_t i(1); i < alignResult_._map.size(); ++i) {
			// Variant in excise region	
			VarUnit var = CallVarInExcise(pre_map, alignResult_._map[i], 
										  alignResult_._strand);
			var.isSuccessAlign = true;
			// mismatch probability should be low
			var.isGoodReAlign  = isgoodAlign() && (var.mismap < MISMAP_VALE);
			var.homoRun        = HomoRun(); //Just usefull in excise region
			var.cipos.first    = (cipos().first > 0) ? cipos().first  - var.target.start : 0; // Just here
			var.cipos.second   = (cipos().second> 0) ? cipos().second - var.target.start : 0; // Just here
			var.ciend.first    = (ciend().first > 0) ? ciend().first  - var.target.end : 0;   // Just here
			var.ciend.second   = (ciend().second> 0) ? ciend().second - var.target.end : 0;   // Just here
			var.identity       = alignResult_._identity;

			vus.push_back(var);
			pre_map = alignResult_._map[i];
		}

		/* I'm not sure whether we should call variant in Flank region or not!!
		// Call the variant in the flank sequence of variant
		for (size_t i(0); i < alignResult_._map.size(); ++i) {

			vector<VarUnit> var = CallVarInFlank(alignResult_._map[i], 
												 alignResult_._map_info[i],
												 alignResult_._strand);
			for (size_t i(0); i < var.size(); ++i) {
				var[i].identity       = alignResult_._identity;
				var[i].isSuccessAlign = true;
                var[i].isGoodReAlign  = isgoodAlign() && (var[i].mismap < MISMAP_VALE);;
				vus.push_back(var[i]); 
			}
		}
		// */
	}	

	return vus;	
}

VarUnit AgeAlignment::CallVarInExcise(pair<MapData, MapData> &lf, // Left side
									  pair<MapData, MapData> &rt, // Right side
									  char strand) {

	if (lf.first._id != rt.first._id || lf.second._id != rt.second._id) {
		cerr << "[BUG] The id is not match!!\n" << lf.first._id << ", "
			 << rt.first._id  << "; " << lf.second._id          << ", " 
			 << rt.second._id << "\n";
		exit(1);
	}

	int qlen = abs(rt.second._start - lf.second._end) - 1; // Query 
	int tlen = rt.first._start  - lf.first._end  - 1;      // Reference
	if (tlen < 0) cerr << "[WARNING] It'll cause bug below!\n";

	VarUnit vu;
	vu.strand    = strand;
	vu.score     = vu_.score; // It's the LAST aligne score
	vu.mismap    = vu_.mismap;// It's the LAST mismap probability
	vu.target.id = lf.first._id;
	vu.query.id  = lf.second._id;
	if (tlen > 0 && qlen == 0) {
	// Pure-Deletion
		vu.type = "DEL";

		// Reference position
		vu.target.start= lf.first._end;
		vu.target.end  = vu.target.start + tlen;

		// Query position 
		// CAUTION: Here is not just the variant region, but include one
		// position which on the boundary of variant. So that we don't have 
		// to add one base of reference at ALT field in VCF file.
		// vu.query.start = (strand == '+') ? lf.second._end : rt.second._start;
		vu.query.start = lf.second._end; // lf.second._end mapped to lf.first._end
		vu.query.end   = vu.query.start;

	} else if (qlen > 0 && tlen == 0) {
	// Pure-Insertion
		vu.type = "INS";
		// Reference position
		vu.target.start = lf.first._end;
		vu.target.end   = vu.target.start;

        // Query position
		// CAUTION: Here is not just the variant region, but include one
		// position which on the boundary of variant. So that we don't have 
		// to add one base of reference at ALT field in VCF file.
		vu.query.start = (strand == '+') ? lf.second._end : rt.second._start + 1;
		vu.query.end   = vu.query.start + qlen;
	} else if (tlen == qlen) {
		vu.type = (tlen == 1) ? "SNP" : "MNP";
		vu.target.start = lf.first._end + 1;
		vu.target.end   = vu.target.start + tlen - 1;
		vu.query.start  = (strand == '+') ? lf.second._end + 1 : rt.second._start + 1;
		vu.query.end    = vu.query.start + qlen - 1;
	} else {
		// Simultaneous gap => REPLACEMENT
		vu.type = "REPLACEMENT";
		vu.target.start = lf.first._end;
		vu.target.end   = vu.target.start + tlen;
		vu.query.start  = (strand == '+') ? lf.second._end : rt.second._start + 1;
		vu.query.end    = vu.query.start + qlen;
	}

	return vu;
}

vector<VarUnit> AgeAlignment::CallVarInFlank(pair<MapData, MapData> &m, 
                                             string &mapInfo, char strand) {
/**
 * GNAGGAGGTAGGCAGATCC-TGGGGCCAGTGGCATATGGGGCCTGGACACAGGGCGGCCT first
 * |.||||||||||||||||| |||||||||||||| |||||||||||||.||||||||||| Map Info
 * GGAGGAGGTAGGCAGATCCCTGGGGCCAGTGGCA-ATGGGGCCTGGACTCAGGGCGGCCT second
 **/
// Should carefully about the breakpoint of Indel. 

	int inc1 = 1;
    int inc2 = 1; if (strand == '-') inc2 = -1;
	int pos1start(m.first._start), pos2start(m.second._start);
	int pos1end(m.first._start),   pos2end(m.second._start);

	vector<VarUnit> vus;
	VarUnit vuTmp;
	vuTmp.strand    = strand;
    vuTmp.score     = vu_.score; // It's the LAST aligne score
    vuTmp.mismap    = vu_.mismap;// It's the LAST mismap probability
    vuTmp.target.id = m.first._id;
    vuTmp.query.id  = m.second._id;

	for (size_t i(0); i < mapInfo.size(); ++i) {

		if (mapInfo[i] == '|') {
		// Homo block		

			vuTmp.type   = "REFCALL-AGE";
			vuTmp.tarSeq = ".";
			vuTmp.qrySeq = ".";

			++i;
			while (mapInfo[i] == '|' && i < mapInfo.size()) {
				pos1end += inc1;
				pos2end += inc2;
				++i;
			}
			--i; // Rock back 1 position
		} else if (mapInfo[i] == ' ') {
		// New Indel!
			vuTmp.type = (m.first._sequence[i] == '-') ? "INS-AGE" : "DEL-AGE";
			if (vuTmp.type == "INS-AGE") { // Insertion
			// Caution: pos2start -= inc2 is to make the first position  
			// match with each other, which on the boundary of varaints
			// not the base in variant region!!!
				pos1start -= inc1; 
				pos1end   -= inc1; 
				pos2start -= inc2; 
			} else { // Deletion
			// Caution: pos1start -= inc1 is to make the first position  
			// match with each other, which on the boundary of varaints
			// not the base in variant region!!!
				pos2start -= inc2; 
				pos2end   -= inc2; 
				pos1start -= inc1; 
			}

			++i;
			while (mapInfo[i] == ' ' && i < mapInfo.size()) {
				if (m.first._sequence[i]  != '-') pos1end += inc1; 
				if (m.second._sequence[i] != '-') pos2end += inc2;
				++i;
			}
			--i; // Rock back 1 position
		} else if (mapInfo[i] == '.') {
		// New SNP or Map 'N'!
			if (toupper(m.first._sequence[i] == 'N')) {
				vuTmp.type   = "N-AGE";
				vuTmp.tarSeq = "N";
            	vuTmp.qrySeq = ".";
			} else if (toupper(m.second._sequence[i]) == 'N') {
				vuTmp.type   = "N-AGE";
				vuTmp.tarSeq = ".";
            	vuTmp.qrySeq = "N";
			} else {
				vuTmp.type   = "SNP-AGE";
				vuTmp.tarSeq = "."; // Do not put base here! Not now!
            	vuTmp.qrySeq = "."; // Do not put base here! Not now!
			}

			++i;
			while (mapInfo[i] == '.' && i < mapInfo.size()) {
				pos1end += inc1;
                pos2end += inc2;
                ++i;
			}
			--i;
			if (vuTmp.type == "SNP-AGE" && pos1end - pos1start > 0)
				vuTmp.type = "MNP-AGE"; // Could just call by AGE process
		} else {
		// Who knows ... It's an ERROR!!
			cerr << "[ERROR] What is it?!" << mapInfo[i] 
				 << " Must be some error in AGE aligne.\nDetail : \n";
			cerr << m.first._sequence << "\t" << m.first._id  << ":" 
				 << m.first._start    << "-"  << m.first._end << "\n"
				 << mapInfo << "\n"
				 << m.second._sequence << "\t" << m.second._id  << ":"
                 << m.second._start    << "-"  << m.second._end << "\n";
			exit(1);
		}

		vuTmp.target.start = pos1start;
        vuTmp.target.end   = pos1end;
        vuTmp.query.start  = (pos2end >= pos2start) ? pos2start : pos2end;
        vuTmp.query.end    = (pos2end >= pos2start) ? pos2end   : pos2start;

		vus.push_back(vuTmp);

		pos1end  += inc1;
		pos2end  += inc2;
        pos1start = pos1end;
        pos2start = pos2end;
	}

	return vus;
}

void AgeAlignment::ExtendVU(long int tarFaSize, 
							long int qryFaSize,
							int extandFlankSzie) {
	if (!isInit_) { 
		cerr<<"[ERROR]You should init AgeAlignment before calling ExtendVU()\n";
		exit(1);
	}
	
	vu_.target.start -= extandFlankSzie;
	vu_.target.end   += extandFlankSzie;
	vu_.query.start  -= extandFlankSzie;
	vu_.query.end    += extandFlankSzie;

	if (vu_.target.start < 1) vu_.target.start = 1;
    if (vu_.target.end   > tarFaSize) vu_.target.end = tarFaSize;	
    if (vu_.query.start  < 1) vu_.query.start = 1; 
    if (vu_.query.end    > qryFaSize) vu_.query.end  = qryFaSize;

	return;
}

bool AgeAlignment::IsHugeMemory(long int n, long int m) { 

	return ((((long double)(5 * n * m)) / 1000000000.0 + 0.5) > 10.0); // 10G
}
