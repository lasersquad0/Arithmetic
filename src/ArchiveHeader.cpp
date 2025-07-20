
#include "CommonFunctions.h"
#include "ArchiveHeader.h"
#include "LogEngine.h"

void ArchiveHeader::ListContent(const string_t& arcFilename, bool verbose)
{
	std::ifstream fin(arcFilename, std::ios::in | std::ios::binary);
	if (fin.fail())
		throw file_error("Cannot open file '" + convert_string<char>(arcFilename) + "' for reading.");

	LoadHeader(&fin);

#if defined(LOG4CPP) || defined(__BORLANDC__)
	printf("%-46s %18s %15s %6s %10s %6s %7s %7s %-19s %13s\n", "File name", "File size", "Compressed",
		"Blocks", "Block size", "Alg", "Model", "Ratio", "Modified", "CRC32");
#else
	LogEngine::Logger& logger = Global::GetLogger();
	logger.InfoFmt("{:<46} {:>18} {:>15} {:>6} {:>10} {:>6} {:>7} {:>7} {:<19} {:>13}", "File name", "File size", "Compressed",
		"Blocks", "Block size", "Alg", "Model", "Ratio", "Modified", "CRC32");
#endif

	for (int i = 0; i < files.size(); i++)
	{
		FileRecord fr = files[i];

		std::string fileModified = DateTimeToStringA(fr.GetModifiedDateAsTimeT());
		std::string algName = convert_string<char>(Parameters::CoderNames[fr.alg]); // fr.alg here does not contain model order already
		std::string modelName = convert_string<char>(Parameters::ModelTypeCode[fr.modelOrder]);

		float ratio = (float)fr.fileSize / (float)fr.compressedSize;
#if defined(LOG4CPP) || defined(__BORLANDC__)
		printf("%-46ls %18ls %15ls %6ls %10ls %6s %6s %7.2f  %19s %13llu\n",
			ellipsis(fr.fileName, 46).c_str(), toStringSep(fr.fileSize).c_str(), toStringSep(fr.compressedSize).c_str(), toStringSep(fr.blockCount).c_str(),
			toStringSep(fr.blockSize).c_str(), algName.c_str(), modelName.c_str(), ratio, fileModified.c_str(), fr.CRC32Value);
#else
		logger.InfoFmt("{:<46} {:>18} {:>15} {:>6} {:>10} {:>6} {:>6} {:7.2f}  {:>19} {:>13}",
			convert_string<char>(ellipsis(fr.fileName, 46)), toStringSepA(fr.fileSize), toStringSepA(fr.compressedSize), toStringSepA(fr.blockCount),
			toStringSepA(fr.blockSize), algName, modelName, ratio, fileModified, fr.CRC32Value);
#endif
	}

	if (verbose)
	{
		for (int i = 0; i < files.size(); i++)
		{
			FileRecord fr = files[i];
#if defined(LOG4CPP) || defined(__BORLANDC__)
		 	printf("\n---------- List of blocks for '%ls' ----------\n", fr.fileName.c_str());
		 	printf("%-4s %10s %12s %7s %13s %7s\n", "#", "Compressed", "Uncompressed", "Ratio", "BWT Line", "Flags");
#else
			logger.InfoFmt("\n---------- List of blocks for '{}' ----------\n", convert_string<char>(fr.fileName));
			logger.InfoFmt("{:<4} {:>10} {:>12} {:>7} {:>13} {:>7}\n", "#", "Compressed", "Uncompressed", "Ratio", "BWT Line", "Flags");
#endif
			for (uint32_t j = 0; j < fr.blockCount; j++)
			{
				uint32_t cBlockSize;
				uint32_t uBlockSize;
				uint32_t bwtLineNum;
				fin.read((char*)&cBlockSize, sizeof(uint32_t));
				fin.read((char*)&uBlockSize, sizeof(uint32_t));
				fin.read((char*)&bwtLineNum, sizeof(uint32_t));

				std::byte bflags = (std::byte)fin.get();

				assert(cBlockSize > 0);
				assert(uBlockSize > 0);

				fin.ignore(cBlockSize);
				float ratio = (float)uBlockSize / (float)cBlockSize;
#if defined(LOG4CPP) || defined(__BORLANDC__)
				printf("%-4u %10ls %12ls %7.2f %13ls %7u\n",
					j, toStringSep(cBlockSize).c_str(), toStringSep(uBlockSize).c_str(), ratio, toStringSep(bwtLineNum).c_str(), (uint32_t)bflags);
			}
		}
        printf("\n");
#else
				logger.InfoFmt("{:<4} {:>10} {:>12} {:>7.2f} {:>13} {:>7}",
						j, toStringSepA(cBlockSize), toStringSepA(uBlockSize), ratio, toStringSepA(bwtLineNum), (uint32_t)bflags);
			}
		}
		logger.Info(""); // just CRLF
