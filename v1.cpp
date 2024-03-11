#include <bits/stdc++.h>
using namespace std;

const char BLANK = '.'; //空地
const char OCEAN = '*'; //海洋
const char BARRITER = '#';  //障碍
const char ROBOT_POSITION = 'A';    //机器人起始位置，共10个，初始化之后应当设为空地。
const char BERTH_POSITION = 'B';    //泊位初始位置，共10个，左上角坐标
const int TO_BERTH_TIME = 500;  //泊位之间移动耗时
const int GOODS_ACTIVE_TIME = 1000; //货物存活时间
const int n = 200;  //地图大小
const int robot_num = 10;   //机器人数量
const int berth_num = 10;   //泊位数量
const int boat_num = 5;     //船数量
const int N = 210;  

int frameID = 0;    //当前帧ID
int money, boat_capacity;

set<int> goods_set; //用于判断某个货物是否已经是某个机器人的目标
int avoid_map[N][N];    //数字越小，占用该位置的优先级越高

//地图       
//存在地图上的移动信息：当前位置到泊位的最短路的下一步
enum StepType{
    STOP = 0,   //还不知道有什么用
    UP = 1,
    RIGHT = 2,
    DOWN = 3,
    LEFT = 4
};   

struct Map{
    StepType nextStep = STOP;  //下一步

    //当前位置的类型
    char positionType = BARRITER;  //位置的类型
    bool occupancyFlag = false; //占用标记，用于碰撞避让
    int goods_value = 0;    //该点的货物价值，如果没有货物则价值为0
    //初始化函数
    void MapInit(){
        nextStep = STOP;
        positionType = BLANK;
        goods_value = 0;
    }
}myMap[n + 5][n + 5];

//货物
int GoodsCount = 0; //货物计数
struct Goods{
    int GoodsID;//货物编号
    int x = 0, y = 0;    //坐标
    int val = 0;    //价值
    int flag = 0;   //是否被锁定
    int start_frame_id = 0;
    Goods(){
        GoodsID = 0;
        x = y = val = flag = 0;
    }
};

//机器人
enum RobotStatus{
    IDLE,   //空闲
    GO_GOODS,  //确定了目标货物并且正在路上
    GET_GOODS, //到了并且拿到货物
    GO_BERTH,   //前往泊位
    PUT_GOODS,  //把货物放到泊位
    RECOVER,    //恢复状态
};

struct Robot{
    int x = 0, y = 0;   //机器人坐标
    int have_goods = 0;   //是否持有货物  -0 未持有 -1 正常持有
    int statu = 0; //官方的机器人状态 -0 恢复状态 -1 正常运行
    int proprity = 0;   //优先级
    int avoding = 0;   //避让状态持续帧数，0表示不在避让状态
    Goods* target_goods = nullptr;  //目标货物指针
    RobotStatus myStatus = RobotStatus::IDLE;   //机器人状态
    deque<StepType> action;
    // Robot() {}
    // Robot(int startX, int startY) {
    //     x = startX;
    //     y = startY;
    // }
}robot[robot_num + 10];

//泊位
struct Berth{
    int x;
    int y;
    int transport_time;
    int loading_speed;
    Berth(){}
    Berth(int x, int y, int transport_time, int loading_speed) {
        this -> x = x;
        this -> y = y;
        this -> transport_time = transport_time;
        this -> loading_speed = loading_speed;
    }
}berth[berth_num + 10];

//船
enum BoatStatus{
    MOVING, //移动中
    WORKING,    //正在装载货物
    IDEL
};
struct Boat{
    int num, pos, status;
    BoatStatus myStatus;
}boat[10];




Goods* map_goods[N][N];   //记录坐标对应的货物指针
char ch[N][N];
int gds[N][N];

struct Coordination{
    int x, y;
};

int dx[4] = {1, -1, 0 ,0};
int dy[4] = {0, 0, 1, -1}; 

