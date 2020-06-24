
//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include "clCRID.h"
#include "clADX.h"
#ifndef _countof
#define _countof(_array) (sizeof(_array)/sizeof(_array[0]))
#endif

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
inline WCHAR bswap(WCHAR v){short r=v&0xFF;r<<=8;v>>=8;r|=v&0xFF;return r;}

//--------------------------------------------------
// 拡張子を取得
//--------------------------------------------------
char *GetExtension(char *extension,int size,const char *path){
	if(size>0)extension[0]='\0';
	for(int i=strlen(path)-1;i>=0;i--){
		if(path[i]=='.'){
			strcpy_s(extension,size,&path[i+1]);
			break;
		}else if(path[i]=='\\'||path[i]=='/'){
			break;
		}
	}
	return extension;
}

//--------------------------------------------------
// ファイル名に使えない文字を大文字に変更
//--------------------------------------------------
char *FixFilename(char *fix_filename,int size,const char *filename){
	memset(fix_filename,0,size);
	for(int i=0,len=strlen(filename);i<len&&i<size-3;i++,filename++){
		switch(*filename){
		case '*':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'＊');break;
		case '|':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'｜');break;
		case '\\':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'￥');break;
		case ':':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'：');break;
		case '"':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'”');break;
		case '<':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'＜');break;
		case '>':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'＞');break;
		case '?':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'？');break;
		case '/':*(WCHAR *)&fix_filename[i++]=bswap((WCHAR)'／');break;
		default:fix_filename[i]=*filename;break;
		}
	}
	return fix_filename;
}

//--------------------------------------------------
// コンストラクタ
//--------------------------------------------------
clCRID::clCRID(unsigned int ciphKey1,unsigned int ciphKey2):_utf(){
	InitMask(ciphKey1,ciphKey2);
}

//--------------------------------------------------
// CRIDファイルチェック
//--------------------------------------------------
bool clCRID::CheckFile(void *data,unsigned int size){
	return (data&&size>=4&&*(unsigned int *)data==0x44495243);
}

//--------------------------------------------------
// ファイルをロード
//--------------------------------------------------
bool clCRID::LoadFile(const char *filename){

	// チェック
	if(!(filename))return false;

	// 開く
	FILE *fp;
	if(fopen_s(&fp,filename,"rb"))return false;

	// ヘッダを取得
	stInfo info;
	fread(&info,sizeof(info),1,fp);
	if(!CheckFile(&info,sizeof(info))){fclose(fp);return false;}
	//info.signature=bswap(info.signature);
	info.dataSize=bswap(info.dataSize);
	info.paddingSize=bswap(info.paddingSize);
	//info.frameTime=bswap(info.frameTime);
	//info.frameRate=bswap(info.frameRate);
	//info.r18=bswap(info.r18);
	//info.r1C=bswap(info.r1C);

	// データ読み込み
	unsigned int size=info.dataSize-info.dataOffset-info.paddingSize;
	unsigned char *data=new unsigned char [size];
	if(!data){fclose(fp);return false;}
	fseek(fp,(int)info.dataOffset-0x18,SEEK_CUR);
	fread(data,size,1,fp);

	// UTF情報を読み込み
	if(info.dataType!=1||!_utf.LoadData(data)){
		delete [] data;
		fclose(fp);
		return false;
	}

	// 開放
	delete [] data;
	fclose(fp);

	return true;
}

