#include "algo_top_parameters.h"
#include "algo_top.h"
#include <algorithm>
#include <utility>
#include "../../../../include/objects.h"
#include "JetObjects.h"
#include "BitonicSort-HLS/bitonic32hls/bitonicSort32.h"
#include "BitonicSort-HLS/bitonic32hls/bitonicSort32.cpp"

using namespace std;
using namespace algo;


void unpackInputLink(hls::stream<algo::axiword576> &ilink, Tower towers[TOWERS_IN_ETA/2]) {
#pragma HLS PIPELINE II=N_WORDS_PER_FRAME
#pragma HLS ARRAY_PARTITION variable=towers complete dim=0
#pragma HLS INTERFACE axis port=ilink
#pragma HLS INLINE

  ap_uint<576> word_576b_;

#ifndef __SYNTHESIS__
  // Avoid simulation warnings
  if (ilink.empty()) return;
#endif

  word_576b_ = ilink.read().data;

  towers[0]  = Tower(word_576b_( 31,   0));
  towers[1]  = Tower(word_576b_( 63,  32));
  towers[2]  = Tower(word_576b_( 95,  64));
  towers[3]  = Tower(word_576b_(127,  96));
  towers[4]  = Tower(word_576b_(159, 128));
  towers[5]  = Tower(word_576b_(191, 160));
  towers[6]  = Tower(word_576b_(223, 192));
  towers[7]  = Tower(word_576b_(255, 224));
  towers[8]  = Tower(word_576b_(287, 256));
  towers[9]  = Tower(word_576b_(319, 288));
  towers[10] = Tower(word_576b_(351, 320));
  towers[11] = Tower(word_576b_(383, 352));
  towers[12] = Tower(word_576b_(415, 384));
  towers[13] = Tower(word_576b_(447, 416));
  towers[14] = Tower(word_576b_(479, 448));

  return;
}


void packOutput(Jet jet[16], hls::stream<algo::axiword576> &olink){
#pragma HLS PIPELINE II=N_OUTPUT_WORDS_PER_FRAME
#pragma HLS ARRAY_PARTITION variable=jet complete dim=0
#pragma HLS INTERFACE axis port=olink
#pragma HLS INLINE

  ap_uint<576> word_576b_;

  word_576b_( 29,   0) = (ap_uint<30>) jet[0].data;
  word_576b_( 59,  30) = (ap_uint<30>) jet[1].data;
  word_576b_( 63,  60) = 0;
  word_576b_( 93,  64) = (ap_uint<30>) jet[2].data;
  word_576b_(123,  94) = (ap_uint<30>) jet[3].data;
  word_576b_(127, 124) = 0;
  word_576b_(157, 128) = (ap_uint<30>) jet[4].data;
  word_576b_(187, 158) = (ap_uint<30>) jet[5].data;
  word_576b_(191, 188) = 0;
  word_576b_(221, 192) = (ap_uint<30>) jet[6].data;
  word_576b_(251, 222) = (ap_uint<30>) jet[7].data;
  word_576b_(255, 252) = 0;
  word_576b_(285, 256) = (ap_uint<30>) jet[8].data;
  word_576b_(315, 286) = (ap_uint<30>) jet[9].data;
  word_576b_(319, 316) = 0;
  word_576b_(349, 320) = (ap_uint<30>) jet[10].data;
  word_576b_(379, 350) = (ap_uint<30>) jet[11].data;
  word_576b_(383, 380) = 0;
  word_576b_(413, 384) = (ap_uint<30>) jet[12].data;
  word_576b_(443, 414) = (ap_uint<30>) jet[13].data;
  word_576b_(447, 444) = 0;
  word_576b_(477, 448) = (ap_uint<30>) jet[14].data;
  word_576b_(507, 478) = (ap_uint<30>) jet[15].data;
  word_576b_(511, 508) = 0;
  word_576b_(541, 512) = (ap_uint<30>) jet[14].data;
  word_576b_(571, 542) = (ap_uint<30>) jet[15].data;
  word_576b_(575, 572) = 0;

  axiword576 r; r.last = 0; r.user = 0;
  r.data = word_576b_;
  
  olink.write(r);

  return;
}


