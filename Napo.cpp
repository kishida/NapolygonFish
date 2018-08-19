#include <windows.h>
#include "resource.h"
#include "math.h"
#include "string.h"

#define NapoChouten 70
#define NapoMen 69
#define MOVECHECK 0
#define FACEPITCH 20
#define FACEWIDTH 20
#define WAVEPITCH 20

extern double Ncod[70][3];//頂点のデータ
extern int Njun[69][5];//面のデータ

LONG FAR PASCAL WndProc(HWND,UINT,WPARAM,LPARAM);
BOOL FAR PASCAL DialogProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam);
VOID WireFrame();

struct TagColor{
	int r;
	int g;
	int b;
};
typedef struct TagColor TagColor;

struct TagLight{
	double r;
	double g;
	double b;
};
typedef struct TagLight TagLight;

class Tag3Dpoint{
public:
	double x;
	double y;
	double z;
	friend Tag3Dpoint operator +(Tag3Dpoint,Tag3Dpoint);
	friend Tag3Dpoint operator -(Tag3Dpoint,Tag3Dpoint);
	friend Tag3Dpoint operator *(Tag3Dpoint,double);
	friend Tag3Dpoint operator /(Tag3Dpoint,double);
	virtual operator /=(double);
	virtual operator *=(double);
	virtual operator =(double[3]);
};
Tag3Dpoint::operator =(double eq[3])
{
	x = eq[0];
	y = eq[1];
	z = eq[2];
}
Tag3Dpoint::operator /=(double mul)
{
	x /= mul;
	y /= mul;
	z /= mul;
}
Tag3Dpoint::operator *=(double mul)
{
	x*= mul;
	y*= mul;
	z*= mul;
}
Tag3Dpoint operator *(Tag3Dpoint sc,double mul)
{
	Tag3Dpoint pt;

	pt.x = sc.x * mul;
	pt.y = sc.y * mul;
	pt.z = sc.z * mul;

	return pt;
}
Tag3Dpoint operator /(Tag3Dpoint sc,double mul)
{
	Tag3Dpoint pt;

	pt.x = sc.x / mul;
	pt.y = sc.y / mul;
	pt.z = sc.z / mul;

	return pt;
}


//typedef struct Tag3Dpoint Tag3Dpoint;

LPSTR szPubClassNme		= "Internet Pub";
LPSTR szClassNme		= "ナポリゴンフィッシュ";
LPSTR szCopyRight = "NAPOLYGON FISH for Windows95\nCopyright 1995-1997 Naoki Kishida";
int SizeX,SizeY;	//ウィンドウの大きさ
HDC hdcbm;			//作業用のビットマップ
int ScrSizeX = 300;	//画面に表示する大きさ
int ScrSizeY = 80;
double ScreenZ = 200;	//画面までの距離
double KasumiOku = 400;	//ポリゴンが見えなくなる点
double KasumiMae = 100;	//ポリゴンが完全に見える点
double Pai = 3.14159;
double WorkCod[NapoChouten][3];
TagColor BackColor = {0,50,100};
TagColor LineColor = {255,255,255};
TagLight EnvColor = {.3,.3,.3};//環境光
TagLight NapoColor = {0,.3,1};//ナポの色
TagLight AirColor = {.6,.7,1};//空の色
#if MOVECHECK//光のむき
Tag3Dpoint Ray = {0,-1,0};
#else
Tag3Dpoint Ray = {-1,2,1};
#endif
Tag3Dpoint Ofset = {0,-50,-100};
int Hire;//ひれの状態
int FlagFace = 0;//海面のフラグ
int FlagPoli = 0;//ポリゴンフラグ
int FlagLine = 0;//枠線フラグ
int FlagKasu = 0;
int FlagRenb = 0;
HINSTANCE hInst;
HWND hWndNapo;

LPSTR szSelection = "NAPOLIGON FISH";
LPSTR szIniFile = "NAPO.INI";

//波を作る
double Wave(double rad)
{
	return (1-fabs(sin(rad/2))*2);
}

