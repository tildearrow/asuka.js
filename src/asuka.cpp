#include "asuka.h"
#include "ct.h"

SDL_AudioDeviceID ai;
SDL_AudioSpec ac;
SDL_AudioSpec ar;

ObjectManager man;
SizeManager sman;

bool audioUp=false;

bool effects[fxMax];

#define CHECK_SUSPEND \
  if (audioUp) { \
    suspendTime=ar.freq*2; \
    SDL_PauseAudioDevice(ai,0); \
  }

// EFFECT VARIABLE BEGIN //

struct fxConfStateS {
  float amp, val;
  unsigned short pos;
  fxConfStateS(): amp(0), val(0), pos(0) {}
} fxConfState;

struct fxHailStateS {
  float amp;
  float tap[2][2048];
  unsigned short pos;
  fxHailStateS(): amp(0), pos(0) {
    memset(tap[0],0,2048*sizeof(float));
    memset(tap[1],0,2048*sizeof(float));
  }
} fxHailState;

struct fxSandStateS {
  float amp;
  float cut, cutp;
  float p0[2];
  float p1[2];
  float nextCut;
  int nextCutIn;
  fxSandStateS(): amp(0), cut(0.3), nextCut(0.3), nextCutIn(10000) {
    p0[0]=0;
    p0[1]=0;
    p1[0]=0;
    p1[1]=0;
  }
} fxSandState;

struct fxSleepStateS {
  float val;
  fxSleepStateS(): val(1) {}
} fxSleepState;

struct Droplet {
  float vol;
  unsigned short pos, freq;
  signed char life;
  Droplet(bool val): freq(521+(rand()%20871)), vol((float(rand()%64)/128.0f)), pos(0), life(0) {}
  Droplet(): freq(521+(rand()%20871)), vol((float(rand()%64)/512.0f)), pos(0), life(0) {}
};

struct fxRainStateS {
  Droplet drops[256];
  unsigned char dropsBegin=0, dropsEnd=0;
  float* prebuf;
  float* postbuf;

  float vol;
} fxRainState;

float dropletVol[128];

// EFFECT VARIABLE END //

std::vector<Music*> mus;
std::vector<Sound*> snd;
int sndc=-1;

int bufSize=1024;

int suspendTime=100000;

char dstr[4096];

Music* findMusic(int id) {
  Music* n;
  for (Music* i: mus) {
    if (i->mid==id) return i;
  }
  n=new Music(id);
  mus.push_back(n);
  return n;
}

Sound* findSound(int id) {
  for (Sound* i: snd) {
    if (i->sid==id) return i;
  }
  return NULL;
}

