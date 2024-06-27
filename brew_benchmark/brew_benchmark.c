/*
 * About:
 *   A simple ELF benchmarking application for Qualcomm BREW platform written using AEE framework.
 *
 * Author:
 *   EXL
 *
 * License:
 *   MIT
 *
 * Additional information:
 *   1. starting_brew.pdf
 *   2. https://github.com/usernameak/brew-cmake-toolchain
 */

#include <AEEAppGen.h>
#include <AEEShell.h>
#include <AEEStdLib.h>
#include <AEEMenu.h>
#include <AEEFile.h>

#include "brew_benchmark.bid"
#include "brew_benchmark.brh"

#include "benchmark.h"

#define WSTR_TITLE_MAX                                   (64)
#define WSTR_TEXT_MAX                                   (128)
#define WSTR_LONG_TEXT_MAX                             (1024)
#define SHOW_NOTIFICATION_DELAY_MS                     (1200) /* 1.2 seconds. */
#define SHOW_NOTIFICATION_IMMEDIATELY_MS                  (1) /* 1 ms. */

typedef enum {
	APP_STATE_MENU_MAIN,
	APP_STATE_NOTIFICATION,
	APP_STATE_STATIC_TEXT,
	APP_STATE_ABOUT,
	APP_STATE_MAX
} APP_STATE_T;

typedef enum {
	APP_MENU_ITEM_CPU,
	APP_MENU_ITEM_GPU,
	APP_MENU_ITEM_RAM,
	APP_MENU_ITEM_HEAP,
	APP_MENU_ITEM_DISK,
	APP_MENU_ITEM_HELP,
	APP_MENU_ITEM_ABOUT,
	APP_MENU_ITEM_EXIT,
	APP_MENU_ITEM_MAX
} APP_MENU_T;

typedef struct {
	AEERect m_RectScreen;

	int16 m_ScreenW;
	int16 m_ScreenH;

	uint16 m_OffsetX;
	uint16 m_OffsetY;
	uint16 m_OffsetW;
	uint16 m_OffsetH;
} APP_DEVICE_T;

typedef struct {
	AEEApplet m_App;

	APP_MENU_T m_AppMenu;
	APP_STATE_T m_AppState;
	APP_DEVICE_T m_AppDevice;

	IFileMgr *m_pIFileMgr;
	IMenuCtl *m_pIMenuMainCtl;
	IStatic *m_pIStatic;

	BENCHMARK_RESULTS_CPU_T cpu_result;
} APP_INSTANCE_T;

static boolean APP_InitAppData(AEEApplet *pMe);
static boolean APP_FreeAppData(AEEApplet *pMe);
static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam);
static boolean APP_DeviceFill(AEEApplet *pMe);

static boolean APP_MenuMainInit(AEEApplet *pMe);
static boolean APP_ShowNotification(
	AEEApplet *pMe, const AECHAR *aTitle, const AECHAR *aText, const AECHAR *aDesc,
	int32 aDelay, PFNNOTIFY aFunc
);
static boolean APP_ShowHelp(AEEApplet *pMe);
static boolean APP_ShowAbout(AEEApplet *pMe);

static boolean APP_ShowMainMenu(AEEApplet *pMe);
static boolean APP_ShowResults(AEEApplet *pMe);
static boolean APP_BenchmarkingInProgress(AEEApplet *pMe);

const AECHAR *wstr_lbl_title = L"Benchmark";

AEEResult AEEClsCreateInstance(AEECLSID ClsId, IShell *pIShell, IModule *pMod, void **ppObj) {
	*ppObj = NULL;
	if (ClsId == AEECLSID_BREW_BENCHMARK) {
		if(
			AEEApplet_New(
				sizeof(APP_INSTANCE_T),                     // Size of our private class.
				ClsId,                                      // Our class ID.
				pIShell,                                    // Shell interface.
				pMod,                                       // Module instance.
				(IApplet **) ppObj,                         // Return object.
				(AEEHANDLER) APP_HandleEvent,               // Our event handler.
				(PFNFREEAPPDATA) APP_FreeAppData            // Special "cleanup" function.
			)
		) {
			if (APP_InitAppData((AEEApplet *) *ppObj)) {
				return AEE_SUCCESS;
			} else {
				IAPPLET_Release((IApplet *) *ppObj);
				return AEE_EFAILED;
			}
		}
	}
	return AEE_EFAILED;
}

static boolean APP_InitAppData(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	APP_DeviceFill(pMe);
	app->m_AppState = APP_STATE_MENU_MAIN;

	if (ISHELL_CreateInstance(app->m_App.m_pIShell, AEECLSID_MENUCTL, (void **) &app->m_pIMenuMainCtl) != AEE_SUCCESS) {
		return FALSE;
	}
	if (ISHELL_CreateInstance(app->m_App.m_pIShell, AEECLSID_FILEMGR, (void **) &app->m_pIFileMgr) != AEE_SUCCESS) {
		return FALSE;
	}
	if (ISHELL_CreateInstance(app->m_App.m_pIShell, AEECLSID_STATIC, (void **) &app->m_pIStatic) != AEE_SUCCESS) {
		return FALSE;
	}

	return TRUE;
}

