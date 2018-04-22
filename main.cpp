#define IS_DEBUG_APPLICATION false

// Archiver and Splitter
// main.cpp
// Created by jaustg

// Created 6/1/2014
// Last modified 6/4/2016

////////////////
//   INCLUDE
////////////////

#include <iostream>

//Finding files in directories
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

//Strings
#include <string>

//Vectors
#include <vector>

//Stringstream
#include <sstream>
//File stream
#include <fstream>

//Working directory
#include <direct.h>

//Sorting
#include <algorithm>

//Making directories
#include <ShlObj.h>

//Finding the directory of a file
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib")

////////////////
//   STRUCTS
////////////////

//Contains the data for one file
struct FileInformationPiece {
	std::string fileName;
	//long long = __int64
	long long fileSize;
};

//Contains a list of FileInformationPieces
struct FileInformation {
	/*std::vector<std::string> fileNames;
	std::vector<long long> fileSizes;*/
	std::vector<FileInformationPiece> files;
};

////////////////
//   CONSTANTS
////////////////

#define ARCHIVE_FILE_TYPE_7Z 0
#define ARCHIVE_FILE_TYPE_ZIP 1

////////////////
//   FUNCTIONS
////////////////
void FileFindAllFilesInAllSubdirectories(std::string directory,
										 FileInformation &fileInfo);
bool FileExists(std::string filename);
int workingDirectorySet(std::string dir);
std::string workingDirectoryGet();
std::string getFileOpenDialog(char* nullSeperatedFilter, char* initialDirectory);
bool compareFileInformationPiece(const FileInformationPiece &a,
								 const FileInformationPiece &b);
void stringReplaceAll(std::string &s, const std::string &search, const std::string &replace);
std::string itos(int i);
std::string getFormattedSizeTitle(long long size);
std::string getTempDirectory();
int filenameGetProperty(std::string filename, std::string &propertyBuffer, unsigned int propertyValue);
void clearTempDirectory(std::string tempDirectory);
std::string dtos(double i);
double floorDoubleAt(double db, double roundTo);
void padWithZeroes(std::string &num, unsigned int minimumNumberLength);
std::string stringRemoveIncluding(std::string fullString, std::string beginningString);

////////////////
//   MAIN
////////////////

// PROGRAM ARGUMENTS
/*
	NOTE: A blank string is equivalent to the default (e.g. using "" in the command prompt).

	Arguments (in order):

	directory - The directory from which you would like to archive files (with final backslash).
	output directory - The output directory of the archive files (with final backslash).
	naming convention - The naming convention of the output files.  +ID_HERE+ is replaced with the file number.
	file type - "zip" or "7z" is acceptable, case-sensitive.
	password - The password to use.
	maximum file size - The maximum file size of an archive in bytes.
	compress files - "compression" to compress, "nocompression" not to compress the files
	arrange by size - If this is enabled, the program will try to arrange the files so that they fit snugly in
		an arrangement that keeps the final file size closest to the maximum file size.  The original file order
		is not preserved.  "arrange_default" and "arrange_fitsize" are accepted.
	start at - The archive number to start at.  All archives before this number are skipped.  This is useful if
		you do not have enough space to archive all the files at once.
	summary only - Only the summary file will be created (no archives produced) if this is "summary_only".
*/

