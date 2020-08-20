#include "TLVPackage.h"
#define ELPP_THREAD_SAFE  
#include "easylogging++.h"
//http://www.cnblogs.com/liping13599168/archive/2011/06/15/2081366.html

TLVPackage::TLVPackage()
{
}


TLVPackage::~TLVPackage()
{
}

// 构造TLV
void TLVPackage::Construct(
	unsigned char* buffer,
	unsigned int bufferLength,
	TLVEntity* tlvEntity,
	unsigned int& entityLength,
	unsigned int status
)
{
	int currentTLVIndex = 0;
	int currentIndex = 0;
	int currentStatus = 'T'; //状态字符
	unsigned long valueSize = 0;
	while (currentIndex < bufferLength)
	{
		switch (currentStatus)
		{
		case 'T':
			valueSize = 0;
			//判断是否单一结构
			if ((status == 1 && buffer[currentIndex] & 0x20) != 0x20)
			{
				tlvEntity[currentTLVIndex].Sub_TLVEntity = NULL; //单一结构时将子Tag置空
																 //判断是否多字节Tag
				if ((buffer[currentIndex] & 0x1f) == 0x1f)
				{
					int endTagIndex = currentIndex;
					while ((buffer[++endTagIndex] & 0x80) == 0x80); //判断第二个字节的最高位是否为1
					int tagSize = endTagIndex - currentIndex + 1; //计算Tag包含多少字节
					tlvEntity[currentTLVIndex].Tag = new unsigned char[tagSize];
					memcpy(tlvEntity[currentTLVIndex].Tag, buffer + currentIndex, tagSize);
					tlvEntity[currentTLVIndex].Tag[tagSize] = 0;
					tlvEntity[currentTLVIndex].TagSize = tagSize;
					currentIndex += tagSize;
				}
				else
				{
					tlvEntity[currentTLVIndex].Tag = new unsigned char[1];
					memcpy(tlvEntity[currentTLVIndex].Tag, buffer + currentIndex, 1);
					tlvEntity[currentTLVIndex].Tag[1] = 0;
					tlvEntity[currentTLVIndex].TagSize = 1;
					currentIndex += 1;
				}
			}
			else
			{
				//判断是否多字节Tag
				if ((buffer[currentIndex] & 0x1f) == 0x1f)
				{
					int endTagIndex = currentIndex;
					while ((buffer[++endTagIndex] & 0x80) == 0x80); //判断第二个字节的最高位是否为1
					int tagSize = endTagIndex - currentIndex + 1; //计算Tag包含多少字节
					tlvEntity[currentTLVIndex].Tag = new unsigned char[tagSize];
					memcpy(tlvEntity[currentTLVIndex].Tag, buffer + currentIndex, tagSize);
					tlvEntity[currentTLVIndex].Tag[tagSize] = 0;
					tlvEntity[currentTLVIndex].TagSize = tagSize;
					currentIndex += tagSize;
				}
				else
				{
					tlvEntity[currentTLVIndex].Tag = new unsigned char[1];
					memcpy(tlvEntity[currentTLVIndex].Tag, buffer + currentIndex, 1);
					tlvEntity[currentTLVIndex].Tag[1] = 0;
					tlvEntity[currentTLVIndex].TagSize = 1;
					currentIndex += 1;
				}
				//分析SubTag
				int subLength = 0;
				unsigned char* temp;
				if ((buffer[currentIndex] & 0x80) == 0x80)
				{
					for (int index = 0; index < 2; index++)
					{
						subLength += buffer[currentIndex + 1 + index] << (index * 8); //计算Length域的长度
					}
					temp = new unsigned char[subLength];
					memcpy(temp, buffer + currentIndex + 3, subLength);
				}
				else
				{
					subLength = buffer[currentIndex];
					temp = new unsigned char[subLength];
					memcpy(temp, buffer + currentIndex + 1, subLength);
				}
				temp[subLength] = 0;
				//memcpy(temp, buffer + currentIndex + 1, subLength);
				unsigned int oLength;
				tlvEntity[currentTLVIndex].Sub_TLVEntity = new TLVEntity[1];
				Construct(temp, subLength, tlvEntity[currentTLVIndex].Sub_TLVEntity, oLength);
			}
			currentStatus = 'L';
			break;
		case 'L':
			//判断长度字节的最高位是否为1，如果为1，则该字节为长度扩展字节，由下一个字节开始决定长度
			if ((buffer[currentIndex] & 0x80) != 0x80)
			{
				tlvEntity[currentTLVIndex].Length = new unsigned char[1];
				memcpy(tlvEntity[currentTLVIndex].Length, buffer + currentIndex, 1);
				tlvEntity[currentTLVIndex].Length[1] = 0;
				tlvEntity[currentTLVIndex].LengthSize = 1;
				valueSize = tlvEntity[currentTLVIndex].Length[0];
				currentIndex += 1;
			}
			else
			{
				//为1的情况
				unsigned int lengthSize = buffer[currentIndex] & 0x7f;
				currentIndex += 1; //从下一个字节开始算Length域
				for (int index = 0; index < lengthSize; index++)
				{
					valueSize += buffer[currentIndex + index] << (index * 8); //计算Length域的长度
				}
				tlvEntity[currentTLVIndex].Length = new unsigned char[lengthSize];
				memcpy(tlvEntity[currentTLVIndex].Length, buffer + currentIndex, lengthSize);
				tlvEntity[currentTLVIndex].Length[lengthSize] = 0;
				tlvEntity[currentTLVIndex].LengthSize = lengthSize;
				currentIndex += lengthSize;
			}
			currentStatus = 'V';
			break;
		case 'V':
			tlvEntity[currentTLVIndex].Value = new unsigned char[valueSize];
			memcpy(tlvEntity[currentTLVIndex].Value, buffer + currentIndex, valueSize);
			tlvEntity[currentTLVIndex].Value[valueSize] = 0;
			currentIndex += valueSize;
			//进入下一个TLV构造循环
			currentTLVIndex += 1;
			currentStatus = 'T';
			break;
		default:
			return;
		}
	}
	entityLength = currentTLVIndex;
}
// 解析TLV
void TLVPackage::Parse(
	TLVEntity* tlvEntity,
	unsigned int entityLength,
	unsigned char* buffer,
	unsigned int& bufferLength
)
{
	int currentIndex = 0;
	int currentTLVIndex = 0;
	unsigned long valueSize = 0;
	while (currentTLVIndex < entityLength)
	{
		valueSize = 0;
		TLVEntity entity = tlvEntity[currentTLVIndex];
		memcpy(buffer + currentIndex, entity.Tag, entity.TagSize);  //解析Tag
		currentIndex += entity.TagSize;
		for (int index = 0; index < entity.LengthSize; index++)
		{
			valueSize += entity.Length[index] << (index * 8); //计算Length域的长度
		}
		if (valueSize > 127)
		{
			buffer[currentIndex] = 0x80 | entity.LengthSize;
			currentIndex += 1;
		}
		memcpy(buffer + currentIndex, entity.Length, entity.LengthSize);    //解析Length
		currentIndex += entity.LengthSize;
		//判断是否包含子嵌套TLV
		if (entity.Sub_TLVEntity == nullptr)
		{
			memcpy(buffer + currentIndex, entity.Value, valueSize); //解析Value
			currentIndex += valueSize;
		}
		else
		{
			unsigned int oLength;
			Parse(entity.Sub_TLVEntity, 1, buffer + currentIndex, oLength); //解析子嵌套TLV
			currentIndex += oLength;
		}
		currentTLVIndex++;
	}
	buffer[currentIndex] = 0;
	bufferLength = currentIndex;
}