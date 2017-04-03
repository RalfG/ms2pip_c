#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include "models/vectors_train_h5B_c.c"
//#include "models/vectors_train_h5Y_c.c"

#include "models/modelB_PDX.c"
#include "models/modelY_PDX.c"

//#include "models/modelBnew.c"
//#include "models/modelYnew.c"

//#include "models/dB.c"
//#include "models/dY.c"

float membuffer[10000];
unsigned int v[30000];
float ions[5000];
float predictions[5000];

//for Omega: comment and uncomment
float amino_masses_tmp[19] = {71.037114,103.00919,115.026943,129.042593,147.068414,57.021464,137.058912,113.084064,128.094963,131.040485,114.042927,97.052764,128.058578,156.101111,87.032028,101.047679,99.068414,186.079313,163.063329};
float* amino_masses;
float ntermmod;

unsigned short bas[19] = {37,35,59,129,94,0,210,81,191,106,101,117,115,343,49,90,60,134,104};
unsigned short heli[19] = {68,23,33,29,70,58,41,73,32,66,38,0,40,39,44,53,71,51,55};
unsigned short hydro[19] = {51,75,25,35,100,16,3,94,0,82,12,0,22,22,21,39,80,98,70};
unsigned short pI[19] = {32,23,0,4,27,32,48,32,69,29,26,35,28,79,29,28,31,31,28};
unsigned short* amino_F;

// This function initializes amino acid masses and PTMs from a configuration file generated by Omega
void c_ms2pip_init(char* amino_masses_fname) {
	int i,j;
	int nummods;
	float mz;
	int numptm;
	int before;
	int after;

	FILE* f = fopen(amino_masses_fname,"rt");
	fscanf(f,"%i\n",&nummods);
	fclose(f);

	//malloc
	amino_masses = (float*) malloc((38+nummods)*sizeof(float));
	amino_F = (unsigned short*) malloc((38+nummods)*sizeof(unsigned short));

	for (i=0; i< 19; i++) {
		amino_masses[i] = amino_masses_tmp[i];
		amino_F[i] = (unsigned short) (amino_masses[i]-57.021464);
	}

	for (i=0; i< 19; i++) {
		amino_masses[19+i]=amino_masses[i];
		amino_F[19+i]=amino_F[i];
		}

	j = 38;
	f = fopen(amino_masses_fname,"rt");
	fscanf(f,"%i\n",&nummods);
	for (i=0; i< nummods; i++) {
		fscanf(f,"%f\n",&amino_masses[j]);
		amino_F[j] = (unsigned short) (amino_masses[j]-57.021464);
		j++;
		}
	fclose(f);
}

//get fragment ion mz values
float* c_ms2pip_get_mz(int peplen, unsigned short* modpeptide, float nptm, float cptm)
	{
	int i,j;
	float mz;
	j=0;
	mz = nptm;
	for (i=0; i < peplen-1; i++) {
		mz += amino_masses[modpeptide[i]];
		membuffer[j++] = mz+1.007236;
	}
	mz = cptm;
	for (i=peplen-1; i >= 1; i--) {
		mz += amino_masses[modpeptide[i]];
		membuffer[j++] = 18.0105647+mz+1.007236;
	}
	return membuffer;
}

