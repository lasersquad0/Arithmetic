#include "CommonFunctions.h"
#include "ArchiveHeader.h"

void ArchiveHeader::listContent(std::string arcFilename, bool verbose)
{
	std::ifstream fin(arcFilename, std::ios::in | std::ios::binary);
	if (fin.fail())
		throw file_error("Cannot open file '" + arcFilename + "' for reading.");

	loadHeader(&fin);

	printf("%-46s %18s %15s %6s %10s %6s %7s %7s %-19s %13s\n", "File name", "File size", "Compressed", 
		"Blocks", "Block size", "Alg", "Model", "Ratio", "Modified", "CRC32");

	for (int i = 0; i < files.size(); i++)
	{
		FileRecord fr = files[i];
		
		std::string fileModified = DateTimeToString(fr.GetModifiedDateAsTimeT());

		std::string algName = Parameters::CoderNames[fr.alg]; // fr.alg here does not contain model order already 
		std::string modelName = Parameters::ModelTypeCode[fr.modelOrder];

		float ratio = (float)fr.fileSize / (float)fr.compressedSize;
		printf("%-46s %18s %15s %6s %10s %6s %6s %7.2f  %19s %13llu\n", 
			truncate(fr.fileName, 46).c_str(), toStringSep(fr.fileSize).c_str(), toStringSep(fr.compressedSize).c_str(), toStringSep(fr.blockCount).c_str(),
			toStringSep(fr.blockSize).c_str(), algName.c_str(), modelName.c_str(), ratio, fileModified.c_str(), fr.CRC32Value);
	}

	if (verbose)
	{
		for (int i = 0; i < files.size(); i++)
		{
			FileRecord fr = files[i];
			printf("\n---------- List of blocks for '%s' ----------\n", fr.fileName.c_str());
			printf("%-4s %10s %12s %7s %13s %7s\n", "#", "Compressed", "Uncompressed", "Ratio", "BWT Line", "Flags");

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
				printf("%-4u %10s %12s %7.2f %13s %7u\n", 
					j, toStringSep(cBlockSize).c_str(), toStringSep(uBlockSize).c_str(), ratio, toStringSep(bwtLineNum).c_str(), bflags);
			}
		}
		printf("\n");
	}

	fin.close();
}