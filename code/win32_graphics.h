#if !defined (WIN32_GRAPHICS_H)

struct Memory
{
	uint64 permanentStorageSize;
	void *permanentStorage;

	uint64 transientStorageSize;
	void *transientStorage;
};

#define WIN32_GRAPHCS_H
#endif
