#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <windows.h>


#define COL_L 0x000077
#define COL_J 0x007700
#define COL_T 0x007777
#define COL_Z 0x770000
#define COL_S 0x770077
#define COL_I 0x777700
#define COL_O 0x777777

struct posinfo {
    int r, x, y, f;
};

enum runtype { SCR_PREV, SCR_CONFIG, SCR_RUN, SCR_ERRPREV };

LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);


char *clsName = "tetromino-1.0.0";
int best_r, best_x;
int xmax, ymax;
POINT maus;
uint8_t *bmp;
uint32_t curbrick[4*2+2];
HBRUSH bgcol;
int kante;
int brick_x=0, brick_y=0;
uint32_t *feld;

uint8_t bpos=0;

#define BCOORD(__X, __Y, __B) __B[2+(__X)+ __B[0]*(__Y)]

uint32_t brick_L[]= {3, 2,
    0,      0,      COL_L,
    COL_L,  COL_L,  COL_L };

uint32_t brick_J[]= {3, 2,
    COL_J,  0,      0,
    COL_J,  COL_J,  COL_J};

uint32_t brick_T[]= {3, 2,
    0,      COL_T,  0,
    COL_T,  COL_T,  COL_T };

uint32_t brick_Z[]= {3, 2,
    COL_Z,  COL_Z,  0,
    0,      COL_Z,  COL_Z };

uint32_t brick_S[]= {3, 2,
    0,      COL_S,  COL_S,
    COL_S,  COL_S,  0 };

uint32_t brick_I[]= {4, 1,
    COL_I,  COL_I,  COL_I,  COL_I };

uint32_t brick_O[]={ 2, 2,
    COL_O, COL_O,
    COL_O, COL_O };


void rotBrick(uint32_t *b) {
    uint32_t t[2+2*4];
    int i, l;

    l=b[0]*b[1]+2;
    for (i=0;i<l;i++) {
        t[i]=b[i];
    }
    b[0]=t[1];
    b[1]=t[0];

    for (i=0;i<b[0];i++) {
        for (l=0;l<b[1];l++) {
            BCOORD(b[0]-i-1, l, b) = BCOORD(l, i, t);
        }
    }
}

void brick2Rect(uint32_t *b, RECT *r) {
    r->left=kante*brick_x;
    r->right=kante*(brick_x+b[0]);
    r->top=kante*brick_y;
    r->bottom=kante*(brick_y+b[1]);
}

int collision(uint32_t *b, int xc, int yc) {
    unsigned x, y;

    for (x=0;x<b[0];x++) {
        for (y=0;y<b[1];y++) {
            if (BCOORD(x,y,b)!=0 && BCOORD(xc+x, yc+y,feld)!=0) {
                return 1;
            }
        }
    }
    return 0;
}

int cntfs(int x, int y) {
    int r;

    r=0;
    while ((y+r<feld[1]) && BCOORD(x, y+r, feld)==0) {
        r++;
    }
    return r;
}

int freiraum(uint32_t *b, int x0, int y0) {
    int x1, y1;
    int r;

    r=0;
    for (x1=0;x1<b[0];x1++) {
        for (y1=b[1]-1;y1>=0;y1--) {
            if (BCOORD(x1, y1, b)) {
                break;
            }
        }
        y1++;
        if ((y0+y1)<feld[1]) {
            r=r+cntfs(x0+x1, y0+y1);
        }
    }
    return r;
}

void calcBestPos() {
    int pi_max, i, min_f, max_y;
    int r, x, y;
    struct posinfo *pi;
    uint32_t tmpbrick[2+2*4];

    pi=malloc(sizeof(struct posinfo)*4*feld[0]);
    memcpy(tmpbrick, curbrick, (4*2+2)*sizeof(uint32_t));

    pi_max=0;
    for (r=0;r<4;r++) {
        for (x=0;(x+tmpbrick[0])<=feld[0];x++) {
            y=0;
            while ((y+tmpbrick[1])<feld[1] && collision(tmpbrick, x, y+1)==0) {
                y++;
            }

            pi[pi_max].f=freiraum(tmpbrick, x, y);
            pi[pi_max].r=r;
            pi[pi_max].x=x;
            pi[pi_max].y=y;
            pi_max++;
        }
        rotBrick(tmpbrick);
    }

    min_f=0;
    for (i=0;i<pi_max;i++) {
        if (pi[min_f].f>pi[i].f) {
            min_f=i;
        }
    }
    
    max_y=min_f;
    for (i=0;i<pi_max;i++) {
        if ((pi[min_f].f==pi[i].f) && (pi[max_y].y<pi[i].y)) {
            max_y=i;
        }
    }
    
    best_r=pi[max_y].r;
    brick_x=best_x=pi[max_y].x;

    free(pi);
}

