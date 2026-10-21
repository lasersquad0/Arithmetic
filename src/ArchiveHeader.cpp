
#include "CommonFunctions.h"
#include "ArchiveHeader.h"
#include "LogEngine2/LogEngine.h"

#define MAX_FN_COLW 46
#define MIN_FN_COLW 10

void ArchiveHeader::ListContent(const string_t& arcFilename, bool verbose)
{
	std::ifstream fin;
	fin.open(arcFilename, std::ios::in | std::ios::binary);
	if (fin.fail())
		throw file_error("Cannot open file '" + wtos(arcFilename) + "' for reading.");

	LoadHeader(&fin);

	GET_LOGGER();

	size_t maxFileName = MIN_FN_COLW;

	//TODO probably add calc of width of more columns here
	for (int i = 0; i < m_files.size(); i++)
	{
		FileRecord& fr = m_files[i];
		if (maxFileName < fr.fileName.size()) maxFileName = fr.fileName.size();
	}

	if (maxFileName > MAX_FN_COLW) maxFileName = MAX_FN_COLW;

	logger.InfoFmt("{:<{}}{:>18}{:>15}{:>8}{:>11}{:>6}{:>7}{:>7}{:>20}{:>13}", 
		"File name", maxFileName, "File size", "Compressed", "Blocks", "Block size", "Alg", "Model", "Ratio", "Modified", "CRC32");

	for (int i = 0; i < m_files.size(); i++)
	{
		FileRecord fr = m_files[i];

		float ratio = (float)fr.fileSize / (float)fr.compressedSize;
		std::string fileModified = DateTimeToStringA(fr.GetModifiedDateAsTimeT());
		std::string algName = wtos(Parameters::CoderNames[fr.alg]); // fr.alg here does not contain model order already
		std::string modelName = wtos(Parameters::ModelTypeCode[fr.modelOrder]);
		std::string fileNameA = wtos(ellipsis(fr.fileName, MAX_FN_COLW));

		logger.InfoFmt("{:<{}}{:>18L}{:>15L}{:>8L}{:>11L}{:>6}{:>7}{:7.2f}{:>20}{:>13}",
			fileNameA, maxFileName, fr.fileSize, fr.compressedSize, fr.blockCount,
			fr.blockSize, algName, modelName, ratio, fileModified, fr.CRC32Value);
	}

	if (verbose)
	{
		for (int i = 0; i < m_files.size(); i++)
		{
			FileRecord fr = m_files[i];

			logger.InfoFmt("\n---------- List of blocks for '{}' ----------\n", wtos(fr.fileName));
			logger.InfoFmt("{:<4} {:>10} {:>12} {:>7} {:>13} {:>7}\n", "#", "Compressed", "Uncompressed", "Ratio", "BWT Line", "Flags");

			float ratio;
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

				//fin.ignore(cBlockSize);
				fin.seekg(cBlockSize, std::ios::cur);
				ratio = (float)uBlockSize / cBlockSize;

				logger.InfoFmt("{:<4} {:>10} {:>12} {:>7.2f} {:>13} {:>7}",	j, cBlockSize, uBlockSize, ratio, bwtLineNum, (uint32_t)bflags);
			}
		}
		logger.Info(""); // just CRLF
	}
	fin.close();
}

/**
 * Adds filenames into vector of FileRecord together with file lengths and modified attributes
 * @param filenames list of files (as strings) to compress.
 */
vect_fr_t& ArchiveHeader::FillFileRecs(const vect_string_t& filenames, const Parameters& params)
{
	m_files.clear();

	for (int i = 0; i < filenames.size(); i++) 
	{
		if (std::filesystem::exists(filenames[i]))
		{
			auto ph = std::filesystem::path(filenames[i]);

			FileRecord fr;
		   //	fr.dirName = ph.wstring(); //fl.getAbsolutePath(); //TODO find a way how to return a directory only. Now it returns full path with filename

			fr.origFilename = convert_string<wchar_t>(filenames[i]);
			fr.fileName = ph.filename().wstring(); //fl.getName();//filenames[i]; // store name of the file without path
			fr.fileSize = std::filesystem::file_size(filenames[i]);
			fr.modifiedDate = std::filesystem::last_write_time(ph).time_since_epoch().count();
			fr.alg = (uint8_t)params.CODER_TYPE;
			fr.modelOrder = (uint8_t)params.MODEL_TYPE;
			fr.blockCount = 0;
			fr.blockSize = params.BLOCK_SIZE;
			fr.compressedSize = 0;
			fr.CRC32Value = 0xFFFFF; //TODO change it

			m_files.push_back(fr);
		}
		else
		{
			std::string mess = std::format("Cannot read file '{}'. Check if file exists.", wtos(filenames[i]));
			throw std::invalid_argument(mess);
			//	logger.warn("File '%s' cannot be found, pass on it.", filenames[i].c_str());
		}
	}
	if (m_files.size() == 0)
		throw std::invalid_argument("There are no files to compress. Exiting...");

	return m_files;
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
		throw file_error("Cannot open file '" + wtos(arcFilename) + "' for writing.");

	uint32_t InitialOffset = FILE_SIGNATURE_LEN + FILE_VERSION_LEN + sizeof(uint16_t); // start of files table in an archive
	constexpr uint32_t CRC32ValueOffset = sizeof(uint64_t);
	constexpr uint32_t CompressedSizeOffset = 3 * sizeof(uint64_t); // filesize + crc32 + modifieddate
	constexpr uint32_t BlockCountOffset = 4 * sizeof(uint64_t) + sizeof(uint8_t); // uint8_t is for alg_mo
	constexpr uint32_t FileRecSize = 4 * sizeof(uint64_t) + 2 * sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);  // uint16_t - this is for saving filename length

	uint32_t pos = InitialOffset;

	for (FileRecord fr : m_files)
	{
		raf.seekp(pos + CRC32ValueOffset);
		//fr.CRC32Value = 0xFFFFF; //TODO remove it later
		raf.write((char*)&fr.CRC32Value, sizeof(uint64_t));

		raf.seekp(pos + CompressedSizeOffset, std::ios::beg);
		raf.write((char*)&fr.compressedSize, sizeof(uint64_t));

		raf.seekp(pos + BlockCountOffset, std::ios_base::beg);
		raf.write((char*)&fr.blockCount, sizeof(uint32_t));

		pos = pos + FileRecSize + (uint32_t)fr.fileName.length()*sizeof(char_t); // length()*2 because writeChars() saves each char as 2 bytes
	}

	raf.close();
}
