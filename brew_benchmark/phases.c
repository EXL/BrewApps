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
boolean BogoMIPS(BENCHMARK_RESULTS_CPU_T *result) {
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

static void *AllocateBiggestBlock(uint32 start_size, uint32 *max_block_size, uint32 step, boolean brew_heap) {
	uint32 size;
	boolean error;
	void *block_address;

	size = start_size;
	error = FALSE;
	block_address = NULL;

	while (!error) {
		if (brew_heap) {
			block_address = MALLOC(size);
			if (block_address == NULL) {
				error = TRUE;
			}
		} else {
			block_address = MALLOC(size);
			if (block_address == NULL) {
				error = TRUE;
			}
		}
		if (!error) {
			if (brew_heap) {
				FREE(block_address);
			} else {
				FREE(block_address);
			}
			size += step * 4;
		} else {
			while (error && size > start_size) {
				size -= step;
				if (brew_heap) {
					block_address = MALLOC(size);
					if (block_address == NULL) {
						error = TRUE;
					}
				} else {
					block_address = MALLOC(size);
					if (block_address == NULL) {
						error = TRUE;
					}
				}
			}
			break;
		}
	}

	if (block_address) {
		*max_block_size = size;
	} else {
		*max_block_size = 0;
	}

	return block_address;
}

boolean TopOfBiggestRamBlocks(BENCHMARK_RESULTS_RAM_T *result) {
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
			RAM_START_SIZE_BLOCK, &top_blocks[i].block_size, RAM_STEP_SIZE, FALSE
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

boolean TotalRamSize(BENCHMARK_RESULTS_RAM_T *result) {
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
			RAM_START_SIZE_TOTAL, &ram_blocks[i].block_size, RAM_STEP_SIZE, FALSE
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

#if 0
uint32 TotalHeapSize(BENCHMARK_RESULTS_HEAP_T *result) {
	UINT16 i;
	uint32 status;
	uint32 total_size;
	uint64 time_start;
	uint64 time_end;
	uint32 time_result;
	HEAP_ALLOCATED_BLOCK_T heap_blocks[HEAP_TOTAL_BLOCKS_COUNT];

	status = RESULT_OK;
	i = 0;
	total_size = 0;

	time_start = suPalReadTime();

	/* Can I use Java Heap functions? */
	heap_blocks[i].block_address = AmMemAllocPointer(HEAP_STEP_SIZE);
	if (heap_blocks[i].block_address != NULL) {
		AmMemFreePointer(heap_blocks[i].block_address);
	} else {
		status = RESULT_FAIL;
	}

/* BUG: Is the M-CORE 7 MiB Heap patch bugged? `AmMemAllocPointer()` can only be called a few times. */
#if defined(EM1) || defined(EM2)
	total_size = HEAP_STEP_SIZE;
#else
	do {
		heap_blocks[i].block_address = AllocateBiggestBlock(
			HEAP_START_SIZE_TOTAL, &heap_blocks[i].block_size, HEAP_STEP_SIZE, TRUE, FALSE
		);
		total_size += heap_blocks[i].block_size;
	} while (heap_blocks[i++].block_address != NULL);
#endif

	time_end = suPalReadTime();

	i -= 1;
	while (i-- > 0) {
		AmMemFreePointer(heap_blocks[i].block_address);
	}

	time_result = (uint32) suPalTicksToMsec(time_end - time_start);

	LOG("HEAP: Total time: %d\n", time_result);
	LOG("HEAP: Total size: %d\n", total_size);

	u_ltou(time_result, result->total);
	u_strcpy(result->total + u_strlen(result->total), L" ms | ");
	u_ltou(total_size, result->total + u_strlen(result->total));
	u_strcpy(result->total + u_strlen(result->total), L" B");

	{
		uint32 time_i;
		uint32 time_f;
		uint32 size_i;
		uint32 size_f;

		time_i = time_result / 1000;
		time_f = ((time_result % 1000) * 100) / 1000;

		size_i = total_size / 1024;
		size_f = ((total_size % 1024) * 100) / 1024;

		sprintf(float_string, "%lu.%02lu", time_i, time_f);
		u_atou(float_string, result->desc);
		u_strcpy(result->desc + u_strlen(result->desc), L" sec | ");

		sprintf(float_string, "%lu.%02lu", size_i, size_f);
		u_atou(float_string, result->desc + u_strlen(result->desc));
		u_strcpy(result->desc + u_strlen(result->desc), L" KiB");
	}

	return status;
}

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

uint32 DisksResult(WCHAR *disk_result) {
	uint32 i;
	uint32 status;
	WCHAR *result;
	VOLUME_DESCR_T volume_description;
	WCHAR volumes[MAX_VOLUMES_COUNT * 3];
	uint32 disk_count;
	WCHAR disks[LENGTH_VOLUME_NAME * MAX_VOLUMES_COUNT];

	disk_count = 0;
	status = RESULT_OK;

	/* Clean it first. */
	disk_result[0] = '\0';
	disk_result[1] = '\0';

	result = DL_FsVolumeEnum(volumes);
	if (!result) {
		u_strcpy(disk_result, L"Error: Cannot get list of disks!\n");
		return RESULT_FAIL;
	}

	for (i = 0; i < MAX_VOLUMES_COUNT; ++i) {
		disks[(i * 4) + 0] = volumes[i * 3];
		disks[(i * 4) + 1] = volumes[i * 3 + 1];
		disks[(i * 4) + 2] = volumes[i * 3];
		disks[(i * 4) + 3] = 0x0000;
		disk_count += 1;
		if (!volumes[i * 3 + 2]) {
			break;
		}
	}

	for (i = 0; i < disk_count; ++i) {
		char disk_a[LENGTH_VOLUME_NAME];
		WCHAR *disk_u = disks + (i * 4);
		uint32 free_size_memory;
		uint32 copy_address;

		u_utoa(disk_u, disk_a);
		if (!DL_FsGetVolumeDescr(disk_u, &volume_description)) {
			u_strcpy(disk_result, L"Error: Cannot get volume description of disk: ");
			u_strcat(disk_result, disk_u);
			u_strcat(disk_result, L"\n");
			return RESULT_FAIL;
		}

#if defined(EP1) || defined(EP2)
		free_size_memory = volume_description.free;
		copy_address = 0x03FC0000; /* iRAM */
#elif defined(EM1) || defined(EM2)
		free_size_memory = volume_description.free_size;
		copy_address = 0x08000000; /* iRAM or RAM */
#else
		free_size_memory = 0;
		copy_address = 0x00000000; /* iROM */
#endif

		LOG("Found volume: %s, free size: %d bytes.\n", disk_a, free_size_memory);
		u_strcat(disk_result, disk_u);
		u_strcat(disk_result, L" (128 KiB file):\n");
		if (free_size_memory < 0x40000) { /* At least 262144 bytes. */
			u_strcat(disk_result, L"No free 262144 bytes.");
		} else {
			DiskBenchmark(disk_result, disk_u, copy_address, 0x1000, 0x20000); /* Chunk: 4096  B, Size: 128 KiB. */
			DiskBenchmark(disk_result, disk_u, copy_address, 0x2000, 0x20000); /* Chunk: 8192  B, Size: 128 KiB. */
			DiskBenchmark(disk_result, disk_u, copy_address, 0x4000, 0x20000); /* Chunk: 16384 B, Size: 128 KiB. */
		}
	}

	if (!disk_count) {
		u_strcpy(disk_result, L"Error: No disks available!\n");
	}

	return status;
}

extern uint32 DiskBenchmark(WCHAR *disk_result, WCHAR *disk, uint32 addr, uint32 c_size, uint32 f_size) {
	uint32 i;
	uint32 rw;
	INT32 res;
	uint32 status;
	FILE_HANDLE_T file;
	WCHAR file_path[FS_MAX_URI_NAME_LENGTH / 4];
	char *read_buffer;
	uint64 time_start;
	uint64 time_end;
	uint32 time_result;
	WCHAR num_u[RESULT_STRING];

	status = RESULT_OK;

	u_strcpy(file_path, L"file:/");
	u_strcat(file_path, disk);
	u_strcat(file_path, L"BENCH.bin");

	u_ltou(c_size / 1024, num_u);
	u_strcat(disk_result, num_u);
	u_strcat(disk_result, L"K, ");

	status |= DeleteFileIfExists(file_path);

	/* Write File */
	time_start = suPalReadTime();
	file = DL_FsOpenFile(file_path, FILE_WRITE_MODE, NULL);
	for (i = addr; i < addr + f_size; i += c_size) {
		status |= DL_FsWriteFile((void *) i, c_size, 1, file, &rw);
		if (rw == 0) {
			status = RESULT_FAIL;
		}
	}
	status |= DL_FsCloseFile(file);

	time_end = suPalReadTime();
	time_result = (uint32) suPalTicksToMsec(time_end - time_start);

	u_ltou(time_result, num_u);
	u_strcat(disk_result, L"W:");
	u_strcat(disk_result, num_u);
	u_strcat(disk_result, L", ");

	/* Read File */
	read_buffer = suAllocMem(c_size, &res);
	time_start = suPalReadTime();
	if (res == RESULT_OK) { /* No Allocation Error. */
		file = DL_FsOpenFile(file_path, FILE_READ_MODE, NULL);
		for (i = addr; i < addr + f_size; i += c_size) {
			status = DL_FsReadFile((void *) read_buffer, c_size, 1, file, &rw);
			if (rw == 0) {
				status = RESULT_FAIL;
			}
		}
		status |= DL_FsCloseFile(file);
		time_end = suPalReadTime();
		time_result = (uint32) suPalTicksToMsec(time_end - time_start);
		suFreeMem(read_buffer);

		u_ltou(time_result, num_u);
		u_strcat(disk_result, L"R:");
		u_strcat(disk_result, num_u);
		u_strcat(disk_result, L", ms.\n");
	} else {
		u_strcat(disk_result, L"Error: Can allocate chunk buffer.\n");
	}

	status |= DeleteFileIfExists(file_path);

	return status;
}

static uint32 DeleteFileIfExists(const AECHAR *file_path) {
	uint32 status;

	status = RESULT_FAIL;

	if (DL_FsFFileExist(file_path)) {
		status = DL_FsDeleteFile(file_path, 0);
	}

	return status;
}
#endif