double Distance(Tag3Dpoint vec)
{
	return sqrt(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);
}

//2つのベクトルの法線ベクトルを求める
Tag3Dpoint Housen(Tag3Dpoint vec1,Tag3Dpoint vec2)
{
	Tag3Dpoint ret;

	ret.x = vec1.z*vec2.y - vec1.y*vec2.z;
	ret.y = vec1.x*vec2.z - vec1.z*vec2.x;
	ret.z = vec1.y*vec2.x - vec1.x*vec2.y;
	return ret;
}

//内積を求める
double Naiseki(Tag3Dpoint vec1,Tag3Dpoint vec2)
{
	return (vec1.x*vec2.x + vec1.y * vec2.y + vec1.z*vec2.z);
}
//2つのベクトルの角度を求める
double Kakudo(Tag3Dpoint vec1,Tag3Dpoint vec2)
{
	return (Naiseki(vec1,vec2)/
		(Distance(vec1)*Distance(vec2)));
}

//３次元の座標を表示用の座標に変える
VOID ScreenPoint(int scr[][2],double dim[][3],int point)
{
	int i;
	double fx,fy,rate,xrate,yrate;
	double CenterX,CenterY;

	xrate = (double)SizeX / (double)ScrSizeX;
	yrate = (double)SizeY / (double)ScrSizeY;
	CenterX = (double)SizeX / 2;
	CenterY = (double)SizeY / 2 + 70*xrate;
	for (i = 0; i < point; ++i){
		rate = ScreenZ / (ScreenZ + dim[i][2]);
		fx = dim[i][0] * rate * xrate + CenterX;
		fy = dim[i][1] * rate * xrate + CenterY;
		scr[i][0] = (int)fx;
		scr[i][1] = (int)fy;
	}
}
//Y軸を中心に回転する
VOID YTurn(double saki[][3],double moto[][3],int point,double rad)
{
	int i;
	double Ysin,Ycos,x;

	Ysin = (double)sin((double)rad);
	Ycos = (double)cos((double)rad);
	for (i = 0; i < point; ++i){
		x		   =  moto[i][0] * Ysin + moto[i][2] * Ycos;
		saki[i][2] = -moto[i][0] * Ycos + moto[i][2] * Ysin;
		saki[i][0] = x;
		saki[i][1] = moto[i][1];
	}
}

VOID MoveData(double saki[][3],double moto[][3],int pt,Tag3Dpoint mv)
{
	int i;

	for (i = 0; i < pt; ++i){
		saki[i][0] = moto[i][0] + mv.x;
		saki[i][1] = moto[i][1] + mv.y;
		saki[i][2] = moto[i][2] + mv.z;
	}
}

