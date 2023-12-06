#pragma once

#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <cassert>
#include <vector>
#include "Parameters.h"
#include "Exceptions.h"

//using namespace std;

#define ALG_MASK     (0x0F)
#define MORDER_MASK  (0xF0)
#define MORDER_SHIFT (4)

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
		uint8_t alg_mo = alg | (modelOrder << MORDER_SHIFT); // Старшие 4 бита отведены под номер model order. младшие 4 бита это номер алгоритма.
		
		sout->write((char*)&fileSize,       sizeof(uint64_t));
		sout->write((char*)&CRC32Value,     sizeof(uint64_t));
		sout->write((char*)&modifiedDate,   sizeof(uint64_t));
		sout->write((char*)&compressedSize, sizeof(uint64_t));
		sout->write((char*)&alg_mo,         sizeof(uint8_t));
		sout->write((char*)&blockCount,     sizeof(uint32_t));
		sout->write((char*)&blockSize,      sizeof(uint32_t));

		uint64_t len = fileName.length();
		sout->write((char*)&len, sizeof(uint16_t));
		sout->write(fileName.c_str(), fileName.length());  // NOTE! writes 2 bytes for each char
	}

	void load(std::ifstream* sin)
	{	
		sin->read((char*)&fileSize,       sizeof(uint64_t));
		sin->read((char*)&CRC32Value,     sizeof(uint64_t));
		sin->read((char*)&modifiedDate,   sizeof(uint64_t));
		sin->read((char*)&compressedSize, sizeof(uint64_t));
		sin->read((char*)&alg,            sizeof(uint8_t));
		sin->read((char*)&blockCount,     sizeof(uint32_t));
		sin->read((char*)&blockSize,      sizeof(uint32_t));

		modelOrder = alg >> MORDER_SHIFT; // выделяем из байта model order
		alg = alg & ALG_MASK; // чистим номер алгоритма от битов model order

		uint16_t filenameSize;
		sin->read((char*)&filenameSize, sizeof(uint16_t));

		char buf[MAX_PATH];
		sin->read((char*)buf, filenameSize);
		fileName.append(buf, filenameSize);
	}
};

#define myassert(_) if(!(_)) throw bad_file_format("Format of archive is incorrect.");

typedef std::vector<FileRecord> vector_fr_t;
typedef std::vector<std::string> vector_string_t;

class ArchiveHeader
{
/*
	Общая структура файла архива
1. Сигнатура типа файла — 4 байта "ROMA"
2. Версия формата файла — 2 байта - '00', '01'
3. Кол-во файлов в архиве. Для последующего корректного чтения Списка файлов.
4. Таблица с именами файлов, размерами и остальными полями.
5. Размер таблицы кодов для первого файла — 2 байта (если требуется)
6. Таблица кодов — 6 * (количество знаков) байт [тройки: символ(byte) — код(int) - длина(byte)]
7. Сжатые данные — кол-во сжатых байт сохраняется в таблице файлов
пункты 6 и 7 повторяются для всех файлов.
*/
private:
	inline static log4cpp::Category& logger = log4cpp::Category::getInstance(Parameters::LOGGER_NAME);
	
	const uint32_t FILE_SIGNATURE_LEN = 4;
	const uint32_t FILE_VERSION_LEN = 2;
	
	char fileSignature[4] = {'R', 'O', 'M', 'A'};
	char fileVersion[2] = {'0', '1'};
	vector_fr_t files;

	void checkHeaderData()
	{
		myassert(fileSignature[0] == 'R');
		myassert(fileSignature[1] == 'O');
		myassert(fileSignature[2] == 'M');
		myassert(fileSignature[3] == 'A');

		myassert((fileVersion[0] >= '0') && (fileVersion[0] <= '9'));
		myassert((fileVersion[1] >= '0') && (fileVersion[1] <= '9'));

		myassert(files.size() > 0);
	}

public:
	vector_fr_t& loadHeader(std::ifstream* sin)
	{
		sin->read(fileSignature, 4);
		sin->read(fileVersion, 2);
		
		if (sin->fail())
			throw bad_file_format("Wrong file format.");

		files.clear();

		uint16_t filesCount = 0;
		sin->read((char*)&filesCount, sizeof(uint16_t));
		
		for (uint16_t i = 0; i < filesCount; i++)
		{
			FileRecord fr;
			fr.load(sin);

			files.push_back(fr);
		}

		checkHeaderData();
		return files;
	}