//get fragment ion peaks from spectrum
float* c_ms2pip_get_t(int peplen, unsigned short* modpeptide, int numpeaks, float* msms, float* peaks, float nptm, float cptm,float tolmz)
	{
	int i,j,tmp;
	float mz;
	int msms_pos;
	int mem_pos;
	float max, tmp2;

	for (i=0; i < 4*(peplen-1); i++) {
		ions[i] = -9.96578428466; //HARD CODED!!
		//ions[i] = 0; //HARD CODED!!
	}

	//b-ions

	mz = nptm;
	for (i=0; i < peplen-1; i++) {
		mz += amino_masses[modpeptide[i]];
		membuffer[i] = mz+1.007236;
	}

	msms_pos = 0;
	mem_pos = 0;
	while (1) {
		if (msms_pos >= numpeaks) {
			break;
		}
		if (mem_pos >= peplen) {
			break;
		}
		mz = membuffer[mem_pos];
		if (msms[msms_pos] > (mz+tolmz)) {
			mem_pos += 1;
		}
		else if (msms[msms_pos] < (mz-tolmz)) {
			msms_pos += 1;
		}
		else {
			max = peaks[msms_pos];
			tmp = msms_pos + 1;
			if (tmp < numpeaks) {
				while (msms[tmp] <= (mz+tolmz)) {
					tmp2 = peaks[tmp];
					if (max < tmp2) {
						max = tmp2;
					}
					tmp += 1;
					if (tmp == numpeaks) {
						break;
					}
				}
			}
			ions[mem_pos] = max;
			mem_pos += 1;
		}
	}

	// y-ions

	mz = cptm;
	j=0;
	for (i=peplen-1; i >= 1; i--) {
		mz += amino_masses[modpeptide[i]];
		membuffer[j] = 18.0105647+mz+1.007236;
		//printf("%f ",membuffer[j]);
		j++;
	}



	msms_pos = 0;
	mem_pos = 0;
	while (1) {
		if (msms_pos >= numpeaks) {
			break;
		}
		if (mem_pos >= peplen) {
			break;
		}
		mz = membuffer[mem_pos];
		if (msms[msms_pos] > (mz+tolmz)) {
			mem_pos += 1;
		}
		else if (msms[msms_pos] < (mz-tolmz)) {
			msms_pos += 1;
		}
		else {
			max = peaks[msms_pos];
			tmp = msms_pos + 1;
			if (tmp < numpeaks) {
				while (msms[tmp] <= (mz+tolmz)) {
					tmp2 = peaks[tmp];
					if (max < tmp2) {
						max = tmp2;
					}
					tmp += 1;
					if (tmp == numpeaks) {
						break;
					}
				}
			}
			ions[(peplen-1)+mem_pos] = max;
			//printf("F %f %f\n",mz,max);
			mem_pos += 1;
		}
	}

	//b++-ions

	mz = nptm;
	for (i=0; i < peplen-1; i++) {
		mz += amino_masses[modpeptide[i]];
		membuffer[i] = (mz+2*1.007236)/2;
	}

	msms_pos = 0;
	mem_pos = 0;
	while (1) {
		if (msms_pos >= numpeaks) {
			break;
		}
		if (mem_pos >= peplen) {
			break;
		}
		mz = membuffer[mem_pos];
		if (msms[msms_pos] > (mz+tolmz)) {
			mem_pos += 1;
		}
		else if (msms[msms_pos] < (mz-tolmz)) {
			msms_pos += 1;
		}
		else {
			max = peaks[msms_pos];
			tmp = msms_pos + 1;
			if (tmp < numpeaks) {
				while (msms[tmp] <= (mz+tolmz)) {
					tmp2 = peaks[tmp];
					if (max < tmp2) {
						max = tmp2;
					}
					tmp += 1;
					if (tmp == numpeaks) {
						break;
					}
				}
			}
			ions[2*(peplen-1)+mem_pos] = max;
			mem_pos += 1;
		}
	}

	// y++-ions

	mz = cptm;
	j=0;
	for (i=peplen-1; i >= 1; i--) {
		mz += amino_masses[modpeptide[i]];
		membuffer[j] = (18.0105647+mz+2*1.007236)/2;
		//printf("%f ",membuffer[j]);
		j++;
	}

	msms_pos = 0;
	mem_pos = 0;
	while (1) {
		if (msms_pos >= numpeaks) {
			break;
		}
		if (mem_pos >= peplen) {
			break;
		}
		mz = membuffer[mem_pos];
		if (msms[msms_pos] > (mz+tolmz)) {
			mem_pos += 1;
		}
		else if (msms[msms_pos] < (mz-tolmz)) {
			msms_pos += 1;
		}
		else {
			max = peaks[msms_pos];
			tmp = msms_pos + 1;
			if (tmp < numpeaks) {
				while (msms[tmp] <= (mz+tolmz)) {
					tmp2 = peaks[tmp];
					if (max < tmp2) {
						max = tmp2;
					}
					tmp += 1;
					if (tmp == numpeaks) {
						break;
					}
				}
			}
			ions[3*(peplen-1)+mem_pos] = max;
			//printf("F %f %f\n",mz,max);
			mem_pos += 1;
		}
	}

	return ions;
}