int main(int argc, char *argv[]) {

	std::string sevenZipFile = "7za.exe";
	std::string summaryFilename = "summary.txt";
	std::string tempDirectoryName = "archiver_splitter";

	//If this option is enabled, files will be compressed (slower)
	//Otherwise, files will only be stored in the archive (faster)
	bool compressFiles = false;

	//If this option is enabled, the order of the files will remain the
	//same, so files will not be grouped together based on their file size
	//If this option is disabled, the order will try to fit the files
	//together nicely
	bool preserveFileOrder = true;

	//In the naming convention below, --> +ID_HERE+ <-- will be replaced with the ID
	//of the archive file.
	std::string namingConvention = "+ID_HERE+.7z";

	//The minimum number of digits for the ID to use
	//--Padding Amount--    --ID--    --Result--
	//        4               16         0016
	//        3               2          002
	//        1               16         16
	int idStringPaddingAmount = 4;

	//If this option is enabled, the program will make a summary file detailing
	//the files in each archive
	bool makeSummaryFile = true;

	//If the option is enabled, the program will only make the summary file
	bool onlyMakeSummaryFile = false;

	//If this option is 0, the summary will include only the names of the files.
	//If it is 1, the summary will also include approximated file sizes.
	int summaryDetailLevel = 1;

	//Make sure that this has no spaces
	std::string password = "";

	//Maximum archive file size = 1 GiB
	long long maxFileSize = 1024 * 1024 * 1024;

	//The directory that the application is in
	std::string applicationDirectory = workingDirectoryGet();

	if (!FileExists(sevenZipFile)) {
		std::cout << sevenZipFile << " could not be found.  Please locate"
			<< " the 7-Zip command-line executable." << std::endl;
		std::cin.get();
		sevenZipFile = getFileOpenDialog("Applications (*.exe)\0*.exe",NULL);
		if (!FileExists(sevenZipFile)) {
			std::cout << sevenZipFile << " could not be found."
				<< std::endl;
			std::cin.get();
			return 0;
		}
	}

	workingDirectorySet(applicationDirectory);

	std::string sevenZipFileFormatted = "\"" + sevenZipFile + "\"";

	//FileInformation object used to find files
	FileInformation fileInfo;

	//Find all the files in the given directory
	std::string directory = "";

	std::string output_directory = "";

	//Get the output file type
	int output_file_type = ARCHIVE_FILE_TYPE_7Z;

	//Archive number to start at (skipping the ones before it)
	int archiveToStartAt = 0;

	//Make sure the required number of arguments exists
	if (!IS_DEBUG_APPLICATION) {
		if (argc < 3) {
			std::cout << "Usage: " << argv[0] << " <input dir> <output_dir> [namingConvention]"
				<< " [file type] [password] [maxFileSize] [compressFiles] [arrangeFilesBySize] [start-at] [summaryOnly]"
				<< std::endl;
			std::cout << "Leave any argument blank (\"\") to use its default." << std::endl;
			std::cout << " - namingConvention: e.g. \"MyArchives_+ID_HERE+.7z\"" << std::endl;
			std::cout << " - file type: \"7z\" or \"zip\"" << std::endl;
			std::cout << " - password: plaintext password or \"\" for no password." << std::endl;
			std::cout << " - maxFileSize: the maximum total file size in bytes of the files used in each archive" << std::endl;
			std::cout << " - compressFiles: \"compression\" or \"nocompression\"" << std::endl;
			std::cout << " - arrangeFilesBySize: \"arrange_default\" (in order) or \"arrange_fitsize\" for the fewest number"
				<< " of archives created." << std::endl;
			std::cout << " - start-at: the archive number to start at (skipping the creation of previous ones), e.g. \"4\""
				<< " (or \"1\" to do a complete run through)." << std::endl;
			std::cout << " - summaryOnly: only the summary file will be created (no archives produced) if this is \"summary_only\"."
				<< std::endl;
			return 0;
		}
	}

	//Parse arguments
	for (int i = 1; i < argc; i ++) {

		if (std::string(argv[i]) == "") {
			continue;
		}

		switch (i) {
			//Input directory
		case 1:
			{
				directory = argv[i];
				break;
			}
			//Output directory
		case 2:
			{
				output_directory = argv[i];
				break;
			}
		case 3:
			{
				namingConvention = argv[i];
				break;
			}
		case 4:
			{
				if (std::string(argv[i]) == "7z") {
					output_file_type = ARCHIVE_FILE_TYPE_7Z;
				} else if (std::string(argv[i]) == "zip") {
					output_file_type = ARCHIVE_FILE_TYPE_ZIP;
				}
				break;
			}
		case 5:
			{
				password = argv[i];
				break;
			}
		case 6:
			{
				std::stringstream sstr(argv[i]);
				__int64 val;
				sstr >> val;
				if (sstr.fail()) {
					std::cout << "ERROR: Error parsing the maximum file size.  " << argv[i]
						<< " is not a valid number." << std::endl;
					std::cin.get();
					return 0;
				}

				maxFileSize = val;
				break;
			}
		case 7:
			{
				if (std::string(argv[i]) == "compression") {
					compressFiles = true;
				} else if (std::string(argv[i]) == "nocompression") {
					compressFiles = false;
				}
				break;
			}
		case 8:
			{
				if (std::string(argv[i]) == "arrange_default") {
					preserveFileOrder = true;
				} else if (std::string(argv[i]) == "arrange_fitsize") {
					preserveFileOrder = false;
				}
				break;
			}
		case 9:
			{
				//Get the archive number to start at.  All archives before it are skipped.
				std::stringstream sstr(argv[i]);
				int val;
				sstr >> val;
				if (sstr.fail()) {
					std::cout << "ERROR: Error parsing the archive start-at value.  "
						<< argv[i] << " is not a valid number." << std::endl;
					std::cin.get();
					return 0;
				}

				archiveToStartAt = val;
				break;
			}
		case 10:
			{
				//If the program only should make the summary file
				if (std::string(argv[i]) == "summary_only") {
					onlyMakeSummaryFile = true;
				}
			}
		}
	}

	//Make sure the output directory already exists, and if not, create it
	SHCreateDirectoryEx(NULL,output_directory.c_str(),NULL);

	//Format the naming convention
	namingConvention = "\"" + output_directory + namingConvention + "\"";

	std::string directory_directoryRepresentation = directory;
	if (filenameGetProperty(directory, directory_directoryRepresentation, 1) != ERROR_SUCCESS) {
		std::cout << "Error parsing directory... Carrying on..." << std::endl;
	}
	
	FileFindAllFilesInAllSubdirectories(directory, fileInfo);

	/////////////////////
	//Make summary file
	/////////////////////
	std::ofstream summaryFile;

	if (makeSummaryFile) {
		//Create summary file
		summaryFile.open(output_directory + summaryFilename, std::ios::out);
	}
	
	//////////////////////////////////////////////////////
	//Make groups of files and use 7-Zip to archive them
	//////////////////////////////////////////////////////

	//Sort vector (greatest to least) ~ Do not sort this if the user does not want that
	if (!preserveFileOrder) {
		std::sort(fileInfo.files.begin(), fileInfo.files.end(), compareFileInformationPiece);
	}

	int currentArchiveId = 1;

	//Find a temporary directory
	std::string tempDirectory = getTempDirectory() + tempDirectoryName;

	//Delete it if it already exists
	clearTempDirectory(tempDirectory);

	int totalFiles = fileInfo.files.size();

	std::cout << "Total files found: " << totalFiles << std::endl;

	while (fileInfo.files.size() > 0) {
		//Vector that holds all the files to add to this archive
		std::vector<FileInformationPiece> currentArchiveFiles;

		long long currentArchiveFileSize = 0;

		//Loop through all file sizes and find the ones that fit into
		//the max file size
		unsigned int filesInArchive = 0;
		for (unsigned int i = 0; i < fileInfo.files.size(); i ++) {
			//If the file fits or it's the largest file,
			//add it to the archive
			if (fileInfo.files[i].fileSize
				+ currentArchiveFileSize < maxFileSize ||
				(i == 0 && filesInArchive == 0)) {

				//Add to current list
				currentArchiveFiles.push_back(fileInfo.files[i]);
				//Add to the current archive file size
				currentArchiveFileSize += fileInfo.files[i].fileSize;

				//Tell the user that the file is greater than
				//the maximum archive size
				if (i == 0 && fileInfo.files[i].fileSize >
					maxFileSize) {
					std::cout << fileInfo.files[i].fileName << "("
						<< getFormattedSizeTitle(fileInfo.files[i].fileSize)
						<< ") was"
						<< " added to its own archive, although it is greater than"
						<< " the maximum archive size." << std::endl;
				}
				//Delete it and change the current position on the
				//loop only if this is ordered by file size.
				//If it is in the regular file order, it will be deleted later
				//to save processing power.
				if (!preserveFileOrder) {
					fileInfo.files.erase(fileInfo.files.begin() + i);
				}

				//Update the window title
				if (filesInArchive % 5 == 0) {
					SetConsoleTitle((dtos(
						floorDoubleAt((double)(totalFiles - fileInfo.files.size())/(double)(totalFiles),00.001) * 100.0)
						+ "% completed.").c_str());
				}

				filesInArchive ++;

				//Tell the user the progress in the console
				if (filesInArchive % 20 == 0) {
					std::cout << itos(filesInArchive)+ " files in archive list of archive #" + itos(currentArchiveId) + ".  Current"
						+ " archive size: " + getFormattedSizeTitle(currentArchiveFileSize) << std::endl;
				}

				//To get through all files (because one was deleted),
				//check the same index again if preserveFileOrder = false
				if (fileInfo.files.size() > 0) {
					if (!preserveFileOrder) {
						i--;
					}
				}
				//Or leave the loop if it is finished
				else {
					break;
				}

			} else if (preserveFileOrder) {
				//Break here; we do not want to mess up the file order
				break;
			}
		}

		//Delete all the entries in this archive from the mega list if the files are ordered by name
		if (preserveFileOrder && filesInArchive > 0) {
			fileInfo.files.erase(fileInfo.files.begin(), fileInfo.files.begin() + filesInArchive);
		}

		if (currentArchiveFiles.size() > 0 && currentArchiveId >= archiveToStartAt) {
			//Output total archive size
			std::cout << itos(filesInArchive)+ " files in archive list of archive #" + itos(currentArchiveId) + ".  Total"
						+ " archive size: " + getFormattedSizeTitle(currentArchiveFileSize) << std::endl;

			//Set naming convention to get the archive filename
			std::string namingConventionCurrent = namingConvention;
			std::string idString = itos(currentArchiveId);
			padWithZeroes(idString, idStringPaddingAmount);
			stringReplaceAll(namingConventionCurrent, "+ID_HERE+", idString);

			////////////////////////
			//Make archive file list
			////////////////////////

			if (makeSummaryFile) {
				//Add the current archive filename and the number of files in it
				summaryFile << namingConventionCurrent << "\n" << currentArchiveFiles.size() << std::endl;
			}

			//Clear the temp directory
			//Delete it if it already exists
			clearTempDirectory(tempDirectory);

			//Copy the file to the temp directory
			for (unsigned int i = 0; i < currentArchiveFiles.size(); i ++) {

				//Find the directory of the file and remove leading three characters (C:\)
				std::string fullPath = currentArchiveFiles[i].fileName;

				//Directory (with leading backslash)
				std::string pathDir = "";

				//Removed 12/2/2014 so that the folders do not include directories before the archiving directory
				if (filenameGetProperty(fullPath, pathDir, 1) != 0) {
					std::cout << "Error getting directory from " << fullPath << std::endl;
				}
                
				//Added 12/2/2014
				pathDir = stringRemoveIncluding(pathDir, directory_directoryRepresentation);

				//Path to the new directory to create
				std::string newPathDir = tempDirectory + "\\" + pathDir;

				//Create the directory and ignore any "already existing" errors
				int result = ERROR_SUCCESS;
				
				//Make the directory
				if (!onlyMakeSummaryFile) {
					SHCreateDirectoryEx(NULL, newPathDir.c_str(), NULL);
				}

				//Get the name of the file (without the path)
				std::string pathFilename = "";
				std::string pathFileExt = "";

				//Get the full filename (filename + extension)
				int err[2];
				err[0] = filenameGetProperty(fullPath, pathFilename, 2);
				err[1] = filenameGetProperty(fullPath, pathFileExt, 3);
				for (unsigned int j = 0; j < 2; j++) {
					if (err[j] != 0) {
						std::cout << "Error getting filename/extension from " << fullPath << std::endl;
					}
				}

				std::string pathFullFilename = pathFilename + pathFileExt;

				//Something went wrong
				if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS
					&& result != ERROR_FILE_EXISTS) {
					//Tell the user that something went wrong
					std::cout << "ERROR: Directory creation failed: " << newPathDir << std::endl;
				}
				//Success
				else {
					//Copy file to the directory
					if (!onlyMakeSummaryFile) {
						CopyFile(fullPath.c_str(),(newPathDir + pathFullFilename).c_str(), false);
					}

					//Output to summary file
					if (makeSummaryFile) {
						if (summaryDetailLevel >= 0) {
							//Output relative filename
							summaryFile << pathDir << pathFullFilename;
						}
						if (summaryDetailLevel >= 1) {
							//Output file size
							summaryFile << " (" << getFormattedSizeTitle(currentArchiveFiles[i].fileSize) << ")";
						}
						summaryFile << std::endl;
					}
				}
			}


			//Close file
			//fclose(fileList);

			///////////////////////
			//Send command to 7-Zip
			///////////////////////

			
			//Tell user some information
			std::cout << "Finished preparation for " << namingConventionCurrent << " Size: "
				<< getFormattedSizeTitle(currentArchiveFileSize) << std::endl;

			//Set the working directory to the application's directory
			std::system(("cd " + workingDirectoryGet()).c_str());

			//Build the command to send
			std::string command = sevenZipFile + " a";

			if (output_file_type == ARCHIVE_FILE_TYPE_7Z) {
				command += " -t7z ";
			} else if (output_file_type == ARCHIVE_FILE_TYPE_ZIP) {
				command += " -tzip ";
			}
			
			if (!onlyMakeSummaryFile) {
				//command += "\"" + namingConventionCurrent + "\"";
				command += namingConventionCurrent;

				//Add the recursive option to store directories
				command += " -r";

				//Set the compression to zero if the user does not want compression
				if (!compressFiles) {
					command += " -mx=0";
				}

				//Add a password, if the user wants one
				if (password != "") {
					command += " -p" + password;
					if (output_file_type == ARCHIVE_FILE_TYPE_7Z) {
						command += " -mhe";
					}
				}

				command += " " + tempDirectory + "\\*";

				//Send the command to the system
				std::system((command).c_str());
			}

			std::cout << "Finished creating archive #" << currentArchiveId << std::endl;

			//Pause if the escape key is pressed and the console window is on top
			if (GetAsyncKeyState(VK_ESCAPE) && (GetConsoleWindow() == GetForegroundWindow())) {
				std::cout << "Pausing... Press Enter to continue." << std::endl;
				std::cin.get();
			}
		}
		//Increment the current archive ID
		currentArchiveId ++;

	}

	//Close summary file
	if (makeSummaryFile) {
		summaryFile.close();
	}
	
	std::cout << "All done archiving!" << std::endl;

	std::cin.get();
}