//--------------------------------------------------
// 分離
//--------------------------------------------------
bool clCRID::Demux(const char *filename,const char *directory,bool adxDecode){

	// 開放
	_utf.Release();

	// チェック
	if(!(filename&&directory))return false;

	// 開く
	FILE *fp,*fpInfo=NULL,*fpVideo=NULL,*fpAudio=NULL;
	if(fopen_s(&fp,filename,"rb"))return false;

	// チェック
	stInfo info;
	fread(&info,sizeof(info),1,fp);
	if(!CheckFile(&info,sizeof(info))){fclose(fp);return false;}

	// 分離
	clADX adx;unsigned int sfaAddress=0;
	fseek(fp,0,SEEK_END);
	int fileSize=ftell(fp);
	fseek(fp,0,SEEK_SET);
	while(fileSize>0){

		// 情報を取得
		fread(&info,sizeof(info),1,fp);fileSize-=sizeof(info);
		info.signature=bswap(info.signature);
		info.dataSize=bswap(info.dataSize);
		info.paddingSize=bswap(info.paddingSize);
		info.frameTime=bswap(info.frameTime);
		info.frameRate=bswap(info.frameRate);
		//info.r18=bswap(info.r18);
		//info.r1C=bswap(info.r1C);

		// データ読み込み
		unsigned int size=info.dataSize-info.dataOffset-info.paddingSize;
		unsigned char *data=new unsigned char [size];
		if(!data){
			fclose(fp);
			if(fpVideo)fclose(fpVideo);
			if(fpAudio)fclose(fpAudio);
			if(fpInfo)fclose(fpInfo);
			return false;
		}
		fseek(fp,(int)info.dataOffset-0x18,SEEK_CUR);
		fread(data,size,1,fp);
		fseek(fp,info.paddingSize,SEEK_CUR);
		fileSize-=info.dataSize-0x18;

		// 種類別に処理
		switch(info.signature){
		case 0x43524944://CRID
			{
				if(info.dataType==1){
					char filename[0x400],fix_filename[0x400];
					_utf.LoadData(data);
					for(unsigned int i=0,count=_utf.GetPageCount();i<count;i++){
						switch(_utf.GetElement(i,"stmid")->GetValueInt()){
						case 0x00000000:
							if(!fpInfo){
								FixFilename(fix_filename,_countof(fix_filename),_utf.GetElement(i,"filename")->GetValueString());
								sprintf_s(filename,_countof(filename),"%s\\%s.ini",directory,fix_filename);
								fopen_s(&fpInfo,filename,"wb");
							}
							break;
						case 0x40534656:
							if(!fpVideo){
								FixFilename(fix_filename,_countof(fix_filename),_utf.GetElement(i,"filename")->GetValueString());
								sprintf_s(filename,_countof(filename),"%s\\%s.m2v",directory,fix_filename);
								fopen_s(&fpVideo,filename,"wb");
							}
							break;
						case 0x40534641:
							if(!fpAudio){
								char ext[4];
								FixFilename(fix_filename,_countof(fix_filename),_utf.GetElement(i,"filename")->GetValueString());
								sprintf_s(filename,_countof(filename),"%s\\%s",directory,fix_filename);
								if(strcmp(GetExtension(ext,_countof(ext),filename),"wav")!=0)strcat_s(filename,_countof(filename),".wav");
								fopen_s(&fpAudio,filename,"wb");
							}
							break;
						}
					}
				}
				if(info.dataType==1||info.dataType==3){
					_utf.SaveFileINI(fpInfo);
				}else if(info.dataType==2){
					WriteInfo(fpInfo,(char *)data);
				}
			}
			break;
		case 0x40534656://@SFV
			{
				if(info.dataType==0){
					MaskVideo(data,size);
					if(fpVideo)fwrite(data,size,1,fpVideo);
				}else if(info.dataType==1||info.dataType==3){
					clUTF utf;
					utf.LoadData(data);
					utf.SaveFileINI(fpInfo);
				}else if(info.dataType==2){
					//WriteInfo(fpInfo,(char *)data);
				}
			}
			break;
		case 0x40534641://@SFA
			{
				if(info.dataType==0){
					MaskAudio(data,size);
					if(fpAudio)adx.Decode(fpAudio,data,size,sfaAddress);
					sfaAddress+=size;
				}else if(info.dataType==1||info.dataType==3){
					clUTF utf;
					utf.LoadData(data);
					utf.SaveFileINI(fpInfo);
				}else if(info.dataType==2){
					//WriteInfo(fpInfo,(char *)data);
				}
			}
			break;
		//default:__asm int 3;break;
		}

		// 開放
		delete [] data;

	}

	// 閉じる
	fclose(fp);
	if(fpVideo)fclose(fpVideo);
	if(fpAudio)fclose(fpAudio);
	if(fpInfo)fclose(fpInfo);

	return true;
}

