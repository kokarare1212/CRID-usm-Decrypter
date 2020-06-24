
//--------------------------------------------------
// インクルード
//--------------------------------------------------
#include "clUTF.h"

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
// ファイルロード
//--------------------------------------------------
unsigned char *LoadFile(const char *filename,int *fileSize=NULL){

	// チェック
	if(!filename)return NULL;

	// 開く
	FILE *fp;
	if(fopen_s(&fp,filename,"rb"))return NULL;

	// サイズを取得
	fseek(fp,0,SEEK_END);
	int size=ftell(fp);

	// 確保
	unsigned char *data=new unsigned char [size+1];
	if(!data){fclose(fp);return NULL;}

	// 読み込み
	fseek(fp,0,SEEK_SET);
	fread(data,size,1,fp);
	data[size]='\0';

	// 閉じる
	fclose(fp);

	// サイズ設定
	if(fileSize)*fileSize=size;

	return data;
}

//--------------------------------------------------
// コンストラクタ/デストラクタ
//--------------------------------------------------
clUTF::clUTF():_string(NULL),_data(NULL),_name(NULL),_pageCount(0),_page(NULL){}
clUTF::~clUTF(){Release();}

//--------------------------------------------------
// 開放
//--------------------------------------------------
void clUTF::Release(void){

	// 開放
	if(_string)delete [] _string;
	if(_data)delete [] _data;
	for(unsigned int i=0;i<_pageCount;i++){
		for(clElement *e=_page[i].first,*next;e;e=next){
			next=e->_next;
			delete e;
		}
	}
	if(_page)delete [] _page;

	// リセット
	_string=NULL;
	_data=NULL;
	_name=NULL;
	_pageCount=0;
	_page=NULL;

}

//--------------------------------------------------
// UTFファイルかチェック
//--------------------------------------------------
bool clUTF::CheckFile(void *data,unsigned int size){
	return (data&&size>=4&&*(unsigned int *)data==0x46545540);
}

//--------------------------------------------------
// ファイルをロード
//--------------------------------------------------
bool clUTF::LoadFile(const char *filename){

	// 開放
	Release();

	// ファイルをロード
	unsigned char *data=::LoadFile(filename);
	if(!data)return false;

	// 解析
	if(!LoadData(data)){delete [] data;return false;}

	// 開放
	delete [] data;

	return true;
}
bool clUTF::LoadData(void *data){

	// 開放
	Release();

	// チェック
	if(!data)return false;

	// ヘッダを取得
	stHeader *header=(stHeader *)data;
	if(!CheckFile(header,sizeof(*header)))return false;
	//header->signature=bswap(header->signature);
	header->dataSize=bswap(header->dataSize);

	// 情報を取得
	stInfo *info=(stInfo *)((unsigned char *)data+sizeof(stHeader));
	info->valueOffset=bswap(info->valueOffset);
	info->stringOffset=bswap(info->stringOffset);
	info->dataOffset=bswap(info->dataOffset);
	info->nameOffset=bswap(info->nameOffset);
	info->elementCount=bswap(info->elementCount);
	info->valueSize=bswap(info->valueSize);
	info->valueCount=bswap(info->valueCount);

	// 文字列を取得
	_string=new char [info->dataOffset-info->stringOffset];
	if(!_string)return false;
	memcpy(_string,(unsigned char *)data+sizeof(stHeader)+info->stringOffset,info->dataOffset-info->stringOffset);

	// データを取得
	_data=new unsigned char [header->dataSize-info->dataOffset];
	if(!_data)return false;
	memcpy(_data,(unsigned char *)data+sizeof(stHeader)+info->dataOffset,header->dataSize-info->dataOffset);

	// 名前を取得
	_name=&_string[info->nameOffset];

	// 項目を取得
	unsigned char null[]={0,0,0,0,0,0,0,0};
	_pageCount=info->valueCount;
	_page=new stPage [info->valueCount];
	if(!_page)return false;
	memset(_page,0,sizeof(stPage)*info->valueCount);
	unsigned char *d=(unsigned char *)data+sizeof(stHeader)+info->valueOffset;
	for(unsigned int i=0;i<info->valueCount;i++){
		unsigned char *s=(unsigned char *)data+sizeof(stHeader)+sizeof(stInfo);
		for(unsigned int j=0;j<info->elementCount;j++){
			unsigned char type=*(s++);
			unsigned int offset=bswap(*(unsigned int *)s);s+=sizeof(offset);
			clElement *e=Add(&_page[i],&_string[offset]);
			if(!e)return false;
			unsigned char **p=NULL,*n;
			switch(type>>5){
			case 0:p=&n;n=null;break;
			case 1:p=&s;break;
			case 2:p=&d;break;
			//default:__asm int 3;break;
			}
			switch(type&0x1F){
			case 0x10:e->SetValueChar(**p);*p+=sizeof(char);break;
			case 0x11:e->SetValueUChar(**p);*p+=sizeof(unsigned char);break;
			case 0x12:e->SetValueShort(bswap(*(short *)*p));*p+=sizeof(short);break;
			case 0x13:e->SetValueUShort(bswap(*(unsigned short *)*p));*p+=sizeof(unsigned short);break;
			case 0x14:e->SetValueInt(bswap(*(int *)*p));*p+=sizeof(int);break;
			case 0x15:e->SetValueUInt(bswap(*(unsigned int *)*p));*p+=sizeof(unsigned int);break;
			case 0x16:e->SetValueLongLong(bswap(*(long long *)*p));*p+=sizeof(long long);break;
			case 0x17:e->SetValueULongLong(bswap(*(unsigned long long *)*p));*p+=sizeof(unsigned long long);break;
			case 0x18:e->SetValueFloat(bswap(*(float *)*p));*p+=sizeof(float);break;
			case 0x1A:e->SetValueString(&_string[bswap(*(unsigned int *)*p)]);*p+=sizeof(unsigned int);break;
			case 0x1B:e->SetValueData(&_data[bswap(*(unsigned int *)*p)],(int)bswap(*(unsigned int *)(*p+4)));*p+=sizeof(unsigned int)+sizeof(unsigned int);break;
			//default:__asm int 3;break;
			}
		}
	}

	return true;
}