//Experiment: features that assume fixed length peptide datasets
unsigned int* c_ms2pip_get_v_bof_chem(int peplen, unsigned short* peptide, int charge)
	{
	int i,j;
	float mz;

	unsigned int max_bas_b = 0;
	unsigned int max_heli_b = 0;
	unsigned int max_hydro_b = 0;
	unsigned int max_pI_b = 0;
	unsigned int max_bas_y = 0;
	unsigned int max_heli_y = 0;
	unsigned int max_hydro_y = 0;
	unsigned int max_pI_y = 0;
	unsigned int min_bas_b = 999;
	unsigned int min_heli_b = 999;
	unsigned int min_hydro_b = 999;
	unsigned int min_pI_b = 999;
	unsigned int min_bas_y = 999;
	unsigned int min_heli_y = 999;
	unsigned int min_hydro_y = 999;
	unsigned int min_pI_y = 999;
	unsigned int total_bas = 0;
	unsigned int total_heli = 0;
	unsigned int total_hydro = 0;
	unsigned int total_pI = 0;
	unsigned int max_bas = 0;
	unsigned int max_heli = 0;
	unsigned int max_hydro = 0;
	unsigned int max_pI = 0;
	unsigned int min_bas = 999;
	unsigned int min_heli = 999;
	unsigned int min_hydro = 999;
	unsigned int min_pI = 999;

	mz = 0.;
	for (i=0; i < peplen; i++) {
		mz += amino_F[peptide[i]];
		total_bas += bas[peptide[i]];
		total_heli += heli[peptide[i]];
		total_hydro += hydro[peptide[i]];
		total_pI += pI[peptide[i]];
		if (max_bas < bas[peptide[i]]) {
			max_bas = bas[peptide[i]];
		}
		if (max_heli < heli[peptide[i]]) {
			max_heli = heli[peptide[i]];
		}
		if (max_hydro < hydro[peptide[i]]) {
			max_hydro = hydro[peptide[i]];
		}
		if (max_pI < pI[peptide[i]]) {
			max_pI = pI[peptide[i]];
		}
		if (min_bas > bas[peptide[i]]) {
			min_bas = bas[peptide[i]];
		}
		if (min_heli > heli[peptide[i]]) {
			min_heli = heli[peptide[i]];
		}
		if (min_hydro > hydro[peptide[i]]) {
			min_hydro = hydro[peptide[i]];
		}
		if (min_pI > pI[peptide[i]]) {
			min_pI = pI[peptide[i]];
		}
	}

	int mean_mz = (int) ((float)mz/peplen);
	int mean_bas = (int) ((float)total_bas/peplen);
	int mean_heli = (int) ((float)total_heli/peplen);
	int mean_hydro = (int) ((float)total_hydro/peplen);
	int mean_pI = (int) ((float)total_pI/peplen);

	float mzb = 0.;
	int sum_bas = 0;
	int sum_heli = 0;
	int sum_hydro = 0;
	int sum_pI = 0;
	int fnum = 0;

	for (i=0; i < peplen-1; i++) {
		max_bas_b = 0;
		max_heli_b = 0;
		max_hydro_b = 0;
		max_pI_b = 0;
		max_bas_y = 0;
		max_heli_y = 0;
		max_hydro_y = 0;
		max_pI_y = 0;
		min_bas_b = 999;
		min_heli_b = 999;
		min_hydro_b = 999;
		min_pI_b = 999;
		min_bas_y = 999;
		min_heli_y = 999;
		min_hydro_y = 999;
		min_pI_y = 999;

		v[fnum++] = mz;
		v[fnum++] = peplen;
		v[fnum++] = i;
		v[fnum++] = (int) 100*(float)i/peplen;
		v[fnum++] = mean_mz;
		v[fnum++] = mean_bas;
		v[fnum++] = mean_heli;
		v[fnum++] = mean_hydro;
		v[fnum++] = mean_pI;
		v[fnum++] = max_bas;
		v[fnum++] = max_heli;
		v[fnum++] = max_hydro;
		v[fnum++] = max_pI;
		v[fnum++] = min_bas;
		v[fnum++] = min_heli;
		v[fnum++] = min_hydro;
		v[fnum++] = min_pI;

		for (j=0; j<=i; j++) {
			if (bas[peptide[j]] > max_bas_b) {
				max_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_b) {
				max_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_b) {
				max_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_b) {
				max_pI_b = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_b) {
				min_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_b) {
				min_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_b) {
				min_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_b) {
				min_pI_b = pI[peptide[j]];
			}
		}
		for (j=i+1; j<peplen; j++) {
			if (bas[peptide[j]] > max_bas_y) {
				max_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_y) {
				max_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_y) {
				max_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_y) {
				max_pI_y = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_y) {
				min_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_y) {
				min_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_y) {
				min_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_y) {
				min_pI_y = pI[peptide[j]];
			}
		}

		v[fnum++] = max_bas_b;
		v[fnum++] = max_heli_b;
		v[fnum++] = max_hydro_b;
		v[fnum++] = max_pI_b;
		v[fnum++] = min_bas_b;
		v[fnum++] = min_heli_b;
		v[fnum++] = min_hydro_b;
		v[fnum++] = min_pI_b;

		v[fnum++] = max_bas_y;
		v[fnum++] = max_heli_y;
		v[fnum++] = max_hydro_y;
		v[fnum++] = max_pI_y;
		v[fnum++] = min_bas_y;
		v[fnum++] = min_heli_y;
		v[fnum++] = min_hydro_y;
		v[fnum++] = min_pI_y;

		mzb += amino_F[peptide[i]];
		v[fnum++] = (int) mzb;
		v[fnum++] = (int) (mz - mzb);
		v[fnum++] = (int) (mzb/(i+1));
		v[fnum++] = (int) ((mz-mzb)/(peplen-1-i));
		sum_bas += bas[peptide[i]];
		v[fnum++] = sum_bas;
		v[fnum++] = total_bas-sum_bas;
		v[fnum++] = (int) ((float)sum_bas/(i+1));
		v[fnum++] = (int) ((float)(total_bas-sum_bas)/(peplen-1-i));
		sum_heli += heli[peptide[i]];
		v[fnum++] = sum_heli;
		v[fnum++] = total_heli-sum_heli;
		v[fnum++] = (int) ((float)sum_heli/(i+1));
		v[fnum++] = (int) ((float)(total_heli-sum_heli)/(peplen-1-i));
		sum_hydro += hydro[peptide[i]];
		v[fnum++] = sum_hydro;
		v[fnum++] = total_hydro-sum_hydro;
		v[fnum++] = (int) ((float)sum_hydro/(i+1));
		v[fnum++] = (int) ((float)(total_hydro-sum_hydro)/(peplen-1-i));
		sum_pI += pI[peptide[i]];
		v[fnum++] = sum_pI;
		v[fnum++] = total_pI-sum_pI;
		v[fnum++] = (int) ((float)sum_pI/(i+1));
		v[fnum++] = (int) ((float)(total_pI-sum_pI)/(peplen-1-i));

		v[fnum++] = bas[peptide[i]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[i+1]];
		v[fnum++] = bas[peptide[i]]*bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]*heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]*hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]*pI[peptide[i+1]];

		v[fnum++] = bas[peptide[i]]-bas[peptide[i+1]]+1000;
		v[fnum++] = heli[peptide[i]]-heli[peptide[i+1]]+1000;
		v[fnum++] = hydro[peptide[i]]-hydro[peptide[i+1]]+1000;
		v[fnum++] = pI[peptide[i]]-pI[peptide[i+1]]+1000;
		v[fnum++] = bas[peptide[i+1]]-bas[peptide[i]]+1000;
		v[fnum++] = heli[peptide[i+1]]-heli[peptide[i]]+1000;
		v[fnum++] = hydro[peptide[i+1]]-hydro[peptide[i]]+1000;
		v[fnum++] = pI[peptide[i+1]]-pI[peptide[i]]+1000;

		v[fnum++] = bas[peptide[i]]+bas[peptide[0]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[0]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[0]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[0]];
		v[fnum++] = bas[peptide[peplen-1]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[peplen-1]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[peplen-1]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[peplen-1]]+pI[peptide[i+1]];

		for (j=0; j < peplen; j++) {
			v[fnum++]=amino_F[peptide[j]];
			v[fnum++]=bas[peptide[j]];
			v[fnum++]=hydro[peptide[j]];
			v[fnum++]=heli[peptide[j]];
			v[fnum++]=pI[peptide[j]];
		}

		v[fnum++] = charge;
	}
	return v;
}