#endif
	}
	fin.close();
}

/**
 * Adds filenames into vector of FileRecord together with file lengths and modified attributes
 * @param filenames list of files (as strings) to compress.
 */
vect_fr_t& ArchiveHeader::FillFileRecs(const vect_string_t& filenames, const Parameters& params)
{
	files.clear();

	for (int i = 0; i < filenames.size(); i++) 
	{
		if (std::filesystem::exists(filenames[i]))
		{
			auto ph = std::filesystem::path(filenames[i]);

			FileRecord fr;
		   //	fr.dirName = ph.wstring(); //fl.getAbsolutePath(); // TODO find a way how to return a directory only. Now it returns full path with filename

			fr.origFilename = filenames[i];
			fr.fileName = ph.filename().wstring(); //fl.getName();//filenames[i]; // store name of the file without path
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
#if defined (__BORLANDC__)
			std::string mess = "Cannot read file '" + convert_string<char>(filenames[i]) + "'.";
#else
			std::string mess = std::format("Cannot read file '{}'.", convert_string<char>(filenames[i]));
#endif
			throw std::invalid_argument(mess);
			//	logger.warn("File '%s' cannot be found, pass on it.", filenames[i].c_str());
		}
	}
	if (files.size() == 0)
		throw std::invalid_argument("There are no files to compress. Exiting...");

	return files;
}


/**
 * Update existing archive with information about sizes of compressed files in it (in bytes)
 * @param arcFilename Name of the archive
 * @throws IOException if something goes wrong
 */
void ArchiveHeader::UpdateHeaders(const string_t& arcFilename)
{
	std::fstream raf;
	//raf.exceptions(ios_base::failbit | ios_base::badbit);
	raf.open(arcFilename, std::ios::in | std::ios::out | std::ios::binary);
	if (raf.fail())
		throw file_error("Cannot open file '" + convert_string<char>(arcFilename) + "' for writing.");

	uint32_t InitialOffset = FILE_SIGNATURE_LEN + FILE_VERSION_LEN + sizeof(uint16_t); // start of files table in an archive
	constexpr uint32_t CRC32ValueOffset = sizeof(uint64_t);
	constexpr uint32_t CompressedSizeOffset = 3 * sizeof(uint64_t); // filesize + crc32 + modifieddate
	constexpr uint32_t BlockCountOffset = 4 * sizeof(uint64_t) + sizeof(uint8_t); // uint8_t is for alg_mo
	constexpr uint32_t FileRecSize = 4 * sizeof(uint64_t) + 2 * sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);  // uint16_t - this is for saving filename length

	uint32_t pos = InitialOffset;

	for (FileRecord fr : files)
	{
		raf.seekp(pos + CRC32ValueOffset);
		//fr.CRC32Value = 0xFFFFF; // TODO remove it later
		raf.write((char*)&fr.CRC32Value, sizeof(uint64_t));

		raf.seekp(pos + CompressedSizeOffset, std::ios::beg);
		raf.write((char*)&fr.compressedSize, sizeof(uint64_t));

		raf.seekp(pos + BlockCountOffset, std::ios_base::beg);
		raf.write((char*)&fr.blockCount, sizeof(uint32_t));

		pos = pos + FileRecSize + (uint32_t)fr.fileName.length()*sizeof(char_t); // length()*2 because writeChars() saves each char as 2 bytes
	}

	raf.close();
}
