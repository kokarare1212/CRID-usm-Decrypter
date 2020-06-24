#pragma once

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include <stdio.h>

//--------------------------------------------------
// ADXクラス
//--------------------------------------------------
class clADX{
public:
	clADX();
	~clADX();

	// チェック
	static bool CheckFile(void *data);

	// デコード
	bool Decode(const char *filename,const char *filenameWAV);
	bool Decode(FILE *fp,void *data,int size,unsigned int address);

private:
	struct stHeader{
		unsigned short signature;    // シグネチャ 0x8000
		unsigned short dataOffset;   // データオフセット(ヘッダサイズ)-4
		unsigned char r04;           // バージョン？ 3
		unsigned char r05;           // ブロックサイズ？ 18
		unsigned char r06;           // ？ 4
		unsigned char channelCount;  // チャンネル数
		unsigned int samplingRate;   // サンプリングレート
		unsigned int sampleCount;    // 合計サンプル数
		unsigned char r10;
		unsigned char r11;
		unsigned char r12;
		unsigned char r13;
		unsigned int r14;
		unsigned short r18;
		unsigned short r1A;
		unsigned short r1C;
		unsigned short r1E;
	};
	struct stInfo{//channelCountが3以上の時に(channelCount-2)回分存在
		unsigned short r00;
		unsigned short r02;
	};
	struct stAINF{
		unsigned int ainf;// 'AINF'
		unsigned int r04;
		unsigned char r08[0x10];
		unsigned short r18;
		unsigned short r1A;
		unsigned short r1C;
		unsigned short r1E;
	};
	stHeader _header;
	int *_data;
	static void Decode(int *d,unsigned char *s);
};