////////////////
//   BEGIN FUNCTIONS IN CODE
////////////////

//Removes everything up to and including the specified string
std::string stringRemoveIncluding(std::string fullString, std::string beginningString) {
	unsigned int position = fullString.find(beginningString);
	if (position == std::string::npos) {
		return fullString;
	}

	return fullString.erase(0, position + beginningString.length());
}

//Pads a string with zeroes until the length
//reaches the minimum number length
//Example: num = "45", minimumNumberLength = 4, RESULT = "0045"
void padWithZeroes(std::string &num, unsigned int minimumNumberLength) {
	while (num.length() < minimumNumberLength) {
		num = "0" + num;
	}
}

void clearTempDirectory(std::string tempDirectory) {
	//Delete it if it already exists
	if (PathIsDirectory(tempDirectory.c_str())) {
		SHFILEOPSTRUCT fileOperation;
		fileOperation.hwnd = NULL;
		fileOperation.wFunc = FO_DELETE;

		//Must be double null-terminated
		char* tempDirectoryCstr = (char*)tempDirectory.c_str();

		//Add the double nulls
		int len = strlen(tempDirectoryCstr);
		char* tempDirectoryCstr_doubleNull = new char[len+2];
		strcpy_s (tempDirectoryCstr_doubleNull, len+2, tempDirectoryCstr);
		tempDirectoryCstr_doubleNull[len] = 0;
		tempDirectoryCstr_doubleNull[len+1] = 0;

		fileOperation.pFrom = tempDirectoryCstr_doubleNull;
		fileOperation.pTo = NULL;
		fileOperation.fFlags = FOF_NO_UI;
		fileOperation.hNameMappings = NULL;

		fileOperation.fAnyOperationsAborted = FALSE;

		//Not used
		fileOperation.lpszProgressTitle = "";

		int err = SHFileOperation(&fileOperation);
		if (err != ERROR_SUCCESS) {
			std::cout << "ERROR on directory delete: " << err << std::endl;
		}

		/*bool test = DeleteDirectory((char*)tempDirectory.c_str(), false);
		std::cout << test << std::endl;*/
	}
	
	CreateDirectory(tempDirectory.c_str(),NULL);
}