static void process(void* d, Uint8* stream, int len) {
  int nframes=len/(ar.channels*sizeof(float));
  float* buf[2];
  int coff;
  bool warned=false;
  bool audible=false;
  buf[0]=(float*)stream;
  buf[1]=(float*)(&buf[0][1]);
  memset(stream,0,len);
  // MUSIC
  for (Music* h: mus) {
    h->fillBuf();
    for (int i=0; i<nframes; i++) {
      // do effect (pre)
      if (effects[fxConf]) { // confusion
        fxConfState.amp+=0.00001;
        if (fxConfState.amp>0.06) fxConfState.amp=0.06;
      } else {
        fxConfState.amp-=0.00001;
        if (fxConfState.amp<0) fxConfState.amp=0;
      }
      fxConfState.val=1+sinT[(fxConfState.pos>>3)&0x3ff]*fxConfState.amp;
      fxConfState.pos++;
      
      if (effects[fxSleep]) {
        fxSleepState.val-=0.00001;
        if (fxSleepState.val<0) fxSleepState.val=0;
      } else {
        fxSleepState.val+=0.00001;
        if (fxSleepState.val>1) fxSleepState.val=1;
      }
  
      // put sample
      if (!h->playing || h->dcPosR==h->dcPosW) { // we have no data
        if (h->playing && !warned) {
          warned=true;
        }
      } else {
        audible=true;
        h->dcPosRLow+=h->rate*h->speed*fxConfState.val*fxSleepState.val;
        while (h->dcPosRLow>=ar.freq && h->dcPosR!=h->dcPosW) {
          h->dcPosRLow-=ar.freq;
          h->cubint[0][0]=h->cubint[0][1];
          h->cubint[0][1]=h->cubint[0][2];
          h->cubint[0][2]=h->cubint[0][3];
          h->cubint[0][3]=h->decodeCache[(h->dcPosR<<1)];
          h->cubint[1][0]=h->cubint[1][1];
          h->cubint[1][1]=h->cubint[1][2];
          h->cubint[1][2]=h->cubint[1][3];
          h->cubint[1][3]=h->decodeCache[(h->dcPosR<<1)+1];
          h->dcPosR=(h->dcPosR+1)&DECODE_CACHE_MASK;
        }
        coff=((512*h->dcPosRLow)/ar.freq)<<2;
        buf[0][i<<1]+=(cT[coff]*h->cubint[0][0]+
                       cT[coff+1]*h->cubint[0][1]+
                       cT[coff+2]*h->cubint[0][2]+
                       cT[coff+3]*h->cubint[0][3])*h->volume;
        buf[1][i<<1]+=(cT[coff]*h->cubint[1][0]+
                       cT[coff+1]*h->cubint[1][1]+
                       cT[coff+2]*h->cubint[1][2]+
                       cT[coff+3]*h->cubint[1][3])*h->volume;
      }
    }
  }
  // SOUNDS
  for (Sound* h: snd) {
    if (!h->ready) continue;
    h->fill();
    for (int i=0; i<nframes; i++) {
      // put sample
      if (!h->ready || h->dcPosR==h->dcPosW) { // we have no data
        if (h->eof) {
          h->killme=true;
          break;
        }
      } else {
        audible=true;
        h->dcPosRLow+=h->rate*h->speed;
        while (h->dcPosRLow>=ar.freq && h->dcPosR!=h->dcPosW) {
          h->dcPosRLow-=ar.freq;
          h->cubint[0][0]=h->cubint[0][1];
          h->cubint[0][1]=h->cubint[0][2];
          h->cubint[0][2]=h->cubint[0][3];
          h->cubint[0][3]=h->decodeCache[(h->dcPosR<<1)];
          h->cubint[1][0]=h->cubint[1][1];
          h->cubint[1][1]=h->cubint[1][2];
          h->cubint[1][2]=h->cubint[1][3];
          h->cubint[1][3]=h->decodeCache[(h->dcPosR<<1)+1];
          h->dcPosR=(h->dcPosR+1)&DECODE_CACHE_MASK;
        }
        coff=((512*h->dcPosRLow)/ar.freq)<<2;
        buf[0][i<<1]+=(cT[coff]*h->cubint[0][0]+
                       cT[coff+1]*h->cubint[0][1]+
                       cT[coff+2]*h->cubint[0][2]+
                       cT[coff+3]*h->cubint[0][3])*h->volume;
        buf[1][i<<1]+=(cT[coff]*h->cubint[1][0]+
                       cT[coff+1]*h->cubint[1][1]+
                       cT[coff+2]*h->cubint[1][2]+
                       cT[coff+3]*h->cubint[1][3])*h->volume;
      }
    }
  }

  for (int i=0; i<snd.size(); i++) {
    if (snd[i]->killme) {
      printf("killing sound %d\n",snd[i]->sid);
      delete snd[i];
      snd.erase(snd.begin()+i);
      i--;
    }
  }

  // EFFECTS
  for (int i=0; i<nframes; i++) {
    if (effects[fxHail]) {
      buf[0][i<<1]+=fxHailState.tap[0][(fxHailState.pos+200)&0x7ff]*0.6;
      buf[1][i<<1]+=fxHailState.tap[1][(fxHailState.pos+90)&0x7ff]*0.6;
      buf[0][i<<1]+=fxHailState.tap[1][(fxHailState.pos+475)&0x7ff]*0.4;
      buf[1][i<<1]+=fxHailState.tap[0][(fxHailState.pos+775)&0x7ff]*0.3;
      buf[0][i<<1]+=fxHailState.tap[1][(fxHailState.pos+1467)&0x7ff]*0.3;
      buf[1][i<<1]+=fxHailState.tap[0][(fxHailState.pos+1167)&0x7ff]*0.4;
      fxHailState.tap[0][fxHailState.pos]=buf[0][i<<1]*0.7;
      fxHailState.tap[1][fxHailState.pos]=buf[1][i<<1]*0.7;
      fxHailState.pos=(fxHailState.pos+1)&0x7ff;
    }
    if (effects[fxSand]) {
      fxSandState.amp+=0.00001;
      if (fxSandState.amp>0.4) fxSandState.amp=0.4;
    } else {
      fxSandState.amp-=0.00001;
      if (fxSandState.amp<0) fxSandState.amp=0;
    }
    if (fxSandState.amp>0) {
      audible=true;
      if (--fxSandState.nextCutIn<0) {
        fxSandState.nextCut=0.05+(float(rand()%256)/256)*0.4;
        fxSandState.nextCutIn=12000;
      }
      fxSandState.cutp+=(fxSandState.nextCut-fxSandState.cutp)*0.00005;
      fxSandState.cut+=(fxSandState.cutp-fxSandState.cut)*0.00005;
      fxSandState.p0[0]+=fxSandState.cut*((-0.5+float(rand()%256)/256)-fxSandState.p0[0]+3*(fxSandState.p0[0]-fxSandState.p1[0]));
      fxSandState.p1[0]+=fxSandState.cut*(fxSandState.p0[0]-fxSandState.p1[0]);
      fxSandState.p1[0]+=fxSandState.cut*(fxSandState.p0[0]-fxSandState.p1[0]);
      buf[0][i<<1]+=fxSandState.p1[0]*fxSandState.amp;
  
      fxSandState.p0[1]+=fxSandState.cut*((-0.5+float(rand()%256)/256)-fxSandState.p0[1]+3*(fxSandState.p0[1]-fxSandState.p1[1]));
      fxSandState.p1[1]+=fxSandState.cut*(fxSandState.p0[1]-fxSandState.p1[1]);
      fxSandState.p1[1]+=fxSandState.cut*(fxSandState.p0[1]-fxSandState.p1[1]);
      buf[1][i<<1]+=fxSandState.p1[1]*fxSandState.amp;
    }

    if (effects[fxRain]) {
      fxRainState.vol+=0.00005;
      if (fxRainState.vol>1.5) fxRainState.vol=1.5;
    } else {
      fxRainState.vol-=0.00005;
      if (fxRainState.vol<0) fxRainState.vol=0;
    }

    if (fxRainState.vol>0) {
      audible=true;
      if (effects[fxRainHeavy]) {
        // background
        if ((rand()&3)==0) {
          fxRainState.drops[++fxRainState.dropsEnd]=Droplet();
        }
  
        // foreground
        if ((rand()&63)==0) {
          fxRainState.drops[++fxRainState.dropsEnd]=Droplet(true);
        }
      } else {
        // background
        if ((rand()&15)==0) {
          fxRainState.drops[++fxRainState.dropsEnd]=Droplet();
        }
  
        // foreground
        if ((rand()&255)==0) {
          fxRainState.drops[++fxRainState.dropsEnd]=Droplet(true);
        }
      }

      // process
      float s=0;
      for (unsigned char j=fxRainState.dropsBegin; j!=fxRainState.dropsEnd; j++) {
        s+=sinT[fxRainState.drops[j].pos>>6]*fxRainState.drops[j].vol*dropletVol[fxRainState.drops[j].life++];

        fxRainState.drops[j].pos+=fxRainState.drops[j].freq;
        fxRainState.drops[j].freq+=70;

        if (fxRainState.drops[j].life<0) fxRainState.dropsBegin++;
      }
      fxRainState.prebuf[i<<1]=s;
      fxRainState.prebuf[1+(i<<1)]=s;
    }
  }

  if (fxRainState.vol>0) {
    for (int i=0; i<nframes; i++) {
      buf[0][i<<1]+=fxRainState.vol*(fxRainState.prebuf[i<<1]*0.5);
      buf[1][i<<1]+=fxRainState.vol*(fxRainState.prebuf[1+(i<<1)]*0.5);
    }
  }

  // check if we can suspend
  if (audible) {
    suspendTime=ar.freq*2;
  } else {
    suspendTime-=ar.samples;
    if (suspendTime<0) {
      printf("suspending audio engine.\n");
      SDL_PauseAudioDevice(ai,1);
    }
  }
}