void bfs(Berth now, int dist[N][N]){    
    int vis[N][N];  //是否在队列中
    memset(vis, 0, sizeof(vis));

    queue<Coordination> Q;
    Q.push({now.x, now.y});
    vis[now.x][now.y] = 1;

    while(!Q.empty()){
        Coordination u = Q.front();
        Q.pop();
        vis[u.x][u.y] = 0;

        Coordination v;
        for(int i = 0; i < 4; ++i){
            v.x = u.x + dx[i];
            v.y = u.y + dy[i];
            if(v.x < 0 || v.x > n || v.y < 0 || v.y > n) continue;  //越界
            if(myMap[v.x][v.y].positionType == BARRITER || myMap[v.x][v.y].positionType == OCEAN) continue;   //障碍 或 海洋
            if(dist[v.x][v.y] > dist[u.x][u.y] + 1){
                dist[v.x][v.y] = dist[u.x][u.y] + 1;
                if(i == 0){
                    myMap[v.x][v.y].nextStep = StepType::UP;
                }else if(i == 1){
                    myMap[v.x][v.y].nextStep = StepType::DOWN;
                }else if(i == 2){
                    myMap[v.x][v.y].nextStep = StepType::LEFT;
                }else if(i == 3){
                    myMap[v.x][v.y].nextStep = StepType::RIGHT;
                }
                if(!vis[v.x][v.y]){
                    vis[v.x][v.y] = 1;
                    Q.push({v.x, v.y});
                }
            }
        }
    }
}

//计算路径，每个泊位为源点BFS。
void calculate_route(){    
    int dist[N][N]; //每个点的最小步数      
    memset(dist, 0x3f, sizeof(dist));  

    for(int i = 0; i < berth_num; ++i){
        bfs(berth[i], dist);
    }
}
//读入初始化数据
void Init(){
    //初始化机器人
    for(int i = 0; i < robot_num; ++i){
        robot[i].myStatus = RobotStatus::IDLE;
        robot[i].have_goods = 0;
    }
    //初始化船
    for(int i = 0; i < boat_num; ++i){
        boat[i].myStatus = IDEL;
        boat[i].pos = -1;   //虚拟点
    }
    //读入地图数据
    for(int i = 0; i < n; ++i){
        for(int j = 0; j < n; ++j){
            myMap[i][j].MapInit();
            scanf("%c", &myMap[i][j].positionType);
            //机器人位置直接修改为空地
            if(myMap[i][j].positionType == ROBOT_POSITION){
                myMap[i][j].positionType = BLANK;
            }
        }
        getchar();
    }
    //读入泊位数据
    for(int i = 0; i < berth_num; i ++)
    {
        int id;
        scanf("%d", &id);
        scanf("%d%d%d%d", &berth[id].x, &berth[id].y, &berth[id].transport_time, &berth[id].loading_speed);
    }
    //船容量
    scanf("%d", &boat_capacity);
    char okk[100];
    scanf("%s", okk);

    //计算每个位置到泊位的最短路
    calculate_route();

    printf("OK\n");
    fflush(stdout);
}

int Input(){
    scanf("%d%d", &frameID, &money);
    int num;
    scanf("%d", &num);
    for(int i = 1; i <= num; i ++){
        int x, y, val;
        scanf("%d%d%d", &x, &y, &val);
        Goods* goods_ptr = new Goods;
        goods_ptr->start_frame_id = frameID;
        goods_ptr->val = val;
        goods_ptr->x = x;
        goods_ptr->y = y;
        map_goods[x][y] = goods_ptr;
    }
    for(int i = 0; i < robot_num; i ++){
        scanf("%d%d%d%d", &robot[i].have_goods, &robot[i].x, &robot[i].y, &robot[i].statu);
        robot[i].proprity = i;  //优先级
        avoid_map[robot[i].x][robot[i].y] = i;  //在优先级地图中占用这个位置

        if(robot[i].have_goods == 0 && robot[i].myStatus >= GET_GOODS){
            //如果输入显示机器人没有拿到货物，但是机器人的状态是拿到货物及之后的状态，那么强制转为空闲
            robot[i].myStatus = RobotStatus::IDLE;
        }
    }
    for(int i = 0; i < 5; i ++)
        scanf("%d%d\n", &boat[i].status, &boat[i].pos);
    char okk[100];
    scanf("%s", okk);
    return frameID;
}