/*
	propertyValue:
	0 - drive
	1 - directory
	2 - filename
	3 - extension

	Returns 0 on success
*/
int filenameGetProperty(std::string filename, std::string &propertyBuffer, unsigned int propertyValue) {

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	errno_t err;

	char* filename_cstr = (char*) filename.c_str();

	//Split filename
	err = _splitpath_s(filename_cstr, drive, dir, fname, ext);
	
	//Success
	if (err == 0) {
		switch (propertyValue) {
		case 0:
			{
				propertyBuffer = drive;
			}
			break;
		case 1:
			{
				propertyBuffer = dir;
			}
			break;
		case 2:
			{
				propertyBuffer = fname;
			}
			break;
		case 3:
			{
				propertyBuffer = ext;
			}
			break;
		default:
			{
				propertyBuffer = fname;
			}
			break;
		}
	} else {
		return -1;
	}
	return 0;

}

//Finds a temporary directory to use
std::string getTempDirectory() {
	DWORD dirBufferSize = MAX_PATH + 1;
	char* dirBuffer = new char[dirBufferSize];

	GetTempPath(dirBufferSize, dirBuffer);

	std::string dirString = dirBuffer;

	delete[] dirBuffer;

	return dirString;
}

//Replaces all 'search string' occurances in 's string' and replaces them with 'replace string'
void stringReplaceAll(std::string &s, const std::string &search, const std::string &replace) {
	for(size_t pos = 0; ; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == std::string::npos) {
			break;
		}
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}

