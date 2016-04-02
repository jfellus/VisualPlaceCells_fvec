/*
 * main.cpp
 *
 *  Created on: 11 déc. 2014
 *      Author: jfellus
 */

#include "utils/utils.h"
#include "matrix/Matrix.h"

using namespace shared_matrices;


float POWER_NORM = get_config("POWER_NORM", 1.0);
uint EXPLO_PANO_NB_IMAGES = get_config("EXPLO_PANO_NB_IMAGES", 20);
uint PC_PANO_NB_IMAGES = get_config("PC_PANO_NB_IMAGES", 15);
uint NB_CODEWORDS = get_config("NB_CODEWORDS", 32);


string DATA_PATH = get_config_str("DATA_PATH", "/home/jfellus/Documents/These/prog/datasets/UCP-ENSEA");

string PC_INDEX = get_config_str("PC_INDEX", TOSTRING(DATA_PATH << "/PC/index_PC_"<< NB_CODEWORDS << ".fvec"));
string PC_DEF_FILE = get_config_str("PC_DEF_FILE", TOSTRING(DATA_PATH << "/PC/panos.txt"));
bool USE_PC_DEF_FILE = true;
string PROJECTORS = get_config_str("PROJECTORS", TOSTRING(DATA_PATH << "/PC/proj"<< NB_CODEWORDS << "_256"));
string CODEBOOK = get_config_str("CODEBOOK", TOSTRING(DATA_PATH << "/features_learning/dico"<< NB_CODEWORDS));

string EXPLO_IMAGES = get_config_str("EXPLO_IMAGES", TOSTRING(DATA_PATH << "/explo/retour1/images"));
string EXPLO_IMAGES_LIST = get_config_str("EXPLO_IMAGES_LIST", TOSTRING(EXPLO_IMAGES << "/../images.txt"));
string EXPLO_INDEX = get_config_str("EXPLO_INDEX", TOSTRING(EXPLO_IMAGES << "/../compact_"<< NB_CODEWORDS << ".fvec"));
string EXPLO_DEF_FILE = get_config_str("EXPLO_DEF_FILE", TOSTRING(EXPLO_IMAGES << "/../panos.txt"));
bool USE_EXPLO_DEF_FILE = false;

string RAW_SIM_FILE = get_config_str("RAW_SIM_FILE", TOSTRING(EXPLO_IMAGES << "/../raw_similarities.txt"));
string PC_SIM_FILE = get_config_str("PC_SIM_FILE", TOSTRING(EXPLO_IMAGES << "/../pc_similarities.txt"));
string PC_WINNER_FILE = get_config_str("PC_WINNER_FILE", TOSTRING(EXPLO_IMAGES << "/../pc_winner.txt"));


typedef struct {	int pc;	int nbViews; } DEF;
std::vector<DEF> pc_defs;
std::vector<DEF> explo_defs;


