#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <AEEStdLib.h>

#define RESULT_STRING                  (64)

/* CPU Benchmark */

#define TICKS_PER_SEC                  (1000)
#define DHRYSTONE_LOOP_RUNS            (20000) /* 2000 - Import from GBA */
#define DMIPS_VAX_11_780_CONST         (1757)

typedef struct {
	AECHAR bogo_time[RESULT_STRING];
	AECHAR bogo_mips[RESULT_STRING];

	AECHAR dhrys_time[RESULT_STRING];
	AECHAR dhrys_score[RESULT_STRING];
	AECHAR dhrys_mips[RESULT_STRING];
} BENCHMARK_RESULTS_CPU_T;

extern void delay_bmips(uint32 loops);

extern boolean BogoMIPS(BENCHMARK_RESULTS_CPU_T *result);

extern int Dhrystone(BENCHMARK_RESULTS_CPU_T *result);

/* GPU Benchmark */

/* Pass 1 */
#define BITMAP_WIDTH_LOW               (32)
#define BITMAP_HEIGHT_LOW              (24)

/* Pass 2 */
#define BITMAP_WIDTH_MID               (48)
#define BITMAP_HEIGHT_MID              (32)

/* Pass 3 */
#define BITMAP_WIDTH_HIGH              (64)
#define BITMAP_HEIGHT_HIGH             (48)

#define START_Y_COORD                  (220)
#define MAX_FPS_COUNT                  (64)

typedef struct {
	uint16 width;
	uint16 height;
	uint16 bmp_width;
	uint16 bmp_height;

	uint8 *p_fire;
	uint16 y_coord;
	boolean flag_restart_demo;
} APP_BITMAP_T;

typedef struct {
	uint16 fps_i;
	uint16 fps_f;
} FPS_T;

typedef struct {
	uint16 size;
	FPS_T values[MAX_FPS_COUNT];
} FPS_VALUES_T;

typedef struct {
	AECHAR fps_pass1[RESULT_STRING];
	AECHAR fms_pass1[RESULT_STRING];
	AECHAR fps_pass2[RESULT_STRING];
	AECHAR fms_pass2[RESULT_STRING];
	AECHAR fps_pass3[RESULT_STRING];
	AECHAR fms_pass3[RESULT_STRING];
	AECHAR properties[RESULT_STRING * 4];
} BENCHMARK_RESULTS_GPU_T;

extern uint32 GFX_Draw_Start(APP_BITMAP_T *ahi);
extern uint32 GFX_Draw_Step(APP_BITMAP_T *ahi);
extern uint32 GFX_Draw_Stop(APP_BITMAP_T *ahi);

extern void FPS_Meter(void);

extern uint32 CalculateAverageFpsAndTime(AECHAR *result_fps, AECHAR *result_fms);

extern uint32 Bench_GPU_Passes(uint32 bmp_width, uint32 bmp_height, AECHAR *fps, AECHAR *fms, AECHAR *props);

/* RAM Benchmark */

#define RAM_TOP_BLOCKS_COUNT           (6)
#define RAM_STEP_SIZE                  (256)
#if defined(EM1) || defined(EM2)
#define RAM_TOTAL_BLOCKS_COUNT         (1024)
#else
#define RAM_TOTAL_BLOCKS_COUNT         (512)
#endif
#define RAM_START_SIZE_TOTAL           (RAM_STEP_SIZE * 4)
#define RAM_START_SIZE_BLOCK           (RAM_STEP_SIZE * 8)

typedef struct {
	AECHAR total[RESULT_STRING];
	AECHAR blocks[RAM_TOP_BLOCKS_COUNT][RESULT_STRING];
} BENCHMARK_RESULTS_RAM_T;

typedef struct {
	void *block_address;
	uint32 block_size;
	uint32 block_time;
} RAM_ALLOCATED_BLOCK_T;

extern uint32 TopOfBiggestRamBlocks(BENCHMARK_RESULTS_RAM_T *result, boolean uis);
extern uint32 TotalRamSize(BENCHMARK_RESULTS_RAM_T *result, boolean uis);

/* Java Heap Benchmark */

/* BUG: Is the M-CORE 7 MiB Heap patch bugged? `AmMemAllocPointer()` can only be called a few times. */
#if defined(EM1) || defined(EM2)
#define HEAP_STEP_SIZE                 (5591256 - 393216)
#else
#define HEAP_STEP_SIZE                 (4096)
#endif
#define HEAP_TOTAL_BLOCKS_COUNT        (512)
#define HEAP_START_SIZE_TOTAL          (HEAP_STEP_SIZE * 8)

typedef struct {
	AECHAR total[RESULT_STRING];
	AECHAR desc[RESULT_STRING];
} BENCHMARK_RESULTS_HEAP_T;

typedef struct {
	void *block_address;
	uint32 block_size;
} HEAP_ALLOCATED_BLOCK_T;

extern uint32 TotalHeapSize(BENCHMARK_RESULTS_HEAP_T *result);

/* Disk Benchmark */

#define MAX_VOLUMES_COUNT              (5)
#define LENGTH_VOLUME_NAME             (3 + 1) /* '/a/\0' */

extern uint32 DisksResult(AECHAR *disk_result);
extern uint32 DiskBenchmark(AECHAR *disk_result, AECHAR *disk, uint32 addr, uint32 c_size, uint32 f_size);

#endif // BENCHMARK_H
