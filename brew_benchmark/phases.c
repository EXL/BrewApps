#include <AEEStdLib.h>

#include "benchmark.h"

#if defined(__BREW__)
#define LOG DBGPRINTF
#endif

#if defined(NO_ASM)
#define delay_bmips delay_bmips_loop
static __inline void delay_bmips_loop(uint32 loops) {
	uint32 i;
	for (i = loops; !!(i > 0); --i) {
#if defined(__GNUC__) /* GCC Only. */
		asm volatile ("" ::: "memory");
#else
		;
#endif
	}
}
#endif

#if defined(LINUX_BOGOMIPS)
extern boolean BogoMIPS(BENCHMARK_RESULTS_CPU_T *result) {
	uint32 loops_per_sec = 1;

	while ((loops_per_sec *= 2)) {
		uint32 delta_a;
		uint32 delta_b;
		uint32 delta;

		delta_a = GETUPTIMEMS();

		delay_bmips(loops_per_sec);

		delta_b = GETUPTIMEMS();

		delta = (uint32) (delta_b - delta_a);

		if (delta >= TICKS_PER_SEC) {
			uint32 lps = loops_per_sec;
			uint32 bmips_i;
			uint32 bmips_f;
			lps = (lps / delta) * TICKS_PER_SEC;

			bmips_i = lps / 500000;
			bmips_f = (lps / 5000) % 100;

			WSPRINTF(result->bogo_time, sizeof(AECHAR) * RESULT_STRING, L"%lu ms", delta);
			WSPRINTF(result->bogo_mips, sizeof(AECHAR) * RESULT_STRING, L"%lu.%02lu BMIPS", bmips_i, bmips_f);

			LOG("CPU: Delta A ms: %llu\n", delta_a);
			LOG("CPU: Delta B ms: %llu\n", delta_b);
			LOG("CPU: Delta ms: %lu\n", delta);
			LOG("CPU: Loops/s: %lu\n", loops_per_sec);
			LOG("CPU: BogoMIPS: %lu.%02lu\n", bmips_i, bmips_f);

			return TRUE;
		}
	}

	WSTRCPY(result->bogo_time, L"Error");
	WSTRCPY(result->bogo_mips, L"Error");

	LOG("CPU: Error: %s\n", "Cannot calculate BogoMIPS!");
	return FALSE;
}
#endif

static void *AllocateBiggestBlock(uint32 start_size, uint32 *max_block_size, uint32 step, IHeap *brew_heap) {
	uint32 size;
	boolean error;
	void *block_address;

	size = start_size;
	error = FALSE;
	block_address = NULL;

	while (!error) {
		if (!brew_heap) {
			block_address = MALLOC(size);
			if (block_address == NULL) {
				error = TRUE;
			}
		} else {
			if (!IHEAP_CheckAvail(brew_heap, size)) {
				error = TRUE;
			}
		}
		if (!error) {
			if (!brew_heap) {
				FREE(block_address);
			}
			size += step * 4;
		} else {
			while (error && size > start_size) {
				size -= step;
				if (!brew_heap) {
					block_address = MALLOC(size);
					if (block_address != NULL) {
						error = FALSE;
					}
				} else {
					if (IHEAP_CheckAvail(brew_heap, size)) {
						error = FALSE;
					}
				}
			}
			break;
		}
	}

	if (block_address || brew_heap) {
		*max_block_size = size;
	} else {
		*max_block_size = 0;
	}

	return block_address;
}