VOID Poligon()
{
	int scr[NapoChouten][2];
	int i,j,a,z,x;
	double Zorder[NapoMen],doubleA;
	int PaintOrder[NapoMen];
	HBRUSH hbrush,oldbrush;
	HPEN hpen,oldpen;
	POINT pt[4];
	int men;
	int wavescr[4][2];
	double dr,dg,db,dcos;
	double dispoli,Kasumi;
	double wavepoli[4][3];
	static double Nami = 0;
	double pai4,pai12,pai10;
	Tag3Dpoint vec1,vec2,VecHou,VecEye;
	Tag3Dpoint face[4],belt[4];
	TagLight SeaColor;//海の色
	int penstl;
	double kasumisa;
	RECT rect;
	COLORREF oldcol;
	int bkmode;

	if (!FlagPoli) {
		WireFrame();
		return;
	}
	ScreenPoint(scr,WorkCod,NapoChouten);
	for (i = 0; i < NapoMen; ++i){
		PaintOrder[i] = i;
	}
	//ポリゴンの深さを計算
	for (i = 0; i < NapoMen; ++i){
		Zorder[i] = WorkCod[Njun[i][0]][2] + WorkCod[Njun[i][1]][2] +
					WorkCod[Njun[i][2]][2] + WorkCod[Njun[i][3]][2];
	}
	//深い順に並べ替え
	for (i = 0; i < NapoMen - 1; ++i){
		for (j = i + 1; j < NapoMen; ++j){
			if (Zorder[i] < Zorder[j]){
				doubleA = Zorder[i];
				Zorder[i] = Zorder[j];
				Zorder[j] = doubleA;
				a = PaintOrder[i];
				PaintOrder[i] = PaintOrder[j];
				PaintOrder[j] = a;
			}
		}
	}
	//描画
	hbrush = CreateSolidBrush(RGB(BackColor.r,BackColor.g,BackColor.b));
	oldbrush = (HBRUSH)SelectObject(hdcbm,hbrush);
	if (!FlagLine) penstl = PS_SOLID;
	else penstl = PS_NULL;
	hpen = CreatePen(penstl,1,RGB(LineColor.r,LineColor.g,LineColor.b));
	oldpen = (HPEN)SelectObject(hdcbm,hpen);
	Rectangle(hdcbm,0,0,SizeX,SizeY);
	SeaColor.r = (double)BackColor.r / 255;
	SeaColor.g = (double)BackColor.g / 255;
	SeaColor.b = (double)BackColor.b / 255;
	if (FlagFace){
		//海面の波
		pai4 = Pai/40;
		pai10 = Pai/100;
		pai12 = Pai/120;
		kasumisa = KasumiOku-KasumiMae;
		for (z = FACEWIDTH/5; z < FACEWIDTH; ++z){
			face[0].z = (FACEPITCH * FACEWIDTH) / 2 -  z * FACEPITCH;
			face[3].z = face[0].z;
			face[1].z = face[0].z + FACEPITCH;
			face[2].z = face[1].z;
			for (x = 0; x < FACEWIDTH; ++x){
				//波を作る
				face[0].x = x * FACEPITCH - (FACEPITCH*FACEWIDTH)/2;
				face[1].x = face[0].x;
				face[2].x = face[0].x + FACEPITCH;
				face[3].x = face[2].x;
				for (i = 0; i < 4; ++i){
					face[i].y = -sin((face[i].z + Nami + sin(face[i].x * pai10)*10) * pai4) *
								sin((face[i].x + Nami) * pai12) * 7 -
								sin((face[i].x + Nami) * pai4) * 3 - 200;
					wavepoli[i][0] = face[i].x;
					wavepoli[i][1] = face[i].y;
					wavepoli[i][2] = face[i].z;
				}
				//色の計算
				vec1.x = wavepoli[1][0] - wavepoli[0][0];
				vec1.y = wavepoli[1][1] - wavepoli[0][1];
				vec1.z = wavepoli[1][2] - wavepoli[0][2];
				vec2.x = wavepoli[2][0] - wavepoli[1][0];
				vec2.y = wavepoli[2][1] - wavepoli[1][1];
				vec2.z = wavepoli[2][2] - wavepoli[1][2];
				VecHou = Housen(vec1,vec2);
				VecEye.x = wavepoli[0][0];
				VecEye.y = wavepoli[0][1];
				VecEye.z = wavepoli[0][2];
				for (j = 1; j < 4; ++j){
					VecEye.x += wavepoli[j][0];
					VecEye.y += wavepoli[j][1];
					VecEye.z += wavepoli[j][2];
				}
				VecEye /= 4;
				VecEye.z += ScreenZ;
				if (Naiseki(VecEye,VecHou) < 0) continue;
				dispoli = Distance(VecEye);//ポリゴンまでの距離
				//霞を計算
				if (FlagKasu){
					if (dispoli > KasumiOku) continue;
					else
						if (dispoli < KasumiMae) Kasumi = 1;
						else Kasumi = (KasumiOku - dispoli)/kasumisa;
				}
				else Kasumi = 1;
				dcos = Kakudo(VecEye,VecHou);
				//dcos = dcos*dcos;
				if (dcos < 0) dcos = 0;
				if (dcos > 1) dcos = 1;
				dr = AirColor.r * dcos;
				dg = AirColor.g * dcos;
				db = AirColor.b * dcos;
				dr = dr * Kasumi + SeaColor.r * (1-Kasumi);
				dg = dg * Kasumi + SeaColor.g * (1-Kasumi);
				db = db * Kasumi + SeaColor.b * (1-Kasumi);
				if (dr > 1) dr = 1;
				if (dg > 1) dg = 1;
				if (db > 1) db = 1;
				if (dr < 0) dr = 0;
				if (dg < 0) dg = 0;
				if (db < 0) db = 0;
				dr *= 255; dg *= 255; db*=255;
				//描画
				ScreenPoint (wavescr,wavepoli,4);
				for (i =0 ; i < 4; ++i){
					pt[i].x = wavescr[i][0];
					pt[i].y = wavescr[i][1];
				}
				for (i = 0; i < 4; ++i){//簡易クリッピング
					if (pt[i].x >= 0 && pt[i].x <= SizeX) break;
					if (pt[i].y >= 0 && pt[i].y <= SizeY) break;
				}
				if (i <4){
					hbrush = CreateSolidBrush(RGB(dr,dg,db));
					DeleteObject(SelectObject(hdcbm,hbrush));
					Polygon(hdcbm,pt,4);
				}
				//なんちゃってレンブラント光
				if (Kakudo(VecHou,Ray)< -.98 && FlagRenb){
					belt[0] = face[0];
					belt[3] = face[3];
					for (j = 0; j < 4; ++j){
						//ポリゴンを作る
						belt[1] = belt[0] + Ray*10;
						belt[2] = belt[3] + Ray*10;
						for (i = 0; i < 4; ++i){
							wavepoli[i][0] = belt[i].x;
							wavepoli[i][1] = belt[i].y;
							wavepoli[i][2] = belt[i].z;
						}
						ScreenPoint(wavescr,wavepoli,4);
						for (i = 0; i < 4; ++i){
							pt[i].x = wavescr[i][0];
							pt[i].y = wavescr[i][1];
						}
						//色の計算
						dr = (SeaColor.r * (j + 1) + 1 * (4-j))/4;
						dg = (SeaColor.g * (j + 1) + 1 * (4-j))/4;
						db = (SeaColor.b * (j + 1) + 1 * (4-j))/4;
						dr = dr * Kasumi + SeaColor.r * (1-Kasumi);
						dg = dg * Kasumi + SeaColor.g * (1-Kasumi);
						db = db * Kasumi + SeaColor.b * (1-Kasumi);
						if (dr > 1) dr = 1;
						if (dg > 1) dg = 1;
						if (db > 1) db = 1;
						if (dr < 0) dr = 0;
						if (dg < 0) dg = 0;
						if (db < 0) db = 0;
						dr *= 255; dg *= 255; db*=255;
						hbrush = CreateSolidBrush(RGB(dr,dg,db));
						DeleteObject(SelectObject(hdcbm,hbrush));
						Polygon(hdcbm,pt,4);
						belt[0] = belt[1];
						belt[3] = belt[2];
					}
				}
			}
		}
	}
	for (i = 0; i < NapoMen; ++i){
		men = PaintOrder[i];
		for (j =0; j < 4; ++j){
			pt[j].x = scr[Njun[men][j]][0];
			pt[j].y = scr[Njun[men][j]][1];
		}
		//法線ベクトルを求める
		vec1.x = WorkCod[Njun[men][1]][0] - WorkCod[Njun[men][0]][0];
		vec1.y = WorkCod[Njun[men][1]][1] - WorkCod[Njun[men][0]][1];
		vec1.z = WorkCod[Njun[men][1]][2] - WorkCod[Njun[men][0]][2];
		vec2.x = WorkCod[Njun[men][2]][0] - WorkCod[Njun[men][1]][0];
		vec2.y = WorkCod[Njun[men][2]][1] - WorkCod[Njun[men][1]][1];
		vec2.z = WorkCod[Njun[men][2]][2] - WorkCod[Njun[men][1]][2];
		VecHou = Housen(vec1,vec2);
		//視線のベクトル
		VecEye.x = 0;
		VecEye.y = 0;
		VecEye.z = 0;
		for (j = 0; j < 4; ++j){
			VecEye.x += WorkCod[Njun[men][j]][0];
			VecEye.y += WorkCod[Njun[men][j]][1];
			VecEye.z += WorkCod[Njun[men][j]][2];
		}
		VecEye /= 4;
		VecEye.z += ScreenZ;
		if (Naiseki(VecEye,VecHou) < 0) continue;
		dispoli = Distance(VecEye);//ポリゴンまでの距離
		//霞を計算
		if (FlagKasu){
			if (dispoli > KasumiOku) continue;
			else
				if (dispoli < KasumiMae) Kasumi = 1;
				else Kasumi = (KasumiOku - dispoli)/(KasumiOku-KasumiMae);
		}
		else Kasumi = 1;
		dcos = Kakudo(Ray,VecHou);
		if (dcos < 0) dcos = 0;
		if (dcos > 1) dcos = 1;
		dr = NapoColor.r * (dcos + EnvColor.r);
		dg = NapoColor.g * (dcos + EnvColor.g);
		db = NapoColor.b * (dcos + EnvColor.b);
		dr = dr * Kasumi + SeaColor.r * (1-Kasumi);
		dg = dg * Kasumi + SeaColor.g * (1-Kasumi);
		db = db * Kasumi + SeaColor.b * (1-Kasumi);
		if (dr > 1) dr = 1;
		if (dg > 1) dg = 1;
		if (db > 1) db = 1;
		if (dr < 0) dr = 0;
		if (dg < 0) dg = 0;
		if (db < 0) db = 0;
		dr *= 255; dg *= 255; db*=255;

		hbrush = CreateSolidBrush(RGB((char)dr,(char)dg,(char)db));
		DeleteObject(SelectObject(hdcbm,hbrush));
		Polygon(hdcbm,pt,4);
	}
	DeleteObject(SelectObject(hdcbm,oldpen));
	DeleteObject(SelectObject(hdcbm,oldbrush));
	GetClientRect(hWndNapo,&rect);
	oldcol = SetTextColor(hdcbm,RGB(170,170,150));
	bkmode = SetBkMode(hdcbm,TRANSPARENT);
	rect.top = rect.bottom - 50;
	DrawText(hdcbm,szCopyRight,-1,&rect,DT_CENTER | DT_WORDBREAK);
	SetTextColor(hdcbm,oldcol);
	SetBkMode(hdcbm,bkmode);
	++Nami;
	if (Nami > 240) Nami = 0;
}

