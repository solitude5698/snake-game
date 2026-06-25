#include <iostream>
#include <conio.h>
#include <windows.h>
#include <vector>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <algorithm>

using namespace std;

// 颜色控制
enum Color {
    BLACK = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    BRIGHT_BLUE = 9,
    BRIGHT_GREEN = 10,
    BRIGHT_CYAN = 11,
    BRIGHT_RED = 12,
    BRIGHT_MAGENTA = 13,
    BRIGHT_YELLOW = 14,
    BRIGHT_WHITE = 15
};

// 游戏配置
class Config {
public:
    static const int WIDTH = 50;
    static const int HEIGHT = 22;
    static const int INITIAL_SPEED = 150; // 毫秒
    static const int MAX_LEVEL = 10;
    static const int SCORE_PER_FOOD = 10;
    static const int BONUS_FOOD_SCORE = 50;
};

// 位置结构
struct Position {
    int x, y;
    Position(int _x = 0, int _y = 0) : x(_x), y(_y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// 食物类
class Food {
private:
    Position pos;
    int type; // 0:普通, 1:加速, 2:减速, 3:双倍积分, 4:无敌
    bool active;
    int timer;

public:
    Food() : active(false), type(0), timer(0) {}

    void generate(const vector<Position>& snake, int width, int height) {
        vector<Position> available;
        for (int i = 1; i < height - 1; i++) {
            for (int j = 1; j < width - 1; j++) {
                Position p(j, i);
                if (find(snake.begin(), snake.end(), p) == snake.end()) {
                    available.push_back(p);
                }
            }
        }
        
        if (!available.empty()) {
            int idx = rand() % available.size();
            pos = available[idx];
            type = rand() % 5; // 0-4
            active = true;
            timer = 100; // 存在时间（帧数）
        }
    }

    Position getPos() const { return pos; }
    int getType() const { return type; }
    bool isActive() const { return active; }
    void setActive(bool a) { active = a; }
    void decrementTimer() { if (active) timer--; }
    bool isExpired() const { return timer <= 0; }
};

// 蛇类
class Snake {
private:
    deque<Position> body;
    int direction; // 0:上, 1:右, 2:下, 3:左
    int nextDirection;
    bool isGrowing;
    bool isInvincible;
    int invincibleTimer;

public:
    Snake() : direction(1), nextDirection(1), isGrowing(false), 
              isInvincible(false), invincibleTimer(0) {
        // 初始蛇：长度为3，水平放置
        body.push_back(Position(10, 10));
        body.push_back(Position(9, 10));
        body.push_back(Position(8, 10));
    }

    void setDirection(int dir) {
        if ((direction + 2) % 4 != dir) { // 不能掉头
            nextDirection = dir;
        }
    }

    void move() {
        direction = nextDirection;
        Position head = body.front();
        
        switch (direction) {
            case 0: head.y--; break;
            case 1: head.x++; break;
            case 2: head.y++; break;
            case 3: head.x--; break;
        }
        
        body.push_front(head);
        
        if (!isGrowing) {
            body.pop_back();
        } else {
            isGrowing = false;
        }
        
        if (isInvincible) {
            invincibleTimer--;
            if (invincibleTimer <= 0) {
                isInvincible = false;
            }
        }
    }

    void grow() { isGrowing = true; }
    void setInvincible(int duration) { 
        isInvincible = true; 
        invincibleTimer = duration;
    }
    bool isInvincibleState() const { return isInvincible; }

    deque<Position>& getBody() { return body; }
    const deque<Position>& getBody() const { return body; }
    Position getHead() const { return body.front(); }
    int getDirection() const { return direction; }

    bool checkCollision(int width, int height) const {
        Position head = body.front();
        // 墙壁碰撞
        if (head.x <= 0 || head.x >= width - 1 || 
            head.y <= 0 || head.y >= height - 1) {
            return true;
        }
        // 自身碰撞（头不能碰到身体其他部分）
        for (size_t i = 1; i < body.size(); i++) {
            if (body[i] == head) {
                return true;
            }
        }
        return false;
    }
};

// 游戏类
class Game {
private:
    Snake snake;
    Food food;
    Food bonusFood; // 额外的奖励食物
    int score;
    int highScore;
    int level;
    int speed;
    bool gameOver;
    bool paused;
    bool showHelp;
    int frameCount;
    int foodEatenCount;
    HANDLE console;
    COORD cursorPos;

    // 墙壁样式
    static const char WALL_CHAR = '#';
    static const char SNAKE_HEAD = 'O';
    static const char SNAKE_BODY = 'o';
    static const char FOOD_CHAR = '*';
    static const char BONUS_FOOD_CHAR = '&';
    static const char EMPTY_CHAR = ' ';

public:
    Game() : score(0), highScore(0), level(1), speed(Config::INITIAL_SPEED),
             gameOver(false), paused(false), showHelp(false), 
             frameCount(0), foodEatenCount(0) {
        console = GetStdHandle(STD_OUTPUT_HANDLE);
        srand(time(0));
        loadHighScore();
        vector<Position> snakeBody(snake.getBody().begin(), snake.getBody().end());
        food.generate(snakeBody, Config::WIDTH, Config::HEIGHT);
    }

    ~Game() {
        saveHighScore();
    }

    void run() {
        while (!gameOver) {
            handleInput();
            if (!paused && !gameOver) {
                update();
                render();
                Sleep(speed);
            } else if (paused) {
                render();
                Sleep(100);
            }
        }
        showGameOver();
    }

private:
    void handleInput() {
        if (_kbhit()) {
            int key = _getch();
            if (key == 224) { // 方向键
                key = _getch();
                switch (key) {
                    case 72: snake.setDirection(0); break;
                    case 77: snake.setDirection(1); break;
                    case 80: snake.setDirection(2); break;
                    case 75: snake.setDirection(3); break;
                }
            } else {
                switch (tolower(key)) {
                    case 'p': paused = !paused; break;
                    case 'h': showHelp = !showHelp; break;
                    case 'r': restart(); break;
                    case 27: gameOver = true; break; // ESC
                }
            }
        }
    }

    void update() {
        frameCount++;
        
        // 更新食物计时器
        food.decrementTimer();
        if (food.isExpired()) {
            vector<Position> snakeBody(snake.getBody().begin(), snake.getBody().end());
            food.generate(snakeBody, Config::WIDTH, Config::HEIGHT);
        }

        // 更新奖励食物
        if (bonusFood.isActive()) {
            bonusFood.decrementTimer();
            if (bonusFood.isExpired()) {
                bonusFood.setActive(false);
            }
        }

        // 偶尔生成奖励食物
        if (frameCount % 30 == 0 && !bonusFood.isActive() && rand() % 5 == 0) {
            vector<Position> snakeBody(snake.getBody().begin(), snake.getBody().end());
            bonusFood.generate(snakeBody, Config::WIDTH, Config::HEIGHT);
        }

        // 移动蛇
        snake.move();

        // 检查碰撞
        if (snake.checkCollision(Config::WIDTH, Config::HEIGHT)) {
            if (snake.isInvincibleState()) {
                // 无敌状态穿墙
                Position head = snake.getHead();
                if (head.x <= 0) head.x = Config::WIDTH - 2;
                else if (head.x >= Config::WIDTH - 1) head.x = 1;
                if (head.y <= 0) head.y = Config::HEIGHT - 2;
                else if (head.y >= Config::HEIGHT - 1) head.y = 1;
                snake.getBody().front() = head;
            } else {
                gameOver = true;
                return;
            }
        }

        // 检查是否吃到食物
        checkFoodCollision();
    }

    void checkFoodCollision() {
        Position head = snake.getHead();
        
        // 普通食物
        if (food.isActive() && head == food.getPos()) {
            eatFood(food.getType());
            food.setActive(false);
            vector<Position> snakeBody(snake.getBody().begin(), snake.getBody().end());
            food.generate(snakeBody, Config::WIDTH, Config::HEIGHT);
            foodEatenCount++;
            
            // 每吃5个食物升一级
            if (foodEatenCount % 5 == 0 && level < Config::MAX_LEVEL) {
                level++;
                speed = max(50, Config::INITIAL_SPEED - (level - 1) * 10);
            }
        }

        // 奖励食物
        if (bonusFood.isActive() && head == bonusFood.getPos()) {
            eatFood(bonusFood.getType());
            bonusFood.setActive(false);
            score += Config::BONUS_FOOD_SCORE;
        }
    }

    void eatFood(int type) {
        snake.grow();
        
        switch (type) {
            case 0: // 普通
                score += Config::SCORE_PER_FOOD;
                break;
            case 1: // 加速
                score += Config::SCORE_PER_FOOD;
                speed = max(50, speed - 20);
                break;
            case 2: // 减速
                score += Config::SCORE_PER_FOOD;
                speed = min(300, speed + 20);
                break;
            case 3: // 双倍积分
                score += Config::SCORE_PER_FOOD * 2;
                break;
            case 4: // 无敌
                score += Config::SCORE_PER_FOOD;
                snake.setInvincible(50);
                break;
        }
        
        if (score > highScore) {
            highScore = score;
        }
    }

    void render() {
        setCursorPosition(0, 0);
        
        // 绘制边框
        setColor(BRIGHT_CYAN);
        for (int i = 0; i < Config::WIDTH; i++) cout << WALL_CHAR;
        cout << endl;

        for (int i = 0; i < Config::HEIGHT; i++) {
            for (int j = 0; j < Config::WIDTH; j++) {
                if (j == 0 || j == Config::WIDTH - 1 || i == 0 || i == Config::HEIGHT - 1) {
                    setColor(BRIGHT_CYAN);
                    cout << WALL_CHAR;
                } else {
                    Position p(j, i);
                    bool printed = false;
                    
                    auto& body = snake.getBody();
                    for (size_t k = 0; k < body.size(); k++) {
                        if (body[k] == p) {
                            if (k == 0) {
                                setColor(snake.isInvincibleState() ? BRIGHT_MAGENTA : BRIGHT_GREEN);
                                cout << SNAKE_HEAD;
                            } else {
                                setColor(GREEN);
                                cout << SNAKE_BODY;
                            }
                            printed = true;
                            break;
                        }
                    }
                    
                    if (!printed) {
                        if (food.isActive() && food.getPos() == p) {
                            setColor(BRIGHT_RED);
                            cout << FOOD_CHAR;
                            printed = true;
                        } else if (bonusFood.isActive() && bonusFood.getPos() == p) {
                            setColor(BRIGHT_YELLOW);
                            cout << BONUS_FOOD_CHAR;
                            printed = true;
                        }
                    }
                    
                    if (!printed) {
                        setColor(WHITE);
                        cout << EMPTY_CHAR;
                    }
                }
            }
            cout << endl;
        }

        setColor(WHITE);
        string info = "分数:" + to_string(score) + " 最高分:" + to_string(highScore) + " 等级:" + to_string(level) + " 速度:" + to_string(speed) + "ms 长度:" + to_string(snake.getBody().size());
        if (info.length() > Config::WIDTH) info = info.substr(0, Config::WIDTH);
        cout << info;
        cout << string(max(0, Config::WIDTH - (int)info.length()), ' ') << endl;
        
        string foodInfo = "当前食物: ";
        switch (food.getType()) {
            case 0: foodInfo += "普通"; break;
            case 1: foodInfo += "加速"; break;
            case 2: foodInfo += "减速"; break;
            case 3: foodInfo += "双倍积分"; break;
            case 4: foodInfo += "无敌"; break;
        }
        if (snake.isInvincibleState()) {
            foodInfo += " ★无敌";
        }
        if (foodInfo.length() > Config::WIDTH) foodInfo = foodInfo.substr(0, Config::WIDTH);
        cout << foodInfo;
        cout << string(max(0, Config::WIDTH - (int)foodInfo.length()), ' ') << endl;
        
        string helpLine = "[P]暂停 [H]帮助 [R]重开 [ESC]退出";
        cout << helpLine;
        cout << string(max(0, Config::WIDTH - (int)helpLine.length()), ' ') << endl;
        
        if (paused) {
            setColor(BRIGHT_YELLOW);
            string pauseLine = "===== 游戏暂停 =====";
            cout << pauseLine;
            cout << string(max(0, Config::WIDTH - (int)pauseLine.length()), ' ') << endl;
        } else {
            cout << string(Config::WIDTH, ' ') << endl;
        }
        
        if (showHelp) {
            setColor(BRIGHT_YELLOW);
            string help1 = "帮助: 方向键移动 | P暂停 | H开关帮助";
            cout << help1;
            cout << string(max(0, Config::WIDTH - (int)help1.length()), ' ') << endl;
            string help2 = "食物: *普通+10 | *加速+10 | *减速+10";
            cout << help2;
            cout << string(max(0, Config::WIDTH - (int)help2.length()), ' ') << endl;
        } else {
            cout << string(Config::WIDTH, ' ') << endl;
            cout << string(Config::WIDTH, ' ') << endl;
        }
        
        setColor(WHITE);
        cout.flush();
    }

    void showGameOver() {
        system("cls");
        setColor(BRIGHT_RED);
        cout << "\n========== 游戏结束 ==========" << endl;
        cout << "最终分数: " << score << endl;
        cout << "最高纪录: " << highScore << endl;
        cout << "到达等级: " << level << endl;
        cout << "蛇身长度: " << snake.getBody().size() << endl;
        cout << "按 R 重新开始, ESC 退出" << endl;
        
        while (true) {
            if (_kbhit()) {
                int key = _getch();
                if (key == 'r' || key == 'R') {
                    restart();
                    run();
                    break;
                } else if (key == 27) {
                    break;
                }
            }
        }
    }

    void restart() {
        snake = Snake();
        score = 0;
        level = 1;
        speed = Config::INITIAL_SPEED;
        gameOver = false;
        paused = false;
        frameCount = 0;
        foodEatenCount = 0;
        vector<Position> snakeBody(snake.getBody().begin(), snake.getBody().end());
        food.generate(snakeBody, Config::WIDTH, Config::HEIGHT);
        bonusFood.setActive(false);
    }

    void setColor(int color) {
        SetConsoleTextAttribute(console, color);
    }

    void setCursorPosition(int x, int y) {
        cursorPos.X = x;
        cursorPos.Y = y;
        SetConsoleCursorPosition(console, cursorPos);
    }

    void loadHighScore() {
        ifstream file("snake_highscore.txt");
        if (file.is_open()) {
            file >> highScore;
            file.close();
        }
    }

    void saveHighScore() {
        ofstream file("snake_highscore.txt");
        if (file.is_open()) {
            file << highScore;
            file.close();
        }
    }
};

// 主函数
int main() {
    // 设置控制台为UTF-8编码
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // 设置控制台
    system("title 贪吃蛇游戏 - Enhanced Edition");
    system("mode con cols=90 lines=40");
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD bufferSize = {90, 40};
    SetConsoleScreenBufferSize(hConsole, bufferSize);
    
    // 隐藏光标
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(console, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(console, &cursorInfo);
    
    cout << "===== 贪吃蛇游戏 Enhanced Edition =====" << endl;
    cout << "按任意键开始..." << endl;
    _getch();
    
    Game game;
    game.run();
    
    // 恢复光标
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(console, &cursorInfo);
    
    return 0;
}