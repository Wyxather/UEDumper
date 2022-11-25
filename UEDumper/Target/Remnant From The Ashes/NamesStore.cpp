#include "NamesStore.hpp"
#include "PatternFinder.hpp"

#include <Psapi.h>

namespace
{
	class FNameEntry
	{
	public:
		class FNameEntry* HashNext; //0x0000
		int32_t Index; //0x0008
		char AnsiName[2048]; //0x000C
	}; //Size: 0x080C

	class TNameEntryArray
	{
	public:
		enum
		{
			ElementsPerChunk = 16 * 1024,
			ChunkTableSize = (4 * 1024 * 1024 + ElementsPerChunk - 1) / ElementsPerChunk
		};

		[[nodiscard]] constexpr auto GetItemPtr(size_t Index) const noexcept
		{
			const auto ChunkIndex = Index / ElementsPerChunk;
			const auto WithinChunkIndex = Index % ElementsPerChunk;
			const auto Chunk = Chunks[ChunkIndex];
			return Chunk + WithinChunkIndex;
		}

		[[nodiscard]] constexpr auto& GetById(size_t Index) const noexcept
		{
			return *GetItemPtr(Index);
		}

		[[nodiscard]] constexpr auto IsValidIndex(size_t Index) const noexcept
		{
			return Index >= 0 && Index < NumElements && GetById(Index);
		}

		FNameEntry* const* Chunks[ChunkTableSize]; //0x0000
		int32_t NumElements; //0x0800
		int32_t NumChunks; //0x0804
	}; //Size: 0x0808
	static_assert(sizeof(TNameEntryArray) == 0x808);

	TNameEntryArray* GlobalNames;
}

auto NamesStore::Initialize() noexcept -> bool
{
	const auto processHandle = GetCurrentProcess();
	if (!processHandle)
		return false;

	const auto moduleHandle = GetModuleHandle(nullptr);
	if (!moduleHandle)
		return false;

	MODULEINFO moduleInfo;
	if (!GetModuleInformation(processHandle, moduleHandle, &moduleInfo, sizeof(moduleInfo)))
		return false;

	const auto address = PatternFinder{
		{ reinterpret_cast<const std::byte*>(moduleInfo.lpBaseOfDll), moduleInfo.SizeOfImage }
	}(PatternView<"\x48\x83\xEC\x28\x48\x8B\x05\xFF\xFF\xFF\xFF\x48\x85\xC0\x75\xFF\xB9\xFF\xFF\x00\x00\x48\x89\x5C\x24\x20\xE8">::value);

	if (!address)
		return false;

	printf("NamesStore:Address = %p\n", address);
	GlobalNames = *RelativeToAbsolute<TNameEntryArray*>(reinterpret_cast<std::uintptr_t>(address) + 7);
	printf("NamesStore:GlobalNames = %p\n", (void*)GlobalNames);

	return true;
}

auto NamesStore::GetAddress() noexcept -> void*
{
	return GlobalNames;
}

auto NamesStore::GetNamesNum() const noexcept -> std::size_t
{
	return static_cast<std::size_t>(GlobalNames->NumElements);
}

auto NamesStore::IsValid(std::size_t id) const noexcept -> bool
{
	return GlobalNames->IsValidIndex(id);
}
auto NamesStore::GetById(std::size_t id) const noexcept -> std::string
{
	return GlobalNames->GetById(id)->AnsiName;
}