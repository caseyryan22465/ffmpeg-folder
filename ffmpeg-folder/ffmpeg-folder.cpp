// ffmpeg-folder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <sstream>
#include <memory>
#include <array>
#include <stdexcept>
#include <windows.h>
#include <stdio.h>
#include <fileapi.h>

volatile bool requeststop = false;

BOOL WINAPI CtrlHandler(_In_ DWORD dwtype) {
	switch (dwtype) {
	case CTRL_C_EVENT:
		std::cout << std::endl << "CTRL C, Cleaning up" << std::endl;
		requeststop = true;
		return TRUE;

	case CTRL_BREAK_EVENT:
		std::cout << std::endl << "CTRL BREAK, Cleaning up" << std::endl;
		requeststop = true;
		return TRUE;

	default:
		return FALSE;
	}
}

//https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

std::string spaceSaved(int difference) {
	std::ostringstream saved;
	if (abs(difference / 1024) > 1024) {
		saved << (difference / 1024) / 1024 << " MB";
	}
	else {
		saved << difference << " Bytes";
	}
	return saved.str();
}

//https://stackoverflow.com/questions/38154985/string-to-lpcwstr-in-c
std::wstring string_to_wstring(const std::string& text) {
	return std::wstring(text.begin(), text.end());
}

int main(int argc, char ** argv)
{
	if (argc < 2) {
		exit(0);
	}
	
	//set windows sigint handler
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
	std::ostringstream res;
	for(auto i = 1; i < argc - 1; i++){
		res << argv[i];
		res << " ";
	}
	res << argv[argc-1];
	auto filename = std::filesystem::path(res.str());
	std::size_t space_saved = 0;
	std::size_t space_added = 0;
	for (const std::filesystem::directory_entry& dir_entry : std::filesystem::recursive_directory_iterator(filename)) {
		if (requeststop) {
			break;
		}
		if (dir_entry.path().extension() == ".mp4") {
			//ffprobe -v error -select_streams v:0 -show_entries format_tags=comment -of default=noprint_wrappers=1 filename.mp4
			//that will display TAG:comment=commenttexthere
			std::string res = exec((std::ostringstream() << "ffprobe -v error -select_streams v:0 -show_entries format_tags=comment -of default=noprint_wrappers=1 " << dir_entry.path()).str().c_str());
			if (!(res.find("comment=cppautotranscoded") != std::string::npos)) {
				std::cout << "Found unencoded: " << dir_entry.path().filename() << ", encoding now" << std::endl;
				std::filesystem::path outfilepath = std::filesystem::path(
					(std::ostringstream() << dir_entry.path().parent_path().string() << "\\" << dir_entry.path().stem().string() << "_265" << dir_entry.path().extension().string()).str()
				);
				//std::cout << "outfile: " << outfilepath << std::endl;
				std::string encoded = exec(
					(
						std::ostringstream() << "ffmpeg -i " << dir_entry.path() << " -loglevel fatal -c:v libx265 -crf 28 -preset fast -c:a copy -metadata comment=\"cppautotranscoded\" -y -x265-params log-level=fatal " << outfilepath
					)
					.str().c_str());
				auto newsize = std::filesystem::file_size(outfilepath);
				if (dir_entry.file_size() > newsize && !requeststop) {
					space_saved += dir_entry.file_size() - newsize;
				}
				else if(!requeststop) {
					std::cout << "larger filesize with " << dir_entry.path() << " and " << outfilepath << std::endl;
					space_added += dir_entry.file_size() - newsize;
				}
				if (!requeststop) {
					//delete stuff here
					std::cout << "Filesize before: " << spaceSaved(dir_entry.file_size()) << ", Filesize after: " << spaceSaved(newsize) << std::endl;
					HANDLE hFile1 = CreateFile(string_to_wstring(dir_entry.path().string()).c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if(hFile1 == INVALID_HANDLE_VALUE){
						std::cout << "Couldn't open handle1" << std::endl;
					}
					else {
						FILETIME created;
						FILETIME accessed;
						FILETIME written;
						if (!GetFileTime(hFile1, &created, &accessed, &written)) {
							std::cout << "Couldn't get file time" << std::endl;
						}
						else {
							CloseHandle(hFile1);
							std::filesystem::remove(dir_entry.path());
							std::filesystem::rename(outfilepath, dir_entry.path());
							HANDLE hFile2 = CreateFile(string_to_wstring(dir_entry.path().string()).c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							if (hFile2 == INVALID_HANDLE_VALUE) {
								std::cout << "Couldn't open handle2" << std::endl;
							}
							else {
								if (!SetFileTime(hFile2, &created, &accessed, &written)) {
									std::cout << "Couldn't set file time" << std::endl;
								}
								else {
									std::cout << "Set file time" << std::endl;//TODO: Remove
								}
								CloseHandle(hFile2);
							}
						}
					}
					//dont delete down here because delete in the file time area. If you dont delete in the file time area because of an error, want to preserve original metadata anyways
				}
				else {
					std::filesystem::remove(outfilepath);

				}
				
			}
			//std::cout << "ffprobe -v error -select_streams v:0 -show_entries format_tags=comment -of default=noprint_wrappers=1 \"" + dir_entry.path().string() + "\"" << std::endl;
		}
	}
	std::cout << "Program finished. " << spaceSaved(space_saved - space_added) << " saved." << std::endl;
	return 0;
}