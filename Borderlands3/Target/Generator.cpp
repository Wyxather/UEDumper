#include "NamesStore.hpp"

class Generator : public IGenerator
{
public:
	bool Initialize(void* module) override
	{
		alignasClasses = {
			{ "ScriptStruct CoreUObject.Plane", 16 },
			{ "ScriptStruct CoreUObject.Quat", 16 },
			{ "ScriptStruct CoreUObject.Transform", 16 },
			{ "ScriptStruct CoreUObject.Vector4", 16 },
			{ "ScriptStruct Engine.RootMotionSourceGroup", 8 }
		};

		virtualFunctionPattern["Class CoreUObject.Object"] = {
			{ "\x44\x88\x7D\x00\x48\x89", "xxxxxx", 0x400, R"(	inline void ProcessEvent(class UFunction* function, void* parms)
	{
		return GetVFunction<void(*)(UObject*, class UFunction*, void*)>(this, %d)(this, function, parms);
	})" }
		};
		virtualFunctionPattern["Class CoreUObject.Class"] = {
			{ "\x4C\x8B\xDC\x57\x48\x81\xEC", "xxxxxxx", 0x200, R"(	inline UObject* CreateDefaultObject()
	{
		return GetVFunction<UObject*(*)(UClass*)>(this, %d)(this);
	})" }
		};

		predefinedMembers["Class CoreUObject.Object"] = {
			{ "void*", "Vtable" },
			{ "int32_t", "ObjectFlags" },
			{ "int32_t", "InternalIndex" },
			{ "class UClass*", "Class" },
			{ "FName", "Name" },
			{ "class UObject*", "Outer" }
		};
		predefinedStaticMembers["Class CoreUObject.Object"] = {
			{ "FUObjectArray*", "GObjects" }
		};
		predefinedMembers["Class CoreUObject.Field"] = {
			{ "class UField*", "Next" }
		};
		predefinedMembers["Class CoreUObject.Struct"] = {
			{ "class UStruct*", "SuperField" },
			{ "class UField*", "Children" },
			{ "int32_t", "PropertySize" },
			{ "int32_t", "MinAlignment" },
			{ "char", "pad_0048[64]"}
		};
		predefinedMembers["Class CoreUObject.Function"] = {
			{ "int32_t", "FunctionFlags" },
			{ "int16_t", "NumParms" },
			{ "int8_t", "ParmsSize" },
			{ "char", "pad_008F[1]" },
			{ "uint16_t", "ReturnValueOffset" },
			{ "char", "pad_0092[30]" },
			{ "void*", "Func" }
		};

		predefinedMethods["ScriptStruct CoreUObject.Vector2D"] = {
			PredefinedMethod::Inline(R"(	constexpr FVector2D() noexcept
		: X(0), Y(0)
	{ })"),
			PredefinedMethod::Inline(R"(	constexpr FVector2D(float x, float y) noexcept
		: X(x),
		  Y(y)
	{ })")
		};
		predefinedMethods["ScriptStruct CoreUObject.LinearColor"] = {
			PredefinedMethod::Inline(R"(	constexpr FLinearColor() noexcept
		: R(0), G(0), B(0), A(0)
	{ })"),
			PredefinedMethod::Inline(R"(	constexpr FLinearColor(float r, float g, float b, float a) noexcept
		: R(r),
		  G(g),
		  B(b),
		  A(a)
	{ })")
		};

		predefinedMethods["Class CoreUObject.Object"] = {
			PredefinedMethod::Inline(R"(	[[nodiscard]] static constexpr auto& GetGlobalObjects() noexcept
	{
		return GObjects->ObjObjects;
	})"),
			PredefinedMethod::Inline(R"(	template <typename T>
	[[nodiscard]] static constexpr auto FindObject(std::string_view name) noexcept -> T*
	{
		for (decltype(GetGlobalObjects().Num()) i = 0, max = GetGlobalObjects().Num(); i < max; ++i) {

			auto object = GetGlobalObjects().GetByIndex(i).Object;
			if (object == nullptr)
				continue;

			if (object->GetFullName() == name)
				return static_cast<T*>(object);
		}
		return nullptr;
	})"),
			PredefinedMethod::Inline(R"(	template <typename T>
	[[nodiscard]] static constexpr auto FindObjects(std::string_view name) noexcept -> std::vector<T*>
	{
		std::vector<T*> objects;
		for (decltype(GetGlobalObjects().Num()) i = 0, max = GetGlobalObjects().Num(); i < max; ++i) {

			auto object = GetGlobalObjects().GetByIndex(i).Object;
			if (object == nullptr)
				continue;

			if (object->GetFullName() == name)
				objects.emplace_back(static_cast<T*>(object));
		}
		return objects;
	})"),
			PredefinedMethod::Inline(R"(	template <typename T>
	[[nodiscard]] static constexpr auto FindObject() noexcept -> T*
	{
		auto StaticClass = T::StaticClass();
		for (decltype(GetGlobalObjects().Num()) i = 0, max = GetGlobalObjects().Num(); i < max; ++i) {

			auto object = GetGlobalObjects().GetByIndex(i).Object;
			if (object == nullptr)
				continue;

			if (object->IsA(StaticClass))
				return static_cast<T*>(object);
		}
		return nullptr;
	})"),
			PredefinedMethod::Inline(R"(	template <typename T>
	[[nodiscard]] static constexpr auto FindObjects() noexcept -> std::vector<T*>
	{
		std::vector<T*> objects;
		auto StaticClass = T::StaticClass();
		for (decltype(GetGlobalObjects().Num()) i = 0, max = GetGlobalObjects().Num(); i < max; ++i) {

			auto object = GetGlobalObjects().GetByIndex(i).Object;
			if (object == nullptr)
				continue;

			if (object->IsA(StaticClass))
				objects.emplace_back(static_cast<T*>(object));
		}
		return objects;
	})"),
			PredefinedMethod::Inline(R"(	[[nodiscard]] static auto FindClass(std::string_view name) noexcept
	{
		return FindObject<UClass>(name);
	})"),
			PredefinedMethod::Inline(R"(	template <typename T>
	[[nodiscard]] static auto GetObjectCasted(std::size_t index) noexcept
	{
		return static_cast<T*>(GetGlobalObjects().GetByIndex(index).Object);
	})"),
			PredefinedMethod::Inline(R"(	static constexpr auto ProcessEvent(void*, void*) noexcept
	{
	})"),
			PredefinedMethod::Default("[[nodiscard]] auto GetName() const noexcept -> std::string",
			R"(auto UObject::GetName() const noexcept -> std::string
{
	std::string name(Name.GetName());
	if (Name.Number > 0)
		name += '_' + std::to_string(Name.Number);

	auto pos = name.rfind('/');
	if (pos == std::string::npos)
		return name;

	return name.substr(pos + 1);
})"),
			PredefinedMethod::Default("[[nodiscard]] auto GetFullName() const noexcept -> std::string",
			R"(auto UObject::GetFullName() const noexcept -> std::string
{
	std::string name;

	if (Class == nullptr)
		return name;

	std::string temp;
	for (auto p = Outer; p; p = p->Outer)
		temp = p->GetName() + "." + temp;

	name = Class->GetName();
	name += " ";
	name += temp;
	name += GetName();

	return name;
})"),
			PredefinedMethod::Default("auto IsA(UClass* cmp) const noexcept -> bool",
			R"(auto UObject::IsA(UClass* cmp) const noexcept -> bool
{
	for (auto super = Class; super; super = static_cast<UClass*>(super->SuperField))
		if (super == cmp)
			return true;
	return false;
})")
		};
		predefinedMethods["Class CoreUObject.Class"] = {
			PredefinedMethod::Inline(R"(	template <typename T>
	constexpr auto CreateDefaultObject() noexcept -> T*
	{
		return static_cast<T*>(CreateDefaultObject());
	})")
		};

		predefinedMethods["Class Engine.World"] = {
			PredefinedMethod::Inline(R"(	static class UWorld** GWorld;
	static constexpr auto GetWorld() noexcept
	{
		return *GWorld;
	};)")
		};

		return true;
	}