//compute feature vectors from peptide
unsigned int* c_ms2pip_get_v(int peplen, unsigned short* peptide, unsigned short* modpeptide, int charge)
	{
	int i,j;
	float mz;

	int max_bas_b = 0;
	int max_heli_b = 0;
	int max_hydro_b = 0;
	int max_pI_b = 0;
	int max_bas_y = 0;
	int max_heli_y = 0;
	int max_hydro_y = 0;
	int max_pI_y = 0;
	int min_bas_b = 999;
	int min_heli_b = 999;
	int min_hydro_b = 999;
	int min_pI_b = 999;
	int min_bas_y = 999;
	int min_heli_y = 999;
	int min_hydro_y = 999;
	int min_pI_y = 999;

	unsigned int buf2[19];
	unsigned int buf3[19];

	for (i=0; i < 19; i++) {
		buf2[i] = 0;
		buf3[i] = 0;
	}

	for (i=0; i < peplen; i++) {
		buf3[peptide[i]]++;
	}

	unsigned int total_bas = 0;
	unsigned int total_heli = 0;
	unsigned int total_hydro = 0;
	unsigned int total_pI = 0;
	unsigned int max_bas = 0;
	unsigned int max_heli = 0;
	unsigned int max_hydro = 0;
	unsigned int max_pI = 0;
	unsigned int min_bas = 999;
	unsigned int min_heli = 999;
	unsigned int min_hydro = 999;
	unsigned int min_pI = 999;

	mz = 0.;
	for (i=0; i < peplen; i++) {
		mz += amino_F[modpeptide[i]];
		total_bas += bas[peptide[i]];
		total_heli += heli[peptide[i]];
		total_hydro += hydro[peptide[i]];
		total_pI += pI[peptide[i]];
		if (max_bas < bas[peptide[i]]) {
			max_bas = bas[peptide[i]];
		}
		if (max_heli < heli[peptide[i]]) {
			max_heli = heli[peptide[i]];
		}
		if (max_hydro < hydro[peptide[i]]) {
			max_hydro = hydro[peptide[i]];
		}
		if (max_pI < pI[peptide[i]]) {
			max_pI = pI[peptide[i]];
		}
		if (min_bas > bas[peptide[i]]) {
			min_bas = bas[peptide[i]];
		}
		if (min_heli > heli[peptide[i]]) {
			min_heli = heli[peptide[i]];
		}
		if (min_hydro > hydro[peptide[i]]) {
			min_hydro = hydro[peptide[i]];
		}
		if (min_pI > pI[peptide[i]]) {
			min_pI = pI[peptide[i]];
		}
	}

	int mean_mz = (int) ((float)mz/peplen);
	int mean_bas = (int) ((float)total_bas/peplen);
	int mean_heli = (int) ((float)total_heli/peplen);
	int mean_hydro = (int) ((float)total_hydro/peplen);
	int mean_pI = (int) ((float)total_pI/peplen);

	float mzb = 0.;
	int sum_bas = 0;
	int sum_heli = 0;
	int sum_hydro = 0;
	int sum_pI = 0;
	int fnum = 0;

	for (i=0; i < peplen-1; i++) {
		max_bas_b = 0;
		max_heli_b = 0;
		max_hydro_b = 0;
		max_pI_b = 0;
		max_bas_y = 0;
		max_heli_y = 0;
		max_hydro_y = 0;
		max_pI_y = 0;
		min_bas_b = 999;
		min_heli_b = 999;
		min_hydro_b = 999;
		min_pI_b = 999;
		min_bas_y = 999;
		min_heli_y = 999;
		min_hydro_y = 999;
		min_pI_y = 999;

		buf2[peptide[i]]++;
		for (j=0; j < 19; j++) {
			v[fnum++] = (int) 100*(((float) buf2[j])/(i+1));
		}
		buf3[peptide[i]]--;
		for (j=0; j < 19; j++) {
			v[fnum++] = (int) 100*(((float) buf3[j])/(peplen-i-1));
		}

		v[fnum++] = mz;
		v[fnum++] = peplen;
		v[fnum++] = i;
		v[fnum++] = (int) 100*(float)i/peplen;
		v[fnum++] = mean_mz;
		v[fnum++] = mean_bas;
		v[fnum++] = mean_heli;
		v[fnum++] = mean_hydro;
		v[fnum++] = mean_pI;
		v[fnum++] = max_bas;
		v[fnum++] = max_heli;
		v[fnum++] = max_hydro;
		v[fnum++] = max_pI;
		v[fnum++] = min_bas;
		v[fnum++] = min_heli;
		v[fnum++] = min_hydro;
		v[fnum++] = min_pI;

		for (j=0; j<=i; j++) {
			if (bas[peptide[j]] > max_bas_b) {
				max_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_b) {
				max_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_b) {
				max_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_b) {
				max_pI_b = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_b) {
				min_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_b) {
				min_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_b) {
				min_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_b) {
				min_pI_b = pI[peptide[j]];
			}
		}
		for (j=i+1; j<peplen; j++) {
			if (bas[peptide[j]] > max_bas_y) {
				max_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_y) {
				max_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_y) {
				max_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_y) {
				max_pI_y = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_y) {
				min_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_y) {
				min_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_y) {
				min_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_y) {
				min_pI_y = pI[peptide[j]];
			}
		}

		v[fnum++] = max_bas_b;
		v[fnum++] = max_heli_b;
		v[fnum++] = max_hydro_b;
		v[fnum++] = max_pI_b;
		v[fnum++] = min_bas_b;
		v[fnum++] = min_heli_b;
		v[fnum++] = min_hydro_b;
		v[fnum++] = min_pI_b;

		v[fnum++] = max_bas_y;
		v[fnum++] = max_heli_y;
		v[fnum++] = max_hydro_y;
		v[fnum++] = max_pI_y;
		v[fnum++] = min_bas_y;
		v[fnum++] = min_heli_y;
		v[fnum++] = min_hydro_y;
		v[fnum++] = min_pI_y;

		mzb += amino_F[modpeptide[i]];
		v[fnum++] = (int) mzb;
		v[fnum++] = (int) (mz - mzb);
		v[fnum++] = (int) (mzb/(i+1));
		v[fnum++] = (int) ((mz-mzb)/(peplen-1-i));
		sum_bas += bas[peptide[i]];
		v[fnum++] = sum_bas;
		v[fnum++] = total_bas-sum_bas;
		v[fnum++] = (int) ((float)sum_bas/(i+1));
		v[fnum++] = (int) ((float)(total_bas-sum_bas)/(peplen-1-i));
		sum_heli += heli[peptide[i]];
		v[fnum++] = sum_heli;
		v[fnum++] = total_heli-sum_heli;
		v[fnum++] = (int) ((float)sum_heli/(i+1));
		v[fnum++] = (int) ((float)(total_heli-sum_heli)/(peplen-1-i));
		sum_hydro += hydro[peptide[i]];
		v[fnum++] = sum_hydro;
		v[fnum++] = total_hydro-sum_hydro;
		v[fnum++] = (int) ((float)sum_hydro/(i+1));
		v[fnum++] = (int) ((float)(total_hydro-sum_hydro)/(peplen-1-i));
		sum_pI += pI[peptide[i]];
		v[fnum++] = sum_pI;
		v[fnum++] = total_pI-sum_pI;
		v[fnum++] = (int) ((float)sum_pI/(i+1));
		v[fnum++] = (int) ((float)(total_pI-sum_pI)/(peplen-1-i));

		v[fnum++] = bas[peptide[i]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[i+1]];
		v[fnum++] = bas[peptide[i]]*bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]*heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]*hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]*pI[peptide[i+1]];

		v[fnum++] = bas[peptide[i]]-bas[peptide[i+1]]+1000;
		v[fnum++] = heli[peptide[i]]-heli[peptide[i+1]]+1000;
		v[fnum++] = hydro[peptide[i]]-hydro[peptide[i+1]]+1000;
		v[fnum++] = pI[peptide[i]]-pI[peptide[i+1]]+1000;
		v[fnum++] = bas[peptide[i+1]]-bas[peptide[i]]+1000;
		v[fnum++] = heli[peptide[i+1]]-heli[peptide[i]]+1000;
		v[fnum++] = hydro[peptide[i+1]]-hydro[peptide[i]]+1000;
		v[fnum++] = pI[peptide[i+1]]-pI[peptide[i]]+1000;

		v[fnum++] = bas[peptide[i]]+bas[peptide[0]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[0]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[0]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[0]];
		v[fnum++] = bas[peptide[peplen-1]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[peplen-1]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[peplen-1]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[peplen-1]]+pI[peptide[i+1]];

		int pos = 0;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = 1;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = peplen-2;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = peplen-1;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		v[fnum] = 0;
		if (peptide[i] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		v[fnum] = 0;
		if (peptide[i+1] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 13) {
			v[fnum] = 1;
		}
		fnum++;


		v[fnum++] = bas[peptide[i]];
		if (i==0) {
			v[fnum++] = bas[peptide[i]];
		}
		else {
			v[fnum++] = bas[peptide[i-1]];
		}
		v[fnum++] = bas[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = bas[peptide[i+1]];
		}
		else {
			v[fnum++] = bas[peptide[i+2]];
		}

		v[fnum++] = heli[peptide[i]];
		if (i==0) {
			v[fnum++] = heli[peptide[i]];
		}
		else {
			v[fnum++] = heli[peptide[i-1]];
		}
		v[fnum++] = heli[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = heli[peptide[i+1]];
		}
		else {
			v[fnum++] = heli[peptide[i+2]];
		}

		v[fnum++] = hydro[peptide[i]];
		if (i==0) {
			v[fnum++] = hydro[peptide[i]];
		}
		else {
			v[fnum++] = hydro[peptide[i-1]];
		}
		v[fnum++] = hydro[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = hydro[peptide[i+1]];
		}
		else {
			v[fnum++] = hydro[peptide[i+2]];
		}

		v[fnum++] = pI[peptide[i]];
		if (i==0) {
			v[fnum++] = pI[peptide[i]];
		}
		else {
			v[fnum++] = pI[peptide[i-1]];
		}
		v[fnum++] = pI[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = pI[peptide[i+1]];
		}
		else {
			v[fnum++] = pI[peptide[i+2]];
		}

		v[fnum++] = amino_F[modpeptide[i]];
		if (i==0) {
			v[fnum++] = amino_F[modpeptide[i]];
		}
		else {
			v[fnum++] = amino_F[modpeptide[i-1]];
		}
		v[fnum++] = amino_F[modpeptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = amino_F[modpeptide[i+1]];
		}
		else {
			v[fnum++] = amino_F[modpeptide[i+2]];
		}

		v[fnum++] = charge;
	}
	return v;
}

