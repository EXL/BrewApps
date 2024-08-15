/*
 * About:
 *   The "Fire Effect" demo is a port of Doom PSX fire splash screen implemented as BREW application.
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

#include <AEEBitmap.h>

#include "brew_fire_effect.bid"

#define START_Y_COORD                     (220)

typedef struct {
	AEEApplet m_App;

	uint16 m_ScreenW;
	uint16 m_ScreenH;

	AEERect m_ScreenRect;

	IDIB *m_pIDIB;

	uint16 y_coord;
	boolean flag_restart_demo;
} APP_INSTANCE_T;

static boolean APP_InitAppData(AEEApplet *pMe);
static boolean APP_FreeAppData(AEEApplet *pMe);
static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam);
static boolean APP_DeviceFill(AEEApplet *pMe);

static const uint32 fire_palette[] = {
	0x07070700, /*  0 */
	0x1F070700, /*  1 */
	0x2F0F0700, /*  2 */
	0x470F0700, /*  3 */
	0x57170700, /*  4 */
	0x671F0700, /*  5 */
	0x771F0700, /*  6 */
	0x8F270700, /*  7 */
	0x9F2F0700, /*  8 */
	0xAF3F0700, /*  9 */
	0xBF470700, /* 10 */
	0xC7470700, /* 11 */
	0xDF4F0700, /* 12 */
	0xDF570700, /* 13 */
	0xDF570700, /* 14 */
	0xD75F0700, /* 15 */
	0xD75F0700, /* 16 */
	0xD7670F00, /* 17 */
	0xCF6F0F00, /* 18 */
	0xCF770F00, /* 19 */
	0xCF7F0F00, /* 20 */
	0xCF871700, /* 21 */
	0xC7871700, /* 22 */
	0xC78F1700, /* 23 */
	0xC7971F00, /* 24 */
	0xBF9F1F00, /* 25 */
	0xBF9F1F00, /* 26 */
	0xBFA72700, /* 27 */
	0xBFA72700, /* 28 */
	0xBFAF2F00, /* 29 */
	0xB7AF2F00, /* 30 */
	0xB7B72F00, /* 31 */
	0xB7B73700, /* 32 */
	0xCFCF6F00, /* 33 */
	0xDFDF9F00, /* 34 */
	0xEFEFC700, /* 35 */
	0xFFFFFF00  /* 36 */
};