VOID WireFrame()
{
	int scr[NapoChouten][2];
	int i;
	RECT rect;
	COLORREF oldcol;
	int bkmode;

	ScreenPoint(scr,WorkCod,NapoChouten);
	Rectangle(hdcbm,0,0,SizeX,SizeY);
	for (i =0; i < NapoMen; ++i){
		MoveToEx(hdcbm, scr[Njun[i][0]][0],scr[Njun[i][0]][1],NULL);
		LineTo(hdcbm, scr[Njun[i][1]][0],scr[Njun[i][1]][1]);
		LineTo(hdcbm, scr[Njun[i][2]][0],scr[Njun[i][2]][1]);
		LineTo(hdcbm, scr[Njun[i][3]][0],scr[Njun[i][3]][1]);
		LineTo(hdcbm, scr[Njun[i][0]][0],scr[Njun[i][0]][1]);
	}
	GetClientRect(hWndNapo,&rect);
	oldcol = SetTextColor(hdcbm,RGB(0,0,0));
	bkmode = SetBkMode(hdcbm,TRANSPARENT);
	rect.top = rect.bottom - 100;
	DrawText(hdcbm,szCopyRight,-1,&rect,DT_CENTER | DT_WORDBREAK);
	SetTextColor(hdcbm,oldcol);
	SetBkMode(hdcbm,bkmode);
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPreInst,LPSTR lpszCmdLine,int nCmdShow)
{
	HWND hWnd;
	MSG lpMsg;
	WNDCLASS Prog;
	HMENU hmenu;
	char IniParam[10];

	hInst = hInstance;
	if (!hPreInst){
		Prog.style			= 0;
		Prog.lpfnWndProc	= WndProc;
		Prog.cbClsExtra		= 0;
		Prog.cbWndExtra		= 0;
		Prog.hInstance		= hInstance;
		Prog.hIcon			= LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON1));
		Prog.hCursor		= LoadCursor(hInstance,MAKEINTRESOURCE(IDC_CURSOR1));
		Prog.hbrBackground	= (HBRUSH)GetStockObject(NULL_BRUSH);
		Prog.lpszMenuName	= MAKEINTRESOURCE(IDR_MENU1);
		Prog.lpszClassName	= szClassNme;
		if (!RegisterClass(&Prog)) return FALSE;
	}
	else{
		MessageBox(NULL,"たくさん立ち上げると\nたくさん遅くなります。","報告",MB_OK);
	}
	hWnd = CreateWindow(szClassNme,
		szClassNme,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,NULL,hInstance,NULL);
	hWndNapo = hWnd;
	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);
	//設定の読み込み
	GetPrivateProfileString(szSelection,"Poli","FALSE",IniParam,10,szIniFile);
	if (!strcmp(IniParam,"TRUE")) FlagPoli = TRUE;
	else FlagPoli = FALSE;
	GetPrivateProfileString(szSelection,"Line","FALSE",IniParam,10,szIniFile);
	if (!strcmp(IniParam,"TRUE")) FlagLine = TRUE;
	else FlagLine = FALSE;
	GetPrivateProfileString(szSelection,"Face","FALSE",IniParam,10,szIniFile);
	if (!strcmp(IniParam,"TRUE")) FlagFace = TRUE;
	else FlagFace = FALSE;
	GetPrivateProfileString(szSelection,"Kasumi","FALSE",IniParam,10,szIniFile);
	if (!strcmp(IniParam,"TRUE")) FlagKasu = TRUE;
	else FlagKasu = FALSE;
	GetPrivateProfileString(szSelection,"Renb","FALSE",IniParam,10,szIniFile);
	if (!strcmp(IniParam,"TRUE")) FlagRenb = TRUE;
	else FlagRenb = FALSE;
	//メニューの設定
	hmenu = GetMenu(hWnd);
	if (FlagPoli){
		EnableMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_ENABLED);
		CheckMenuItem(hmenu,IDM_POLI,MF_BYCOMMAND | MF_CHECKED);
	}
	else{
		EnableMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		EnableMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		EnableMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		EnableMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		CheckMenuItem(hmenu,IDM_POLI,MF_BYCOMMAND | MF_UNCHECKED);
	}
	if (FlagLine){
		CheckMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_UNCHECKED);
	}
	else{
		CheckMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_CHECKED);
	}
	if (FlagFace){
		CheckMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_CHECKED);
	}
	else{
		CheckMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_UNCHECKED);
	}
	if (FlagRenb){
		CheckMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_CHECKED);
	}
	else{
		CheckMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_UNCHECKED);
	}
	if (FlagKasu){
		CheckMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_CHECKED);
	}
	else{
		CheckMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_UNCHECKED);
	}
	//いつものメインループ
	while(GetMessage(&lpMsg,NULL,NULL,NULL)){
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg);
	}
	return (lpMsg.wParam);
}