//compute feature vector from peptide + predict intensities
float* c_ms2pip_get_p(int peplen, unsigned short* peptide, unsigned short* modpeptide, int charge)
	{
	int i,j;
	float mz;

	int max_bas_b = 0;
	int max_heli_b = 0;
	int max_hydro_b = 0;
	int max_pI_b = 0;
	int max_bas_y = 0;
	int max_heli_y = 0;
	int max_hydro_y = 0;
	int max_pI_y = 0;
	int min_bas_b = 999;
	int min_heli_b = 999;
	int min_hydro_b = 999;
	int min_pI_b = 999;
	int min_bas_y = 999;
	int min_heli_y = 999;
	int min_hydro_y = 999;
	int min_pI_y = 999;

	unsigned int buf2[19];
	unsigned int buf3[19];

	for (i=0; i < 19; i++) {
		buf2[i] = 0;
		buf3[i] = 0;
	}

	for (i=0; i < peplen; i++) {
		buf3[peptide[i]]++;
	}

	unsigned int total_bas = 0;
	unsigned int total_heli = 0;
	unsigned int total_hydro = 0;
	unsigned int total_pI = 0;
	unsigned int max_bas = 0;
	unsigned int max_heli = 0;
	unsigned int max_hydro = 0;
	unsigned int max_pI = 0;
	unsigned int min_bas = 999;
	unsigned int min_heli = 999;
	unsigned int min_hydro = 999;
	unsigned int min_pI = 999;

	mz = 0.;
	for (i=0; i < peplen; i++) {
		mz += amino_F[modpeptide[i]];
		total_bas += bas[peptide[i]];
		total_heli += heli[peptide[i]];
		total_hydro += hydro[peptide[i]];
		total_pI += pI[peptide[i]];
		if (max_bas < bas[peptide[i]]) {
			max_bas = bas[peptide[i]];
		}
		if (max_heli < heli[peptide[i]]) {
			max_heli = heli[peptide[i]];
		}
		if (max_hydro < hydro[peptide[i]]) {
			max_hydro = hydro[peptide[i]];
		}
		if (max_pI < pI[peptide[i]]) {
			max_pI = pI[peptide[i]];
		}
		if (min_bas > bas[peptide[i]]) {
			min_bas = bas[peptide[i]];
		}
		if (min_heli > heli[peptide[i]]) {
			min_heli = heli[peptide[i]];
		}
		if (min_hydro > hydro[peptide[i]]) {
			min_hydro = hydro[peptide[i]];
		}
		if (min_pI > pI[peptide[i]]) {
			min_pI = pI[peptide[i]];
		}
	}

	int mean_mz = (int) ((float)mz/peplen);
	int mean_bas = (int) ((float)total_bas/peplen);
	int mean_heli = (int) ((float)total_heli/peplen);
	int mean_hydro = (int) ((float)total_hydro/peplen);
	int mean_pI = (int) ((float)total_pI/peplen);

	float mzb = 0.;
	int sum_bas = 0;
	int sum_heli = 0;
	int sum_hydro = 0;
	int sum_pI = 0;

	for (i=0; i < peplen-1; i++) {
		int fnum = 0;
		max_bas_b = 0;
		max_heli_b = 0;
		max_hydro_b = 0;
		max_pI_b = 0;
		max_bas_y = 0;
		max_heli_y = 0;
		max_hydro_y = 0;
		max_pI_y = 0;
		min_bas_b = 999;
		min_heli_b = 999;
		min_hydro_b = 999;
		min_pI_b = 999;
		min_bas_y = 999;
		min_heli_y = 999;
		min_hydro_y = 999;
		min_pI_y = 999;

		buf2[peptide[i]]++;
		for (j=0; j < 19; j++) {
			v[fnum++] = (int) 100*(((float) buf2[j])/(i+1));
		}
		buf3[peptide[i]]--;
		for (j=0; j < 19; j++) {
			v[fnum++] = (int) 100*(((float) buf3[j])/(peplen-i-1));
		}

		v[fnum++] = mz;
		v[fnum++] = peplen;
		v[fnum++] = i;
		v[fnum++] = (int) 100*(float)i/peplen;
		v[fnum++] = mean_mz;
		v[fnum++] = mean_bas;
		v[fnum++] = mean_heli;
		v[fnum++] = mean_hydro;
		v[fnum++] = mean_pI;
		v[fnum++] = max_bas;
		v[fnum++] = max_heli;
		v[fnum++] = max_hydro;
		v[fnum++] = max_pI;
		v[fnum++] = min_bas;
		v[fnum++] = min_heli;
		v[fnum++] = min_hydro;
		v[fnum++] = min_pI;

		for (j=0; j<=i; j++) {
			if (bas[peptide[j]] > max_bas_b) {
				max_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_b) {
				max_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_b) {
				max_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_b) {
				max_pI_b = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_b) {
				min_bas_b = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_b) {
				min_heli_b = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_b) {
				min_hydro_b = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_b) {
				min_pI_b = pI[peptide[j]];
			}
		}
		for (j=i+1; j<peplen; j++) {
			if (bas[peptide[j]] > max_bas_y) {
				max_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] > max_heli_y) {
				max_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] > max_hydro_y) {
				max_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] > max_pI_y) {
				max_pI_y = pI[peptide[j]];
			}
			if (bas[peptide[j]] < min_bas_y) {
				min_bas_y = bas[peptide[j]];
			}
			if (heli[peptide[j]] < min_heli_y) {
				min_heli_y = heli[peptide[j]];
			}
			if (hydro[peptide[j]] < min_hydro_y) {
				min_hydro_y = hydro[peptide[j]];
			}
			if (pI[peptide[j]] < min_pI_y) {
				min_pI_y = pI[peptide[j]];
			}
		}

		v[fnum++] = max_bas_b;
		v[fnum++] = max_heli_b;
		v[fnum++] = max_hydro_b;
		v[fnum++] = max_pI_b;
		v[fnum++] = min_bas_b;
		v[fnum++] = min_heli_b;
		v[fnum++] = min_hydro_b;
		v[fnum++] = min_pI_b;

		v[fnum++] = max_bas_y;
		v[fnum++] = max_heli_y;
		v[fnum++] = max_hydro_y;
		v[fnum++] = max_pI_y;
		v[fnum++] = min_bas_y;
		v[fnum++] = min_heli_y;
		v[fnum++] = min_hydro_y;
		v[fnum++] = min_pI_y;

		mzb += amino_F[modpeptide[i]];
		v[fnum++] = (int) mzb;
		v[fnum++] = (int) (mz - mzb);
		v[fnum++] = (int) (mzb/(i+1));
		v[fnum++] = (int) ((mz-mzb)/(peplen-1-i));
		sum_bas += bas[peptide[i]];
		v[fnum++] = sum_bas;
		v[fnum++] = total_bas-sum_bas;
		v[fnum++] = (int) ((float)sum_bas/(i+1));
		v[fnum++] = (int) ((float)(total_bas-sum_bas)/(peplen-1-i));
		sum_heli += heli[peptide[i]];
		v[fnum++] = sum_heli;
		v[fnum++] = total_heli-sum_heli;
		v[fnum++] = (int) ((float)sum_heli/(i+1));
		v[fnum++] = (int) ((float)(total_heli-sum_heli)/(peplen-1-i));
		sum_hydro += hydro[peptide[i]];
		v[fnum++] = sum_hydro;
		v[fnum++] = total_hydro-sum_hydro;
		v[fnum++] = (int) ((float)sum_hydro/(i+1));
		v[fnum++] = (int) ((float)(total_hydro-sum_hydro)/(peplen-1-i));
		sum_pI += pI[peptide[i]];
		v[fnum++] = sum_pI;
		v[fnum++] = total_pI-sum_pI;
		v[fnum++] = (int) ((float)sum_pI/(i+1));
		v[fnum++] = (int) ((float)(total_pI-sum_pI)/(peplen-1-i));

		v[fnum++] = bas[peptide[i]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[i+1]];
		v[fnum++] = bas[peptide[i]]*bas[peptide[i+1]];
		v[fnum++] = heli[peptide[i]]*heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[i]]*hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[i]]*pI[peptide[i+1]];

		v[fnum++] = bas[peptide[i]]-bas[peptide[i+1]]+1000;
		v[fnum++] = heli[peptide[i]]-heli[peptide[i+1]]+1000;
		v[fnum++] = hydro[peptide[i]]-hydro[peptide[i+1]]+1000;
		v[fnum++] = pI[peptide[i]]-pI[peptide[i+1]]+1000;
		v[fnum++] = bas[peptide[i+1]]-bas[peptide[i]]+1000;
		v[fnum++] = heli[peptide[i+1]]-heli[peptide[i]]+1000;
		v[fnum++] = hydro[peptide[i+1]]-hydro[peptide[i]]+1000;
		v[fnum++] = pI[peptide[i+1]]-pI[peptide[i]]+1000;

		v[fnum++] = bas[peptide[i]]+bas[peptide[0]];
		v[fnum++] = heli[peptide[i]]+heli[peptide[0]];
		v[fnum++] = hydro[peptide[i]]+hydro[peptide[0]];
		v[fnum++] = pI[peptide[i]]+pI[peptide[0]];
		v[fnum++] = bas[peptide[peplen-1]]+bas[peptide[i+1]];
		v[fnum++] = heli[peptide[peplen-1]]+heli[peptide[i+1]];
		v[fnum++] = hydro[peptide[peplen-1]]+hydro[peptide[i+1]];
		v[fnum++] = pI[peptide[peplen-1]]+pI[peptide[i+1]];

		int pos = 0;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = 1;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = peplen-2;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		pos = peplen-1;
		v[fnum++] = amino_F[modpeptide[pos]];
		v[fnum++] = bas[peptide[pos]];
		v[fnum++] = heli[peptide[pos]];
		v[fnum++] = hydro[peptide[pos]];
		v[fnum++] = pI[peptide[pos]];
		v[fnum] = 0;
		if (peptide[pos] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[pos] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		v[fnum] = 0;
		if (peptide[i] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i] == 13) {
			v[fnum] = 1;
		}
		fnum++;

		v[fnum] = 0;
		if (peptide[i+1] == 11) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 2) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 3) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 8) {
			v[fnum] = 1;
		}
		fnum++;
		v[fnum] = 0;
		if (peptide[i+1] == 13) {
			v[fnum] = 1;
		}
		fnum++;


		v[fnum++] = bas[peptide[i]];
		if (i==0) {
			v[fnum++] = bas[peptide[i]];
		}
		else {
			v[fnum++] = bas[peptide[i-1]];
		}
		v[fnum++] = bas[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = bas[peptide[i+1]];
		}
		else {
			v[fnum++] = bas[peptide[i+2]];
		}

		v[fnum++] = heli[peptide[i]];
		if (i==0) {
			v[fnum++] = heli[peptide[i]];
		}
		else {
			v[fnum++] = heli[peptide[i-1]];
		}
		v[fnum++] = heli[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = heli[peptide[i+1]];
		}
		else {
			v[fnum++] = heli[peptide[i+2]];
		}

		v[fnum++] = hydro[peptide[i]];
		if (i==0) {
			v[fnum++] = hydro[peptide[i]];
		}
		else {
			v[fnum++] = hydro[peptide[i-1]];
		}
		v[fnum++] = hydro[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = hydro[peptide[i+1]];
		}
		else {
			v[fnum++] = hydro[peptide[i+2]];
		}

		v[fnum++] = pI[peptide[i]];
		if (i==0) {
			v[fnum++] = pI[peptide[i]];
		}
		else {
			v[fnum++] = pI[peptide[i-1]];
		}
		v[fnum++] = pI[peptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = pI[peptide[i+1]];
		}
		else {
			v[fnum++] = pI[peptide[i+2]];
		}

		v[fnum++] = amino_F[modpeptide[i]];
		if (i==0) {
			v[fnum++] = amino_F[modpeptide[i]];
		}
		else {
			v[fnum++] = amino_F[modpeptide[i-1]];
		}
		v[fnum++] = amino_F[modpeptide[i+1]];
		if (i==(peplen-2)) {
			v[fnum++] = amino_F[modpeptide[i+1]];
		}
		else {
			v[fnum++] = amino_F[modpeptide[i+2]];
		}

		v[fnum++] = charge;

		predictions[i] = score_B(v);
		predictions[2*(peplen-1)-i-1] = score_Y(v);
	}
	return predictions;
}