float getMusicSpeed(int scene, int id) {
  return findMusic(id)->speed;
}

void setMusicSpeed(int scene, int id, float speed) {
  CHECK_SUSPEND
  findMusic(id)->speed=speed;
}

int initAudio() {
  if (audioUp) return 1;

  SDL_Init(SDL_INIT_AUDIO);

  ac.freq=44100;
  ac.format=AUDIO_F32;
  ac.channels=2;
  ac.samples=bufSize;
  ac.callback=process;
  ac.userdata=NULL;

  printf("creating audio!\n");
  ai=SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0,0),0,&ac,&ar,SDL_AUDIO_ALLOW_ANY_CHANGE);
  if (ai<0) {
    printf("failed to init audio.\n");
    return 0;
  }
  fxRainState.prebuf=new float[ar.samples*ar.channels];
  fxRainState.postbuf=new float[ar.samples*ar.channels];

  float v=1;
  for (int i=0; i<128; i++) {
    v*=0.92;
    dropletVol[i]=v;
  }

  audioUp=true;

  suspendTime=ar.freq*2;
  SDL_PauseAudioDevice(ai,0);
  
  return 1;
}

void play(int scene, int id) {
  CHECK_SUSPEND
  findMusic(id)->play();
}

void stop(int scene, int id) {
  CHECK_SUSPEND
  findMusic(id)->stop();
}