//--------------------------------------------------
// 保存
//--------------------------------------------------
//bool clUTF::SaveFile(const char *filename){
//	return false;//未実装@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//}

//--------------------------------------------------
// 保存
//--------------------------------------------------
bool clUTF::SaveFileINI(const char *filename,bool subUTF){

	// チェック
	if(!(filename))return false;

	// 開く
	FILE *fp;
	if(fopen_s(&fp,filename,"wb"))return false;

	// 保存
	SaveFileINI(fp,subUTF);

	// 閉じる
	fclose(fp);

	return false;
}
bool clUTF::SaveFileINI(FILE *fp,bool subUTF,int tab){

	// チェック
	if(!fp)return false;

	//
	for(unsigned int i=0,count=_pageCount;i<count;i++){
		fprintf(fp,"\r\n");
		for(int j=tab;j>0;j--)fprintf(fp,"  ");
		fprintf(fp,"[%s_%d]\r\n",_name,i+1);
		for(clElement *e=_page[i].first;e;e=e->_next){
			for(int j=tab;j>0;j--)fprintf(fp,"  ");
			fprintf(fp,"%s = ",e->_name);
			switch(e->_type){
			case clElement::TYPE_CHAR:fprintf(fp,"%d",e->_valueChar);break;
			case clElement::TYPE_UCHAR:fprintf(fp,"%u",e->_valueUChar);break;
			case clElement::TYPE_SHORT:fprintf(fp,"%d",e->_valueShort);break;
			case clElement::TYPE_USHORT:fprintf(fp,"%u",e->_valueUShort);break;
			case clElement::TYPE_INT:fprintf(fp,"%d",e->_valueInt);break;
			case clElement::TYPE_UINT:fprintf(fp,"%u",e->_valueUInt);break;
			case clElement::TYPE_LONGLONG:fprintf(fp,"%lld",e->_valueLongLong);break;
			case clElement::TYPE_ULONGLONG:fprintf(fp,"%llu",e->_valueULongLong);break;
			case clElement::TYPE_FLOAT:fprintf(fp,"%g",e->_valueFloat);break;
			case clElement::TYPE_STRING:fprintf(fp,"%s",e->_valueString);break;
			case clElement::TYPE_DATA:
				if(subUTF&&clUTF::CheckFile(e->GetData(),e->GetDataSize())){
					clUTF utf;
					utf.LoadData(e->GetData());
					utf.SaveFileINI(fp,true,tab+1);
				}else{
					for(unsigned char *s=(unsigned char *)e->GetData(),*p=s+e->GetDataSize();s<p;s++)fprintf(fp,"%02X ",*s);
				}
				break;
			}
			fprintf(fp,"\r\n");
		}
	}

	return true;
}

//--------------------------------------------------
// 追加
//--------------------------------------------------
clUTF::clElement *clUTF::Add(stPage *parent,const char *name){
	if(!parent)return NULL;
	clElement *element=new clElement;
	if(element){
		element->SetName(name);
		element->_prev=parent->last;
		element->_next=NULL;
		if(parent->last){
			parent->last->_next=element;
		}else{
			parent->first=element;
		}
		parent->last=element;
	}
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,char value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueChar(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,unsigned char value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueChar(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,short value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueShort(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,unsigned short value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueShort(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,int value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueInt(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,unsigned int value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueInt(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,long long value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueLongLong(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,unsigned long long value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueLongLong(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,float value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueFloat(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,char *value){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueString(value);
	return element;
}
clUTF::clElement *clUTF::Add(unsigned int pageIndex,const char *name,void *data,unsigned int size){
	clElement *element=Add(&_page[pageIndex],name);
	if(element)element->SetValueData(data,size);
	return element;
}

//--------------------------------------------------
// 取得
//--------------------------------------------------
clUTF::clElement *clUTF::GetElement(unsigned int pageIndex){
	static clElement null;
	clElement *e=(pageIndex<_pageCount)?_page[pageIndex].first:NULL;
	return e?e:&null;
}
clUTF::clElement *clUTF::GetElement(unsigned int pageIndex,const char *name){
	clElement *e;
	for(e=GetElement(pageIndex);e&&!e->IsNULL();e=e->_next){
		if(strcmp(e->_name,name)==0)break;
	}
	return e;
}
