/*
Borrowed from meh321
TODO: Ask him what the license is.
*/

#pragma once

#include <map>
#include <fstream>
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#endif

class VersionDb
{
public:
	VersionDb() { Clear(); }
	~VersionDb() { }

private:
	std::map<std::uint64_t, std::uint64_t> _data;
	std::map<std::uint64_t, std::uint64_t> _rdata;
	int _ver[4];
	std::string _verStr;
	std::string _moduleName;
	std::uint64_t _base = 0x140000000;

	template <typename T>
	static T read(std::ifstream& file)
	{
		T v;
		file.read((char*)&v, sizeof(T));
		return v;
	}

	static void* ToPointer(std::uint64_t v)
	{
		return (void*)v;
	}

	static std::uint64_t FromPointer(void* ptr)
	{
		return (std::uint64_t)ptr;
	}
	
	static bool ParseVersionFromString(const char* ptr, int& major, int& minor, int& revision, int& build)
	{
		return sscanf_s(ptr, "%d.%d.%d.%d", &major, &minor, &revision, &build) == 4 && ((major != 1 && major != 0) || minor != 0 || revision != 0 || build != 0);
	}

public:

	const std::string& GetModuleName() const { return _moduleName; }
	const std::string& GetLoadedVersionString() const { return _verStr; }

	const std::map<std::uint64_t, std::uint64_t>& GetOffsetMap() const
	{
		return _data;
	}

	void* FindAddressById(std::uint64_t id) const
	{
		std::uint64_t b = _base;
		if (b == 0)
			return NULL;

		std::uint64_t offset = 0;
		if (!FindOffsetById(id, offset))
			return NULL;

		return ToPointer(b + offset);
	}

	bool FindOffsetById(std::uint64_t id, std::uint64_t& result) const
	{
		auto itr = _data.find(id);
		if (itr != _data.end())
		{
			result = itr->second;
			return true;
		}
		return false;
	}

	bool FindIdByAddress(void* ptr, std::uint64_t& result) const
	{
		std::uint64_t b = _base;
		if (b == 0)
			return false;

		std::uint64_t addr = FromPointer(ptr);
		return FindIdByOffset(addr - b, result);
	}

	bool FindIdByOffset(std::uint64_t offset, std::uint64_t& result) const
	{
		auto itr = _rdata.find(offset);
		if (itr == _rdata.end())
			return false;

		result = itr->second;
		return true;
	}

	bool GetExecutableVersion(int& major, int& minor, int& revision, int& build) const
	{
#ifdef _WIN32
		TCHAR szVersionFile[MAX_PATH];
		GetModuleFileName(NULL, szVersionFile, MAX_PATH);

		DWORD verHandle = 0;
		UINT size = 0;
		LPBYTE lpBuffer = NULL;
		DWORD verSize = GetFileVersionInfoSize(szVersionFile, &verHandle);

		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfo(szVersionFile, verHandle, verSize, verData))
			{
				{
					char * vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData, "\\StringFileInfo\\040904B0\\ProductVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							delete[] verData;
							return true;
						}
					}
				}

				{
					char * vstr = NULL;
					UINT vlen = 0;
					if (VerQueryValueA(verData, "\\StringFileInfo\\040904B0\\FileVersion", (LPVOID*)&vstr, &vlen) && vlen && vstr && *vstr)
					{
						if (ParseVersionFromString(vstr, major, minor, revision, build))
						{
							delete[] verData;
							return true;
						}
					}
				}
			}

			delete[] verData;
		}