	std::string GetGameName() const override
	{
		return "Borderlands 3";
	}

	std::string GetGameNameShort() const override
	{
		return "BL3";
	}

	std::string GetGameVersion() const override
	{
		return "V1.0";
	}

	std::string GetNamespaceName() const override
	{
		return "SDK";
	}

	std::vector<std::string> GetIncludes() const override
	{
		return { };
	}

	std::string GetOutputDirectory() const
	{
		return "C:/SDK_GEN"; //TODO: add a feature so users can customize this
	}

	std::string GetBasicDeclarations() const override
	{
		return R"(template<typename Fn>
inline Fn GetVFunction(const void *instance, std::size_t index)
{
	auto vtable = *reinterpret_cast<const void***>(const_cast<void*>(instance));
	return reinterpret_cast<Fn>(vtable[index]);
}

class FNameEntry
{
public:
	[[nodiscard]] constexpr auto GetAnsiName() const noexcept
	{
		return AnsiName;

	}

	[[nodiscard]] constexpr auto GetWideName() const noexcept
	{
		return WideName;
	}

private:
	std::int32_t Index; //0x0000
	char pad_0004[4]; //0x0004
	class FNameEntry* HashNext; //0x0008

	union //0x0010
	{
		char AnsiName[1024];
		wchar_t WideName[1024];
	};
};

class TNameEntryArray
{
	enum
	{
		ElementsPerChunk = 16 * 1024,
		ChunkTableSize = (2 * 1024 * 1024 + ElementsPerChunk - 1) / ElementsPerChunk
	};

	[[nodiscard]] constexpr auto GetItemPtr(std::size_t Index) const noexcept
	{
		const auto ChunkIndex = Index / ElementsPerChunk;
		const auto WithinChunkIndex = Index % ElementsPerChunk;
		return Chunks[ChunkIndex] + WithinChunkIndex;
	}

public:
	[[nodiscard]] constexpr auto Num() const noexcept
	{
		return NumElements;

	}

	[[nodiscard]] constexpr auto IsValidIndex(std::int32_t Index) const noexcept
	{
		return Index < Num() && Index >= 0;
	}

	[[nodiscard]] constexpr auto& operator[](std::int32_t Index) const noexcept
	{
		return *GetItemPtr(Index);
	}

private:
	FNameEntry* const* Chunks[ChunkTableSize];
	std::int32_t NumElements;
	std::int32_t NumChunks;
};

struct FName
{
	static TNameEntryArray* GNames;

	[[nodiscard]] static constexpr auto& GetGlobalNames() noexcept
	{
		return *GNames;
	};

	[[nodiscard]] constexpr auto GetName() const noexcept
	{
		return GetGlobalNames()[ComparisonIndex]->GetAnsiName();
	};

	[[nodiscard]] constexpr auto operator==(const FName& other) const noexcept
	{
		return ComparisonIndex == other.ComparisonIndex;
	};

	std::int32_t ComparisonIndex;
	std::int32_t Number;

	constexpr FName() noexcept :
		ComparisonIndex(0),
		Number(0)
	{
	};

	constexpr FName(std::int32_t i) noexcept :
		ComparisonIndex(i),
		Number(0)
	{
	};

	FName(const char* nameToFind) noexcept :
		ComparisonIndex(0),
		Number(0)
	{
		static std::unordered_set<std::int32_t> cache;

		for (auto i : cache) {
			if (!std::strcmp(GetGlobalNames()[i]->GetAnsiName(), nameToFind)) {
				ComparisonIndex = i;
				return;
			}
		}

		for (decltype(GetGlobalNames().Num()) i = 0, max = GetGlobalNames().Num(); i < max; ++i) {
			if (GetGlobalNames()[i] != nullptr) {
				if (!std::strcmp(GetGlobalNames()[i]->GetAnsiName(), nameToFind)) {
					cache.insert(i);
					ComparisonIndex = i;
					return;
				}
			}
		}
	};
};

class FUObjectItem
{
public:
	class UObject* Object;
	std::int32_t Flags;
	std::int32_t ClusterRootIndex;
	std::int32_t SerialNumber;
	char pad_0014[4];
};

class FChunkedFixedUObjectArray
{
public:
	enum
	{
		NumElementsPerChunk = 64 * 1024
	};

	[[nodiscard]] constexpr auto Num() const noexcept
	{
		return NumElements;
	}

	[[nodiscard]] constexpr auto GetObjectPtr(std::int32_t Index) const noexcept
	{
		const auto ChunkIndex = Index / NumElementsPerChunk;
		const auto WithinChunkIndex = Index % NumElementsPerChunk;
		return Objects[ChunkIndex] + WithinChunkIndex;
	}

	[[nodiscard]] constexpr auto& GetByIndex(std::int32_t Index) const noexcept
	{
		return *GetObjectPtr(Index);
	}

private:
	class FUObjectItem** Objects;
	class FUObjectItem* PreAllocatedObjects;
	std::int32_t MaxElements;
	std::int32_t NumElements;
	std::int32_t MaxChunks;
	std::int32_t NumChunks;
};

class FUObjectArray
{
public:
	std::int32_t ObjFirstGCIndex;
	std::int32_t ObjLastNonGCIndex;
	std::int32_t MaxObjectsNotConsideredByGC;
	bool OpenForDisregardForGC;
	FChunkedFixedUObjectArray ObjObjects;
};

template<class T>
class TArray
{
	friend class FString;

public:
	constexpr TArray() noexcept
	{
		Data = nullptr;
		Count = Max = 0;
	};

	[[nodiscard]] constexpr auto Num() const noexcept
	{
		return Count;
	};

	[[nodiscard]] constexpr auto& operator[](std::int32_t i) noexcept
	{
		return Data[i];
	};

	[[nodiscard]] constexpr const auto& operator[](std::int32_t i) const noexcept
	{
		return Data[i];
	};

	[[nodiscard]] constexpr auto IsValidIndex(std::int32_t i) const noexcept
	{
		return i < Num();
	}

private:
	T* Data;
	std::int32_t Count;
	std::int32_t Max;
};

class FString : public TArray<wchar_t>
{
public:
	constexpr FString() noexcept 
	{
	};

	constexpr FString(const wchar_t* other) noexcept
	{
		Max = Count = *other ? static_cast<std::int32_t>(std::wcslen(other)) + 1 : 0;

		if (Count)
			Data = const_cast<wchar_t*>(other);
	};

	[[nodiscard]] constexpr auto IsValid() const noexcept
	{
		return Data != nullptr;
	}

	[[nodiscard]] constexpr auto c_str() const noexcept
	{
		return Data;
	}

	[[nodiscard]] auto ToString() const noexcept
	{
		const auto length = std::wcslen(Data);
		std::string str(length, '\0');
		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);
		return str;
	}
};

template<class TEnum>
class TEnumAsByte
{
public:
	constexpr TEnumAsByte() noexcept
	{
	}

