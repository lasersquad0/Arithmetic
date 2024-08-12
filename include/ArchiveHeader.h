#pragma once

#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <cassert>
#include <vector>
#include "Parameters.h"
#include "ARIExceptions.h"
#include "FileRecord.h"


#define myassert(_) if(!(_)) throw bad_file_format("Format of archive is incorrect.");

typedef std::vector<FileRecord> vect_fr_t;
typedef std::vector<string_t> vect_string_t;

class ArchiveHeader
{
	/*
		Common structure of Archive file
	1. Signature of the archive file — 4 bytes "ROMA"
	2. Version of file format — 2 bytes - '00', '01'
	3. Number of files in archive. 2 bytes, up to 65535 files in archive. Needed for proper reading list of files from archive
	4. Table with file names, file sizes and other file fields
	5. Size of table of Huffman codes for first file if required (2 bytes)
	6. Table of Huffman codes  — 6 * (number of codes) bytes [triples: symbol (byte) — code(int) - length(byte)]
	7. Compressed data, size of compressed data for each file is stored in Table of files (see above)
	items 6 and 7 repeat for each file.
	*/

private:
	//inline static log4cpp::Category& logger = log4cpp::Category::getInstance(ParametersG::LOGGER_NAME);

	const uint32_t FILE_SIGNATURE_LEN = 4;
	const uint32_t FILE_VERSION_LEN = 2;

	char fileSignature[4] = {'R', 'O', 'M', 'A'}; // TODO: shall it be wchar type?
	char fileVersion[2] = {'0', '1'};
	vect_fr_t files;

	void checkHeaderData()
	{
		myassert(fileSignature[0] == 'R');
		myassert(fileSignature[1] == 'O');
		myassert(fileSignature[2] == 'M');
		myassert(fileSignature[3] == 'A');

		myassert((fileVersion[0] >= '0') && (fileVersion[0] <= '9'));
		myassert((fileVersion[1] >= '0') && (fileVersion[1] <= '9'));
	}

public:
	void listContent(const string_t& arcFilename, bool verbose);

	bool CheckSignature(const string_t& ArchiveName)
	{
		std::ifstream fin(ArchiveName, std::ios::in | std::ios::binary);
		if (fin.fail()) return false; // return false even in case we cannot open file, either file does not exist or we do not have permissions

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

	bool FileInList(const string_t& FileName)
	{
		for (auto item: files)
			if (item.fileName == FileName)
				return true;
		return false;
	}

	void RemoveFileFromList(const string_t& FileName)
	{
		for (vect_fr_t::iterator iter = files.begin(); iter != files.end(); iter++)
		{
			if (iter->fileName == FileName)
			{
				files.erase(iter);
				break;
			}
		}
	}

	void RemoveFilesFromList(const vect_string_t& FileList)
	{
		for (vect_fr_t::iterator iter = files.begin(); iter != files.end(); )
		{
			auto result = std::find(FileList.begin(), FileList.end(), iter->fileName);

			if (result != FileList.end())
				iter = files.erase(iter);
			else
				iter++;
		}
	}

	vect_fr_t& loadHeader(std::ifstream* sin)
	{
		sin->read(fileSignature, 4);
		sin->read(fileVersion, 2);

		checkHeaderData();

		if (sin->fail())
			throw bad_file_format("Wrong file format.");

		files.clear();

		uint16_t filesCount = 0;
		sin->read((char*)&filesCount, sizeof(uint16_t)); // up to 65535 files in archive

		for (uint16_t i = 0; i < filesCount; i++)
		{
			FileRecord fr;
			fr.load(sin);

			files.push_back(fr);
		}

		return files;
	}

	/**
	 * Saves header data into output stream (usually it is a file). Header data includes: signature (4 bytes), archive format version (2 bytes), 
	 * list of file names in archive together with other fields (modified date, file size, CRC, etc.) 
	 * @param sout stream to write header data to (usually file stream).
	 */
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
	 * Adds filenames into vector of FileRecord together with file lengths and modified datetime attributes
	 * @param filenames list of files (as strings).
	 */
	vect_fr_t& fillFileRecs(const vect_string_t& filenames, Parameters& params);

	/**
	 * Update existing archive with information about sizes of compressed files in it (in bytes)
	 * @param arcFilename Name of the archive
	 * @throws IOException if something goes wrong
	 */
	void updateHeaders(const string_t& arcFilename);

	
	string_t ellipsis(const string_t& str, uint len)
	{
		return (str.length() > len) ? str.substr(0, len - 3) + _T("...") : str;
	}


};


