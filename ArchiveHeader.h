#pragma once

#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <cassert>
#include <vector>
#include "Parameters.h"
#include "Exceptions.h"
#include "FileRecord.h"


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
	//inline static log4cpp::Category& logger = log4cpp::Category::getInstance(ParametersG::LOGGER_NAME);
	
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

		//myassert(files.size() > 0);
	}

public:
	void listContent(std::string arcFilename, bool verbose);

	bool CheckSignature(std::string ArchiveName)
	{
		std::ifstream fin(ArchiveName, std::ios::in | std::ios::binary);
		if (!fin) return false; // return false even in case we cannot open file, either file does not exist or we do not have permissions

		char signature[4];
		char version[2];

		fin.read(signature, 4);
		fin.read(version, 2);

		fin.close();

		return signature[0] == 'R' && signature[1] == 'O' &&
	    	   signature[2] == 'M' && signature[3] == 'A' &&
			   version[0] >= '0'   && version[0] <= '9'   &&
			   version[1] >= '0'   && version[1] <= '9';

	}

	bool FileInList(std::string FileName)
	{
		for (auto item: files)
			if (item.fileName == FileName)
				return true;
		return false;
	}

	void RemoveFileFromList(std::string FileName)
	{
		for (vector_fr_t::iterator iter = files.begin(); iter != files.end(); iter++)
		{
			if (iter->fileName == FileName)
			{
				files.erase(iter);
				break;
			}
		}
	}

	void RemoveFilesFromList(vector_string_t& FileList)
	{
		for (vector_fr_t::iterator iter = files.begin(); iter != files.end(); )
		{
			auto result = std::find(FileList.begin(), FileList.end(), iter->fileName);

			if (result != FileList.end())
				iter = files.erase(iter);
			else 
				iter++;
		}
	}

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
	 * Adds filenames into vector of FileRecord together with file lengths and modified attributes
	 * @param filenames list of files (as strings) to compress.
	 */
	vector_fr_t& fillFileRecs(const vector_string_t& filenames, Parameters params)
	{
		files.clear(); 

		for (int i = 0; i < filenames.size(); i++) // first item in a list is archive file name, bypass it.
		{		
			if (std::filesystem::exists(filenames[i]))
			{
				auto ph = std::filesystem::path(filenames[i]);
				
				FileRecord fr;
				fr.dirName = ph.string(); //fl.getAbsolutePath(); // найти как возвращать только директорию. здесь сейчас возвращается весь путь с файлом вместо директории.
				fr.origFilename = filenames[i];
				fr.fileName = ph.filename().string(); //fl.getName();//filenames[i]; // store name of the file without path
				fr.fileSize = std::filesystem::file_size(filenames[i]);
				fr.modifiedDate = std::filesystem::last_write_time(ph).time_since_epoch().count(); 
				fr.alg = (uint8_t)params.CODER_TYPE; 
				fr.modelOrder = (uint8_t)params.MODEL_TYPE;
				fr.blockCount = 0;
				fr.blockSize = params.BLOCK_SIZE;
				fr.compressedSize = 0;
				fr.CRC32Value = 0xFFFFF; // TODO change it

				files.push_back(fr);
			}
			else
			{
				std::string mess = std::format("Cannot read file '%s'.", filenames[i]);
				throw std::invalid_argument(mess);
			//	logger.warn("File '%s' cannot be found, pass on it.", filenames[i].c_str());
			}
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
			//fr.CRC32Value = 0xFFFFF; // TODO remove it later
			raf.write((char*)&fr.CRC32Value, sizeof(uint64_t));

			raf.seekp(pos + CompressedSizeOffset, std::ios::beg);
			raf.write((char*)&fr.compressedSize, sizeof(uint64_t));

			raf.seekp(pos + BlockCountOffset , std::ios_base::beg);
			raf.write((char*)&fr.blockCount, sizeof(uint32_t));

			pos = pos + FileRecSize + (uint32_t)fr.fileName.length(); // length()*2 because writeChars() saves each char as 2 bytes
		}

		raf.close();
	}

	std::string truncate(std::string str, int len)
	{
		return (str.length() > len) ? str.substr(0, len - 3) + "..." : str;
	}


};