int newBrick() {
    int i;
    uint32_t *nb=NULL;
    int r=1+(rand()%7);

    switch (r) {
        case 1:
            nb=brick_L;
            break;
        case 2:
            nb=brick_J;
            break;
        case 3:
            nb=brick_T;
            break;
        case 4:
            nb=brick_Z;
            break;
        case 5:
            nb=brick_S;
            break;
        case 6:
            nb=brick_I;
            break;
        case 7:
            nb=brick_O;
            break;
    }

    for (i=0;i<(nb[0]*nb[1]+2);i++) {
        curbrick[i]=nb[i];
    }
    brick_y=0;
    brick_x=rand()%(feld[0]-curbrick[0]);
    if (collision(curbrick, brick_x, brick_y)) {
        for (i=0;i<(feld[0]*feld[1]);i++) {
            feld[2+i]=0x000000;
        }
        memset(bmp, 0, 4*xmax*ymax);
        bpos++;
    }
    calcBestPos();
    return 0;
}

int down() {
    int x, y;

    if ((brick_y+curbrick[1])>=feld[1] || collision(curbrick, brick_x, brick_y+1)) {
        for (x=0;x<curbrick[0];x++) {
            for (y=0;y<curbrick[1];y++) {
                if (BCOORD(x,y,curbrick)) {
                    BCOORD(brick_x+x, brick_y+y, feld)=BCOORD(x,y,curbrick);
                }
            }
        }
        newBrick();
        return 0;
    }
    brick_y++;
    return 1;
}

void updateBmp(RECT *p) {
    int x, y, i, fx, fy;
    uint32_t rgb;
    uint8_t r, g, b;
    int ra, ba, rmodx, rmody;

    rmodx=xmax%kante;
    rmody=ymax%kante;
    ra=(bpos&1)*rmodx;
    ba=((bpos>>1)&1)*rmody;

    for (x=p->left;x<p->right;x++) {
        for (y=p->top;y<p->bottom;y++) {
            i=(x+ra)+(y+ba)*xmax;
            fx=x/kante;
            fy=y/kante;

            rgb=0;
            if (fx>=brick_x && fx<(brick_x+curbrick[0]) && fy>=brick_y && fy<(brick_y+curbrick[1])) {
                rgb=BCOORD(fx-brick_x, fy-brick_y, curbrick);
            }

            if (!rgb && fx<feld[0] && fy<feld[1]) {
                rgb=BCOORD(fx, fy, feld);
            }

            r=(rgb>>16)&0xff;
            g=(rgb>>8)&0xff;
            b=rgb&0xff;
            if (((x%kante)==0 || (y%kante)==0) && rgb && (x+ra)<xmax && (y+ba)<ymax) {
                bmp[4*i]=b+100;
                bmp[4*i+1]=g+100;
                bmp[4*i+2]=r+100;
            } else if ((x+ra)<xmax && (y+ba)<ymax) {
                bmp[4*i]=b;
                bmp[4*i+1]=g;
                bmp[4*i+2]=r;
            }
        } 
    }
}

enum runtype getRunType(LPSTR args, HWND *hWnd) {
    long long res;

    if (strlen(args)<2) {
        return SCR_RUN;
    }
    if (*args!='/') {
        return SCR_CONFIG;
    }
    args++;
    switch (*args) {
        case 'c':
        case 'C':
            return SCR_CONFIG;
        
        case 'p':
        case 'P':
            args++;
            if (strlen(args)==0) {
                return SCR_ERRPREV;
            }
            res=0;
            while (*args==' ') {
                args++;
            }
            while (*args!='\0') {
                char c=*args;
                int ziff;
                args++;
                if (c<'0' || c>'9') {
                    return SCR_ERRPREV;
                }
                ziff=c-'0';
                res=10*res+ziff;
            }
            *hWnd=(HWND)res;
            return SCR_PREV;
    }

