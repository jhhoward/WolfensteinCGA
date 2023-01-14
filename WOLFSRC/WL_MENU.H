//
// WL_MENU.H
//
#ifdef SPEAR

#define BORDCOLOR	0x99
#define BORD2COLOR	0x93
#define DEACTIVE	0x9b
#define BKGDCOLOR	0x9d
//#define STRIPE		0x9c

#define MenuFadeOut()	VL_FadeOut(0,255,0,0,51,10)

#else

#ifdef WITH_VGA
#define BORDCOLOR	0x29
#define BORD2COLOR	0x23
#define DEACTIVE	0x2b
#define BKGDCOLOR	0x2d
#define STRIPE		0x2c
#define VIEWPORTBORDER 127
#define PG13BG		0x82
#define BACKCOLOR		0x11
#else
#define BORDCOLOR	menucolors[cgamode].bordcolor
#define BORD2COLOR	menucolors[cgamode].bordcolor2
#define DEACTIVE	(0x33cc)
#define BKGDCOLOR	menucolors[cgamode].bkgdcolor
#define STRIPE		menucolors[cgamode].stripe
#define VIEWPORTBORDER	menucolors[cgamode].viewportborder
#define PG13BG	menucolors[cgamode].pg13bg
#define BACKCOLOR		menucolors[cgamode].backcolor

#endif

#define MenuFadeOut()	VL_FadeOut(0,255,43,0,0,10)

#endif

#ifdef WITH_VGA
#define READCOLOR    0x4a
#define READHCOLOR    0x47
#define VIEWCOLOR    0x7f
#define TEXTCOLOR    0x17	
#define HIGHLIGHT    0x13
#else
#define READCOLOR	menucolors[cgamode].readcolor
#define READHCOLOR	menucolors[cgamode].readhcolor
#define VIEWCOLOR	menucolors[cgamode].viewcolor
#define TEXTCOLOR	menucolors[cgamode].textcolor
#define HIGHLIGHT	menucolors[cgamode].highlight
#define SIGNONFILL	menucolors[cgamode].signonfill
#define _READCOLOR    0xffff
#define _READHCOLOR    0xffff
#define _VIEWCOLOR    0xffff
#define _TEXTCOLOR    0xffff
#define _HIGHLIGHT    0xffff
#endif

#define MenuFadeIn()	VL_FadeIn(0,255,&gamepal,10)

typedef struct {
		unsigned bordcolor;
		unsigned bordcolor2;
		unsigned bkgdcolor;
		unsigned stripe;
		unsigned readcolor;
		unsigned readhcolor;
		unsigned viewcolor;
		unsigned textcolor;
		unsigned highlight;
		unsigned viewportborder;
		unsigned pg13bg;
		unsigned backcolor;
		unsigned signonfill;
	} MenuColors; 
	
extern MenuColors far menucolors[];

#define MENUSONG	WONDERIN_MUS

#ifndef SPEAR
#define INTROSONG	NAZI_NOR_MUS
#else
#define INTROSONG	XTOWER2_MUS
#endif

#define SENSITIVE	60
#define CENTER		SENSITIVE*2

#define MENU_X	76
#define MENU_Y	55
#define MENU_W	178
#ifndef SPEAR
#define MENU_H	13*10+6
#else
#define MENU_H	13*9+6
#endif

#define SM_X	48
#define SM_W	250

#define SM_Y1	20
#define SM_H1	4*13-7
#define SM_Y2	SM_Y1+5*13
#define SM_H2	4*13-7
#define SM_Y3	SM_Y2+5*13
#define SM_H3	3*13-7

#define CTL_X	24
#define CTL_Y	70
#define CTL_W	284
#define CTL_H	13*7-7

#define LSM_X	85
#define LSM_Y	55
#define LSM_W	175
#define LSM_H	10*13+10

#define NM_X	50
#define NM_Y	100
#define NM_W	225
#define NM_H	13*4+15

#define NE_X	10
#define NE_Y	23
#define NE_W	320-NE_X*2
#define NE_H	200-NE_Y*2

#define CST_X		20
#define CST_Y		48
#define CST_START	60
#define CST_SPC	60