AEEResult AEEClsCreateInstance(AEECLSID ClsId, IShell *pIShell, IModule *pMod, void **ppObj) {
	*ppObj = NULL;
	if (ClsId == AEECLSID_BREW_FIRE_EFFECT) {
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
	AEEDeviceInfo device_info;

	device_info.wStructSize = sizeof(AEEDeviceInfo);
	ISHELL_GetDeviceInfo(app->m_App.m_pIShell, &device_info);

	app->m_ScreenW = 64 /*device_info.cxScreen*/;
	app->m_ScreenH = 48 /*device_info.cyScreen*/;

	SETAEERECT(&app->m_ScreenRect, 0, 0, app->m_ScreenW, app->m_ScreenH);

	if (IDISPLAY_CreateDIBitmap(app->m_App.m_pIDisplay, &app->m_pIDIB, 8, app->m_ScreenW, app->m_ScreenH)) {
		return FALSE;
	}
	app->m_pIDIB->nColorScheme = IDIB_COLORSCHEME_NONE;
	app->m_pIDIB->cntRGB = sizeof(fire_palette) / sizeof(fire_palette[0]);
	app->m_pIDIB->pRGB = (uint32 *) fire_palette;

	app->y_coord = START_Y_COORD;
	app->flag_restart_demo = FALSE;

	return TRUE;
}

static boolean APP_FreeAppData(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	IDIB_Release(app->m_pIDIB);

	return TRUE;
}

static inline int rand(void) {
	int random_number;
	GETRAND((byte *) &random_number, sizeof(int));
	return random_number;
}

static boolean GFX_Draw_Start(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	app->y_coord = START_Y_COORD;

	/* Fill all screen to RGB(0x07, 0x07, 0x07) except last line. */
	MEMSET(app->m_pIDIB->pBmp, 0, app->m_ScreenW * (app->m_ScreenH - 1));

	/* Fill last line to RGB(0xFF, 0xFF, 0xFF). */
	MEMSET((uint8 *) (app->m_pIDIB->pBmp + (app->m_ScreenH - 1) * app->m_ScreenW), 36, app->m_ScreenW);

	IDISPLAY_BitBlt(
		app->m_App.m_pIDisplay, 0, 0, app->m_ScreenW, app->m_ScreenH, IDIB_TO_IBITMAP(app->m_pIDIB), 0, 0, AEE_RO_COPY
	);

	return TRUE;
}

static boolean Fire_Demo_Is_Screen_Empty(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	uint16 i;
	uint16 stop;

	stop = app->m_ScreenW * app->m_ScreenH;
	for (i = 0; i < stop; ++i) {
		if (app->m_pIDIB->pBmp[i]) {
			return FALSE;
		}
	}

	return TRUE;
}

static boolean GFX_Draw_Step(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	uint16 x;
	uint16 y;
	uint16 start;
	uint16 stop;

	if (app->flag_restart_demo) {
		GFX_Draw_Start(pMe);
		app->flag_restart_demo = FALSE;
		return TRUE;
	}

	for (x = 0; x < app->m_ScreenW; ++x) {
		for (y = 1; y < app->m_ScreenH; ++y) {
			const uint8 pixel = app->m_pIDIB->pBmp[y * app->m_ScreenW + x];
			if (pixel == 0) {
				app->m_pIDIB->pBmp[(y * app->m_ScreenW + x) - app->m_ScreenW] = 0;
			} else {
				const uint8 rand_idx = rand() % 4;
				const uint16 destination = (y * app->m_ScreenW + x) - rand_idx + 1;
				app->m_pIDIB->pBmp[destination - app->m_ScreenW] = pixel - (rand_idx & 1);
			}
		}
	}

	start = app->m_ScreenH - 1;
	stop = app->m_ScreenH - 8;

	if (app->y_coord != app->m_ScreenH / 4) {
		app->y_coord -= 2;
	} else {
		for(y = start; y > stop; --y) {
			for(x = 0; x < app->m_ScreenW; ++x) {
				if (app->m_pIDIB->pBmp[y * app->m_ScreenW + x] > 0) {
					app->m_pIDIB->pBmp[y * app->m_ScreenW + x] -= ((rand() % 2) & 3);
				}
			}
		}
		app->flag_restart_demo = Fire_Demo_Is_Screen_Empty(pMe);
	}

	IDISPLAY_BitBlt(
		app->m_App.m_pIDisplay, 0, 0, app->m_ScreenW, app->m_ScreenH, IDIB_TO_IBITMAP(app->m_pIDIB), 0, 0, AEE_RO_COPY
	);

	return TRUE;
}

static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	switch (eCode) {
		case EVT_APP_START:
//			IDISPLAY_SetColor(app->m_App.m_pIDisplay, CLR_USER_BACKGROUND, RGB_WHITE);
//			IDISPLAY_ClearScreen(app->m_App.m_pIDisplay);
			GFX_Draw_Start(pMe);
			IDISPLAY_Update(app->m_App.m_pIDisplay);
			return TRUE;
		case EVT_APP_STOP:
			return TRUE;
		case EVT_KEY:
			switch (wParam) {
				case AVK_SOFT1:
					ISHELL_CloseApplet(app->m_App.m_pIShell, FALSE);
					break;
				case AVK_SOFT2:
					GFX_Draw_Step(pMe);
					IDISPLAY_Update(app->m_App.m_pIDisplay);
					break;
				default:
					break;
			}
			return TRUE;
		case EVT_KEY_PRESS:
			break;
		case EVT_KEY_RELEASE:
			break;
		default:
			break;
	}

	return FALSE;
}
