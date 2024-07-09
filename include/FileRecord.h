#pragma once

#include <string>


#define CODER_MASK  (0x0F)
#define MODEL_MASK  (0xF0)
#define MODEL_SHIFT (4)

/*
Структура записи о файле в архиве
1. Размер ориг файла — 8 байт (long)
2. CRC32 ориг файла для проверки правильности разархивации и общей целостности — 8 байт
3. Modified date ориг файла
4. Размер сжатого файла — 8 байт (long) - задает размер данных для разархивации
5. Код алгоритма которым был сжат файл
6. Количество блоков сжатого файла (для неблочных алгоритмов записывается ноль)
7. Размер строки имени файла
8. Строка имени файла который находится в архиве. На каждый символ используется 2 байта при записи.
*/
class FileRecord
{
public:
	// **** Нужно внести изменения в HashMap когда добавляются новые поля сюда!! *********
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
		uint8_t alg_mo = alg | (modelOrder << MODEL_SHIFT); // Старшие 4 бита отведены под номер model order. младшие 4 бита это номер алгоритма.

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

		modelOrder = alg >> MODEL_SHIFT; // выделяем из байта model type
		alg = alg & CODER_MASK; // чистим номер алгоритма от битов model type

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