extern boolean TopOfBiggestRamBlocks(BENCHMARK_RESULTS_RAM_T *result) {
	uint16 i;
	boolean error;
	uint32 time_start;
	uint32 time_end;
	RAM_ALLOCATED_BLOCK_T top_blocks[RAM_TOP_BLOCKS_COUNT];

	error = FALSE;

	for (i = 0; i < RAM_TOP_BLOCKS_COUNT; ++i) {
		top_blocks[i].block_address = NULL;
		top_blocks[i].block_size = 0;
		top_blocks[i].block_time = 0;

		time_start = GETUPTIMEMS();
		top_blocks[i].block_address = AllocateBiggestBlock(
			RAM_START_SIZE_BLOCK, &top_blocks[i].block_size, RAM_STEP_SIZE, NULL
		);
		time_end = GETUPTIMEMS();
		top_blocks[i].block_time = time_end - time_start;

		LOG("RAM: Block %d time: %d\n", i + 1, top_blocks[i].block_time);
		LOG("RAM: Block %d size: %d\n", i + 1, top_blocks[i].block_size);

		WSPRINTF(result->blocks[i], sizeof(AECHAR) * RESULT_STRING, L"%lu ms | %lu B",
			top_blocks[i].block_time, top_blocks[i].block_size);
	}

	for (i = 0; i < RAM_TOP_BLOCKS_COUNT; ++i) {
		if (!top_blocks[i].block_address) {
			error = TRUE;
		} else {
			FREE(top_blocks[i].block_address);
		}
	}

	return error;
}

extern boolean TotalRamSize(BENCHMARK_RESULTS_RAM_T *result) {
	uint16 i;
	boolean error;
	uint32 total_size;
	uint32 time_start;
	uint32 time_end;
	uint32 time_result;
	RAM_ALLOCATED_BLOCK_T ram_blocks[RAM_TOTAL_BLOCKS_COUNT];

	error = FALSE;
	i = 0;
	total_size = 0;

	time_start = GETUPTIMEMS();

	do {
		ram_blocks[i].block_address = AllocateBiggestBlock(
			RAM_START_SIZE_TOTAL, &ram_blocks[i].block_size, RAM_STEP_SIZE, NULL
		);
		total_size += ram_blocks[i].block_size;
	} while (ram_blocks[i++].block_address != NULL);

	time_end = GETUPTIMEMS();

	i -= 1;
	while (i-- > 0) {
		FREE(ram_blocks[i].block_address);
	}

	time_result = time_end - time_start;

	LOG("RAM: Total time: %d\n", time_result);
	LOG("RAM: Total size: %d\n", total_size);

	WSPRINTF(result->total, sizeof(AECHAR) * RESULT_STRING, L"%lu ms | %lu B",
		time_result, total_size);

	return error;
}

extern boolean TotalHeapSize(BENCHMARK_RESULTS_HEAP_T *result, IShell *shell, IHeap *heap) {
	AEEDeviceInfo device_info;
	uint16 i;
	boolean error;
	uint32 total_size;
	uint32 time_start;
	uint32 time_end;
	uint32 time_result;
	uint32 mem_used;
	uint32 mem_total;
	HEAP_ALLOCATED_BLOCK_T heap_blocks[HEAP_TOTAL_BLOCKS_COUNT];

	error = FALSE;
	i = 0;
	total_size = 0;

	time_start = GETUPTIMEMS();

	do {
		heap_blocks[i].block_address = AllocateBiggestBlock(
			HEAP_START_SIZE_TOTAL, &heap_blocks[i].block_size, HEAP_STEP_SIZE, heap
		);
		total_size += heap_blocks[i].block_size;
	} while (heap_blocks[i++].block_address != NULL);

	time_end = GETUPTIMEMS();

	time_result = time_end - time_start;

	LOG("HEAP: Total time: %d\n", time_result);
	LOG("HEAP: Total size: %d\n", total_size);

	WSPRINTF(result->total, sizeof(AECHAR) * RESULT_STRING, L"%lu ms | %lu B", time_result, total_size);

	{
		uint32 time_i;
		uint32 time_f;
		uint32 size_i;
		uint32 size_f;

		time_i = time_result / 1000;
		time_f = ((time_result % 1000) * 100) / 1000;

		size_i = total_size / 1024;
		size_f = ((total_size % 1024) * 100) / 1024;

		WSPRINTF(result->desc, sizeof(AECHAR) * RESULT_STRING, L"%lu.%02lu sec | %lu.%02lu KiB",
			time_i, time_f, size_i, size_f);
	}

	mem_used = IHEAP_GetMemStats(heap);

	device_info.wStructSize = sizeof(AEEDeviceInfo);
	ISHELL_GetDeviceInfo(shell, &device_info);
	mem_total = device_info.dwRAM;

	WSPRINTF(result->used, sizeof(AECHAR) * RESULT_STRING, L"%lu B / %lu B", mem_used, mem_total);

	return error;
}

