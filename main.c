/****************************************************************
 *腾讯游戏大厅连连看宇宙无敌全自动秒图
 *感谢: JiangJinXiao, WangHaoRan
 *最后编辑: 15-4-26
 *E-Mail: i@tinkrr.cc
 *Blog: blog.tinkrr.cc      
 ****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <locale.h>
#include <assert.h>

typedef struct point{//点结构
    int x;
    int y;
}s_point, *p_point;

typedef struct rect{//矩形结构
    int l;
    int r;
    int t;
    int b;
}s_rect, *p_rect;

typedef struct partten{//图案结构，用来保存从游戏中读取的待消除图案颜色值
    COLORREF colors[16];//每个图案取16个颜色点，防止重复
}s_pattern, *p_pattern;

typedef enum game_status{//当前游戏窗口状态
    GS_INGAME,//处在游戏中
    GS_OVER,//游戏超时，等待其他玩家完成消除并结束一场游戏
    GS_INROOM,//在房间里
    GS_CLOSED,//游戏窗口被关闭（主动关闭或者被踢出房间
    GS_MINIMIZED,//游戏窗口被最小化（无法获取颜色值）
    GS_UNKNOWN//未定义状态(这是错误)
}s_game_status, *p_game_status;

#define BACK_COLOR RGB(0x30,0x4C,0x70)//游戏客户区背景色
void init_map(char **map, int widht, int height)//根据长宽初始化一张二维表
{
    int i;
    printf("%d\n", widht*height);
    for (i = 0; i<widht*height; i++)
        ((char*)map)[i] = 0;//初始值为0，表示无图案
}

void show_map(char **map, int width, int height)//显示该表
{
    int i, j;
    putchar('\n');
    char chararr[128] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//要将图案转换成的字符
    putchar(' ');
    putchar(' ');
    for (j = 0; j<width; j++)//打印横坐标位置标识
        printf("%-2X", j);
    putchar('\n');
    for (i = 0; i<height; i++)//双层循环[i,j]，扫描整张表
    {
        for (j = 0; j<width; j++)
        {
            if (j == 0)
                printf("%-2X", i);//打印纵坐标位置标识
            char val = *((char*)map + i*width + j);//从表中(i,j)位置取值
            if (val == 0)//如果值为0，标识该处无图案，输出空白字符占位
            {
                putchar(' ');
                putchar(' ');
            }
            else{
                printf("%c ", val <= 25 ? chararr[val] : (char)'@');
                //如果该值不大于25，表示该值可以从chararr数组中找到，否则全部用@符号替代
            }
        }
        putchar('\n');
    }
}

void rand_map(char **map, int width, int height)//随机生成一张表，用以调试，实际程序中没有使用
{
    int i, j;
    for (i = 0; i<height; i++)
    {
        for (j = 0; j<width; j++)
        {
            char *val = ((char*)map + ((i)*width) + (j));//i+1,j+1
            *val = rand() % 10 + 1;//生成1到10的随机数
        }
    }

}

int exists_point(s_point pt, p_point pt_table, int size)
{
    int i;
    for (i = 0; i<size; i++)
    if (pt.x == pt_table[i].x && pt.y == pt_table[i].y)//判断点pt是否在pt_table数组中
        return 1;//存在返回1(true)
    return 0;
}

void try_to_set(s_point* st, int *size, int x, int y, char val)//尝试将点(x,y)加入st表中
{
    if (val>0)//如果点(x,y)处的值大于0(有图案)，则将该点加入点st数组
    {
        st[*size].x = x;
        st[*size].y = y;
        (*size)++;//将数组长度计数器+1
    }
}
//扫描整张图，获取扫描序列，保存到st点结构数组中,st和size为传出数据
//扫描顺序为顺时针方向扫，从表最外层到最内层
void enum_map(char **map, int width, int height, s_rect rc, s_point* st, int *size)
{
    int x, y;
    if (rc.r - rc.l<0 || rc.b - rc.t<0)//判断是否扫描完毕
    {
        return;
    }
    //向右扫描最上一行
    y = rc.t;
    for (x = rc.l; x<rc.r; x++)
    {
        try_to_set(st, size, x, y, ((char*)map)[width*y + x]);
    }
    //向下扫描最右一行
    x = rc.r;
    for (y = rc.t; y<rc.b; y++)
    {
        try_to_set(st, size, x, y, *(((char*)map) + width*y + x));
    }

    if (rc.t<rc.b)//判断矩形rc内是否有多行
    {
        y = rc.b;
        for (x = rc.r; x>rc.l; x--)//向左扫描最后一行
        {
            try_to_set(st, size, x, y, *(((char*)map) + width*y + x));
        }
    }
    else{//只有一行则扫描最后一个点
        x = rc.r; y = rc.b;
        try_to_set(st, size, x, y, *(((char*)map) + width*y + x));
    }
    if (rc.r>rc.l)//判断矩形内是否有多列
    {
        x = rc.l;
        for (y = rc.b; y>rc.t; y--)//向上扫描第一列
        {
            try_to_set(st, size, x, y, *(((char*)map) + width*y + x));
        }
    }
    else{//只有一列只扫描最下一个点
        x = rc.l; y = rc.b;

        try_to_set(st, size, x, y, *(((char*)map) + width*y + x));
    }

    rc.l++; rc.r--; rc.t++; rc.b--;//将矩形各边向里缩进一个单位，用来做递归
    enum_map(map, width, height, rc, st, size);//递归，你应该懂的
}


//扫描二维表map，判断点pt1和点pt2之间是否存在小于等于两个拐点以内的通路
//这个算法比较笨，考虑到游戏数据量比较小，所以本人未进行算法优化
//返回值1为存在通路，0为不存在通路
int exists_path(char **map, int width, int height, s_point pt1, s_point pt2)
{

    int i, j, x1, x2, y1, y2;
    int x, y;
    for (x = 0; x<width; x++)//判断 当两个拐点之间的线为垂直方向时，是否存在通路
    {
        //使x1在x2左侧，以便向右遍历
        if (x<pt1.x){
            x1 = x; x2 = pt1.x;
        }
        else{
            x1 = pt1.x; x2 = x;
        }

        for (i = x1; i <= x2; i++)//点(x1,pt1.y)到点(x2,pt1.y)如果有障碍，则退出本条连线测试（将x向右移动一个单位）
            if (((char*)map)[pt1.y*width + i] != 0 && i != pt1.x)
                goto cont;

        if (x<pt2.x){
            x1 = x; x2 = pt2.x;
        }
        else{
            x1 = pt2.x; x2 = x;
        }
        for (i = x1; i <= x2; i++)
            if (((char*)map)[pt2.y*width + i] != 0 && i != pt2.x)
                goto cont;
        if (pt1.y<pt2.y){
            y1 = pt1.y; y2 = pt2.y;
        }
        else{
            y1 = pt2.y; y2 = pt1.y;
        }

        for (i = y1 + 1; i<y2; i++)//测试垂直连线是否为通路，不是则退出本条线路测试
            if (((char*)map)[i*width + x] != 0)
                goto cont;
        return 1;
    cont:;
    }


    //判断 当两个拐点之间的线为水平方向时，是否存在通路，算法同上
    for (y = 0; y<height; y++)
    {
        //int y1,y2;
        if (y<pt1.y){
            y1 = y; y2 = pt1.y;
        }
        else{
            y1 = pt1.y; y2 = y;
        }

        for (i = y1; i <= y2; i++)
            if (((char*)map)[i*width + pt1.x] != 0 && i != pt1.y)
                goto cont1;
        if (y<pt2.y){
            y1 = y; y2 = pt2.y;
        }
        else{
            y1 = pt2.y; y2 = y;
        }

        for (i = y1; i <= y2; i++)
            if (((char*)map)[i*width + pt2.x] != 0 && i != pt2.y)
                goto cont1;
        if (pt1.x<pt2.x){
            x1 = pt1.x; x2 = pt2.x;
        }
        else{
            x1 = pt2.x; x2 = pt1.x;
        }
        for (i = x1 + 1; i<x2; i++)
            if (((char*)map)[y*width + i] != 0)
                goto cont1;
        return 1;

    cont1:;
    }
    return 0;
}

//这个比较重要，根据二维map表生成 “消除序列” acts
void dispel_map(char **map, int width, int height, p_point ptl, int size, p_point acts, int *act_size)
{
    int i, j;
    int n = 0;
    s_point *pass_table;
    //int table_size;
    //pass_table=malloc((width*height)*sizeof(s_point));//pass_table用来保存测试时无通路的点
    //table_size=0;
    //该处注释代码可能会用到

    for (i = 0; i<size; i++)
    {
        if (((char*)map)[width*ptl[i].y + ptl[i].x] == 0)
            continue;//跳过已经消除的点
        for (j = 0; j<size; j++)
        {
            if (((char*)map)[width*ptl[j].y + ptl[j].x] == 0)
                continue;//跳过已经消除的点
            //puts("m3");
            if (i == j || ((char*)map)[width*ptl[i].y + ptl[i].x] != ((char*)map)[width*ptl[j].y + ptl[j].x])
                continue;//去除两点不同以及两点重叠情况


            if (exists_path(map, width, height, ptl[i], ptl[j]))
            //如果两点ptl[i],ptl[j]之间存在通路、
            {
                ((char*)map)[width*ptl[i].y + ptl[i].x] = 0;//从表中清除两点
                ((char*)map)[width*ptl[j].y + ptl[j].x] = 0;

                assert(ptl[i].x<19);//感谢这几个断言，感谢师傅
                assert(ptl[j].y<11);
                assert(ptl[j].x<19);
                assert(ptl[i].y<11);

                acts[(*act_size)++] = ptl[i];//将两点加入到消除序列acts中
                acts[(*act_size)++] = ptl[j];
                break;

            }
            else
            {
                //
                //try_to_set(pass_table,&table_size,ptl[i].x,ptl[i].y,1);
                //将无通路的点加入pass_table
                //break;
            }

            //printf("(%d,%d)-(%d,%d) ",ptl[i].x,ptl[i].y,ptl[j].x,ptl[j].y);
        }
    }
    //free(pass_table);
    //free掉pass_table
}

//从游戏窗口读取图案数据，将图案数据转换为数字，保存到二维表map中
void make_map(HDC hDC, char **map, int width, int height)
{
    int i, j, k;

    s_pattern ptns[1024];//由于图案种类数量未知，数组定的大一点（1024）
    int ptn_count = 0;//图案个数

    for (i = 0; i<11; i++)//这两层循环很关键，不是扫描二维表，而是扫描游戏窗口客户区
    {
        for (j = 0; j<19; j++)
        {
            int k, l;
            int idx;
            int found = 0;
            COLORREF color;
            s_pattern ptn;
            int isptn = 0;//是否为背景色标记，一旦16个点全部为背景色，标识该处无图案
            for (k = 0; k<4; k++)//每个图案读取4*4=16个颜色点
            {

                for (l = 0; l<4; l++)
                {

                    ptn.colors[k * 4 + l] = GetPixel(hDC, 9 + j * 31 + 13 + l, 180 + i * 35 + 13 + k);
                    //从点(9 + j * 31 + 13 + l, 180 + i * 35 + 13 + k)处读取颜色
                    if (ptn.colors[k * 4 + l] != BACK_COLOR)//判断此点是否为背景色
                        isptn = 1;

                }

            }
            if (isptn == 0)//无图案则进入下一次循环
                continue;
            found = 0;
            for (k = 0; k<ptn_count; k++)//判断是图案是否在之前的扫描中出现过
            {
                int diff = 0;
                for (l = 0; l<16; l++)
                {
                    if (ptn.colors[l] != ptns[k].colors[l])
                    {
                        diff = 1;
                        break;
                    }
                }
                if (diff == 0)
                {
                    idx = k;
                    found = 1;
                    break;
                }
            }
            if (found == 0)//如果从图案表中未找到该团，则将该团添加到图案表中
            {
                idx = ptn_count;
                ptns[ptn_count] = ptn;
                ptn_count++;
            }
            ((char*)map)[width*(i)+j] = idx + 1;//将该图案转换为数字，添加到表中
        }

        //Sleep(500);
    }
    printf("Pattern Count: %d.\n", ptn_count);
}

//将消除序列转换为鼠标消息发送到消息窗口
void send_action(HWND hWnd, p_point acts, int size)
{
    int i;

    SetForegroundWindow(hWnd);//窗口前置
    SetActiveWindow(hWnd);//激活窗口
    for (i = 0; i<size; i++)//遍历“消除序列”
    {
        DWORD lParam;
        int x, y;
        x = 9 + (acts[i].x) * 31 + 13;//将map中的坐标转换为游戏窗口的坐标
        y = 180 + (acts[i].y) * 35 + 13;
        lParam = MAKELPARAM(x, y);//将坐标封装为DWORD
        SendMessageA(hWnd, WM_LBUTTONDOWN, 0, lParam);//发送鼠标按下消息
        SendMessageA(hWnd, WM_LBUTTONUP, 0, lParam);//发送鼠标谈起消息
        Sleep(1);
    }
}

s_game_status get_game_status(HWND hWnd)//获取游戏窗口状态
{
    HDC hDC;
    COLORREF color;
    if (!IsWindow(hWnd))//IsWindow()判断窗口句柄是否存在
        return GS_CLOSED;
    if (IsIconic(hWnd))//IsIconic()判断窗口是否最小化
        return GS_MINIMIZED;
    hDC = GetDC(hWnd);

    do{//该循环用来等待窗口绘图从不正常状态过度到正常状态，防止它抽风对我造成影响
        color = GetPixel(hDC,10,10);
        Sleep(10);
    }
    while (color == RGB(0xFF, 0xFF, 0xFF));//


    color = GetPixel(hDC, 10, 180);//这几个点的判断你自己琢磨吧，说多了没用
    if (color == RGB(0x00, 0x68, 0xA0))
        return GS_INROOM;
    color = GetPixel(hDC, 200, 315);
    if (color == RGB(0xF0, 0xF4, 0x00))
        return GS_OVER;

    color = GetPixel(hDC, 10, 180);
    if (color == RGB(0x30, 0x4C, 0x70))
    {
        return GS_INGAME;//F0 F4 00
    }
    return GS_UNKNOWN;
}

int quick_enter()//快速加入游戏，你能看懂
{
    HWND hWnd;
    WCHAR szClassName[1024];
    DWORD lParam;
    hWnd = FindWindowW(NULL, L"连连看");
    if (hWnd == NULL)
        return 0;

    lParam = MAKELPARAM(160, 105);
    SendMessageA(hWnd, WM_LBUTTONDOWN, 0, lParam);
    SendMessageA(hWnd, WM_LBUTTONUP, 0, lParam);
    Sleep(1000);
    lParam = MAKELPARAM(280, 135);
    SendMessageA(hWnd, WM_LBUTTONDOWN, 0, lParam);
    SendMessageA(hWnd, WM_LBUTTONUP, 0, lParam);
    Sleep(1);

    return 1;

}

void exit_room(HWND hWnd)//退出房间
{
    // 780 10
    DWORD lParam;
    DWORD dwPID=0;

    //点右上角的乂，退出房间
    lParam = MAKELPARAM(780, 10);
    SendMessageA(hWnd, WM_LBUTTONDOWN, 0, lParam);
    SendMessageA(hWnd, WM_LBUTTONUP, 0, lParam);
    Sleep(100);

    if(!IsWindowVisible(hWnd))//如果窗口为隐藏状态，则表示有其他窗口打开占用了进程，必须强行结束进程
    {
        //puts("m2");
        GetWindowThreadProcessId(hWnd,&dwPID);
        if(dwPID)
            TerminateProcess(OpenProcess(PROCESS_ALL_ACCESS,FALSE,dwPID),0);//强制结束游戏进程
    }

}


//这个是重中之重，从取点到发送消除鼠标事件的函数封装
//返回值： 1为全部清除，2为由于无解无法继续
int process(HWND hWnd)
{
    //puts("enter process()");
    int i;
    char * ptn_map;

    int map_width = 19, map_height = 11;//21,13
    p_point scan_table;
    int scan_table_size;
    p_point action_table;
    int action_table_size;
    int pre_scan_table_size = -1;
    HDC hDC;
    srand(time(NULL));
    scan_table = malloc((map_width*map_height)*sizeof(s_point));
    scan_table_size = 0;
    action_table = malloc((map_width*map_height)*sizeof(s_point));

    ptn_map = malloc((map_height)*(map_width)*sizeof(char));
    puts("Initializing Memory Map . . .");
    init_map(ptn_map, map_width, map_height);


    hDC = GetDC(hWnd);
    puts("Reading Data From Game Program . . .");
    make_map(hDC, ptn_map, map_width, map_height);

    show_map(ptn_map, map_width, map_height);
    //getchar();
    s_rect rc;
    rc.l = rc.t = 0;
    rc.r = map_width - 1;
    rc.b = map_height - 1;
    puts("Calculating Sequence For Scanning . . .");

    action_table_size = 0;
    while (1)
    //while (scan_table_size != pre_scan_table_size)
    {

        //pre_scan_table_size = scan_table_size;
        scan_table_size = 0;
        action_table_size = 0;
        enum_map(ptn_map, map_width, map_height, rc, scan_table, &scan_table_size);
        if(scan_table_size==0)
            break;
        if(scan_table_size==pre_scan_table_size)
        {
            //puts("Try To Resort . . .");
            //resort(hWnd);
            puts("Cannot Remove More Pattern.");
            return 0;//未全部消除
        }else
            pre_scan_table_size=scan_table_size;
        //printf("scan_table_size = %d\n",scan_table_size);
        //getchar();
        puts("Calculating Cancellation Queue . . .");

        dispel_map(ptn_map, map_width, map_height, scan_table, scan_table_size, action_table, &action_table_size);
        //show_map(ptn_map,map_width,map_height);
        puts("Sending Mouse Event . . .");
        send_action(hWnd, action_table, action_table_size);
        //show_map(ptn_map,map_width,map_height);
        //printf("scan_table_size = %d\n",scan_table_size);
        //Sleep(10);



    }
    return 1;//全部消除

    free(ptn_map);
    free(action_table);
    free(scan_table);
    //puts("exit process()");
    return;
}

//点击重列道具，消除无解情况
void resort(HWND hWnd)
{
    DWORD lParam;

    lParam = MAKELPARAM(650, 200);
    SendMessage(hWnd, WM_LBUTTONDOWN, 0, lParam);
    SendMessage(hWnd, WM_LBUTTONUP, 0, lParam);
    Sleep(8000);//等待那行该死的提示文字消失，否则会影响我的视线
}


int main()
{
    HWND hWnd;
#if 0
    puts("[Tencent LianLianKan Assistant 1.0]\n\n"
         "> Writen By Poilynx\n"
         "> poilynx@gmx.com\n"
         "> http://poilynx.nsscn.org\n"
         "> Neusoft Network Security Studio\n"
         "> All Rights Reserved.\n"
         "> 2015-4-25\n\n");
#else
    puts("[腾讯连连看辅助 版本1.0]\n\n"
         "> 作者：李希林\n"
         "> 邮箱：poilynx@gmx.com\n"
         "> 主页：http://poilynx.nsscn.org\n"
         "> 东软网络安全工作室\n"
         "> 保留所有版权\n"
         "> 2015-4-25\n\n");
#endif
    while (1)
    {
        HDC hDC;
        s_game_status pre_status;
        int time_sum = 0;
        puts("Finding Window . . .");
        //Sleep(1000);
        puts("Try to Enter Room . . .");
        //环检查是否存在游戏窗口，没有则继续循环
        while (!(hWnd = FindWindowW(L"#32770", L"QQ游戏 - 连连看角色版"))){
            quick_enter();//尝试快速加入
            Sleep(1000);
        }
        printf("Game Window Found, HWND: %p\n", hWnd);
        //Sleep(1000);

        hDC = GetDC(hWnd);
        //获取窗口DC
        pre_status = GS_CLOSED;
        while (1)
        {

            const int delay = 500;
            s_game_status status;
            status = get_game_status(hWnd);//delay毫秒获取一次窗口状态
            //printf("s = %d\n",status);
            int is_unknow=0;
            switch (status)
            {
            case GS_INGAME:

                if (pre_status != GS_INGAME)//判断上一次状态是否为本次状态，保证状态处理只有一次，下同
                {
                    //pre_status=GS_INGAME;
                    puts("Game Started! Let's Fuck It.");
                    Sleep(1900);

                    while(get_game_status(hWnd)==GS_INGAME)//再确认一次当前状态
                    {
                        if(process(hWnd))//获取数据并消除，详细请参见该函数实现部分
                        {
                            puts("Complated.");
                            break;
                        }
                        resort(hWnd);//重列，详细请参见该函数实现部分
                    }
                    /*
                    //游戏结束后前置换房间
                    Sleep(1800);
                    exit_room(hWnd);
                    Sleep(2000);
                    */
                }
                break;
            case GS_MINIMIZED:
                if (pre_status != GS_MINIMIZED)
                {
                    puts("Game Window Is Minimized,I Cannot Read Anything.");
                }
                break;
            case GS_OVER:
                if (pre_status != GS_OVER)
                {
                    puts("Game Over!");//游戏超时禁止再继续消除，这种估计不可能出现，目前没出现过。
                }
                break;
            case GS_INROOM:
                if (pre_status != GS_INROOM)
                {
                    DWORD lParam;
                    puts("Enter Room Now.");
                    lParam = MAKELPARAM(650, 570);
                    SendMessage(hWnd, WM_LBUTTONDOWN, 0, lParam);//点击开始按钮
                    SendMessage(hWnd, WM_LBUTTONUP, 0, lParam);
                    Sleep(2000);//取消游戏结束后闪烁


                    time_sum = 0;
                }
                else{
                    if(time_sum*delay / 1000>5)
                    {
                        DWORD lParam;
                        lParam = MAKELPARAM(650, 570);
                        SendMessage(hWnd, WM_LBUTTONDOWN, 0, lParam);//点击开始按钮
                        SendMessage(hWnd, WM_LBUTTONUP, 0, lParam);
                    }
                    if (time_sum*delay / 1000>20)//如果在房间等待的时间超时20秒
                    {
                        puts("Wait Timeout, I Will Leave Here.");
                        exit_room(hWnd);//退出房间，详细请参见该函数实现部分
                    }
                    else
                        time_sum++;
                }
                break;
            case GS_CLOSED:
                puts("Game Window Closed.");
                pre_status = status;
                goto closed;
            case GS_UNKNOWN:
                is_unknow=1;
            }
            if(!is_unknow)//忽略未知状态
                pre_status = status;//设置“上一次状态”
            Sleep(delay);
        }
    closed:;

    }
}



#if 0
int main()//取点测试用例
{

    while(1)
    {
    POINT pt;
    RECT rc;
    HDC hDC;
    HWND hWnd;
    GetCursorPos(&pt);
    hWnd=GetForegroundWindow();
    GetWindowRect(hWnd,&rc);
    SetCursorPos(rc.left+9,rc.top+180);

    hDC=GetDC(0);
    COLORREF c=GetPixel(hDC,pt.x,pt.y);
    printf("%X,%X,%X\n",GetRValue(c),GetGValue(c),GetBValue(c));
    Sleep(500);
    }
}

#endif