//
// TYPEDEFS
//
typedef struct {
		int x,y,amount,curpos,indent;
		} CP_iteminfo;

typedef struct {
		int active;
		char string[36];
		void (* routine)(int temp1);
		} CP_itemtype;

typedef struct {
		int allowed[4];
		} CustomCtrls;

extern CP_itemtype far MainMenu[],far NewEMenu[];
extern CP_iteminfo MainItems;

//
// FUNCTION PROTOTYPES
//
void SetupControlPanel(void);
void CleanupControlPanel(void);

void DrawMenu(CP_iteminfo *item_i,CP_itemtype far *items);
int  HandleMenu(CP_iteminfo *item_i,
		CP_itemtype far *items,
		void (*routine)(int w));
void ClearMScreen(void);
void DrawWindow(int x,int y,int w,int h,int wcolor);
void DrawOutline(int x,int y,int w,int h,int color1,int color2);
void WaitKeyUp(void);
void ReadAnyControl(ControlInfo *ci);
void TicDelay(int count);
void CacheLump(int lumpstart,int lumpend);
void UnCacheLump(int lumpstart,int lumpend);
void StartCPMusic(int song);
int  Confirm(char far *string);
void Message(char far *string);
void CheckPause(void);
void ShootSnd(void);
void CheckSecretMissions(void);
void BossKey(void);

void DrawGun(CP_iteminfo *item_i,CP_itemtype far *items,int x,int *y,int which,int basey,void (*routine)(int w));
void DrawHalfStep(int x,int y);
void EraseGun(CP_iteminfo *item_i,CP_itemtype far *items,int x,int y,int which);
void SetTextColor(CP_itemtype far *items,int hlight);
void DrawMenuGun(CP_iteminfo *iteminfo);
void DrawStripes(int y);

void DefineMouseBtns(void);
void DefineJoyBtns(void);
void DefineKeyBtns(void);
void DefineKeyMove(void);
void EnterCtrlData(int index,CustomCtrls *cust,void (*DrawRtn)(int),void (*PrintRtn)(int),int type);

void DrawMainMenu(void);
void DrawSoundMenu(void);
void DrawLoadSaveScreen(int loadsave);
void DrawNewEpisode(void);
void DrawNewGame(void);
void DrawChangeView(int view);
void DrawMouseSens(void);
void DrawCtlScreen(void);
void DrawCustomScreen(void);
void DrawLSAction(int which);
void DrawCustMouse(int hilight);
void DrawCustJoy(int hilight);
void DrawCustKeybd(int hilight);
void DrawCustKeys(int hilight);
void PrintCustMouse(int i);
void PrintCustJoy(int i);
void PrintCustKeybd(int i);
void PrintCustKeys(int i);

void PrintLSEntry(int w,int color);
void TrackWhichGame(int w);
void DrawNewGameDiff(int w);
void FixupCustom(int w);

void CP_NewGame(void);
void CP_Sound(void);
int  CP_LoadGame(int quick);
int  CP_SaveGame(int quick);
void CP_Control(void);
void CP_ChangeView(void);
void CP_ExitOptions(void);
void CP_Quit(void);
void CP_ViewScores(void);
int  CP_EndGame(void);
int  CP_CheckQuick(unsigned scancode);
void CustomControls(void);
void MouseSensitivity(void);

void CheckForEpisodes(void);

//
// VARIABLES
//
extern int SaveGamesAvail[10],StartGame,SoundStatus;
extern char SaveGameNames[10][32],SaveName[13];

enum {MOUSE,JOYSTICK,KEYBOARDBTNS,KEYBOARDMOVE};	// FOR INPUT TYPES

enum
{
	newgame,
	soundmenu,
	control,
	loadgame,
	savegame,
	changeview,

#ifndef GOODTIMES
#ifndef SPEAR
	readthis,
#endif
#endif

	viewscores,
	backtodemo,
	quit
} menuitems;

//
// WL_INTER
//
typedef struct {
		int kill,secret,treasure;
		long time;
		} LRstruct;

extern LRstruct LevelRatios[];

void Write (int x,int y,char *string);
void NonShareware(void);
int GetYorN(int x,int y,int pic);


