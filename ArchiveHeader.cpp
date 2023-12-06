#include "Functions.h"
#include "ArchiveHeader.h"

std::string CompAlgSymbols[] = { "NONE", "HUF", "AHUF", "RLE", "ARI", "ARI32", "ARI64", "AARI", "AARI32", "AARI64", "BITARI" };


void ArchiveHeader::listContent(std::string arcFilename)
{
	std::ifstream fin(arcFilename, std::ios::in | std::ios::binary);
	if (!fin)
		throw file_error("Cannot open file '" + arcFilename + "' for reading.");

	loadHeader(&fin);

	printf("%-46s %18s %15s %7s %10s %6s %7s %7s %-18s %13s\n", "File name", "File size", "Compressed", "Blocks", "Block size", "Alg", 
		"M Order", "Ratio", "Modified", "CRC32");

	for (int i = 0; i < files.size(); i++)
	{
		FileRecord fr = files[i];
		
		std::chrono::file_clock::time_point dt{ std::chrono::file_clock::duration{ fr.modifiedDate } };
		auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(dt -std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		time_t tt = std::chrono::system_clock::to_time_t(sctp);
		std::string fileModified = DateTimeToStr(tt);

		std::string algName = CompAlgSymbols[fr.alg]; // fr.alg здесь уже очищен от model order

		float ratio = (float)fr.fileSize / (float)fr.compressedSize;
		printf("%-46s %18s %15s %7s %10s %6s %6u %7.2f %18s %13llu\n", 
			truncate(fr.fileName, 46).c_str(), toStringSep(fr.fileSize).c_str(), toStringSep(fr.compressedSize).c_str(), toStringSep(fr.blockCount).c_str(),
			toStringSep(fr.blockSize).c_str(), algName.c_str(), fr.modelOrder, ratio, fileModified.c_str(), fr.CRC32Value);
	}

	if (Parameters::VERBOSE)
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