    return SCR_RUN;
}

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int show) {
    MSG messages;
    WNDCLASSEX wndcl;
    HWND hWnd;
    int feldsz;

    wndcl.cbClsExtra=0;
    wndcl.cbSize=sizeof(WNDCLASSEX);
    wndcl.cbWndExtra=0;
    wndcl.hbrBackground=(HBRUSH)CreateSolidBrush(RGB(0,0,0));
    wndcl.hCursor=NULL;
    wndcl.hIcon=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hIconSm=LoadIcon(NULL, IDI_APPLICATION);
    wndcl.hInstance=hThisInstance;
    wndcl.lpfnWndProc=wndProc;
    wndcl.lpszClassName=clsName;
    wndcl.lpszMenuName=NULL;
    wndcl.style=0;

    srand(time(NULL));
    bgcol=CreateSolidBrush(RGB(0,0,0));

    switch (getRunType(szCmdLine, &hWnd)) {
        case SCR_RUN:
            xmax = GetSystemMetrics(SM_CXSCREEN);
            ymax = GetSystemMetrics(SM_CYSCREEN);
            bmp=malloc(4*xmax*ymax);
            memset(bmp, 0, 4*xmax*ymax);
            
            kante=xmax/50;
            feldsz=2+(xmax/kante)*(ymax/kante);
            feld=malloc(feldsz*sizeof(uint32_t));
            memset(feld, 0, feldsz*sizeof(uint32_t));
            feld[0]=xmax/kante;
            feld[1]=ymax/kante;

            newBrick();

            if (xmax==0 || ymax==0) {
                MessageBox(NULL, "Konnte Bildschirmdimension nicht abfragen.", "GetSystemMetrics", MB_ICONERROR|MB_OK);
                return -1;
            }

            if (RegisterClassEx(&wndcl)==0) {
                MessageBox(NULL, "Konnte Fensterklasse nicht registrieren.", "RegisterClassEx", MB_ICONERROR|MB_OK);
                return -1;
            }

            hWnd=CreateWindowEx(
                WS_EX_APPWINDOW, clsName, NULL, WS_VISIBLE, 0, 0,
                xmax, ymax, HWND_DESKTOP, NULL, hThisInstance,  NULL
            );
            if (hWnd==NULL) {
                MessageBox(NULL, "Fenster konnte nicht erstellt werden.", "CreateWindowEx", MB_ICONERROR|MB_OK);
                return -1;
            }

            if (GetCursorPos(&maus)==0) {
                MessageBox(NULL, "Kursorposition konnte nicht ermittelt werden.", "GetCursorPos",MB_ICONERROR|MB_OK);
                return -1;
            }
            if (ShowWindow(hWnd, show)==0) {
                MessageBox(NULL, "Bildschirmschoner konnte nicht hergestellt werden", "ShowWindow", MB_ICONERROR|MB_OK);
                return -1;
            }
            break;

        case SCR_PREV: {
            return 0; // Vorschau bisher nicht implementiert.
        }
        case SCR_CONFIG:
            MessageBox(0,"Dieser Bildschirmschoner ist nicht konfigurierbar.",0 , 0);
            return 1;

        case SCR_ERRPREV:
            MessageBox(0,"Ungültige Parameter.", 0, 0);
            break;

        default: //DEBUG!!
            MessageBox(0,"Bla",0,0);
            return 0;

    }

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }

    return 0;
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
        case WM_CREATE: {
            UINT_PTR tid_down=1;
            UINT_PTR tid_move=2;
            DWORD dw;

            dw=GetWindowLong(hWnd, GWL_STYLE);
            dw=dw&(~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX));
            SetWindowLong(hWnd, GWL_STYLE, dw);
            SetTimer(hWnd, tid_down, 150, NULL);
            SetTimer(hWnd, tid_move, 50, NULL);
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc, hdcmem;
            HBITMAP hbmp;

            hdc=BeginPaint(hWnd, &ps);

            hbmp=CreateBitmap(xmax, ymax, 1, 32, bmp);
            hdcmem=CreateCompatibleDC(hdc);
            SelectObject(hdcmem, hbmp);
            BitBlt(hdc, 0, 0, xmax, ymax, hdcmem, 0, 0, SRCCOPY);
            DeleteObject(hbmp);
            DeleteDC(hdcmem);

            EndPaint(hWnd, &ps);
            break;
        }
        case WM_TIMER: {
            RECT r0, r1, r;

            if (wParam==1) {
                down();
            }
            if (wParam==2) {
                if(best_r) {
                    brick2Rect(curbrick, &r0);
                    rotBrick(curbrick);
                    brick2Rect(curbrick, &r1);
                    
                    r.top=max(min(r0.top, r1.top)-kante, 0);
                    r.left=max(min(r0.left, r1.left)-kante, 0);
                    r.right=min(max(r0.right, r1.right)+kante, xmax);
                    r.bottom=min(max(r0.bottom, r1.bottom)+kante, ymax);

                    updateBmp(&r);
                    InvalidateRect(hWnd, NULL, FALSE);
                    best_r--;
                }
                break;
            }
            brick2Rect(curbrick, &r1);
            r.top=max(r1.top-kante, 0);
            r.left=max(r1.left-kante, 0);
            r.right=min(r1.right+kante, xmax);
            r.bottom=min(r1.bottom+kante, ymax);

            updateBmp(&r);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case WM_CLOSE:
            PostQuitMessage(0);
            break;


        //DefScreenSaverProc:
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP:
        case WM_NCACTIVATE:
            if (wParam==FALSE) {
                SendMessage(hWnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_SETCURSOR:
            SetCursor(NULL);
            return TRUE;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_KEYDOWN:
        case WM_KEYUP:
            PostQuitMessage(0);
            break;

        case WM_MOUSEMOVE: {
            DWORD lmp=(maus.y<<16)|maus.x;
            if (lmp!=lParam) {
                PostQuitMessage(0);
            }
            return 0;
        }

        case WM_DESTROY:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case WM_SYSCOMMAND:
            if (wParam==SC_CLOSE || wParam==SC_SCREENSAVE) {
                return FALSE;
            }
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}
