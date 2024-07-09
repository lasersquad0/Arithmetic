#pragma once

#include <string>


#define CODER_MASK  (0x0F)
#define MODEL_MASK  (0xF0)
#define MODEL_SHIFT (4)

/*
��������� ������ � ����� � ������
1. ������ ���� ����� � 8 ���� (long)
2. CRC32 ���� ����� ��� �������� ������������ ������������ � ����� ����������� � 8 ����
3. Modified date ���� �����
4. ������ ������� ����� � 8 ���� (long) - ������ ������ ������ ��� ������������
5. ��� ��������� ������� ��� ���� ����
6. ���������� ������ ������� ����� (��� ��������� ���������� ������������ ����)
7. ������ ������ ����� �����
8. ������ ����� ����� ������� ��������� � ������. �� ������ ������ ������������ 2 ����� ��� ������.
*/
class FileRecord
{
public:
	// **** ����� ������ ��������� � HashMap ����� ����������� ����� ���� ����!! *********
	std::string fileName;
	std::string origFilename;
	std::string dirName; // for future extensions, not used now
	uint64_t fileSize = 0L;
	uint64_t CRC32Value = 0L;
	uint64_t modifiedDate = 0L;
	uint64_t compressedSize = 0L;
	uint8_t  alg = 0;    // when saving it contains both alg and model order number used to compress this file
	uint32_t blockCount = 0;
	uint32_t blockSize = 0;
	uint8_t modelOrder = 0;

	void save(std::ofstream* sout)
	{
		uint8_t alg_mo = alg | (modelOrder << MODEL_SHIFT); // ������� 4 ���� �������� ��� ����� model order. ������� 4 ���� ��� ����� ���������.

		sout->write((char*)&fileSize, sizeof(uint64_t));
		sout->write((char*)&CRC32Value, sizeof(uint64_t));
		sout->write((char*)&modifiedDate, sizeof(uint64_t));
		sout->write((char*)&compressedSize, sizeof(uint64_t));
		sout->write((char*)&alg_mo, sizeof(uint8_t));
		sout->write((char*)&blockCount, sizeof(uint32_t));
		sout->write((char*)&blockSize, sizeof(uint32_t));

		uint64_t len = fileName.length();
		sout->write((char*)&len, sizeof(uint16_t));
		sout->write(fileName.c_str(), fileName.length());  // NOTE! writes 2 bytes for each char
	}

	void load(std::ifstream* sin)
	{
		sin->read((char*)&fileSize, sizeof(uint64_t));
		sin->read((char*)&CRC32Value, sizeof(uint64_t));
		sin->read((char*)&modifiedDate, sizeof(uint64_t));
		sin->read((char*)&compressedSize, sizeof(uint64_t));
		sin->read((char*)&alg, sizeof(uint8_t));
		sin->read((char*)&blockCount, sizeof(uint32_t));
		sin->read((char*)&blockSize, sizeof(uint32_t));

		modelOrder = alg >> MODEL_SHIFT; // �������� �� ����� model type
		alg = alg & CODER_MASK; // ������ ����� ��������� �� ����� model type

		uint16_t filenameSize;
		sin->read((char*)&filenameSize, sizeof(uint16_t));

		char buf[MAX_PATH];
		sin->read((char*)buf, filenameSize);
		fileName.append(buf, filenameSize);
	}

	time_t GetModifiedDateAsTimeT()
	{
		std::filesystem::file_time_type dt{ std::filesystem::file_time_type::duration{ modifiedDate } };
		//std::chrono::file_clock::time_point dt{ std::chrono::file_clock::duration{ modifiedDate } };
		auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(dt - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		return std::chrono::system_clock::to_time_t(sctp);
	}
};
