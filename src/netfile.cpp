#include "asuka.h"

// FETCH

// global
void fetchGotSize(emscripten_fetch_t* fetch) {
  if (man.has(fetch->userData)) {
    ((NetFile*)(fetch->userData))->fGotSize(fetch);
  } else {
    emscripten_fetch_close(fetch);
  }
}
void fetchFinish(emscripten_fetch_t* fetch) {
  if (man.has(fetch->userData)) {
    ((NetFile*)(fetch->userData))->fFinish(fetch);
  } else {
    emscripten_fetch_close(fetch);
  }
}
void fetchFail(emscripten_fetch_t* fetch) {
  if (man.has(fetch->userData)) {
    ((NetFile*)(fetch->userData))->fFail(fetch);
  } else {
    emscripten_fetch_close(fetch);
  }
}

// local
void NetFile::fFinish(emscripten_fetch_t* fetch) {
  int ftb=fetch->totalBytes;
  if (fetch!=curFetch) {
    printf("dropping fetch. we never wanted this.\n");
    emscripten_fetch_close(fetch);
    return;
  }

  // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
  memcpy(dataPtr+fPos,fetch->data,fetch->totalBytes);
  haveChunk[fPos/FETCH_SIZE]=true;
  emscripten_fetch_close(fetch);
  curFetch=NULL;

  fNext(ftb);
}

void NetFile::fFail(emscripten_fetch_t* fetch) {
  printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
  if (uponFailure!=NULL) uponFailure(this,uponFailureUserData);
  if (fetch) {
    emscripten_fetch_close(fetch);
  }
  curFetch=NULL;
}

void NetFile::fNext(int lastSize) {
  bool knowNext=false;
  for (int i=0; i<numChunks; i++) {
    if (wantChunk[i]) {
      if (!haveChunk[i]) {
        fPos=i*FETCH_SIZE;
        knowNext=true;
        break;
      } else {
        wantChunk[i]=false;
      }
    }
  }
  if (!knowNext) {
    // now execute the decoder part
    if (!uponFirstChunksDone) {
      if (uponFirstChunks!=NULL) uponFirstChunks(this,uponFirstChunksUserData);
      uponFirstChunksDone=true;
    }
    for (int i=0; i<numChunks; i++) {
      if (!haveChunk[i]) {
        fPos=i*FETCH_SIZE;
        knowNext=true;
        break;
      }
    }
  }
  if (!knowNext) {
    printf("completed.\n");
    complete=true;

    if (uponCompletion!=NULL) uponCompletion(this,uponCompletionUserData);
    return;
  }

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  if (openFlags&nfPersist) {
    attr.attributes |= EMSCRIPTEN_FETCH_PERSIST_FILE;
  }
  attr.onsuccess = fetchFinish;
  attr.onerror = fetchFail;
  attr.userData=this;
  //attr.timeoutMSecs = 10000;

  fetchHead[0]=new char[128];
  fetchHead[1]=new char[128];
  fetchHead[2]=NULL;
  fetchHead[3]=NULL;
  strcpy(fetchHead[0],"Range");
  sprintf(fetchHead[1],"bytes=%d-%d",fPos,fPos+FETCH_SIZE-1);
  attr.requestHeaders=fetchHead;
  printf("fetching chunk %d\n",fPos/FETCH_SIZE);
  curFetch=emscripten_fetch(&attr,finalFetchPath);
  if (curFetch==NULL) {
    printf("error. curFetch is null...\n");
  }
}

void NetFile::fGotSize(emscripten_fetch_t* fetch) {
  char* rawHeaders=NULL;
  char** headers=NULL;
  printf("fGotSize\n");
  if (fetch!=curFetch) {
    printf("dropping fetch. we never wanted this.\n");
    emscripten_fetch_close(fetch);
    return;
  }
  printf("past check. our fetch is %p\n",fetch);
  int hl=emscripten_fetch_get_response_headers_length(fetch);

  printf("hl: %d\n",hl);

  rawHeaders=new char[hl+1];
  emscripten_fetch_get_response_headers(fetch,rawHeaders,hl+1);

  headers=emscripten_fetch_unpack_response_headers(rawHeaders);

  mustHave=0;
  for (int i=0; headers[i*2]!=NULL; i++) {
    if (strcmp(headers[i*2],"content-length")==0) {
      // the size
      mustHave=atoi(headers[i*2+1]);
      break;
    }
  }

  emscripten_fetch_free_unpacked_response_headers(headers);
  delete[] rawHeaders;

  // register the size
  sman.regSize(fetchPath,mustHave);

  printf("the size is %d! allocating %d and doing %d chunks\n",mustHave,FETCH_SIZE*(1+(mustHave-1)/FETCH_SIZE),(1+(mustHave-1)/FETCH_SIZE));

  if (mustHave<=0) {
    printf("what the heck?\n");
    return;
  }
  emscripten_fetch_close(fetch);
  curFetch=NULL;

  initData();
  fNext(FETCH_SIZE);
}

void NetFile::initData() {
  dataPtr=new char[FETCH_SIZE*(1+(mustHave-1)/FETCH_SIZE)];
  haveChunk=new bool[1+(mustHave-1)/FETCH_SIZE];
  wantChunk=new bool[1+(mustHave-1)/FETCH_SIZE];
  numChunks=1+(mustHave-1)/FETCH_SIZE;
  memset(haveChunk,0,(1+(mustHave-1)/FETCH_SIZE));
  memset(wantChunk,0,(1+(mustHave-1)/FETCH_SIZE));
  // mark the last 2 chunks as "want" in advance
  wantChunk[0]=true;
  if (numChunks>=2) {
    wantChunk[numChunks-1]=true;
    wantChunk[numChunks-2]=true;
  }
}