	constexpr TEnumAsByte(TEnum _value) noexcept :
		value(static_cast<std::uint8_t>(_value))
	{
	}

	constexpr explicit TEnumAsByte(std::int32_t _value) noexcept :
		value(static_cast<std::uint8_t>(_value))
	{
	}

	constexpr explicit TEnumAsByte(std::uint8_t _value) noexcept :
		value(_value)
	{
	}

	[[nodiscard]] constexpr operator TEnum() const noexcept
	{
		return (TEnum)value;
	}

	[[nodiscard]] constexpr TEnum GetValue() const noexcept
	{
		return (TEnum)value;
	}

private:
	std::uint8_t value;
};

#pragma push_macro("GetObject")
#undef GetObject
class FScriptInterface
{
private:
	class UObject* ObjectPointer;
	void* InterfacePointer;

public:
	[[nodiscard]] constexpr auto GetObject() const noexcept
	{
		return ObjectPointer;
	}

	[[nodiscard]] constexpr auto& GetObjectRef() noexcept
	{
		return ObjectPointer;
	}

	[[nodiscard]] constexpr auto GetInterface() const noexcept
	{
		return ObjectPointer != nullptr ? InterfacePointer : nullptr;
	}
};
#pragma pop_macro("GetObject")

template<class InterfaceType>
class TScriptInterface : public FScriptInterface
{
public:
	[[nodiscard]] constexpr auto operator->() const noexcept -> InterfaceType*
	{
		return reinterpret_cast<InterfaceType*>(GetInterface());
	}