	void saveHeader(std::ofstream* sout)
	{
		checkHeaderData();

		sout->write(fileSignature, 4);
		sout->write(fileVersion, 2);

		uint16_t sz = (uint16_t)files.size();
		sout->write((char*)&sz, sizeof(uint16_t)); // assumed archive will have less than 65535 files in it
		
		for (FileRecord fr : files)
		{
			fr.save(sout);
		}
	}

	/**
	 * Adds filenames into ArrayList of HFFileRec together with file lengths and modified attributes
	 * @param filenames list of files to compress. Note, that zero index in this array contains archive name, so first filename is filenames[1]
	 */
	vector_fr_t& fillFileRecs(const vector_string_t& filenames /*, Utils.CompTypes alg*/)
	{
		files.clear();  // just in case

		for (int i = 1; i < filenames.size(); i++) // first item in a list is archive file name, bypass it.
		{		
			if (std::filesystem::exists(filenames[i]))
			{
				auto ph = std::filesystem::path(filenames[i]);
				
				FileRecord fr;
				fr.dirName = ph.string(); //fl.getAbsolutePath(); // найти как возвращать только директорию. здесь сейчас возвращается весь путь с файлом вместо директории.
				fr.origFilename = filenames[i];
				fr.fileName = ph.filename().string(); //fl.getName();//filenames[i]; // store name of the file without path
				fr.fileSize = std::filesystem::file_size(filenames[i]);
				fr.modifiedDate = std::filesystem::last_write_time(ph).time_since_epoch().count(); // TODO fl.lastModified(); //another way to do the same is Files.getLastModifiedTime()
				fr.alg = Parameters::COMPRESSION_ALG; 
				fr.modelOrder = Parameters::MODEL_ORDER;
				fr.blockCount = 0;
				fr.blockSize = Parameters::BLOCK_SIZE;
				fr.compressedSize = 0;
				//fr.CRC32Value

				files.push_back(fr);
			}
			else
				logger.warn("File '%s' cannot be found, pass on it.", filenames[i].c_str());
		}
		if (files.size() == 0)
			throw std::invalid_argument("There are no files to compress. Exiting...");
		
		return files;
	}

	/**
	 * Записываем в уже сформированный архив размер закодированных (сжатых) потоков в байтах для каждого файла в архиве
	 * @param arcFilename Name of the archive
	 * @throws IOException if something goes wrong
	 */
	void updateHeaders(std::string arcFilename)
	{
		std::fstream raf;
		//raf.exceptions(ios_base::failbit | ios_base::badbit);
		raf.open(arcFilename, std::ios::in | std::ios::out | std::ios::binary);
		if(!raf)
			throw file_error("Cannot open file '" + arcFilename + "' for writing.");
		
		uint32_t InitialOffset = FILE_SIGNATURE_LEN + FILE_VERSION_LEN + sizeof(uint16_t); // start of files table in an archive
		constexpr uint32_t CRC32ValueOffset = sizeof(uint64_t);
		constexpr uint32_t CompressedSizeOffset = 3 * sizeof(uint64_t);
		constexpr uint32_t BlockCountOffset = 4 * sizeof(uint64_t) + sizeof(uint8_t);
		constexpr uint32_t FileRecSize = 4 * sizeof(uint64_t) + 2 * sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);  // Short.BYTES - this is for saving filename length

		uint32_t pos = InitialOffset;

		for (FileRecord fr : files)
		{
			raf.seekp(pos + CRC32ValueOffset);
			fr.CRC32Value = 0xFFFFFF; // TODO remove it later
			raf.write((char*)&fr.CRC32Value, sizeof(uint64_t));

			raf.seekp(pos + CompressedSizeOffset, std::ios::beg);
			raf.write((char*)&fr.compressedSize, sizeof(uint64_t));

			raf.seekp(pos + BlockCountOffset , std::ios_base::beg);
			raf.write((char*)&fr.blockCount, sizeof(uint32_t));

			pos = pos + FileRecSize + (uint32_t)fr.fileName.length(); // length()*2 because writeChars() saves each char as 2 bytes
		}

		raf.close();
	}

	void listContent(std::string arcFilename);

	std::string truncate(std::string str, int len)
	{
		return (str.length() > len) ? str.substr(0, len - 3) + "..." : str;
	}


};