int main(int argc, char **argv) {
	try {
		DBGV(DATA_PATH);
		DBGV(PC_INDEX);
		DBGV(PROJECTORS);
		DBGV(CODEBOOK);
		DBGV(EXPLO_IMAGES);
		DBGV(EXPLO_IMAGES_LIST);
		DBGV(EXPLO_INDEX);

		shell(TOSTRING("mkdir -p `dirname " << PC_SIM_FILE << "`"));


		if(USE_PC_DEF_FILE) {
			DBG("Load PC_DEF_FILE " << PC_DEF_FILE);
			std::ifstream f(PC_DEF_FILE);
			while(f.good()) {
				DEF d; f >> d.pc; f >> d.nbViews;
				pc_defs.push_back(d);
			}
			f.close();
			DBG("ok");
		}
		if(USE_EXPLO_DEF_FILE) {
			DBG("Load EXPLO_DEF_FILE " << EXPLO_DEF_FILE);
			std::ifstream f(EXPLO_DEF_FILE);
			while(f.good()) {
				DEF d; f >> d.pc; f >> d.nbViews;
				explo_defs.push_back(d);
			}
			f.close();
			DBG("ok");
		}


		DBG("Load PC_INDEX " << PC_INDEX);
		Matrix pc_index(PC_INDEX);
		if(!pc_index) ERROR(PC_INDEX << "can't be loaded !");
		DBG(pc_index.height << " features of size "<< pc_index.width);

		DBG("Load EXPLO_INDEX " << EXPLO_INDEX);
		Matrix explo_index(EXPLO_INDEX);
		if(!explo_index) ERROR(EXPLO_INDEX << "can't be loaded !");
		DBG(explo_index.height << " features of size "<< explo_index.width);

		if(explo_index.width != pc_index.width) ERROR("Incompatible place cells and explo indexes !");

		DBG("Power normalization ... ");
		vector_pow_float(pc_index, POWER_NORM, pc_index.height*pc_index.width);
		vector_pow_float(explo_index, POWER_NORM, explo_index.height*explo_index.width);
		DBG("L2 normalization ... ");
		for(size_t i = 0 ; i < pc_index.height ; i++) {
			float n2 = vector_n2_float(pc_index.get_row(i), pc_index.width);
			vector_sdiv_float(pc_index.get_row(i), n2, pc_index.width);
		}
		for(size_t i = 0 ; i < explo_index.height ; i++) {
			float n2 = vector_n2_float(explo_index.get_row(i), explo_index.width);
			vector_sdiv_float(explo_index.get_row(i), n2, explo_index.width);
		}
		DBG("ok");

		DBG("Compute similarities ... ");
		Matrix sim(pc_index.height, explo_index.height);
		DBG("RAw SIMILARITIES IS AN " << sim.height << "x" << sim.width << "matrix");
		for(size_t i=0; i<explo_index.height; i++) {
			for(size_t pc=0; pc<pc_index.height; pc++) {
				float dot = vector_ps_float(explo_index.get_row(i), pc_index.get_row(pc), explo_index.width);
				sim[i*pc_index.height + pc] = dot;
			}
		}
		DBG("DONE");

		if(RAW_SIM_FILE[RAW_SIM_FILE.length()-1]=='t') {
			std::ofstream f(RAW_SIM_FILE);
			for(uint i=0; i<sim.height; i++) {
				for(uint j=0; j<sim.width; j++) {
					if(j!=0) f << " ";
					f << sim[i*sim.width + j];
				}
				f << "\n";
			}
			f.close();
		} else sim.save(RAW_SIM_FILE);

		if(PC_SIM_FILE[PC_SIM_FILE.length()-1]=='t') {
			std::ofstream f(PC_SIM_FILE);

			uint NB_PC = USE_PC_DEF_FILE ? pc_defs.size() : sim.width/PC_PANO_NB_IMAGES;
			uint NB_EXPLO = USE_EXPLO_DEF_FILE ? explo_defs.size() : sim.height/EXPLO_PANO_NB_IMAGES;

			DBG("PC SIMILARITIES IS A " << NB_EXPLO << "x" << NB_PC << " matrix");

			uint first_pano_view=0;
			for(uint pano=0; pano<NB_EXPLO; pano++) {
				uint nbExploViews = USE_EXPLO_DEF_FILE ? explo_defs[pano].nbViews : EXPLO_PANO_NB_IMAGES;
				f << pano << " ";

				uint first_pc_view=0;
				for(uint pc=0; pc<NB_PC; pc++) {
					uint nbPcViews = USE_PC_DEF_FILE ? pc_defs[pc].nbViews : PC_PANO_NB_IMAGES;

					if(pc!=0) f << " ";

					float score = 0;
					for(uint i=0; i<nbExploViews; i++) {
						float bestmatch = -100000;
						for(uint j=0; j<nbPcViews; j++) {
							float dot = sim[first_pc_view + j + (first_pano_view + i)*sim.width];
							if(dot > bestmatch) bestmatch = dot;
						}
						score += bestmatch;
					}
					score/=nbExploViews;
					f << score;
					first_pc_view += nbPcViews;
				}
				f << "\n";
				first_pano_view += nbExploViews;
			}
			f.close();
		} else sim.save(PC_SIM_FILE);

		if(!PC_WINNER_FILE.empty()) {
			std::ofstream f(PC_WINNER_FILE);

			uint NB_PC = USE_PC_DEF_FILE ? pc_defs.size() : sim.width/PC_PANO_NB_IMAGES;
			uint NB_EXPLO = USE_EXPLO_DEF_FILE ? explo_defs.size() : sim.height/EXPLO_PANO_NB_IMAGES;

			DBG("PC SIMILARITIES IS A " << NB_EXPLO << "x" << NB_PC << " matrix");

			uint first_pano_view=0;
			for(uint pano=0; pano<NB_EXPLO; pano++) {
				uint nbExploViews = USE_EXPLO_DEF_FILE ? explo_defs[pano].nbViews : EXPLO_PANO_NB_IMAGES;
				f << pano << " ";

				uint first_pc_view=0;
				float winner_score = 0;
				int winner_pc = -1;
				for(uint pc=0; pc<NB_PC; pc++) {
					uint nbPcViews = USE_PC_DEF_FILE ? pc_defs[pc].nbViews : PC_PANO_NB_IMAGES;

					float score = 0;
					for(uint i=0; i<nbExploViews; i++) {
						float bestmatch = -100000;
						for(uint j=0; j<nbPcViews; j++) {
							float dot = sim[first_pc_view + j + (first_pano_view + i)*sim.width];
							if(dot > bestmatch) bestmatch = dot;
						}
						score += bestmatch;
					}
					score/=nbExploViews;
					first_pc_view += nbPcViews;

					if(score >= winner_score) {	winner_pc = pc; winner_score = score; }
				}
				f << winner_pc << " " << winner_score << "\n";
				first_pano_view += nbExploViews;
			}
			f.close();
		}

		return 0;
	} catch(const char* msg) {	ERROR(msg);	}
}
