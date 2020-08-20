#ifndef TLVPACKAGE_H__
#define TLVPACKAGE_H__


// TLV结构体

struct TLVEntity {
	unsigned char* Tag;         //标记
	unsigned char* Length;      //数据长度
	unsigned char* Value;       //数据
	unsigned int TagSize;       //标记占用字节数
	unsigned int LengthSize;    //数据长度占用字节数
	TLVEntity* Sub_TLVEntity;   //子嵌套TLV实体
};

class TLVPackage
{
private:
	TLVPackage();
	virtual ~TLVPackage();

public:
	//构造TLV实体
	static void Construct(unsigned char* buffer, unsigned int bufferLength, TLVEntity* tlvEntity, unsigned int& entityLength, unsigned int status = 0);

	//解析TLV字节数组
	static void Parse(TLVEntity* tlvEntity, unsigned int entityLength, unsigned char* buffer, unsigned int& bufferLength);
};
#endif // !TLVPACKAGE_H__