void goBegin(int scene, int id) {
  CHECK_SUSPEND
  findMusic(id)->goBegin();
}

int playSound(int scene, string path) {
  if (!audioUp) {
    printf("first loading audio\n");
    if (!initAudio()) return -1;
  }
  CHECK_SUSPEND
  Sound* s=new Sound(++sndc);
  s->load(path);
  snd.push_back(s);
  return sndc;
}

int playSoundVol(int scene, string path, float vol) {
  if (!audioUp) {
    printf("first loading audio\n");
    if (!initAudio()) return -1;
  }
  CHECK_SUSPEND
  Sound* s=new Sound(++sndc);
  s->volume=vol;
  s->load(path);
  snd.push_back(s);
  return sndc;
}

int playSoundEx(int scene, string path, float vol, float pitch) {
  if (!audioUp) {
    printf("first loading audio\n");
    if (!initAudio()) return -1;
  }
  CHECK_SUSPEND
  Sound* s=new Sound(++sndc);
  s->volume=vol;
  s->speed=pitch;
  s->load(path);
  snd.push_back(s);
  return sndc;
}

void stopSound(int scene, int id) {
  Sound* s=findSound(id);
  CHECK_SUSPEND
  if (s) s->killme=true;
}

void loadSong(int scene, int id, string path) {
  if (!audioUp) {
    printf("first loading audio\n");
    if (!initAudio()) return;
  }
  CHECK_SUSPEND
  findMusic(id)->load(path);
}

bool isPlaying(int scene, int id) {
  return findMusic(id)->playing;
}

float getMusicVolume(int scene, int id) {
  return findMusic(id)->volume;
}

void setMusicVolume(int scene, int id, float v) {
  CHECK_SUSPEND
  findMusic(id)->volume=v;
}

bool getEffect(unsigned char id) {
  if (id>=fxMax) return false;
  return effects[id];
}

void setEffect(unsigned char id, bool value) {
  if (id<fxMax) {
    CHECK_SUSPEND
    effects[id]=value;
  }
}

string debugInfo() {
  string d;
  sprintf(dstr,"%d samples @ %dHz (%s)<br/>",ar.samples,ar.freq,(suspendTime<0)?("suspended"):("running"));
  d=dstr;
  sprintf(dstr,"object: %lu | hash: %lu | music: %lu | sound: %lu<br/>objects:<br/>",man.items.size(),sman.items.size(),mus.size(),snd.size());
  d+=dstr;
  for (ObjectInfo& i: man.items) {
    sprintf(dstr,"- %d: %p<br/>",i.id,i.ptr);
    d+=dstr;
  }
  d+="music:<br/>";
  for (Music* i: mus) {
    sprintf(dstr,"- %d (obj %d): %s %s (%c:%d/%d) %gx v%g @ %dHz<br/>",i->mid,i->id,(i->playing)?"playing":"stopped",i->path.c_str(),(i->inLoop)?'l':'i',i->dcPosR,i->dcPosW,i->speed,i->volume,i->rate);
    d+=dstr;
  }
  d+="sound:<br/>";
  for (Sound* i: snd) {
    sprintf(dstr,"- %d (obj %d): %s (%d/%d) %gx v%g @ %dHz<br/>",i->sid,i->id,i->path.c_str(),i->dcPosR,i->dcPosW,i->speed,i->volume,i->rate);
    d+=dstr;
  }
  return d;
}

void setBufSize(int n) {
  bufSize=n;
}

EMSCRIPTEN_BINDINGS(asuka) {
  emscripten::function("getMusicSpeed",&getMusicSpeed);
  emscripten::function("setMusicSpeed",&setMusicSpeed);
  emscripten::function("getMusicVolume",&getMusicVolume);
  emscripten::function("setMusicVolume",&setMusicVolume);
  emscripten::function("getEffect",&getEffect);
  emscripten::function("setEffect",&setEffect);
  emscripten::function("isPlaying",&isPlaying);
  emscripten::function("play",&play);
  emscripten::function("playSound",&playSound);
  emscripten::function("playSoundVol",&playSoundVol);
  emscripten::function("playSoundEx",&playSoundEx);
  emscripten::function("stop",&stop);
  emscripten::function("stopSound",&stopSound);
  emscripten::function("goBegin",&goBegin);
  emscripten::function("initAudio",&initAudio);
  emscripten::function("loadSong",&loadSong);
  emscripten::function("debugInfo",&debugInfo);
  emscripten::function("setBufSize",&setBufSize);
}