static boolean APP_FreeAppData(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	IMENUCTL_Release(app->m_pIMenuMainCtl);
	IFILEMGR_Release(app->m_pIFileMgr);
	ISTATIC_Release(app->m_pIStatic);

	return TRUE;
}

static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	switch (eCode) {
		case EVT_APP_START:
			APP_MenuMainInit(pMe);
			return APP_ShowMainMenu(pMe);
		case EVT_APP_STOP:
			return TRUE;
		case EVT_DIALOG_END:
		case EVT_COPYRIGHT_END:
			return APP_ShowMainMenu(pMe);
		case EVT_COMMAND:
			switch (wParam) {
				case APP_MENU_ITEM_CPU:
					app->m_AppMenu = APP_MENU_ITEM_CPU;
					return APP_BenchmarkingInProgress(pMe);
				case APP_MENU_ITEM_GPU:
					app->m_AppMenu = APP_MENU_ITEM_GPU;
					return APP_BenchmarkingInProgress(pMe);
				case APP_MENU_ITEM_RAM:
					app->m_AppMenu = APP_MENU_ITEM_RAM;
					return APP_BenchmarkingInProgress(pMe);
				case APP_MENU_ITEM_HEAP:
					app->m_AppMenu = APP_MENU_ITEM_HEAP;
					return APP_BenchmarkingInProgress(pMe);
				case APP_MENU_ITEM_DISK:
					app->m_AppMenu = APP_MENU_ITEM_DISK;
					return APP_BenchmarkingInProgress(pMe);
				case APP_MENU_ITEM_HELP:
					return APP_ShowHelp(pMe);
				case APP_MENU_ITEM_ABOUT:
					return APP_ShowAbout(pMe);
				case APP_MENU_ITEM_EXIT:
					return ISHELL_CloseApplet(app->m_App.m_pIShell, FALSE);
				default:
					break;
			}
			break;
		case EVT_KEY:
			switch (app->m_AppState) {
				case APP_STATE_MENU_MAIN:
					if (wParam == AVK_CLR || wParam == AVK_SOFT2) {
						return ISHELL_CloseApplet(app->m_App.m_pIShell, FALSE);
					} else {
						return IMENUCTL_HandleEvent(app->m_pIMenuMainCtl, eCode, wParam, dwParam);
					}
					break;
				case APP_STATE_STATIC_TEXT:
					if (wParam == AVK_CLR || wParam == AVK_SELECT || wParam == AVK_SOFT2) {
						ISTATIC_SetActive(app->m_pIStatic, FALSE);
						return APP_ShowMainMenu(pMe);
					} else {
						return ISTATIC_HandleEvent(app->m_pIStatic, eCode, wParam, dwParam);
					}
				default:
					break;
			}
			break;
		default:
			break;
	}

	return FALSE;
}

static boolean APP_DeviceFill(AEEApplet *pMe) {
	AEEDeviceInfo device_info;
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	device_info.wStructSize = sizeof(AEEDeviceInfo);
	ISHELL_GetDeviceInfo(app->m_App.m_pIShell, &device_info);

	app->m_AppDevice.m_ScreenW = device_info.cxScreen;
	app->m_AppDevice.m_ScreenH = device_info.cyScreen;

	SETAEERECT(&app->m_AppDevice.m_RectScreen, 0, 0, app->m_AppDevice.m_ScreenW, app->m_AppDevice.m_ScreenH);

	switch (app->m_AppDevice.m_ScreenW) {
		default:
		case 240: /* Precalculated 240x320 values. */
			app->m_AppDevice.m_OffsetX = 6;
			app->m_AppDevice.m_OffsetY = 6;
			app->m_AppDevice.m_OffsetW = 10;
			app->m_AppDevice.m_OffsetH = 6;
			break;
		case 176: /* Precalculated 176x220 values. */
			app->m_AppDevice.m_OffsetX = 4;
			app->m_AppDevice.m_OffsetY = 4;
			app->m_AppDevice.m_OffsetW = 10;
			app->m_AppDevice.m_OffsetH = 4;
			break;
	}

	return TRUE;
}

