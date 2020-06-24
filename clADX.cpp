
//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include "clADX.h"
#include <stdio.h>
#include <memory.h>

//--------------------------------------------------
// インライン関数
//--------------------------------------------------
inline short bswap(short v){short r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline unsigned short bswap(unsigned short v){unsigned short r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline int bswap(int v){int r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline unsigned int bswap(unsigned int v){unsigned int r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline long long bswap(long long v){long long r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline unsigned long long bswap(unsigned long long v){unsigned long long r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}
inline float bswap(float v){unsigned int i=bswap(*(unsigned int *)&v);return *(float *)&i;}

//--------------------------------------------------
// コンストラクタ
//--------------------------------------------------
clADX::clADX():_data(NULL){
	memset(&_header,0,sizeof(_header));
}

//--------------------------------------------------
// デストラクタ
//--------------------------------------------------
clADX::~clADX(){
	if(_data){
		delete [] _data;
		_data=NULL;
	}
}

//--------------------------------------------------
// ADXチェック
//--------------------------------------------------
bool clADX::CheckFile(void *data){
	return (data&&*(unsigned short *)data==0x0080);
}

//--------------------------------------------------
// デコード
//--------------------------------------------------
bool clADX::Decode(const char *filename,const char *filenameWAV){

	// チェック
	if(!(filename&&filenameWAV))return false;

	// 開く
	FILE *fp,*fp2;
	if(fopen_s(&fp,filename,"rb"))return false;
	if(fopen_s(&fp2,filenameWAV,"wb")){fclose(fp);return false;}

	//
	fread(&_header,sizeof(_header),1,fp);
	if(!Decode(fp2,&_header,sizeof(_header),0)){fclose(fp2);fclose(fp);return false;}

	// 
	unsigned int size=18*_header.channelCount;
	unsigned char *data=new unsigned char [size];
	if(!data){fclose(fp2);fclose(fp);return false;}
	fseek(fp,_header.dataOffset,SEEK_SET);
	while(_header.sampleCount){
		fread(data,size,1,fp);
		if(!Decode(fp2,data,size,_header.dataOffset)){
			delete [] data;
			fclose(fp2);
			fclose(fp);
			return false;
		}
	}
	delete [] data;

	// 閉じる
	fclose(fp);
	fclose(fp2);

	return true;
}

//--------------------------------------------------
// デコード
//--------------------------------------------------
bool clADX::Decode(FILE *fp,void *data,int size,unsigned int address){

	// チェック
	if(!(fp&&data))return false;

	// ヘッダ
	if(address==0){
		if(size<sizeof(_header))return false;

		// ヘッダを取得
		memcpy(&_header,data,sizeof(_header));
		if(!CheckFile(&_header))return false;
		//_header.signature=bswap(_header.signature);
		_header.dataOffset=bswap(_header.dataOffset)+4;
		_header.samplingRate=bswap(_header.samplingRate);
		_header.sampleCount=bswap(_header.sampleCount);

		// WAVEヘッダを書き込み
		struct stWAVEHeader{
			char riff[4];
			unsigned int riffSize;
			char wave[4];
			char fmt[4];
			unsigned int fmtSize;
			unsigned short fmtType;
			unsigned short fmtChannelCount;
			unsigned int fmtSamplingRate;
			unsigned int fmtSamplesPerSec;
			unsigned short fmtSamplingSize;
			unsigned short fmtBitCount;
			char data[4];
			unsigned int dataSize;
		}wav={'R','I','F','F',0,'W','A','V','E','f','m','t',' ',0x10,1,0,0,0,0,16,'d','a','t','a',0};
		wav.fmtChannelCount=_header.channelCount;
		wav.fmtSamplingRate=_header.samplingRate;
		wav.fmtSamplingSize=2*wav.fmtChannelCount;
		wav.fmtSamplesPerSec=wav.fmtSamplingRate*wav.fmtSamplingSize;
		wav.dataSize=_header.sampleCount*wav.fmtSamplingSize;
		wav.riffSize=wav.dataSize+0x24;
		fwrite(&wav,sizeof(wav),1,fp);

		//
		if(_data)delete [] _data;
		_data=new int [32*_header.channelCount];
		if(!_data)return false;
		memset(_data,0,sizeof(int)*32*_header.channelCount);

	}

	// データ
	else if(address>=_header.dataOffset&&_data){
		for(unsigned char *s=(unsigned char *)data,*e=s+size-18*_header.channelCount;s<=e;){
			int *d=_data;
			for(unsigned int i=_header.channelCount;i>0;i--,d+=32,s+=18){
				Decode(d,s);
			}
			d=_data;
			for(int i=32;i>0&&_header.sampleCount;i--,d++,_header.sampleCount--){
				for(unsigned int j=0;j<_header.channelCount;j++){
					int v=d[j*32];
					if(v>0x7FFF)v=0x7FFF;
					else if(v<-0x8000)v=-0x8000;
					fwrite(&v,2,1,fp);
				}
			}
		}
	}

	else{
		return false;
	}

	return true;
}

//--------------------------------------------------
// デコード ※検証中
//--------------------------------------------------
void clADX::Decode(int *d,unsigned char *s){
	int scale=bswap(*(unsigned short *)s);s+=2;
	int v,p=d[31],pp=d[30];
	for(int i=16;i>0;i--,s++){
		v=*s>>4;if(v&8)v-=16;
		v=(v*scale*0x4000+p*0x7298-pp*0x3350)>>14;
		pp=p;p=v;*(d++)=v;
		v=*s&0xF;if(v&8)v-=16;
		v=(v*scale*0x4000+p*0x7298-pp*0x3350)>>14;
		pp=p;p=v;*(d++)=v;
	}
}
