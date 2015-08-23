#include "remotecontrol.h"
#include <iostream>
#include <QPixmap>
#include <QRgb>
#include <stdio.h>
#include <QStack>
#include <pthread.h>

static const int bpp=4;
static int maxx=0, maxy=0;
static int clientCount=0;  //the client number
Display *display;
Window winFocus;
Window winRoot;
typedef struct ClientData{
    rfbBool oldButton;
    int oldx, oldy;
}ClientData;

static int sendKeys(int keycode, bool press);
static int sendPointers(int buttonMask, int x, int y);

RemoteControl::RemoteControl(int argc, char **argv, QWidget *parent):
    QWidget(parent)
{
    this->argc = argc;
    this->argv = argv;
    timer = NULL;

}

void RemoteControl::initBuffer(unsigned char *buffer)
{
    QPixmap pm;
    pm = QPixmap::grabWindow(QApplication::desktop()->winId());

    pm = pm.scaled(maxx, maxy,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage image = pm.toImage();

    int i, j;
    for(i=0; i<maxy; i++)
    {
        const uchar* data = image.scanLine(i);
        memcpy(&buffer[i*maxx*bpp], data, maxx*bpp);
    }

    //开始计时
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));
    timer->start(100);

}

void RemoteControl::timeoutSlot()
{
    char *oldBuffer = rfbScreen->frameBuffer;

    QPixmap pm;
    pm = QPixmap::grabWindow(QApplication::desktop()->winId());
    pm = pm.scaled(maxx, maxy,Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QImage image = pm.toImage();
    int i, j;
    memset(oldBuffer, 0x00, maxx*maxy*bpp);
    for(i=0; i<maxy; i++)
    {
        const uchar* data = image.scanLine(i);
        memcpy(&oldBuffer[i*maxx*bpp], data, maxx*bpp);
    }

    rfbMarkRectAsModified(this->rfbScreen,0, 0, maxx-1, maxy-1);
}

static void clientgone(rfbClientPtr cl)
{
    clientCount--;
    rfbShutdownServer(cl->screen,TRUE);
    free(cl->clientData);

}

static enum rfbNewClientAction newclient(rfbClientPtr cl)
{
    clientCount++;
    cl->clientData = (void *)calloc(sizeof(ClientData),1);
    cl->clientGoneHook = clientgone;
    return RFB_CLIENT_ACCEPT;
}


static void doptr(int buttonMask, int x, int y, rfbClientPtr cl)
{
    static QStack<int> pressedButtons;
    /**
     *buttonask(mouse)说明：
     *左键按下：1； 中键按下：2；  右键按下：4；
     *滚轮向上：8； 滚轮向下：16； 松开：0
     */
    x= x*4/3; y=y*4/3;

    ClientData *cd = (ClientData*)cl->clientData;
    printf("buttonMask: %i\n"
             "x: %i\n" "y: %i\n", buttonMask, x, y);

    int tmp;
    XWarpPointer(display, None, winRoot, 0, 0, 0, 0, x, y);
    XFlush(display);

    XEvent event;
    memset(&event, 0x00, sizeof(event));
    event.xbutton.same_screen = True;

    if(0==buttonMask)  //松开按键或者仅仅是移动鼠标
    {
        if(pressedButtons.isEmpty()){
            return;
        }else{
            event.type = ButtonRelease;
            event.xbutton.button = pressedButtons.pop();
            event.xbutton.state = 0x100;
            printf("Button released: %i\n",  event.xbutton.button);
        }
    }else if(1==buttonMask) //左键按下
    {
        event.xbutton.button = Button1;
        int tempButton = 0;
        if(!pressedButtons.isEmpty())
            tempButton = pressedButtons.top();
        std::cerr << "last button:"<< tempButton << "  ";
        if(tempButton == Button1)//连续按住左键
        {
            event.type = MotionNotify;
            printf("Button motion: %i\n", Button1);
        }else{
            event.type = ButtonPress;
            pressedButtons.push(Button1);
            printf("Button pressed: %i\n", Button1);
        }

    }else if(2==buttonMask)//中键按下
    {
        event.xbutton.button = Button2;
        int tempButton = 0;
        if(!pressedButtons.isEmpty())
            tempButton = pressedButtons.top();
        std::cerr << "last button:"<< tempButton << "  ";
        if(tempButton == Button2)//连续按住左键
        {
            event.type = MotionNotify;
            printf("Button motion: %i\n", Button2);
        }else{
            event.type = ButtonPress;
            pressedButtons.push(Button2);
            printf("Button pressed: %i\n", Button2);
        }
    }else if(4==buttonMask) //右键按下
    {
        event.xbutton.button = Button3;
        int tempButton = 0;
        if(!pressedButtons.isEmpty())
            tempButton = pressedButtons.top();
        std::cerr << "last button:"<< tempButton << "  ";
        if(tempButton == Button3)//连续按住右键
        {
            event.type = MotionNotify;
            printf("Button motion: %i\n", Button3);
        }else{
            event.type = ButtonPress;
            pressedButtons.push(Button3);
            printf("Button pressed: %i\n", Button3);
        }
    }else if(8==buttonMask) //滚轮向上
    {
        event.xbutton.button = Button4;
        event.type = ButtonPress;
        pressedButtons.push(Button4);
        printf("Button pressed: %i\n", Button4);
    }else if(16==buttonMask) //滚轮向下
    {
        event.xbutton.button = Button5;
        event.type = ButtonPress;
        pressedButtons.push(Button5);
        printf("Button pressed: %i\n", Button5);
    }
    fflush(NULL);

    XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root,
                  &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

    event.xbutton.subwindow = event.xbutton.window;
    while(event.xbutton.subwindow)
    {
        event.xbutton.window = event.xbutton.subwindow;
        XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow,
                      &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x,
                      &event.xbutton.y, &event.xbutton.state);
    }

    if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)
        printf("Errore nell'invio dell'evento !!!\n");
    XFlush(display);

    //rfbDefaultPtrAddEvent(buttonMask, x, y, cl);
}