//搜索合适的货物
bool find_goods(int x,int y, Robot& mRobot, deque<StepType>& action){
    bool flag_found = false; //是否找到目标货物标记
    int step_map[N][N]; //记录路径的
    memset(step_map, 0, sizeof(step_map[N][N]));
    int vis[N][N];  //是否在队列中
    memset(vis, 0, sizeof(vis));
    int dist[N][N];
    memset(dist, 0, sizeof(dist));
    queue<Coordination> Q;
    Q.push({x,y});
    vis[x][y] = 1;
    while(!Q.empty()){
        Coordination u = Q.front();
        Q.pop();
        vis[u.x][u.y] = 0;

        Coordination v;
        for(int i = 0; i < 4; ++i){
            v.x = u.x + dx[i];
            v.y = u.y + dy[i];
            if(v.x < 0 || v.x > n || v.y < 0 || v.y > n) continue;  //越界
            if(myMap[v.x][v.y].positionType == BARRITER || myMap[v.x][v.y].positionType == OCEAN) continue;   //障碍 或 海洋
            if(dist[v.x][v.y] > dist[u.x][u.y] + 1){    //更新短路径
                dist[v.x][v.y] = dist[u.x][u.y] + 1;
                if(i == 0){
                    step_map[v.x][v.y] = StepType::UP;
                }else if(i == 1){
                    step_map[v.x][v.y] = StepType::DOWN;
                }else if(i == 2){
                    step_map[v.x][v.y] = StepType::LEFT;
                }else if(i == 3){
                    step_map[v.x][v.y] = StepType::DOWN;
                }
                if(!vis[v.x][v.y]){
                    vis[v.x][v.y] = 1;
                    Q.push({v.x, v.y});
                }
            }
            if(myMap[v.x][v.y].goods_value != 0 && //如果这个位置上有货物,
             map_goods[v.x][v.y]->flag == false && //这个货物没有被任何机器人作为目标
             dist[v.x][v.y] + 10 < GOODS_ACTIVE_TIME - (frameID - map_goods[v.x][v.y]->start_frame_id)){ //能够在时间内到达，+10是为了给可能的避让留出时间。
                flag_found = true;
                map_goods[v.x][v.y]->flag = true;   //成为目标
                mRobot.target_goods = map_goods[v.x][v.y];
                break;
            }            
        }
        if(flag_found == true)
            break;
    }
    if(flag_found == false){//没有找到任何一个合适的货物
        return false;
    }
    //找到了目标货物
    //从目标货物开始回溯，并记录路径
    Coordination now;
    now.x = x;
    now.y = y;
    while(now.x != x || now.y != y){
        int t = step_map[now.x][now.y];
        if(t == StepType::UP){
            action.push_front(StepType::DOWN);
            now.x--;
        }else if(t == StepType::DOWN){
            action.push_front(StepType::UP);
            now.x++;
        }else if(t == StepType::LEFT){
            action.push_front(StepType::RIGHT);
            now.y--;
        }else if(t == StepType::RIGHT){
            action.push_front(StepType::LEFT);
            now.y++;
        }
    }
    return true;
}

//判断是否需要避让
bool need_avoid(const Coordination& v, int now_proprity){
    if(avoid_map[v.x][v.y] < now_proprity){ //需要避让

    }
}

void go_goods(Robot& now){
    StepType now_step = now.action.front(); //获取这一步的计划（可能会改变，比如为了避让）
    Coordination v;
    v.x = now.x;
    v.y = now.y;
    switch (now_step){
        case StepType::UP:
            v.x = now.x - 1;
            break;
        case StepType::DOWN:
            v.x = now.x + 1;
            break;
        case StepType::RIGHT:
            v.y = now.y + 1;
            break;
        case StepType::LEFT:
            v.y = now.y - 1;
            break;
        default:
            break;
    }
    if(need_avoid(v, now.proprity)){

    }

}

void robot_work(){
    for(int i = 0; i < robot_num; ++i){
        if(robot[i].avoding)
        switch (robot[i].myStatus){
            case RobotStatus::IDLE:
                Goods target;                
                while(!robot[i].action.empty()) //清空行动队列
                    robot[i].action.pop_back();
                if(find_goods(robot[i].x, robot[i].y, robot[i], robot[i].action)){    //是否成功找到目标货物
                    robot[i].myStatus = RobotStatus::GO_GOODS;
                }else{
                    //未找到货物则保持当前状态
                    break;
                }
                break;
            case RobotStatus::GO_GOODS:
                go_goods(robot[i]);
                break;
            default:
                break;
        }
    }
}

void ship_work(){
    
}

int main(){
    Init();
    for(int frame = 1; frame <= 15000; frame ++){
        Input();
        for(int i = 0; i < robot_num; i ++)
            printf("move %d %d\n", i, rand() % 4);
        puts("OK");
        fflush(stdout);
    }

    return 0;
}