static boolean APP_MenuMainInit(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	CtlAddItem menu_item;

	IMENUCTL_SetRect(app->m_pIMenuMainCtl, &app->m_AppDevice.m_RectScreen);
	IMENUCTL_SetTitle(app->m_pIMenuMainCtl, BREW_BENCHMARK_RES_FILE, IDS_MENU_TITLE_MAIN, NULL);
	IMENUCTL_SetProperties(app->m_pIMenuMainCtl, MP_UNDERLINE_TITLE | MP_WRAPSCROLL);

	menu_item.pText = NULL;
	menu_item.pImage = NULL;
	menu_item.pszResText = BREW_BENCHMARK_RES_FILE;
	menu_item.pszResImage = BREW_BENCHMARK_RES_FILE;
	menu_item.wFont = AEE_FONT_NORMAL;
	menu_item.dwData = 0;

	menu_item.wText = IDS_MENU_ITEM_CPU;
	menu_item.wImage = IDI_ICON_GENERAL;
	menu_item.wItemID = APP_MENU_ITEM_CPU;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_GPU;
	menu_item.wImage = IDI_ICON_GENERAL;
	menu_item.wItemID = APP_MENU_ITEM_GPU;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_RAM;
	menu_item.wImage = IDI_ICON_GENERAL;
	menu_item.wItemID = APP_MENU_ITEM_RAM;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_HEAP;
	menu_item.wImage = IDI_ICON_GENERAL;
	menu_item.wItemID = APP_MENU_ITEM_HEAP;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_DISK;
	menu_item.wImage = IDI_ICON_GENERAL;
	menu_item.wItemID = APP_MENU_ITEM_DISK;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_HELP;
	menu_item.wImage = IDI_ICON_HELP;
	menu_item.wItemID = APP_MENU_ITEM_HELP;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_ABOUT;
	menu_item.wImage = IDI_ICON_ABOUT;
	menu_item.wItemID = APP_MENU_ITEM_ABOUT;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	menu_item.wText = IDS_MENU_ITEM_EXIT;
	menu_item.wImage = IDI_ICON_EXIT;
	menu_item.wItemID = APP_MENU_ITEM_EXIT;
	IMENUCTL_AddItemEx(app->m_pIMenuMainCtl, &menu_item);

	IMENUCTL_EnableCommand(app->m_pIMenuMainCtl, TRUE);

	return TRUE;
}

static boolean APP_ShowNotification(
	AEEApplet *pMe, const AECHAR *aTitle, const AECHAR *aText, const AECHAR *aDesc,
	int32 aDelay, PFNNOTIFY aFunc
) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	AEERect rNot;
	int nAscent;
	int nDescent;
	int text_h;

	app->m_AppState = APP_STATE_NOTIFICATION;
	IMENUCTL_SetActive(app->m_pIMenuMainCtl, FALSE);

	IDISPLAY_GetFontMetrics(app->m_App.m_pIDisplay, AEE_FONT_BOLD, &nAscent, &nDescent);
	text_h = nAscent + nDescent;
	SETAEERECT(&rNot,
		app->m_AppDevice.m_OffsetX,
		(app->m_AppDevice.m_RectScreen.dy / 2) - text_h * 2,
		app->m_AppDevice.m_RectScreen.dx - (app->m_AppDevice.m_OffsetX * 2),
		text_h * 4
	);

	IDISPLAY_ClearScreen(app->m_App.m_pIDisplay);

	IDISPLAY_DrawRect(app->m_App.m_pIDisplay, &rNot, RGB_BLACK, RGB_WHITE, IDF_RECT_FRAME | IDF_RECT_FILL);

	rNot.x  += 1;
	rNot.dx -= 1 * 2;
	rNot.y  += 2;
	rNot.dy -= 2 * 2;

	IDISPLAY_DrawText(app->m_App.m_pIDisplay, AEE_FONT_BOLD, aTitle, -1, 0, 0, &rNot,
		IDF_ALIGN_CENTER | IDF_ALIGN_TOP);
	IDISPLAY_DrawText(app->m_App.m_pIDisplay, AEE_FONT_NORMAL, aText, -1, 0, 0, &rNot,
		IDF_ALIGN_CENTER | IDF_ALIGN_MIDDLE);
	if (aDesc) {
		IDISPLAY_DrawText(app->m_App.m_pIDisplay, AEE_FONT_NORMAL, aDesc, -1, 0, 0, &rNot,
			IDF_ALIGN_CENTER | IDF_ALIGN_BOTTOM);
	}

	IDISPLAY_Update(app->m_App.m_pIDisplay);
	ISHELL_SetTimer(app->m_App.m_pIShell, aDelay, aFunc, (void *) pMe);

	return TRUE;
}