	[[nodiscard]] constexpr InterfaceType& operator*() const noexcept
	{
		return *reinterpret_cast<InterfaceType*>(GetInterface());
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return GetInterface() != nullptr;
	}
};

class FTextData
{
public:
	char UnknownData[0x28];
	wchar_t* Name;
	__int32 Length;
};

struct FText
{
	FTextData* Data;
	char UnknownData[0x10];

	[[nodiscard]] constexpr auto Get() const noexcept -> wchar_t*
	{
		if (Data)
			return Data->Name;

		return nullptr;
	}
};

struct FScriptDelegate
{
	char UnknownData[0x14];
};

struct FScriptMulticastDelegate
{
	char UnknownData[0x10];
};

template<typename Key, typename Value>
class TMap
{
	char UnknownData[0x50];
};

struct FWeakObjectPtr
{
public:
	[[nodiscard]] auto IsValid() const noexcept -> bool;
	[[nodiscard]] auto Get() const noexcept -> class UObject*;

	std::int32_t ObjectIndex;
	std::int32_t ObjectSerialNumber;
};

template<class T, class TWeakObjectPtrBase = FWeakObjectPtr>
struct TWeakObjectPtr : private TWeakObjectPtrBase
{
public:
	constexpr auto Get() const noexcept -> T*
	{
		return reinterpret_cast<T*>(TWeakObjectPtrBase::Get());
	}