void algo_top(hls::stream<axiword576> link_in[N_INPUT_LINKS], hls::stream<axiword576> link_out[N_OUTPUT_LINKS]) {
#pragma HLS INTERFACE axis port=link_in
#pragma HLS INTERFACE axis port=link_out
#pragma HLS PIPELINE II=N_WORDS_PER_FRAME

#pragma HLS ARRAY_PARTITION variable=link_in complete dim=0
#pragma HLS ARRAY_PARTITION variable=link_out complete dim=0


  // Step 1: Unpack links
  // Input is 60 links carrying 30phix30eta towers
  Tower towers[TOWERS_IN_PHI][TOWERS_IN_ETA];
#pragma HLS ARRAY_PARTITION variable=towers complete dim=0     
  for (size_t ilink = 0; ilink < N_INPUT_LINKS/2; ilink++) {
#pragma LOOP UNROLL
#pragma HLS latency min=1
    size_t iPosEta = ilink;
    size_t iNegEta = ilink+N_INPUT_LINKS/2;
    unpackInputLink(link_in[iNegEta], &towers[ilink][0]);
    unpackInputLink(link_in[iPosEta], &towers[ilink][TOWERS_IN_ETA/2]);
  }

	
  // Step 2: Jet Algo goes here
  Region3x3 reg3x3[12][12];//one extra tower on each side for reg9x9 algo
#pragma HLS ARRAY_PARTITION variable=reg3x3 complete dim=0 
  size_t pseueta=0;
  size_t pseuphi=0;
  for(pseueta = 0; pseueta<12; pseueta+=1){
#pragma LOOP UNROLL
    for(pseuphi = 0; pseuphi<12; pseuphi+=1){
#pragma LOOP UNROLL
      if (pseueta == 0 || pseuphi == 0 || pseueta == 11 || pseuphi == 11){
         reg3x3[pseuphi][pseueta] = Region3x3(0,0,0,0,0);// create empty region around the 10 by 10 towers	    
      }   
    }
  }
	
  for(size_t ieta = 1, pseueta = 1; ieta<TOWERS_IN_ETA-1; ieta+=3, pseueta+=1){
#pragma LOOP UNROLL
    for(size_t iphi = 1, pseuphi = 1; iphi<TOWERS_IN_PHI-1; iphi+=3, pseuphi+=1){
#pragma LOOP UNROLL
      ap_uint<10> seed_et = towers[iphi][ieta].tower_et();
      ap_uint<6> tphi = iphi;
      ap_uint<5> teta = ieta;
      ap_uint<3> time = towers[iphi][ieta].peak_time();
      ap_uint<14> region_et = get3x3Sum(
	  towers[iphi+1][ieta-1], towers[iphi+1][ieta], towers[iphi+1][ieta+1],
	  towers[iphi][ieta-1]  , towers[iphi][ieta]  , towers[iphi][ieta+1]  ,
	  towers[iphi-1][ieta-1], towers[iphi-1][ieta], towers[iphi-1][ieta+1]);
      reg3x3[pseuphi][pseueta]= Region3x3(seed_et, region_et, tphi, teta, time);//create the 10x10 reg3x3 supertowers	
    }
  }
	
  Region9x9 reg9x9[10][10];
#pragma HLS ARRAY_PARTITION variable=reg9x9 complete dim=0
  for(pseueta = 1; pseueta<11; pseueta+=1){
#pragma LOOP UNROLL
    for(pseuphi = 1; pseuphi<11; pseuphi+=1){
#pragma LOOP UNROLL
      ap_uint<10> seed_et = reg3x3[pseuphi][pseueta].seed_et();
      ap_uint<6>  tphi = reg3x3[pseuphi][pseueta].phi();
      ap_uint<5>  teta = reg3x3[pseuphi][pseueta].eta();
      ap_uint<3>  time = reg3x3[pseuphi][pseueta].time();
      ap_uint<14> region_et = get9x9Sum(
          reg3x3[pseuphi+1][pseueta-1], reg3x3[pseuphi+1][pseueta], reg3x3[pseuphi+1][pseueta+1],
          reg3x3[pseuphi][pseueta-1]  , reg3x3[pseuphi][pseueta]  , reg3x3[pseuphi][pseueta+1]  ,
	  reg3x3[pseuphi-1][pseueta-1], reg3x3[pseuphi-1][pseueta], reg3x3[pseuphi-1][pseueta+1]);
      reg9x9[pseuphi-1][pseueta-1] = Region9x9(seed_et, region_et, tphi, teta, time);//create the 10x10 reg9x9 supertowers	      
    }
  }

  Jet jet[10][10];
#pragma HLS ARRAY_PARTITION variable=jet complete dim=0
  for(pseueta = 1; pseueta<9; pseueta+=1){
#pragma LOOP UNROLL
    for(pseuphi = 1; pseuphi<9; pseuphi+=1){
#pragma LOOP UNROLL
      ap_uint<6> tphi = reg9x9[pseuphi][pseueta].phi();
      ap_uint<6> teta = reg9x9[pseuphi][pseueta].eta();
      ap_uint<3> time = reg9x9[pseuphi][pseueta].time();
      ap_uint<14> et = isJet(
          reg9x9[pseuphi+1][pseueta-1], reg9x9[pseuphi+1][pseueta], reg9x9[pseuphi+1][pseueta+1],
          reg9x9[pseuphi][pseueta-1]  , reg9x9[pseuphi][pseueta]  , reg9x9[pseuphi][pseueta+1]  ,
	  reg9x9[pseuphi-1][pseueta-1], reg9x9[pseuphi-1][pseueta], reg9x9[pseuphi-1][pseueta+1]);
      jet[pseuphi][pseueta] = Jet(et, tphi, teta, time); 
    }
  } 
  //clean the falsed jet
  for(pseuphi = 8; pseuphi>0; pseuphi-=1){
#pragma LOOP UNROLL
    for(pseueta = 1; pseueta<9; pseueta+=1){
#pragma LOOP UNROLL
       /*	    
       if (checkJetrow(reg9x9[pseuphi][pseueta-1], reg9x9[pseuphi][pseueta], reg9x9[pseuphi][pseueta+1]) == 1){
	   jet[pseuphi][pseueta-1] = Jet(0, jet[pseuphi][pseueta-1].phi(), jet[pseuphi][pseueta-1].eta(), jet[pseuphi][pseueta-1].time());
	   jet[pseuphi][pseueta+1] = Jet(0, jet[pseuphi][pseueta+1].phi(), jet[pseuphi][pseueta+1].eta(), jet[pseuphi][pseueta+1].time());
	   //cout << "jet[" << jet[pseuphi][pseueta].phi() << "," << jet[pseuphi][pseueta].eta() << "row clear" << jet[pseuphi][pseueta].et() << endl;
       }
       if (checkJetcol(reg9x9[pseuphi-1][pseueta], reg9x9[pseuphi][pseueta], reg9x9[pseuphi+1][pseueta]) == 1){
	   jet[pseuphi+1][pseueta] = Jet(0, jet[pseuphi+1][pseueta].phi(), jet[pseuphi+1][pseueta].eta(), jet[pseuphi+1][pseueta].time());
	   jet[pseuphi-1][pseueta] = Jet(0, jet[pseuphi-1][pseueta].phi(), jet[pseuphi-1][pseueta].eta(), jet[pseuphi-1][pseueta].time());
	   //cout << "jet[" << jet[pseuphi][pseueta].phi() << "," << jet[pseuphi][pseueta].eta() << "col clear" << jet[pseuphi][pseueta].et() << endl;
       }
       */
       jetrowcheck(pseuphi, pseueta, reg9x9, jet);
       jetcolcheck(pseuphi, pseueta, reg9x9, jet);
    }
  }
 //find the real jet array
  Jet reljet[32];
#pragma HLS ARRAY_PARTITION variable=reljet complete dim=0
  size_t jetnum = 0;
  for(size_t pseuphi = 8; pseuphi>0; pseuphi-=1){
#pragma HLS UNROLL 
     for(size_t pseueta = 1; pseueta<9; pseueta+=1){
#pragma HLS UNROLL 
	jetnum = jetindcvt(pseuphi, pseueta, jet, reljet, jetnum);
     }
   }
  Jet sortjet[N]; 
#pragma HLS ARRAY_PARTITION variable=sortjet complete dim=0
  bitonicSort32(reljet, sortjet);//sort the mix of jets and empty jets
  Jet partjet[N/2];// divide into two part for output
#pragma HLS ARRAY_PARTITION variable=partjet complete dim=0 
  // Step 3: Pack the outputs
  for(size_t olink=0; olink<2; olink++){
#pragma LOOP UNROLL
    for(size_t olink2=0; olink2<N/2; olink2++){
#pragma LOOP UNROLL
      partjet[olink2]=(N-jetnum+olink*16+olink2<N)? sortjet[N-jetnum+olink*16+olink2] : Jet();
    }
    packOutput(partjet, link_out[olink]); 
  }
//-  for (size_t olink = 0; olink < N_OUTPUT_LINKS/2; olink++) {
//-#pragma LOOP UNROLL
//-#pragma HLS latency min=1
//-    size_t iPosEta = olink;              
//-    size_t iNegEta = olink + (N_OUTPUT_LINKS/2);
//-    packOutput(&towers[olink][0], link_out[iNegEta]);
//-    packOutput(&towers[olink][TOWERS_IN_ETA/2], link_out[iPosEta]);
//-  }
  return;
}
