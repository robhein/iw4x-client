#include "STDInclude.hpp"

namespace Components
{
	FileSystem::File::File(std::string file) : Name(file), Handle(0)
	{
		this->Size = Game::FS_FOpenFileRead(this->Name.data(), &this->Handle, 0);
	}

	FileSystem::File::~File()
	{
		if (this->Exists())
		{
			Game::FS_FCloseFile(this->Handle);
		}
	}

	bool FileSystem::File::Exists()
	{
		return (this->Size > 0);
	}

	std::string FileSystem::File::GetName()
	{
		return this->Name;
	}

	std::string FileSystem::File::GetBuffer()
	{
		Utils::Memory::Allocator allocator;
		if (!this->Exists()) return std::string();

		int position = Game::FS_FTell(this->Handle);
		this->Seek(0, FS_SEEK_SET);

		char* buffer = allocator.AllocateArray<char>(this->Size);
		if (!FileSystem::File::Read(buffer, this->Size))
		{
			this->Seek(position, FS_SEEK_SET);
			return std::string();
		}

		this->Seek(position, FS_SEEK_SET);

		return std::string(buffer, this->Size);
	}

	bool FileSystem::File::Read(void* buffer, size_t size)
	{
		if (!this->Exists() || static_cast<size_t>(this->Size) < size || Game::FS_Read(buffer, size, this->Handle) != static_cast<int>(size))
		{
			return false;
		}

		return true;
	}

	void FileSystem::File::Seek(int offset, int origin)
	{
		if (this->Exists())
		{
			Game::FS_Seek(this->Handle, offset, origin);
		}
	}

	void FileSystem::FileWriter::Write(std::string data)
	{
		if (this->Handle)
		{
			Game::FS_Write(data.data(), data.size(), this->Handle);
		}
	}

	void FileSystem::FileWriter::Open()
	{
		this->Handle = Game::FS_FOpenFileWrite(this->FilePath.data());
	}

	void FileSystem::FileWriter::Close()
	{
		if (this->Handle)
		{
			Game::FS_FCloseFile(this->Handle);
		}
	}

	std::vector<std::string> FileSystem::GetFileList(std::string path, std::string extension)
	{
		std::vector<std::string> fileList;

		int numFiles = 0;
		char** files = Game::FS_GetFileList(path.data(), extension.data(), Game::FS_LIST_PURE_ONLY, &numFiles, 0);

		if (files)
		{
			for (int i = 0; i < numFiles; ++i)
			{
				if (files[i])
				{
					fileList.push_back(files[i]);
				}
			}

			Game::FS_FreeFileList(files);
		}

		return fileList;
	}

	std::vector<std::string> FileSystem::GetSysFileList(std::string path, std::string extension, bool folders)
	{
		std::vector<std::string> fileList;

		int numFiles = 0;
		char** files = Game::Sys_ListFiles(path.data(), extension.data(), NULL, &numFiles, folders);

		if (files)
		{
			for (int i = 0; i < numFiles; ++i)
			{
				if (files[i])
				{
					fileList.push_back(files[i]);
				}
			}

			Game::Sys_FreeFileList(files);
		}

		return fileList;
	}

	void FileSystem::DeleteFile(std::string folder, std::string file)
	{
		char path[MAX_PATH] = { 0 };
		Game::FS_BuildPathToFile(Dvar::Var("fs_basepath").Get<const char*>(), reinterpret_cast<char*>(0x63D0BB8), Utils::String::VA("%s/%s", folder.data(), file.data()), reinterpret_cast<char**>(&path));
		Game::FS_Remove(path);
	}

	void FileSystem::RegisterFolder(const char* folder)
	{
		std::string fs_cdpath = Dvar::Var("fs_cdpath").Get<std::string>();
		std::string fs_basepath = Dvar::Var("fs_basepath").Get<std::string>();
		std::string fs_homepath = Dvar::Var("fs_homepath").Get<std::string>();

		if (!fs_cdpath.empty())   Game::FS_AddLocalizedGameDirectory(fs_cdpath.data(),   folder);
		if (!fs_basepath.empty()) Game::FS_AddLocalizedGameDirectory(fs_basepath.data(), folder);
		if (!fs_homepath.empty()) Game::FS_AddLocalizedGameDirectory(fs_homepath.data(), folder);
	}

	void FileSystem::RegisterFolders()
	{
		FileSystem::RegisterFolder("userraw");
	}

	__declspec(naked) void FileSystem::StartupStub()
	{
		__asm
		{
			push esi
			call FileSystem::RegisterFolders
			pop esi

			mov edx, ds:63D0CC0h

			mov eax, 48264Dh
			jmp eax
		}
	}

	int FileSystem::ExecIsFSStub(const char* execFilename)
	{
		return !File(execFilename).Exists();
	}

	FileSystem::FileSystem()
	{
		// Filesystem config checks
		Utils::Hook(0x6098FD, FileSystem::ExecIsFSStub, HOOK_CALL).Install()->Quick();

		// Register additional folders
		Utils::Hook(0x482647, FileSystem::StartupStub, HOOK_JUMP).Install()->Quick();

		// exec whitelist removal (YAYFINITY WARD)
		Utils::Hook::Nop(0x609685, 5);
		Utils::Hook::Nop(0x60968C, 2);

		// ignore 'no iwd files found in main'
		Utils::Hook::Nop(0x642A4B, 5);

		// Ignore bad magic, when trying to free hunk when it's already cleared
		Utils::Hook::Set<WORD>(0x49AACE, 0xC35E);
	}
}
