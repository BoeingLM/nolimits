#include <stdint.h>

#ifdef WIN32
#include <Windows.h>
#elif __linux__
#include <unistd.h>
#include <sys/mman.h>
const int pagesize = sysconf(_SC_PAGE_SIZE);
const int pagemask = ~(pagesize-1);
#endif

class VTFHook
{	
public:
	VTFHook(void* instance, void* hook, int offset) 
	{
		intptr_t vtable = *((intptr_t*)instance);
		entry = vtable + sizeof(intptr_t) * offset;
		original = *((intptr_t*) entry);

		int original_protection = unprotect((void*)entry);
		*((intptr_t*)entry) = (intptr_t)hook;
		protect((void*)entry, original_protection);
	}
	~VTFHook() 
	{
		int original_protection = unprotect((void*)entry);
		*((intptr_t*)entry) = (intptr_t)original;
		protect((void*)entry, original_protection);
	}
	
private:
	intptr_t entry;
	intptr_t original;
	int unprotect(void* region) 
	{
		#ifdef WIN32
				MEMORY_BASIC_INFORMATION mbi;
				VirtualQuery((LPCVOID)region, &mbi, sizeof(mbi));
				VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);
				return mbi.Protect;
		#elif __linux__
				mprotect((void*) ((intptr_t)region & pagemask), pagesize, PROT_READ|PROT_WRITE|PROT_EXEC);
				return PROT_READ|PROT_EXEC;
		#endif
	}

	void protect(void* region, int protection) 
	{
		#ifdef WIN32
				MEMORY_BASIC_INFORMATION mbi;
				VirtualQuery((LPCVOID)region, &mbi, sizeof(mbi));
				VirtualProtect(mbi.BaseAddress, mbi.RegionSize, protection, &mbi.Protect);
		#elif __linux__
				mprotect((void*) ((intptr_t)region & pagemask), pagesize, protection);
		#endif
	}	
};
