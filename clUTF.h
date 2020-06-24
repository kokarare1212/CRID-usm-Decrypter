#pragma once

//--------------------------------------------------
// インクルード
//--------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

//--------------------------------------------------
// UTFクラス
//--------------------------------------------------
class clUTF{
public:
	class clElement{
	public:
		enum TYPE{
			TYPE_NULL,       // 空要素
			TYPE_CHAR,       // char
			TYPE_UCHAR,      // unsigned char
			TYPE_SHORT,      // short
			TYPE_USHORT,     // unsigned short
			TYPE_INT,        // int
			TYPE_UINT,       // unsigned int
			TYPE_LONGLONG,   // long long
			TYPE_ULONGLONG,  // unsigned long long
			TYPE_FLOAT,      // float
			TYPE_STRING,     // char *
			TYPE_DATA,       // void *
		};
	public:
		clElement():_prev(NULL),_next(NULL),_name(NULL),_type(TYPE_NULL),_valueLongLong(0){}
		bool IsNULL(void){return _type==TYPE_NULL;}
		clElement *GetPrev(void){return _prev;}
		clElement *GetNext(void){return _next;}
		const char *GetName(void){return _name;}
		TYPE GetType(void){return _type;}
		char GetValueChar(void){return _valueChar;}
		unsigned char GetValueUChar(void){return _valueUChar;}
		short GetValueShort(void){return _valueShort;}
		unsigned short GetValueUShort(void){return _valueUShort;}
		int GetValueInt(void){return _valueInt;}
		unsigned int GetValueUInt(void){return _valueUInt;}
		long long GetValueLongLong(void){return _valueLongLong;}
		unsigned long long GetValueULongLong(void){return _valueULongLong;}
		float GetValueFloat(void){return _valueFloat;}
		char *GetValueString(void){return _valueString;}
		void *GetData(void){return _valueData.data;}
		unsigned int GetDataSize(void){return _valueData.size;}
		void SetName(const char *name){_name=name;}
		void SetValueChar(char vaule){_valueChar=vaule;_type=TYPE_CHAR;}
		void SetValueUChar(unsigned char vaule){_valueUChar=vaule;_type=TYPE_UCHAR;}
		void SetValueShort(short vaule){_valueShort=vaule;_type=TYPE_SHORT;}
		void SetValueUShort(unsigned short vaule){_valueUShort=vaule;_type=TYPE_USHORT;}
		void SetValueInt(int vaule){_valueInt=vaule;_type=TYPE_INT;}
		void SetValueUInt(unsigned int vaule){_valueUInt=vaule;_type=TYPE_UINT;}
		void SetValueLongLong(long long vaule){_valueLongLong=vaule;_type=TYPE_LONGLONG;}
		void SetValueULongLong(unsigned long long vaule){_valueULongLong=vaule;_type=TYPE_ULONGLONG;}
		void SetValueFloat(float vaule){_valueFloat=vaule;_type=TYPE_FLOAT;}
		void SetValueString(char *vaule){_valueString=vaule;_type=TYPE_STRING;}
		void SetValueData(void *data,unsigned int size){_valueData.data=data;_valueData.size=size;_type=TYPE_DATA;}
	private:
		clElement *_prev;
		clElement *_next;
		const char *_name;
		TYPE _type;
		union{
			char _valueChar;
			unsigned char _valueUChar;
			short _valueShort;
			unsigned short _valueUShort;
			int _valueInt;
			unsigned int _valueUInt;
			long long _valueLongLong;
			unsigned long long _valueULongLong;
			float _valueFloat;
			char *_valueString;
			struct{
				void *data;
				unsigned int size;
			}_valueData;
		};
		friend clUTF;
	};

public:
	clUTF();
	~clUTF();

	// ロード
	static bool CheckFile(void *data,unsigned int size);
	bool LoadFile(const char *filename);
	bool LoadData(void *data);
	//bool SaveFile(const char *filename);
	bool SaveFileINI(const char *filename,bool subUTF=false);
	bool SaveFileINI(FILE *fp,bool subUTF=false,int tab=0);
	void Release(void);

	// 要素
	clElement *Add(unsigned int pageIndex,const char *name,char value);
	clElement *Add(unsigned int pageIndex,const char *name,unsigned char value);
	clElement *Add(unsigned int pageIndex,const char *name,short value);
	clElement *Add(unsigned int pageIndex,const char *name,unsigned short value);
	clElement *Add(unsigned int pageIndex,const char *name,int value);
	clElement *Add(unsigned int pageIndex,const char *name,unsigned int value);
	clElement *Add(unsigned int pageIndex,const char *name,long long value);
	clElement *Add(unsigned int pageIndex,const char *name,unsigned long long value);
	clElement *Add(unsigned int pageIndex,const char *name,float value);
	clElement *Add(unsigned int pageIndex,const char *name,char *value);
	clElement *Add(unsigned int pageIndex,const char *name,void *data,unsigned int size);
	
	// 取得
	const char *GetName(void){return _name;}
	unsigned int GetPageCount(void){return _pageCount;}
	clElement *GetElement(unsigned int pageIndex);                   // ※見つからない場合、TYPE_NULLの要素が返る
	clElement *GetElement(unsigned int pageIndex,const char *name);  // ※見つからない場合、TYPE_NULLの要素が返る

private:
	struct stHeader{
		unsigned int signature;
		unsigned int dataSize;
	};
	struct stInfo{
		unsigned int valueOffset;
		unsigned int stringOffset;
		unsigned int dataOffset;
		unsigned int nameOffset;
		unsigned short elementCount;
		unsigned short valueSize;
		unsigned int valueCount;
	};
	struct stPage{
		clElement *first;
		clElement *last;
	};
	char *_string;
	unsigned char *_data;
	char *_name;
	unsigned int _pageCount;
	stPage *_page;
	clElement *Add(stPage *parent,const char *name);
};