static boolean APP_ShowHelp(AEEApplet *pMe) {
	const uint32 text_size = sizeof(AECHAR) * WSTR_LONG_TEXT_MAX; /* 2048 bytes, 1024 characters. */
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	AECHAR title[WSTR_TITLE_MAX];
	AECHAR *text;
	boolean load_ok;

	app->m_AppState = APP_STATE_STATIC_TEXT;
	IMENUCTL_SetActive(app->m_pIMenuMainCtl, FALSE);

	IDISPLAY_ClearScreen(app->m_App.m_pIDisplay);

	text = (AECHAR *) MALLOC(text_size);
	load_ok = ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE, IDS_HELP_TITLE, title, WSTR_TITLE_MAX);
	load_ok = ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE, IDS_HELP_TEXT, text, text_size);

	if (!text || !load_ok) {
		if (text) {
			FREE(text);
		}
		return FALSE;
	}

	ISTATIC_SetProperties(app->m_pIStatic, ST_TEXTALLOC | ST_NOSCROLL);
	ISTATIC_SetText(app->m_pIStatic, title, text, AEE_FONT_BOLD, AEE_FONT_NORMAL);
	ISTATIC_SetActive(app->m_pIStatic, TRUE);
	ISTATIC_Redraw(app->m_pIStatic);

	return TRUE;
}

static boolean APP_ShowAbout(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	app->m_AppState = APP_STATE_ABOUT;
	IMENUCTL_SetActive(app->m_pIMenuMainCtl, FALSE);

	IDISPLAY_ClearScreen(app->m_App.m_pIDisplay);

	ISHELL_ShowCopyright(app->m_App.m_pIShell);

	return TRUE;
}

static boolean APP_ShowMainMenu(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	app->m_AppState = APP_STATE_MENU_MAIN;
	IMENUCTL_SetActive(app->m_pIMenuMainCtl, TRUE);

	return TRUE;
}

static boolean APP_ShowResults(AEEApplet *pMe) {
	const uint32 text_size = sizeof(AECHAR) * WSTR_LONG_TEXT_MAX; /* 2048 bytes, 1024 characters. */
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	AECHAR title[WSTR_TITLE_MAX];
	AECHAR *text;
	boolean load_ok;
	uint16 aStrResId;

	app->m_AppState = APP_STATE_STATIC_TEXT;

	switch (app->m_AppMenu) {
		case APP_MENU_ITEM_CPU:  aStrResId = IDS_RESULT_CPU;   break;
		case APP_MENU_ITEM_GPU:  aStrResId = IDS_RESULT_GPU;   break;
		case APP_MENU_ITEM_RAM:  aStrResId = IDS_RESULT_RAM;   break;
		case APP_MENU_ITEM_HEAP: aStrResId = IDS_RESULT_HEAP;  break;
		case APP_MENU_ITEM_DISK: aStrResId = IDS_RESULT_DISK;  break;
		default:                 aStrResId = IDS_RESULT_ERROR; break;
	}

	text = (AECHAR *) MALLOC(text_size);
	load_ok = ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE, aStrResId, title, WSTR_TITLE_MAX);

	if (!text || !load_ok) {
		if (text) {
			FREE(text);
		}
		return FALSE;
	}

	switch (app->m_AppMenu) {
		case APP_MENU_ITEM_CPU:
			BogoMIPS(&app->cpu_result);
			Dhrystone(&app->cpu_result);
			WSPRINTF(text, sizeof(AECHAR) * 1024, L"%s\n%s\n\n%s\n%s\n%s",
				app->cpu_result.bogo_time, app->cpu_result.bogo_mips,
				app->cpu_result.dhrys_mips, app->cpu_result.dhrys_time, app->cpu_result.dhrys_score);
			break;
		default:
			ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE,
				IDS_NOT_IMPLEMENTED, text, WSTR_TITLE_MAX);
			break;
	}

	ISTATIC_SetProperties(app->m_pIStatic, ST_TEXTALLOC | ST_NOSCROLL);
	ISTATIC_SetText(app->m_pIStatic, title, text, AEE_FONT_BOLD, AEE_FONT_NORMAL);
	ISTATIC_SetActive(app->m_pIStatic, TRUE);
	ISTATIC_Redraw(app->m_pIStatic);

	return TRUE;
}

static boolean APP_BenchmarkingInProgress(AEEApplet *pMe) {
	const uint32 s = sizeof(AECHAR);
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	AECHAR title[WSTR_TITLE_MAX];
	AECHAR text[WSTR_TEXT_MAX];

	ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE, IDS_TITLE_WAIT, title, s * WSTR_TITLE_MAX);
	if (app->m_AppDevice.m_ScreenW < 240) {
		ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE,
			IDS_BENCHMARKING_SHORT, text, s * WSTR_TEXT_MAX);
	} else {
		ISHELL_LoadResString(app->m_App.m_pIShell, BREW_BENCHMARK_RES_FILE,
			IDS_BENCHMARKING_LONG, text, s * WSTR_TEXT_MAX);
	}

	APP_ShowNotification(pMe, title, text, NULL, SHOW_NOTIFICATION_IMMEDIATELY_MS, (PFNNOTIFY) APP_ShowResults);

	return TRUE;
}