//Use for sorting
//Taken from
//http://stackoverflow.com/questions/4892680/sorting-a-vector-of-structs
bool compareFileInformationPiece(const FileInformationPiece &a,
								 const FileInformationPiece &b) {
	//sort from greatest to least
	return a.fileSize > b.fileSize;
}

//Shows a file open dialog
std::string getFileOpenDialog(char* nullSeperatedFilter, char* initialDirectory) {
	//Show file open dialog
	OPENFILENAME ofn;
	char szFile[1024];
	ZeroMemory(&ofn , sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = nullSeperatedFilter;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initialDirectory;
	ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

	SetWindowText(ofn.hwndOwner,"OMG LOLZ");

	GetOpenFileName(&ofn);
	return std::string(ofn.lpstrFile);
}

//Gets current working directory
std::string workingDirectoryGet() {
	char cCurrentPath[FILENAME_MAX];

	if (!_getcwd(cCurrentPath, sizeof(cCurrentPath)))
	{
		return "";
	}

	cCurrentPath[sizeof(cCurrentPath) - 1] = '\0';
	return cCurrentPath;
}

//Sets current working directory
int workingDirectorySet(std::string dir) {
	return _chdir(dir.c_str());
}

//Checks if a file exists
bool FileExists(std::string filename) {
	std::ifstream ifile(filename);
	return ifile;
}

//Returns -1 on error, 0 for normal
int FileSize(std::string name, long long &file_size) {
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesEx(name.c_str(), GetFileExInfoStandard, &fad)) {
		return -1; // error condition, could call GetLastError to find out more
	}
	LARGE_INTEGER size;
	size.HighPart = fad.nFileSizeHigh;
	size.LowPart = fad.nFileSizeLow;
	file_size = size.QuadPart;
	return 0;
}