	[[nodiscard]] constexpr auto& operator*() const noexcept
	{
		return *Get();
	}

	[[nodiscard]] constexpr auto operator->() const noexcept
	{
		return Get();
	}

	[[nodiscard]] constexpr auto IsValid() const noexcept
	{
		return TWeakObjectPtrBase::IsValid();
	}
};

template<class T, class TBASE>
class TAutoPointer : public TBASE
{
public:
	[[nodiscard]] constexpr operator T*() const noexcept
	{
		return TBASE::Get();
	}

	[[nodiscard]] constexpr operator const T*() const noexcept
	{
		return (const T*)TBASE::Get();
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return TBASE::Get() != nullptr;
	}
};

template<class T>
class TAutoWeakObjectPtr : public TAutoPointer<T, TWeakObjectPtr<T>>
{
};

template<typename TObjectID>
class TPersistentObjectPtr
{
public:
	FWeakObjectPtr WeakPtr;
	std::int32_t TagAtLastTest;
	TObjectID ObjectID;
};

struct FStringAssetReference_
{
	char UnknownData[0x10];
};

class FAssetPtr : public TPersistentObjectPtr<FStringAssetReference_>
{
};

template<typename ObjectType>
class TAssetPtr : FAssetPtr
{
};

struct FSoftObjectPath
{
	FName AssetPathName;
	FString SubPathString;
};

class FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath>
{
};

template<typename ObjectType>
class TSoftObjectPtr : FSoftObjectPtr
{
};

struct FUniqueObjectGuid_
{
	char UnknownData[0x10];
};

class FLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid_>
{
};

template<typename ObjectType>
class TLazyObjectPtr : FLazyObjectPtr
{
};

template<class TClass>
class TSubclassOf
{
public:
	constexpr TSubclassOf(class UClass* Class) noexcept
	{
		this->Class = Class;
	}

	[[nodiscard]] constexpr auto GetClass() noexcept
	{
		return Class;
	}

private:
	class UClass* Class;
};)";
	}

	std::string GetBasicDefinitions() const override
	{
		return R"(TNameEntryArray* FName::GNames = nullptr;
FUObjectArray* UObject::GObjects = nullptr;
UWorld** UWorld::GWorld = nullptr;)";
	}
};

Generator _generator;
IGenerator* generator = &_generator;