static boolean GetFreeSpaceSize(IFileMgr *aFileMgr, boolean sdcard, uint32 *aFree, uint32 *aTotal) {
	int32 result;
	uint32 free_size;

	if (sdcard) {
		result = IFILEMGR_GetFreeSpaceEx(aFileMgr, AEEFS_CARD0_DIR, &free_size, aTotal);
		*aFree = free_size;
		return (result == AEE_SUCCESS) && (free_size > BENCHMARK_FILE_SIZE);
	} else {
		free_size = IFILEMGR_GetFreeSpace(aFileMgr, aTotal);
		*aFree = free_size;
		return (free_size > BENCHMARK_FILE_SIZE);
	}

	return FALSE;
}

static boolean DeleteFileIfExist(IFileMgr *aFileMgr, const char *aFilePath) {
	if (IFILEMGR_Test(aFileMgr, aFilePath) == AEE_SUCCESS) {
		return IFILEMGR_Remove(aFileMgr, aFilePath);
	}
	return FALSE;
}

extern boolean DiskBenchmark(AECHAR *res, IFileMgr *aFileMgr, const char *aPath, byte *data, uint32 ch, uint32 sz) {
	uint32 i;
	uint32 delta_a;
	uint32 delta_b;
	uint32 delta_w;
	uint32 delta_r;

	IFile *pIFile;

	if (ch > MEMORY_CHUNK_BUFFER) {
		return FALSE;
	}

	DeleteFileIfExist(aFileMgr, aPath);

	/* Write File Benchmark. */
	delta_a = GETUPTIMEMS();
	pIFile = IFILEMGR_OpenFile(aFileMgr, aPath, _OFM_CREATE);
	if (pIFile == NULL) {
		return FALSE;
	}
	for (i = 0; i < sz; i += ch) {
		IFILE_Write(pIFile, (PACKED const void *) data, ch);
	}
	IFILE_Release(pIFile);
	delta_b = GETUPTIMEMS();
	delta_w = delta_b - delta_a;

	/* Read File Benchmark. */
	delta_a = GETUPTIMEMS();
	pIFile = IFILEMGR_OpenFile(aFileMgr, aPath, _OFM_READ);
	if (pIFile == NULL) {
		return FALSE;
	}
	for (i = 0; i < sz; i += ch) {
		IFILE_Read(pIFile, (PACKED void *) data, ch);
	}
	IFILE_Release(pIFile);
	delta_b = GETUPTIMEMS();
	delta_r = delta_b - delta_a;

	DeleteFileIfExist(aFileMgr, aPath);

	WSPRINTF(res, sizeof(AECHAR) * RESULT_STRING, L"%luK, W:%lu, R:%lu, ms.\n", ch / 1024, delta_w, delta_r);

	return TRUE;
}