//Converts an integer to an std::string
std::string itos(int i) {
	std::ostringstream convert;
	convert << i;
	return convert.str();
}

//Converts a double to an std::string
std::string dtos(double i) {
	std::ostringstream convert;
	convert << i;
	return convert.str();
}

//Floors a double at a certain place
//roundTo is the place to floor to
//Example: number: 45.67 roundTo: 0.1 Output: 45.6
double floorDoubleAt(double db, double roundTo) {
	return (double)floor(db / roundTo) * roundTo;
}

//Returns a formatted file size
//Updated 7/8/2014 to include a fixed decimal size (1 decimal place)
std::string getFormattedSizeTitle(long long size) {

	if (size < 1024) {
		return itos(size) + " bytes";
	} if (size < pow(1024,2)) {
		std::ostringstream strs;
		strs << std::fixed;
		strs.precision(1);
		strs << floorDoubleAt((double)size / 1024.0,0.1);
		return strs.str() + " KiB";
	} if (size < pow(1024,3)) {
		std::ostringstream strs;
		strs << std::fixed;
		strs.precision(1);
		strs << floorDoubleAt((double)size / (double)(pow(1024,2)),0.1);
		return strs.str() + " MiB";
	} else {
		std::ostringstream strs;
		strs << std::fixed;
		strs.precision(1);
		strs << floorDoubleAt((double)size / (double)(pow(1024,3)),0.1);
		return strs.str() + " GiB";
	}
}

