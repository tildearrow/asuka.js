#include "asuka.h"

void nOpenIntro(NetFile*, void* u) {
  ((Music*)u)->openIntro();
}

void nOpenLoopPart(NetFile*, void* u) {
  ((Music*)u)->openLoopPart();
}

void nOpenLoop(NetFile*, void* u) {
  ((Music*)u)->openLoop();
}

void Music::openIntro() {
  fIntro=nfIntro->openSF(&siIntro);

  if (fIntro==NULL) {
    printf("could not decode! %s\n",sf_strerror(NULL));
    return;
  }

  rate=siIntro.samplerate;
}

void Music::openLoopPart() {
  fLoop=nfLoop->openSF(&siLoop);

  if (fLoop==NULL) {
    printf("could not decode! %s\n",sf_strerror(NULL));
    return;
  }
}

void Music::openLoop() {
  nfLoop->uponFirstChunks=nOpenLoopPart;
  nfLoop->uponFirstChunksUserData=this;
  nfLoop->open(path+S("_loop." FETCH_FORMAT));
}

void Music::fillBuf() {
  redo:
  NetFile* nf=(inLoop)?(nfLoop):(nfIntro);
  SNDFILE* f=(inLoop)?(fLoop):(fIntro);
  SF_INFO& si=(inLoop)?(siLoop):(siIntro);
  unsigned int howMuch=(dcPosR-dcPosW-1)&DECODE_CACHE_MASK;
  // check if we even have the path opened
  if (f==NULL) return;
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
      printf("end of file!\n");
      if (inLoop) {
        sf_seek(f,0,SEEK_SET);
      } else {
        inLoop=true;
      }
      goto redo;
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

void Music::load(string p) {
  if (nfIntro->isOpen) {
    printf("closing song\n");
    sf_close(fIntro);
    fIntro=NULL;
    sf_close(fLoop);
    fLoop=NULL;
    nfIntro->close();
    nfLoop->close();
    stop();
  }

  path=p;

  goBegin();
  nfIntro->uponFirstChunks=nOpenIntro;
  nfIntro->uponFirstChunksUserData=this;
  nfIntro->uponCompletion=nOpenLoop;
  nfIntro->uponCompletionUserData=this;
  nfIntro->open(path+S("_intro." FETCH_FORMAT));
}

void Music::play() {
  playing=true;
}

void Music::stop() {
  playing=false;
}

void Music::goBegin() {
  if (fIntro!=NULL) {
    sf_seek(fIntro,0,SEEK_SET);
  }
  if (fLoop!=NULL) {
    sf_seek(fLoop,0,SEEK_SET);
  }
  inLoop=false;
  // fast-forward the needle
  dcPosR=dcPosW;
  // force-run the decoding if needed
  // TODO
}