//--------------------------------------------------
// マルチプレクサ
//--------------------------------------------------
bool clCRID::Mux(const char *filename,const char *filenameMovie,const char *filenameAudio){
	return false;//未実装@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
}

//--------------------------------------------------
// マスク初期化
//--------------------------------------------------
void clCRID::InitMask(unsigned int key1,unsigned int key2){

	// テーブルを生成
	unsigned char t[0x20];
	t[0x00]=((unsigned char *)&key1)[0];
	t[0x01]=((unsigned char *)&key1)[1];
	t[0x02]=((unsigned char *)&key1)[2];
	t[0x03]=((unsigned char *)&key1)[3]-0x34;
	t[0x04]=((unsigned char *)&key2)[0]+0xF9;
	t[0x05]=((unsigned char *)&key2)[1]^0x13;
	t[0x06]=((unsigned char *)&key2)[2]+0x61;
	t[0x07]=t[0x00]^0xFF;
	t[0x08]=t[0x02]+t[0x01];
	t[0x09]=t[0x01]-t[0x07];
	t[0x0A]=t[0x02]^0xFF;
	t[0x0B]=t[0x01]^0xFF;
	t[0x0C]=t[0x0B]+t[0x09];
	t[0x0D]=t[0x08]-t[0x03];
	t[0x0E]=t[0x0D]^0xFF;
	t[0x0F]=t[0x0A]-t[0x0B];
	t[0x10]=t[0x08]-t[0x0F];
	t[0x11]=t[0x10]^t[0x07];
	t[0x12]=t[0x0F]^0xFF;
	t[0x13]=t[0x03]^0x10;
	t[0x14]=t[0x04]-0x32;
	t[0x15]=t[0x05]+0xED;
	t[0x16]=t[0x06]^0xF3;
	t[0x17]=t[0x13]-t[0x0F];
	t[0x18]=t[0x15]+t[0x07];
	t[0x19]=0x21-t[0x13];
	t[0x1A]=t[0x14]^t[0x17];
	t[0x1B]=t[0x16]+t[0x16];
	t[0x1C]=t[0x17]+0x44;
	t[0x1D]=t[0x03]+t[0x04];
	t[0x1E]=t[0x05]-t[0x16];
	t[0x1F]=t[0x1D]^t[0x13];

	// マスクを生成
	unsigned char t2[4]={'U','R','U','C'};
	for(int i=0;i<0x20;i++){
		_videoMask1[i]=t[i];
		_videoMask2[i]=t[i]^0xFF;
		_audioMask[i]=(i&1)?t2[(i>>1)&3]:t[i]^0xFF;
	}

}

//--------------------------------------------------
// ビデオマスク
//--------------------------------------------------
void clCRID::MaskVideo(unsigned char *data,int size){
	data+=0x40;
	size-=0x40;
	if(size>=0x200){
		unsigned char mask[0x20];
		memcpy(mask,_videoMask2,sizeof(mask));
		for(int i=0x100;i<size;i++){
			mask[i&0x1F]=(data[i]^=mask[i&0x1F])^_videoMask2[i&0x1F];
		}
		memcpy(mask,_videoMask1,sizeof(mask));
		for(int i=0;i<0x100;i++){
			data[i]^=(mask[i&0x1F]^=data[0x100+i]);
		}
	}
}

//--------------------------------------------------
// オーディオマスク
//--------------------------------------------------
void clCRID::MaskAudio(unsigned char *data,int size){
	data+=0x140;
	size-=0x140;
	for(int i=0;i<size;i++){
		*(data++)^=_audioMask[i&0x1F];
	}
}

//--------------------------------------------------
// 文字列を書き込み
//--------------------------------------------------
void clCRID::WriteInfo(FILE *fp,const char *string){
	if(fp&&string)fprintf(fp,"%s\r\n",string);
}