#endif
		return false;
	}

	void GetLoadedVersion(int& major, int& minor, int& revision, int& build) const
	{
		major = _ver[0];
		minor = _ver[1];
		revision = _ver[2];
		build = _ver[3];
	}

	void Clear()
	{
		_data.clear();
		_rdata.clear();
		for (int i = 0; i < 4; i++) _ver[i] = 0;
		_moduleName = std::string();
		//_base = 0;
	}

	bool Load()
	{
		int major, minor, revision, build;
		
		if (!GetExecutableVersion(major, minor, revision, build))
			return false;

		return Load(major, minor, revision, build);
	}
	
	bool Load(int major, int minor, int revision, int build)
	{
		return Load(std::format("Data\\SKSE\\Plugins\\versionlib-{}-{}-{}-{}.bin", major, minor, revision, build));
	}

	bool Load(const std::string& fileName)
	{
		Clear();
		std::ifstream file(fileName, std::ios::binary);

		if (!file.good())
			throw std::runtime_error(std::format("ERROR: failed to open database file {}", fileName));

		int format = read<int>(file);

		if (format < 1 || format > 2){
			if (format > 4)
			 	throw std::runtime_error(std::format("ERROR: Unsupported db format version {} (Did you mean to specify --fallout4?)", format));
		 	throw std::runtime_error(std::format("ERROR: Unsupported db format version {}", format));

		}

		for (int i = 0; i < 4; i++)
			_ver[i] = read<int>(file);

		{
			char verName[64];
			_snprintf_s(verName, 64, "%d.%d.%d.%d", _ver[0], _ver[1], _ver[2], _ver[3]);
			_verStr = verName;
		}

		int tnLen = read<int>(file);

		if (tnLen < 0 || tnLen >= 0x10000)
			throw std::runtime_error("ERROR: Error parsing database");


		if(tnLen > 0)
		{
			char* tnbuf = (char*)malloc(tnLen + 1);
			file.read(tnbuf, tnLen);
			tnbuf[tnLen] = '\0';
			_moduleName = tnbuf;
			free(tnbuf);
		}
#ifdef _WIN32

		{
			//HMODULE handle = GetModuleHandleA(_moduleName.empty() ? NULL : _moduleName.c_str());
			//_base = (std::uint64_t)handle;
		}
#endif

		int ptrSize = read<int>(file);

		int addrCount = read<int>(file);

		unsigned char type, low, high;
		unsigned char b1, b2;
		unsigned short w1, w2;
		unsigned int d1, d2;
		std::uint64_t q1, q2;
		std::uint64_t pvid = 0;
		std::uint64_t poffset = 0;
		std::uint64_t tpoffset;
		for (int i = 0; i < addrCount; i++)
		{
			type = read<unsigned char>(file);
			low = type & 0xF;
			high = type >> 4;

			switch (low)
			{
			case 0: q1 = read<std::uint64_t>(file); break;
			case 1: q1 = pvid + 1; break;
			case 2: b1 = read<unsigned char>(file); q1 = pvid + b1; break;
			case 3: b1 = read<unsigned char>(file); q1 = pvid - b1; break;
			case 4: w1 = read<unsigned short>(file); q1 = pvid + w1; break;
			case 5: w1 = read<unsigned short>(file); q1 = pvid - w1; break;
			case 6: w1 = read<unsigned short>(file); q1 = w1; break;
			case 7: d1 = read<unsigned int>(file); q1 = d1; break;
			default:
			{
				Clear();
				throw std::runtime_error("ERROR: Error parsing database");
			}
			}

			tpoffset = (high & 8) != 0 ? (poffset / (std::uint64_t)ptrSize) : poffset;

			switch (high & 7)
			{
			case 0: q2 = read<std::uint64_t>(file); break;
			case 1: q2 = tpoffset + 1; break;
			case 2: b2 = read<unsigned char>(file); q2 = tpoffset + b2; break;
			case 3: b2 = read<unsigned char>(file); q2 = tpoffset - b2; break;
			case 4: w2 = read<unsigned short>(file); q2 = tpoffset + w2; break;
			case 5: w2 = read<unsigned short>(file); q2 = tpoffset - w2; break;
			case 6: w2 = read<unsigned short>(file); q2 = w2; break;
			case 7: d2 = read<unsigned int>(file); q2 = d2; break;
			default: throw std::exception("ERROR: Error parsing database");
			}

			if ((high & 8) != 0)
				q2 *= (std::uint64_t)ptrSize;

			_data[q1] = q2;
			_rdata[q2] = q1;

			poffset = q2;
			pvid = q1;
		}

		return true;
	}

	bool Dump(const std::string& path, bool use_base)
	{
		std::ofstream f = std::ofstream(path.c_str());
		if (!f.good())
			throw std::runtime_error(std::format("ERROR: failed to open {} for writing", path));
		for (auto itr = _data.begin(); itr != _data.end(); ++itr)
		{
			auto offset = itr->second;
			if (use_base) {
				offset += _base;
			}
			f << std::format("{:<10}\t0x{:09X}\n", itr->first, offset);
		}
		return true;
	}
};
