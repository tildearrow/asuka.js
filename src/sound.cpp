#include "asuka.h"

void nOpenSound(NetFile*, void* u) {
  ((Sound*)u)->go();
}

void nFailed(NetFile*, void* u) {
  ((Sound*)u)->fail();
}

void Sound::go() {
  f=nf->openSF(&si);

  if (f==NULL) {
    printf("could not decode! %s\n",sf_strerror(NULL));
    killme=true;
    return;
  }

  rate=si.samplerate;
  ready=true;
}

void Sound::fail() {
  killme=true;
}

void Sound::fill() {
  unsigned int howMuch=(dcPosR-dcPosW-1)&DECODE_CACHE_MASK;
  // check if we even have the path opened
  if (f==NULL || eof) return;
  // try to predict whether we are going to hit a missing block
  int futureChunk=(nf->tell()+32768)/FETCH_SIZE;
  if (futureChunk<nf->numChunks) {
    if (!nf->haveChunk[futureChunk]) {
      printf("waiting 'cuz we don't have the next chunk...\n");
      return;
    }
  }
  // decode
  if (howMuch!=0) {
    int howReallyMuch=sf_readf_float(f,tDecodeCache,howMuch);
    if (howReallyMuch>howMuch) {
      printf("ERROR: we read more than usual!\n");
    }
    if (howReallyMuch==0) {
      eof=true;
      return;
    }
    if (si.channels==1) {
      for (int i=0; i<howReallyMuch; i++) {
        decodeCache[dcPosW<<1]=tDecodeCache[i];
        decodeCache[1+(dcPosW<<1)]=tDecodeCache[i];
        dcPosW=(dcPosW+1)&DECODE_CACHE_MASK;
      }
    } else {
      for (int i=0; i<howReallyMuch; i++) {
        decodeCache[dcPosW<<1]=tDecodeCache[i<<1];
        decodeCache[1+(dcPosW<<1)]=tDecodeCache[(i<<1)+1];
        dcPosW=(dcPosW+1)&DECODE_CACHE_MASK;
      }
    }
  }
}

void Sound::load(string p) {
  if (nf->isOpen) {
    printf("but we already opened a file!\n");
    return;
  }

  path=p;

  nf->uponCompletion=nOpenSound;
  nf->uponCompletionUserData=this;
  nf->uponFailure=nFailed;
  nf->uponFailureUserData=this;
  nf->open(path,nfNoChunk);
}
