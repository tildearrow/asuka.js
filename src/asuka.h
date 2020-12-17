#ifndef _ASUKA_H
#define _ASUKA_H
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/fetch.h>
#include <emscripten/html5.h>
#include <sndfile.h>
#include <SDL.h>

#define FETCH_FORMAT "ogg"

#define FETCH_SIZE 65536
#define DECODE_CACHE_SIZE 16384
#define DECODE_CACHE_MASK (DECODE_CACHE_SIZE-1)

typedef std::string string;
#define S(x) std::string(x)

enum {
  fxConf,
  fxHail,
  fxSand,
  fxSleep,
  fxRain,
  fxRainHeavy,
  fxMax
};

struct ObjectInfo {
  int id;
  void* ptr;
  ObjectInfo(int a, void* b): id(a), ptr(b) {}
  ObjectInfo(): id(-1), ptr(NULL) {}
};

class ObjectManager {
  public:
  std::vector<ObjectInfo> items;
  int c;
  int acquire(void* me);
  bool remove(int id);
  bool has(void* p);
  ObjectManager(): c(-1) {}
};

struct SizeInfo {
  unsigned int hash;
  int size;
  SizeInfo(unsigned int a, int b): hash(a), size(b) {}
  SizeInfo(): hash(0), size(0) {}
};

class SizeManager {
  public:
  std::vector<SizeInfo> items;
  unsigned int getHash(string s);
  int regSize(string path, int size);
  int getSize(string path);
};

extern ObjectManager man;
extern SizeManager sman;

enum NetFileFlags {
  nfPersist=1,
  nfNoChunk=2
};

class NetFile {
  public:
  int id;
  emscripten_fetch_attr_t attr;
  char* fetchHead[4];
  char* dataPtr;
  bool* haveChunk;
  bool* wantChunk;

  int mustHave;
  int numChunks;
  int fPos;
  bool complete;
  bool isOpen;
  string fetchPath;
  char finalFetchPath[4096];

  emscripten_fetch_t* curFetch;

  bool uponFirstChunksDone;
  void (*uponFirstChunks)(NetFile*,void*);
  void* uponFirstChunksUserData;
  void (*uponCompletion)(NetFile*,void*);
  void* uponCompletionUserData;
  void (*uponFailure)(NetFile*,void*);
  void* uponFailureUserData;

  SF_VIRTUAL_IO vio;
  int vioCurSeek;

  int openFlags;
  
  void open(string path, int flags=0);
  void initData();
  void close();

  void fFinish(emscripten_fetch_t* fetch);
  void fFail(emscripten_fetch_t* fetch);
  void fNext(int lastSize);
  void fGotSize(emscripten_fetch_t* fetch);

  sf_count_t size();
  sf_count_t seek(sf_count_t offset, int whence);
  sf_count_t read(void* buf, sf_count_t len);
  sf_count_t tell();
  SNDFILE* openSF(SF_INFO* si);

  NetFile():
    dataPtr(NULL),
    haveChunk(NULL),
    wantChunk(NULL),
    mustHave(0),
    numChunks(0),
    fPos(0),
    complete(false),
    isOpen(false),
    curFetch(NULL),
    uponFirstChunksDone(false),
    uponFirstChunks(NULL),
    uponFirstChunksUserData(NULL),
    uponCompletion(NULL),
    uponCompletionUserData(NULL),
    uponFailure(NULL),
    uponFailureUserData(NULL),
    vioCurSeek(0) {
    fetchHead[0]=new char[128];
    fetchHead[1]=new char[128];
    fetchHead[2]=0;
    fetchHead[3]=0;

    finalFetchPath[0]=0;
    
    id=man.acquire(this);
  }

  ~NetFile() {
    man.remove(id);
    delete[] fetchHead[0];
    delete[] fetchHead[1];

    if (dataPtr) delete[] dataPtr;
    if (haveChunk) delete[] haveChunk;
    if (wantChunk) delete[] wantChunk;
  }
};

class Music {
  public:
  int id;
  int mid;
  NetFile* nfIntro;
  NetFile* nfLoop;
  SNDFILE* fIntro;
  SNDFILE* fLoop;
  SF_INFO siIntro, siLoop;

  string path;

  float* decodeCache;
  float* tDecodeCache;
  int dcPosRLow;
  int dcPosR;
  int dcPosW;

  float speed;
  float volume;
  int rate;

  float cubint[2][4];

  bool inLoop;
  bool playing;
  bool ready;

  void fillBuf();
  void openIntro();
  void openLoopPart();
  void openLoop();

  void goBegin();
  void play();
  void stop();
  void load(string p);
  Music(int m):
    mid(m),
    nfIntro(NULL), nfLoop(NULL),
    fIntro(NULL), fLoop(NULL),
    path(""),
    decodeCache(NULL), tDecodeCache(NULL),
    dcPosRLow(0), dcPosR(0), dcPosW(0),
    speed(1), volume(1), rate(0),
    inLoop(false), playing(false), ready(false) {
    nfIntro=new NetFile;
    nfLoop=new NetFile;
    decodeCache=new float[DECODE_CACHE_SIZE*2];
    tDecodeCache=new float[DECODE_CACHE_SIZE*2];

    for (int i=0; i<2; i++) {
      cubint[i][0]=0;
      cubint[i][1]=0;
      cubint[i][2]=0;
      cubint[i][3]=0;
    }
  
    id=man.acquire(this);
  }
  ~Music() {
    man.remove(id);

    if (fIntro) sf_close(fIntro);
    if (fLoop) sf_close(fLoop);

    delete nfIntro;
    delete nfLoop;
    delete[] decodeCache;
    delete[] tDecodeCache;
  }
};

class Sound {
  public:
  int id;
  int sid;
  NetFile* nf;
  SNDFILE* f;
  SF_INFO si;

  string path;

  float* decodeCache;
  float* tDecodeCache;
  int dcPosRLow;
  int dcPosR;
  int dcPosW;

  float speed;
  float volume;
  int rate;

  float cubint[2][4];

  bool ready, eof, killme;

  void go();
  void fill();
  void fail();

  void load(string p);
  Sound(int s):
    sid(s),
    nf(NULL), f(NULL),
    decodeCache(NULL), tDecodeCache(NULL),
    dcPosRLow(0), dcPosR(0), dcPosW(0),
    speed(1), volume(1), rate(0),
    ready(false), eof(false), killme(false) {
    nf=new NetFile;
    for (int i=0; i<2; i++) {
      cubint[i][0]=0;
      cubint[i][1]=0;
      cubint[i][2]=0;
      cubint[i][3]=0;
    }
    decodeCache=new float[DECODE_CACHE_SIZE*2];
    tDecodeCache=new float[DECODE_CACHE_SIZE*2];
    id=man.acquire(this);
  }
  ~Sound() {
    man.remove(id);
    delete[] decodeCache;
    delete[] tDecodeCache;
    if (f) sf_close(f);
    delete nf;
  }
};

#endif