extern boolean DisksResult(IFileMgr *aFileMgr, AECHAR *aDiskResult) {
	uint32 free;
	uint32 total;
	byte *buffer;
	AECHAR result[RESULT_STRING];

	/* Clean string first. */
	aDiskResult[0] = '\0';

	/* Memory Buffer. */
	buffer = MALLOC(MEMORY_CHUNK_BUFFER);
	if (buffer  == NULL) {
		WSTRCAT(aDiskResult, L"Cannot allocate memory buffer!\n");
		return FALSE;
	} else {
		GETRAND(buffer, MEMORY_CHUNK_BUFFER);
	}

	/* Phone Memory Bench. */
	if (GetFreeSpaceSize(aFileMgr, FALSE, &free, &total)) {
		WSPRINTF(result, sizeof(AECHAR) * RESULT_STRING, L"Phone (%lu KiB file):\n", BENCHMARK_FILE_SIZE / 1024);
		WSTRCAT(aDiskResult, result);

		DiskBenchmark(result, aFileMgr, "brew_benchmark.bin", buffer, 0x1000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, "brew_benchmark.bin", buffer, 0x2000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, "brew_benchmark.bin", buffer, 0x4000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, "brew_benchmark.bin", buffer, 0x8000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);

		WSPRINTF(result, sizeof(AECHAR) * RESULT_STRING, L"Free: %lu B\nTotal: %lu B\n\n", free, total);
		WSTRCAT(aDiskResult, result);
	} else {
		WSTRCAT(aDiskResult, L"Phone Memory Error!\n");
	}

	/* Card Memory Bench. */
	if (GetFreeSpaceSize(aFileMgr, TRUE, &free, &total)) {
		WSPRINTF(result, sizeof(AECHAR) * RESULT_STRING, L"MemCard (%lu KiB file):\n", BENCHMARK_FILE_SIZE / 1024);
		WSTRCAT(aDiskResult, result);

		DiskBenchmark(result, aFileMgr, AEEFS_CARD0_DIR "brew_benchmark.bin", buffer, 0x1000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, AEEFS_CARD0_DIR "brew_benchmark.bin", buffer, 0x2000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, AEEFS_CARD0_DIR "brew_benchmark.bin", buffer, 0x4000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);
		DiskBenchmark(result, aFileMgr, AEEFS_CARD0_DIR "brew_benchmark.bin", buffer, 0x8000, BENCHMARK_FILE_SIZE);
		WSTRCAT(aDiskResult, result);

		WSPRINTF(result, sizeof(AECHAR) * RESULT_STRING, L"Free: %lu B\nTotal: %lu B\n\n", free, total);
		WSTRCAT(aDiskResult, result);
	} else {
		WSTRCAT(aDiskResult, L"MemCard is full / not present!");
	}

	/* Free Memory Buffer. */
	FREE(buffer);

	return FALSE;
}

#if 0
uint32 Bench_GPU_Passes(uint32 bmp_width, uint32 bmp_height, WCHAR *fps, WCHAR *fms, WCHAR *props) {
	uint32 status;

	status = RESULT_OK;

#if defined(EP1) || defined(EP2)
	APP_AHI_T ahi;

	ahi.info_driver = NULL;
	ahi.bmp_width = bmp_width;
	ahi.bmp_height = bmp_height;
	ahi.p_fire = NULL;
	ahi.flag_restart_demo = FALSE;

	ATI_Driver_Start(&ahi, props);
	GFX_Draw_Start(&ahi);

	do {
		FPS_Meter();
		status = GFX_Draw_Step(&ahi);
		ATI_Driver_Flush(&ahi);
	} while (status == RESULT_OK);

	GFX_Draw_Stop(&ahi);
	ATI_Driver_Stop(&ahi);
#else
	uint32 i = 0;
	Nvidia_Driver_Start();

//	do {
//		UINT8 *bitmwap = uisAllocateMemory(240 * 320 * 2, &status);

//		memset(bitmwap, rand() % 255, 240 * 320 * 2);

//		Nvidia_Driver_Flush(bitmwap, 240, 320, 0, 0);
//		uisFreeMemory(bitmwap);
//		suSleep(10, NULL);
//		i++;
//	} while (i < 100);

#endif
//	CalculateAverageFpsAndTime(fps, fms);

	return status;
}
#endif