VOID MoveHire(int hi)
{
	int i,j;
	double sn,cs,rad;
	const int jun[6] = {64,65,62,63,30,31};

	for (i = 0; i < NapoChouten; ++i){
		for (j = 0; j < 3; ++j){
			WorkCod[i][j] = Ncod[i][j];
		}
	}
	i = hi -15;
	if (i > 40) i -= 40;
	if (i < 0) i += 40;
	rad = i;
	rad = 10 * sin(rad * Pai / 20);
	sn = sin(rad * Pai / 60);
	cs = cos(rad * Pai / 60);
	for (i = 0; i < 2; ++i){
		WorkCod[jun[i]][0] = (Ncod[jun[i]][0] + 35) * cs - 35;
		WorkCod[jun[i]][2] = (Ncod[jun[i]][0] + 35) * sn;
	}
	rad = hi;
	rad = 10 * sin(rad * Pai / 20);
	sn = sin(rad * Pai / 60);
	cs = cos(rad * Pai / 60);
	for (i = 0; i < 6; ++i){
		rad					= (WorkCod[jun[i]][0] + 25) * sn + (WorkCod[jun[i]][2] * cs);
		WorkCod[jun[i]][0]	= (WorkCod[jun[i]][0] + 25) * cs - (WorkCod[jun[i]][2] * sn) - 25;
		WorkCod[jun[i]][2]	= rad;
	}
#if MOVECHECK
	for (i =0 ; i < NapoChouten; ++i){
		rad = WorkCod[i][2];
		WorkCod[i][2] = WorkCod[i][1];
		WorkCod[i][1] = rad;
	}
#endif
}

