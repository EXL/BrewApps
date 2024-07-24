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

typedef struct {
	AEEApplet m_App;

	uint16 m_ScreenW;
	uint16 m_ScreenH;

	AEERect m_ScreenRect;

	IDIB *m_pIDIB;
} APP_INSTANCE_T;

static boolean APP_InitAppData(AEEApplet *pMe);
static boolean APP_FreeAppData(AEEApplet *pMe);
static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam);
static boolean APP_DeviceFill(AEEApplet *pMe);

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

	app->m_ScreenH = device_info.cxScreen;
	app->m_ScreenW = device_info.cyScreen;

	SETAEERECT(&app->m_ScreenRect, 0, 0, app->m_ScreenH, app->m_ScreenW);

	if (IDISPLAY_CreateDIBitmap(app->m_App.m_pIDisplay, &app->m_pIDIB, 16, app->m_ScreenW, app->m_ScreenH)) {
		return FALSE;
	}
	app->m_pIDIB->nColorScheme = IDIB_COLORSCHEME_565;

	return TRUE;
}

static boolean APP_FreeAppData(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	IDIB_Release(app->m_pIDIB);

	return TRUE;
}

static inline byte rand(void) {
	byte random_number;
	GETRAND((byte *) &random_number, sizeof(byte));
	return random_number;
}

static boolean fill_test(AEEApplet *pMe) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;
	int x = rand(), y;

	for (y = 0; y <= app->m_ScreenH * app->m_ScreenW * 2; ++y) {
		app->m_pIDIB->pBmp[y] = x;
	}

	return TRUE;
}

static boolean APP_HandleEvent(AEEApplet *pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam) {
	APP_INSTANCE_T *app = (APP_INSTANCE_T *) pMe;

	switch (eCode) {
		case EVT_APP_START:
			IDISPLAY_SetColor(app->m_App.m_pIDisplay, CLR_USER_BACKGROUND, RGB_BLACK);
			IDISPLAY_ClearScreen(app->m_App.m_pIDisplay);
			IDISPLAY_Update(app->m_App.m_pIDisplay);
			return TRUE;
		case EVT_APP_STOP:
			return TRUE;
		case EVT_KEY:
			switch (wParam) {
				case AVK_SOFT1:
					ISHELL_CloseApplet(app->m_App.m_pIShell, FALSE);
				case AVK_SOFT2:
					fill_test(pMe);
					IDISPLAY_BitBlt(app->m_App.m_pIDisplay,
						0, 0, app->m_ScreenW, app->m_ScreenW,
						IDIB_TO_IBITMAP(app->m_pIDIB), 0, 0, AEE_RO_COPY);
					IDISPLAY_Update(app->m_App.m_pIDisplay);
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