void FileFindAllFilesInAllSubdirectories(std::string directory,
										 FileInformation &fileInfo) {
	//Used to find files
	WIN32_FIND_DATA fileFindData;
	//Handle to find files with
	HANDLE hFind;
	//Search path buffer (string to char*)
	//Why not use more than MAX_PATH?
	//[MAX_PATH];
	char searchPath[1024];

	//Put the string in searchPath
	sprintf_s(searchPath, "%s*.*", directory.c_str());

	//Find the first file
	hFind = ::FindFirstFile(searchPath, &fileFindData);

	//Make sure it's not invalid
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			//Read all files in the folder (and their respective sizes)

			//If the found file is a directory, search in this, too
			if (fileFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				//Make sure the directory found is not the current directory
				//or the parent directory
				if (std::string(fileFindData.cFileName) !=
					std::string(".") && std::string(fileFindData.cFileName) !=
					std::string("..")) {
					
					//Find the files in this directory (recursively)
					//if it's not the same directory
					FileFindAllFilesInAllSubdirectories(directory
						+ fileFindData.cFileName + "\\", fileInfo);
				}
			} else {
				
				//Holds the file size
				long long fileSize = 0;

				//Add the file size to the list
				//by finding the size of the file in bytes
				if (FileSize(directory + fileFindData.cFileName,fileSize) == 0) {
					
					//Create a FileInformationPiece to add to the master list
					FileInformationPiece file;

					//Add the file to the FileInformationPiece
					file.fileName = directory + fileFindData.cFileName;
					
					//Add the file size to the FileInformationPiece
					file.fileSize = fileSize;

					//Add the FileInformationPiece to the list
					fileInfo.files.push_back(file);

					if (fileInfo.files.size() % 50 == 0) {
						std::cout << fileInfo.files.size() << " files found." << std::endl;
					}

				} else {
					std::cout << "Getting file size of " << fileFindData.cFileName
						<< " failed." << std::endl;
				}
			}
		} while (::FindNextFile(hFind, &fileFindData));

		//Close the file find
		::FindClose(hFind);
	}

}