void NetFile::open(string path, int flags) {
  emscripten_fetch_attr_t attr;
  fetchPath=path;
  openFlags=flags;
  strcpy(finalFetchPath,path.c_str());

  // check if we have the size
  printf("%s has hash %.8x\n",path.c_str(),sman.getHash(path));
  if ((mustHave=sman.getSize(path))>0) {
    // yes we do
    printf("preparing fetch of %s (cached)\n",finalFetchPath);
    initData();
    fNext(FETCH_SIZE);
  } else {
    // fetch and register size
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "HEAD");
    attr.attributes = 0;
    attr.onsuccess = fetchGotSize;
    attr.onerror = fetchFail;
    attr.userData=this;
    //attr.timeoutMSecs = 10000;
    printf("preparing fetch of %s (uncached)\n",finalFetchPath);
    curFetch=emscripten_fetch(&attr,finalFetchPath);
  
    if (curFetch==NULL) {
      printf("error. curFetch is null...\n");
    }
  }

  isOpen=true;
}

void NetFile::close() {
  //if (curFetch) olderFetch=curFetch;

  fetchPath="";
  strcpy(finalFetchPath,"");

  delete[] dataPtr;
  dataPtr=NULL;
  delete[] haveChunk;
  haveChunk=NULL;
  delete[] wantChunk;
  wantChunk=NULL;
  mustHave=0;
  numChunks=0;
  fPos=0;
  complete=false;
  isOpen=false;
  curFetch=NULL;
  vioCurSeek=0;
  uponFirstChunksDone=false;
}

// FILE OPS

sf_count_t NetFile::size() {
  printf("vioSize = %d\n",mustHave);
  return mustHave;
}

sf_count_t NetFile::seek(sf_count_t offset, int whence) {
  int oldcs;
  oldcs=vioCurSeek;
  switch (whence) {
    case SEEK_SET:
      vioCurSeek=offset;
      printf("vioSeek(%lld,SET) = %d\n",offset,vioCurSeek);
      break;
    case SEEK_CUR:
      vioCurSeek+=offset;
      printf("vioSeek(%lld,CUR) = %d\n",offset,vioCurSeek);
      break;
    case SEEK_END:
      vioCurSeek=mustHave+offset;
      printf("vioSeek(%lld,END) = %d\n",offset,vioCurSeek);
      break;
    default:
      // who uses trigraphs anyway?!
      printf("vioSeek(%lld,?""?""?) = EINVAL\n",offset);
      return -EINVAL;
      break;
  }
  if (vioCurSeek>mustHave) {
    vioCurSeek=oldcs;
    return -EINVAL;
    //vioCurSeek=have;
  }
  if (vioCurSeek<0) {
    vioCurSeek=oldcs;
    return -EINVAL;
    //vioCurSeek=0;
  }
  return vioCurSeek;
}

sf_count_t NetFile::read(void* buf, sf_count_t len) {
  bool doWeHave;
  sf_count_t actual=len;
  if (vioCurSeek>=mustHave) {
    //printf("vioRead(%lld) = 0\n",len);
    return 0;
  }
  if (actual+vioCurSeek>mustHave) {
    actual=mustHave-vioCurSeek;
  }
  // check if we have the data
  if (!complete) do {
    doWeHave=true;
    for (int i=vioCurSeek/FETCH_SIZE; i<=(vioCurSeek+actual)/FETCH_SIZE; i++) {
      if (!haveChunk[i]) {
        doWeHave=false;
        printf("we need %d but not present!\n",i);
        wantChunk[i]=true;
      };
    }
    if (!doWeHave) {
      // we can't sleep. error out...
      printf("ERROR: not available!\n");
      return -EAGAIN;
      //emscripten_sleep(50);
    }
  } while (!doWeHave);
  memcpy(buf,dataPtr+vioCurSeek,actual);
  //printf("vioRead(%lld) = %lld\n",len,actual);
  //printf("%d:",vioCurSeek);
  //for (int i=0; i<actual; i++) {
  //  printf(" %.2x",dataPtr[vioCurSeek+i]);
  //}
  //printf("\n");
  vioCurSeek+=actual;
  return actual;
}

sf_count_t NetFile::tell() {
  //printf("vioTell = %d\n",vioCurSeek);
  return vioCurSeek;
}
// NETFILE END //

// VIRTUAL I/O BEGIN //

sf_count_t vioSize(void* user) {
  return ((NetFile*)user)->size();
}

sf_count_t vioSeek(sf_count_t offset, int whence, void* user) {
  return ((NetFile*)user)->seek(offset,whence);
}

sf_count_t vioRead(void* buf, sf_count_t len, void* user) {
  return ((NetFile*)user)->read(buf,len);
}

// ignored
sf_count_t vioWrite(const void* buf, sf_count_t len, void* user) {
  return -EBADF;
}

sf_count_t vioTell(void* user) {
  return ((NetFile*)user)->tell();
}

// VIRTUAL I/O END //

SNDFILE* NetFile::openSF(SF_INFO* si) {
  vio.get_filelen=vioSize;
  vio.seek=vioSeek;
  vio.read=vioRead;
  vio.write=vioWrite;
  vio.tell=vioTell;
  return sf_open_virtual(&vio,SFM_READ,si,this);
}