LONG FAR PASCAL WndProc(HWND hWnd,UINT msg, WPARAM wParam,LPARAM lParam)
{
	static HBITMAP oldbm;
	HBITMAP hbmdummy;
	RECT rect;
	HDC hdc;
	PAINTSTRUCT paintst;
	static double rad;
//	FARPROC fndlg;
	HMENU hmenu;
//	COLORREF oldcol;
//	int bkmode;
	HWND hWndPub;

	switch(msg)
	{
	 case WM_CREATE:
		Hire = 0;
		hdc = GetDC(hWnd);
		hdcbm = CreateCompatibleDC(hdc);
		GetClientRect(hWnd,&rect);
		SizeX = rect.right;
		SizeY = rect.bottom;
		hbmdummy = CreateCompatibleBitmap(hdc,SizeX,SizeY);
		oldbm = (HBITMAP)SelectObject(hdcbm,hbmdummy);
		ReleaseDC(hWnd,hdc);
		rad = 0;
		MoveData(WorkCod,Ncod,NapoChouten,Ofset);
		YTurn(WorkCod,WorkCod,NapoChouten,rad);
		Poligon();
		SetTimer(hWnd,1,100,NULL);
		break;
	 case WM_PAINT:
		hdc = BeginPaint(hWnd,&paintst);
		BitBlt(hdc,0,0,SizeX,SizeY,hdcbm,0,0,SRCCOPY);
		ValidateRect(hWnd,NULL);
		EndPaint(hWnd,&paintst);
		break;
	 case WM_SIZE:
		SizeX = LOWORD(lParam);
		SizeY = HIWORD(lParam);
		hbmdummy = CreateCompatibleBitmap(hdcbm,SizeX,SizeY);
		DeleteObject(SelectObject(hdcbm,hbmdummy));
		Poligon();
		InvalidateRect(hWnd,NULL,FALSE);
		UpdateWindow(hWnd);
		break;
	 case WM_TIMER:
		++Hire;
		if (Hire >= 40) Hire = 0;
		MoveHire(Hire);
		++rad;
		if (rad >= 600) rad = 0;
#if MOVECHECK
		rad = 50;
#endif
		MoveData(WorkCod,WorkCod,NapoChouten,Ofset);
		YTurn(WorkCod,WorkCod,NapoChouten,rad*Pai/150);
		Poligon();
		InvalidateRect(hWnd,NULL,FALSE);
		UpdateWindow(hWnd);
		break;
	 case WM_COMMAND:
		hmenu = GetMenu(hWnd);
		switch(LOWORD(wParam)){
/*
		 case IDM_INI:
			fndlg = MakeProcInstance((FARPROC)DialogProc,hInst);
			DialogBox(hInst,MAKEINTRESOURCE(IDD_DIALOG1),hWnd,fndlg);
			break;
*/
		 case IDM_POLI:
			FlagPoli = !FlagPoli;
			if (FlagPoli){
				EnableMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_ENABLED);
				CheckMenuItem(hmenu,IDM_POLI,MF_BYCOMMAND | MF_CHECKED);
				WritePrivateProfileString(szSelection,"Poli","TRUE",szIniFile);
			}
			else{
				EnableMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				EnableMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				EnableMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				EnableMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				CheckMenuItem(hmenu,IDM_POLI,MF_BYCOMMAND | MF_UNCHECKED);
				WritePrivateProfileString(szSelection,"Poli","FALSE",szIniFile);
			}
			break;
		 case IDM_LINE:
			FlagLine = !FlagLine;
			if (FlagLine){
				CheckMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_UNCHECKED);
				WritePrivateProfileString(szSelection,"Line","TRUE",szIniFile);
			}
			else{
				CheckMenuItem(hmenu,IDM_LINE,MF_BYCOMMAND | MF_CHECKED);
				WritePrivateProfileString(szSelection,"Line","FALSE",szIniFile);
			}
			break;
		 case IDM_FACE:
			FlagFace = !FlagFace;
			if (FlagFace){
				CheckMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_CHECKED);
				WritePrivateProfileString(szSelection,"Face","TRUE",szIniFile);
			}
			else{
				CheckMenuItem(hmenu,IDM_FACE,MF_BYCOMMAND | MF_UNCHECKED);
				WritePrivateProfileString(szSelection,"Face","FALSE",szIniFile);
			}
			break;
		 case IDM_KASUMI:
			FlagKasu = !FlagKasu;
			if (FlagKasu){
				CheckMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_CHECKED);
				WritePrivateProfileString(szSelection,"Kasumi","TRUE",szIniFile);
			}
			else{
				CheckMenuItem(hmenu,IDM_KASUMI,MF_BYCOMMAND | MF_UNCHECKED);
				WritePrivateProfileString(szSelection,"Kasumi","FALSE",szIniFile);
			}
			break;
		 case IDM_RENB:
			FlagRenb = !FlagRenb;
			if (FlagRenb){
				CheckMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_CHECKED);
				WritePrivateProfileString(szSelection,"Renb","TRUE",szIniFile);
			}
			else{
				CheckMenuItem(hmenu,IDM_RENB,MF_BYCOMMAND | MF_UNCHECKED);
				WritePrivateProfileString(szSelection,"Renb","FALSE",szIniFile);
			}
			break;
		 case IDM_HELP:
			WinHelp(hWnd,"Napo.hlp",HELP_CONTENTS,0L);
			break;
		}
		break;
	 case WM_DESTROY:
		DeleteObject(SelectObject(hdcbm,oldbm));
		DeleteDC(hdcbm);
		if (hWndPub = FindWindow(szPubClassNme,szPubClassNme)){
			SendMessage(hWndPub,WM_USER + 2, 0,0L);
		}
		PostQuitMessage(0);
		break;
	 default:
		return (DefWindowProc(hWnd,msg,wParam,lParam));
	}
	return 0L;
}
//初期設定のダイアログ
BOOL FAR PASCAL DialogProc(HWND hdlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg){
	 case WM_INITDIALOG:
		SendDlgItemMessage(hdlg,IDC_CHECK1,BM_SETCHECK,FlagPoli,0L);
		SendDlgItemMessage(hdlg,IDC_CHECK2,BM_SETCHECK,FlagLine,0L);
		SendDlgItemMessage(hdlg,IDC_CHECK3,BM_SETCHECK,FlagFace,0L);
		break;
	 case WM_COMMAND:
		switch (LOWORD(wParam)){
		 case IDOK:
			FlagPoli	= (BOOL)SendDlgItemMessage(hdlg,IDC_CHECK1,BM_GETCHECK,0L,0L);
			FlagLine	= (BOOL)SendDlgItemMessage(hdlg,IDC_CHECK2,BM_GETCHECK,0L,0L);
			FlagFace	= (BOOL)SendDlgItemMessage(hdlg,IDC_CHECK3,BM_GETCHECK,0L,0L);
			EndDialog(hdlg,TRUE);
			break;
		case IDCANCEL:
			EndDialog(hdlg,FALSE);
			break;
		 default:
			return FALSE;
		}
		break;
	 default:
		return FALSE;
	}
	return TRUE;
}