static void MakeRichCursor(rfbScreenInfoPtr rfbScreen)
{
    int i, j, w=14, h=12;
    rfbCursorPtr c = rfbScreen->cursor;
    char bitmap[]=
                "**    **    **"
                " **   **   ** "
                "  **  **  **  "
                "   ** ** **   "
                "     ****     "
                "**************"
                "**************"
                "     ****     "
                "   ** ** **   "
                "  **  **  **  "
                " **   **   ** "
                "**    **    **";

    c = rfbScreen->cursor = rfbMakeXCursor(w,h, bitmap, bitmap);
    c->xhot = w/2; c->yhot = h/2;

    c->richSource = (unsigned char*)malloc(w*h*bpp);
    memset(c->richSource, 0XFF, w*h*bpp);
    memset(c->richSource, 0X00, w*bpp);
    memset(&c->richSource[(h-1)*w*bpp], 0X00, w*bpp);
    for(int j=1; j<h-1; j++)
    {
        memset(&c->richSource[j*w*bpp], 0X00, bpp);
        memset(&c->richSource[j*w*bpp+(w-1)*bpp], 0X00, bpp);
    }
    c->cleanupRichSource = TRUE;

}

static void dokey(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    //if(down)
    //{
        sendKeys((int)key, (bool)down);
    //}
    return;
}

static XKeyEvent createKeyEvent(Display *display, Window &win,
                               Window &winRoot, bool press, int keycode, int modifiers)
{
    XKeyEvent event;

    event.display    = display;
    event.window      = win;
    event.root        = winRoot;
    event.subwindow  = None;
    event.time        = CurrentTime;
    event.x          = 1;
    event.y          = 1;
    event.x_root      = 1;
    event.y_root      = 1;
    event.same_screen = TRUE;
    event.state      = modifiers;
    event.keycode    = XKeysymToKeycode(display,keycode);
    if(press)
        event.type = KeyPress;
    else
        event.type = KeyRelease;

    return event;
}

static int sendKeys(int keycode, bool press)
{
    int modifier= 0; //pubModifier->text().toInt();

    // Find the window which has the current keyboard focus.
    Window winFocus;
    int revert=RevertToParent;

    XGetInputFocus(display, &winFocus,&revert);

    // Send a fake key press event to the window.
    XSelectInput(display, winFocus,FocusChangeMask|KeyPressMask|KeyReleaseMask);
    XMapWindow(display, winFocus);

    XKeyEvent event = createKeyEvent(display, winFocus, winRoot, press, keycode, modifier);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
/*
    event = createKeyEvent(display, winFocus, winRoot, false, keycode, modifier);
    XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
*/
    return 0;
}

void RemoteControl::startServer()
{
    if ( (display=XOpenDisplay(NULL)) == NULL )
    {
            fprintf( stderr, "Cannot connect to X server\n");
            exit( -1 );
    }
    winRoot = DefaultRootWindow(display);
    int display_width = DisplayWidth(display, DefaultScreen(display));
    int display_height = DisplayHeight(display, DefaultScreen(display));
    maxx = display_width*3/4;
    maxy = display_height*3/4;
    rfbScreen = rfbGetScreen(&argc,argv,maxx,maxy,8,3,bpp);
    if(!rfbScreen)
       return;
    rfbScreen->desktopName = "LibVNCServer Example";
    rfbScreen->frameBuffer = (char*)malloc(maxx*maxy*bpp);
    rfbScreen->alwaysShared = TRUE;
    rfbScreen->ptrAddEvent = doptr;
    rfbScreen->kbdAddEvent = dokey;
    rfbScreen->newClientHook = newclient;


    initBuffer((unsigned char*)rfbScreen->frameBuffer);
    //rfbDrawString(rfbScreen,&radonFont,20,100,"Hello, World!",0xffffff);
    MakeRichCursor(rfbScreen);
    rfbInitServer(rfbScreen);
    rfbRunEventLoop(rfbScreen,-1,TRUE);
    return;
}

void RemoteControl::shutdownServer()
{
    rfbShutdownServer(rfbScreen,TRUE);
}

static void* clientFunc(void* arg)
{
    char *cmd = (char*)arg;
    std::cerr << cmd << "\n";
    system(cmd);
    return 0;
}

void RemoteControl::startClient(const char *ip)
{
    pthread_t pid;
    int err;

    QString cmd = QString("vncviewer %1:0").arg(ip);
    err = pthread_create(&pid,NULL,clientFunc,(void*)cmd.toStdString().c_str());
    if(err != 0){
       printf("can't create thread: %s\n",strerror(err));
       return;
    }

}



