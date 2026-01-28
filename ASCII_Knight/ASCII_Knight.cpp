#include <iostream>
#include <conio.h>
#include <windows.h>
#include <cstdlib>
#include <ctime>
using namespace std;

struct Fireball {
	int x, y;
	int dirX, dirY;
	bool active;
	clock_t lastMove;
};

int lastDrawnHP = -1;
int lastDrawnBossHP = -1;

bool gameOver = false;
const int ARENA_WIDTH = 25;
const int ARENA_LENGTH = 100;
const int GRAVITY = 1;
const double firstPlatfromProb = 0.2;
const int minPlatformLength = 15;
const int maxPlatformLength = 30;
int globalPlatformX = 0;
int globalPlatformY = 0;
int globalPlatformLength = 0;
int totalEnemies = 0;
int currentWave = 0;
const int TOTAL_WAVES = 4;
bool spawningWave = true;


int walkers = 0;
const int Walkers_HP = 2;
int** walkersPositions;
int** walkerSpots;
int walkerSpotsCount = 0;
int* walkersDirections;
const double WALKER_STEP_TIME = 0.25;
clock_t lastWalkerStep = 0;

int fliers = 0;
const int Fliers_HP = 2;
int** fliersPositions;
int* fliersDirections;
const double FLIER_STEP_TIME = 0.25;
clock_t lastFlierStep = 0;



int crawlers = 0;
const int Crawlers_HP = 2;
int** crawlersPositions;
int** crawlerSpots;
int crawlerSpotsCount = 0;
const char CRAWLER_CHAR = 'C';


int jumpers = 0;
const int Jumpers_HP = 2;
int** jumpersPositions;
int* jumpersDirections;
bool* jumpersIsJumping;
int* jumpersJumpStep;
clock_t* jumpersLastJump;
int** jumperSpots;
int jumperSpotsCount = 0;
const double JUMPER_STEP_TIME = 0.25;
const double JUMPER_JUMP_TIME = 0.08;
clock_t lastJumperStep = 0;

const int BOSS_SIZE = 3;
int BOSS_HP = 10;
int BOSS_X = 60; 
int BOSS_Y = 5;
bool bossShield = false;
bool bossSplit = false;
bool isBossSpawned = false;
const char BOSS_CHAR = 'B';
clock_t lastFireball = 0;
int bossFireCooldown = 0;
clock_t lastSummon = 0;
const double BOSS_SUMMON_COOLDOWN = 5.0;
const int MAX_FIREBALLS = 12;
const double FIREBALL_STEP_TIME = 0.15;
const int BOSS_FIRE_COOLDOWN_MAX = 45;
const int MAX_BOSS_SUMMON_WALKERS = 10;

Fireball fireballs[MAX_FIREBALLS];

int HERO_HP = 5;
const int HERO_JUMP_HEIGHT = 5;
bool isJumping = false;
int jumpCount = 0;
int HERO_X = 50;
int HERO_Y = 23;

bool isKnockback = false;
int knockbackStep = 0;
int knockbackDir = 0;
clock_t lastKnockbackStep = 0;

const double KNOCKBACK_STEP_TIME = 0.06;


const char JUMP = 'w';
const char LEFT = 'a';
const char RIGHT = 'd';

const char ATTACK_UP = 'i';
const char ATTACK_LEFT = 'j';
const char ATTACK_DOWN = 'k';
const char ATTACK_RIGHT = 'l';

const char attackUp[3] = { '/', '-', '\\' };
const char attackLeft[3] = { '/', '|', '\\' };
const char attackDown[3] = { '\\', '_', '/' };
const char attackRight[3] = { '\\', '|', '/' };

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

const int COLOR_DEFAULT = 7;
const int COLOR_JUMPER = 11;
const int COLOR_FLIER = 9;
const int COLOR_CRAWLER = 10;
const int COLOR_WALKER = 7;
const int COLOR_BOSS = 14;


bool attackActive = false;
int attackX = 0;
int attackY = 0;
int attackDir = 0;
clock_t attackStartTime = 0;
int attackStartY = 0;
int attackStep = 0;
clock_t attackLastStep = 0;

const double ATTACK_STEP_TIME = 0.07; 
const int ATTACK_MAX_STEPS = 2;


const double ATTACK_DURATION = 0.15; 

char arena[ARENA_WIDTH][ARENA_LENGTH];

const double FRAME_TIME = 1.0 / 13.0;
const double ATTACK_FRAME_TIME = 1.0 / 5.0;
clock_t lastFrame = clock();

void heroJump(int jumpDiff=0);
void action();
void gravityPull();
void drawChar(int x, int y, char c, int color=COLOR_DEFAULT);
bool isSolid(int x, int y);
bool enemyAt(int x, int y);
bool isBossTile(int x, int y);

void isPlayerDead() {
	if (HERO_HP<=0)
		gameOver = true;
}

bool checkInBoundaries() {
	bool isInTheArena = HERO_Y > 1 && HERO_Y < ARENA_WIDTH - 2;
	bool isOverPlatform = arena[HERO_Y + 1][HERO_X] == '=';
	return isInTheArena && !isOverPlatform;
}

bool isHeroAt(int x, int y) {
	return x == HERO_X && y == HERO_Y;
}

bool canHeroMoveTo(int x, int y) {
	return arena[y][x] == ' ' &&
		arena[y][x] != '#' &&
		arena[y][x] != '=';
}

bool walkerAt(int x, int y) {
	for (int i = 0; i < walkers; i++) {
		if (!walkersPositions[i]) continue;
		if (walkersPositions[i][0] == x &&
			walkersPositions[i][1] == y)
			return true;
	}
	return false;
}

bool isBlocked(int x, int y) {
	return arena[y][x] == '#' ||
		arena[y][x] == '=' ||
		arena[y][x] == 'E';
}

void clearScreen() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD written;

	GetConsoleScreenBufferInfo(hConsole, &csbi);

	DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
	COORD home = { 0, 0 };

	FillConsoleOutputCharacter(hConsole, ' ', size, home, &written);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, size, home, &written);
	SetConsoleCursorPosition(hConsole, home);
}



//hero knockback

void startKnockback(int dir) {
	if (isKnockback) return;

	isKnockback = true;
	knockbackStep = 0;
	knockbackDir = dir;
	lastKnockbackStep = clock();
	isJumping = false;
}

void updateKnockback() {
	if (!isKnockback) return;

	double delta = double(clock() - lastKnockbackStep) / CLOCKS_PER_SEC;
	if (delta < KNOCKBACK_STEP_TIME) return;

	int oldX = HERO_X;
	int oldY = HERO_Y;

	if (knockbackStep == 0 || knockbackStep == 2) {
		int nx = HERO_X + knockbackDir;
		if (canHeroMoveTo(nx, HERO_Y))
			HERO_X = nx;
	}
	else if (knockbackStep == 1) {
		int nx = HERO_X + knockbackDir;
		int ny = HERO_Y - 1;

		if (canHeroMoveTo(nx, ny)) {
			HERO_X = nx;
			HERO_Y = ny;
		}
	}

	drawChar(oldX, oldY, ' ');
	drawChar(HERO_X, HERO_Y, '@');

	knockbackStep++;
	lastKnockbackStep = clock();

	if (knockbackStep >= 3) {
		isKnockback = false;
	}
}




//walkers logic

void collectWalkerSpots() {
	walkerSpotsCount = 0;

	for (int y = 1; y < ARENA_WIDTH - 1; y++) {
		for (int x = 1; x < ARENA_LENGTH - 1; x++) {
			if (arena[y][x] == ' ' && (arena[y + 1][x] == '=' || arena[y+1][x] == '#')) {
				walkerSpotsCount++;
			}
		}
	}

	walkerSpots = new int* [walkerSpotsCount];
	for (int i = 0; i < walkerSpotsCount; i++)
		walkerSpots[i] = new int[2];

	int idx = 0;
	for (int y = 1; y < ARENA_WIDTH - 1; y++) {
		for (int x = 1; x < ARENA_LENGTH - 1; x++) {
			if (arena[y][x] == ' ' && (arena[y + 1][x] == '=' || arena[y + 1][x] == '#')) {
				walkerSpots[idx][0] = x;
				walkerSpots[idx][1] = y;
				idx++;
			}
		}
	}
}


void setWalkers(int count) {
	walkers = 0;
	walkersPositions = new int* [count];
	for (int i = 0; i < count; i++)
		walkersPositions[i] = new int[2];
	walkersDirections = new int[count];

	for (int i = 0; i < count; i++) {
		int r = rand() % walkerSpotsCount;
		int x = walkerSpots[r][0];
		int y = walkerSpots[r][1];

		drawChar(x, y, 'E', COLOR_WALKER);
		walkersPositions[i][0] = x;
		walkersPositions[i][1] = y;
		walkersDirections[i] = rand() % 2;
		walkers++;
		totalEnemies++;
	}
}

void redrawEnemy(int x, int y, int newX, int newY, char enemy, int color) {
	drawChar(x, y, ' ', color);
	drawChar(newX, newY, enemy, color);
}

void updateWalkers() {
	double delta = double(clock() - lastWalkerStep) / CLOCKS_PER_SEC;
	if (delta < WALKER_STEP_TIME) return;

	lastWalkerStep = clock();

	for (int i = 0; i < walkers; i++) {
		if (!walkersPositions[i]) continue;

		int step = walkersDirections[i] ? 1 : -1;
		int x = walkersPositions[i][0];
		int y = walkersPositions[i][1];

		if (arena[y + 1][x] == ' ' && !walkerAt(x, y + 1)) {
			drawChar(x, y, ' ');
			walkersPositions[i][1]++;
			drawChar(x, y+1, 'E', COLOR_WALKER);

			continue;
		}

		if (arena[y][x + step] == ' ' &&
			(isSolid(x+step, y+1) || isBossTile(x+step, y+1)) &&
			!isHeroAt(x+step,y) && !enemyAt(x+step, y)){

			walkersPositions[i][0] += step;
			redrawEnemy(x, y, x + step, y, 'E', COLOR_WALKER);
		}
		else {
			if (isHeroAt(x+step, y)) {
				startKnockback(step > 0 ? 1 : -1);
				HERO_HP--;
			}

			walkersDirections[i] = !walkersDirections[i];
		}
	}
}

void removeWalker(int i) {
	if (!walkersPositions[i]) return;

	drawChar(walkersPositions[i][0], walkersPositions[i][1], ' ');

	delete[] walkersPositions[i];
	walkersPositions[i] = nullptr;
	walkersDirections[i] = -1;
}

//fliers

void setFliers(int count) {
	fliers = 0;
	fliersPositions = new int* [count];
	fliersDirections = new int[count];

	for (int i = 0; i < count; i++)
		fliersPositions[i] = new int[2];

	for (int i = 0; i < count; i++) {
		int x, y;
		do {
			x = rand() % (ARENA_LENGTH - 2) + 1;
			y = rand() % (ARENA_WIDTH - 2) + 1;
		} while (arena[y][x] != ' ');

		drawChar(x, y, 'F', COLOR_FLIER);

		fliersPositions[i][0] = x;
		fliersPositions[i][1] = y;
		fliersDirections[i] = rand() % 2;
		fliers++;
		totalEnemies++;
	}
}

void updateFliers() {
	double delta = double(clock() - lastFlierStep) / CLOCKS_PER_SEC;
	if (delta < FLIER_STEP_TIME) return;

	lastFlierStep = clock();

	for (int i = 0; i < fliers; i++) {
		if (!fliersPositions[i]) continue;

		int step = fliersDirections[i] ? 1 : -1;
		int x = fliersPositions[i][0];
		int y = fliersPositions[i][1];
		int ny = y + step;

		if (arena[ny][x] == ' ' && !isHeroAt(x, ny)) {
			fliersPositions[i][1] += step;
			redrawEnemy(x, y, x, ny, 'F', COLOR_FLIER);
		}
		else {
			if (isHeroAt(x, ny)) {
				startKnockback(0);
				HERO_HP--;
			}
			fliersDirections[i] = !fliersDirections[i];
		}
	}
}

void removeFlier(int i) {
	if (!fliersPositions[i]) return;

	drawChar(fliersPositions[i][0], fliersPositions[i][1], ' ');
	delete[] fliersPositions[i];
	fliersPositions[i] = nullptr;
	fliersDirections[i] = -1;
}

bool flierAt(int x, int y) {
	for (int i = 0; i < fliers; i++) {
		if (!fliersPositions[i]) continue;
		if (fliersPositions[i][0] == x &&
			fliersPositions[i][1] == y)
			return true;
	}
	return false;
}

//crawlers

bool canPlaceCrawlerAt(int x, int y) {
	if (arena[y][x] != ' ') return false;

	return
		(isSolid(x, y + 1) ||
			isSolid(x, y - 1) ||
			isSolid(x - 1, y) ||
			isSolid(x + 1, y)) && y > 15;
}


void collectCrawlerSpots() {
	crawlerSpotsCount = 0;

	for (int y = 1; y < ARENA_WIDTH - 1; y++) {
		for (int x = 1; x < ARENA_LENGTH - 1; x++) {
			if (canPlaceCrawlerAt(x, y))
				crawlerSpotsCount++;
		}
	}

	crawlerSpots = new int* [crawlerSpotsCount];
	for (int i = 0; i < crawlerSpotsCount; i++)
		crawlerSpots[i] = new int[2];

	int idx = 0;
	for (int y = 1; y < ARENA_WIDTH - 1; y++) {
		for (int x = 1; x < ARENA_LENGTH - 1; x++) {
			if (canPlaceCrawlerAt(x, y)) {
				crawlerSpots[idx][0] = x;
				crawlerSpots[idx][1] = y;
				idx++;
			}
		}
	}
}

void setCrawlers(int count) {
	crawlers = 0;
	crawlersPositions = new int* [count];
	for (int i = 0; i < count; i++)
		crawlersPositions[i] = new int[2];

	for (int i = 0; i < count; i++) {
		int r = rand() % crawlerSpotsCount;
		int x = crawlerSpots[r][0];
		int y = crawlerSpots[r][1];

		drawChar(x, y, 'C', COLOR_CRAWLER);
		crawlersPositions[i][0] = x;
		crawlersPositions[i][1] = y;
		crawlers++;
		totalEnemies++;
	}
}


bool crawlerAt(int x, int y) {
	for (int i = 0; i < crawlers; i++) {
		if (!crawlersPositions[i]) continue;
		if (crawlersPositions[i][0] == x &&
			crawlersPositions[i][1] == y)
			return true;
	}
	return false;
}


//jumpers
int dirToHero(int x) {
	return (HERO_X > x) ? 1 : -1;
}

void collectJumperSpots() {
	jumperSpotsCount = 0;

	for (int y = 1; y < ARENA_WIDTH - 2; y++) {
		for (int x = 1; x < ARENA_LENGTH - 2; x++) {
			if (arena[y][x] == ' ' &&
				(arena[y + 1][x] == '=' || arena[y + 1][x] == '#')) {
				jumperSpotsCount++;
			}
		}
	}

	jumperSpots = new int* [jumperSpotsCount];
	for (int i = 0; i < jumperSpotsCount; i++)
		jumperSpots[i] = new int[2];

	int idx = 0;
	for (int y = 1; y < ARENA_WIDTH - 2; y++) {
		for (int x = 1; x < ARENA_LENGTH - 2; x++) {
			if (arena[y][x] == ' ' &&
				(arena[y + 1][x] == '=' || arena[y + 1][x] == '#')) {
				jumperSpots[idx][0] = x;
				jumperSpots[idx][1] = y;
				idx++;
			}
		}
	}
}

void setJumpers(int count) {
	jumpers = 0;

	jumpersPositions = new int* [count];
	for (int i = 0; i < count; i++)
		jumpersPositions[i] = new int[2];

	jumpersDirections = new int[count];
	jumpersIsJumping = new bool[count];
	jumpersJumpStep = new int[count];
	jumpersLastJump = new clock_t[count];

	for (int i = 0; i < count; i++) {
		int r = rand() % jumperSpotsCount;

		int x = jumperSpots[r][0];
		int y = jumperSpots[r][1];

		jumpersPositions[i][0] = x;
		jumpersPositions[i][1] = y;

		jumpersDirections[i] = rand() % 2;
		jumpersIsJumping[i] = false;
		jumpersJumpStep[i] = 0;
		jumpersLastJump[i] = 0;

		drawChar(x, y, 'J', COLOR_JUMPER);
		jumpers++;
		totalEnemies++;
	}
}

void updateJumpers() {
	double delta = double(clock() - lastJumperStep) / CLOCKS_PER_SEC;
	if (delta < JUMPER_STEP_TIME) return;

	lastJumperStep = clock();

	for (int i = 0; i < jumpers; i++) {
		if (!jumpersPositions[i]) continue;

		int x = jumpersPositions[i][0];
		int y = jumpersPositions[i][1];
		int dir = jumpersDirections[i] ? 1 : -1;

		auto jumperAt = [&](int tx, int ty) {
			for (int j = 0; j < jumpers; j++) {
				if (j == i || !jumpersPositions[j]) continue;
				if (jumpersPositions[j][0] == tx &&
					jumpersPositions[j][1] == ty)
					return true;
			}
			return false;
			};

		if (jumpersIsJumping[i]) {
			double jumpDelta = double(clock() - jumpersLastJump[i]) / CLOCKS_PER_SEC;
			if (jumpDelta < JUMPER_JUMP_TIME) continue;

			int nx = x;
			int ny = y;

			if (jumpersJumpStep[i] == 0) { nx += dir; ny -= 1; }
			else if (jumpersJumpStep[i] == 1) { nx += dir; }
			else if (jumpersJumpStep[i] == 2) { nx += dir; ny += 1; }

			if (arena[ny][nx] == ' ' && !jumperAt(nx, ny)) {
				drawChar(x, y, ' ');
				jumpersPositions[i][0] = nx;
				jumpersPositions[i][1] = ny;
				drawChar(nx, ny, 'J', COLOR_JUMPER);
			}

			jumpersJumpStep[i]++;
			jumpersLastJump[i] = clock();

			if (jumpersJumpStep[i] > 2)
				jumpersIsJumping[i] = false;

			continue;
		}

		if (arena[y + 1][x] == ' ' && !jumperAt(x, y + 1)) {
			drawChar(x, y, ' ');
			jumpersPositions[i][1]++;
			drawChar(x, y + 1, 'J', COLOR_JUMPER);
			continue;
		}

		int dist = abs(HERO_X - x);
		bool grounded = (arena[y + 1][x] == '=' || arena[y + 1][x] == '#');

		if (grounded && dist >= 3 && dist <= 5) {
			jumpersIsJumping[i] = true;
			jumpersJumpStep[i] = 0;
			jumpersDirections[i] = (HERO_X > x);
			jumpersLastJump[i] = clock();
			continue;
		}

		int nx = x + dir;

		bool canWalk =
			arena[y][nx] == ' ' &&
			(arena[y + 1][nx] == '=' || arena[y + 1][nx] == '#') &&
			!jumperAt(nx, y) &&
			!isHeroAt(nx, y);

		if (canWalk) {
			drawChar(x, y, ' ');
			jumpersPositions[i][0] = nx;
			drawChar(nx, y, 'J', COLOR_JUMPER);
		}
		else {
			jumpersDirections[i] = !jumpersDirections[i];
		}
	}
}




bool jumperAt(int x, int y) {
	for (int i = 0; i < jumpers; i++) {
		if (!jumpersPositions[i]) continue;
		if (jumpersPositions[i][0] == x &&
			jumpersPositions[i][1] == y)
			return true;
	}
	return false;
}

bool enemyAt(int x, int y) {
	return walkerAt(x, y) ||
		jumperAt(x, y) ||
		flierAt(x, y) ||
		crawlerAt(x, y) ||
		isBossTile(x,y);
}

//boss

void spawnBoss() {
	for (int y = 0; y < BOSS_SIZE; y++) {
		for (int x = 0; x < BOSS_SIZE; x++) {
			arena[BOSS_Y + y][BOSS_X + x] = 'B';
			drawChar(BOSS_X + x, BOSS_Y + y, 'B', COLOR_BOSS);
		}
	}
	isBossSpawned = true;
}

void bossSummonEnemies() {
	double delta = double(clock() - lastSummon) / CLOCKS_PER_SEC;
	if (delta < BOSS_SUMMON_COOLDOWN) return;
	lastSummon = clock();

	int canSpawn = MAX_BOSS_SUMMON_WALKERS - walkers;
	if (canSpawn <= 0) return; 

	int spawnCount = min(3, canSpawn);

	for (int i = 0; i < spawnCount; i++) {
		int offsetX = rand() % (BOSS_SIZE + 2) - 1;
		int offsetY = rand() % (BOSS_SIZE + 2) - 1;
		int spawnX = BOSS_X + offsetX;
		int spawnY = BOSS_Y + offsetY;

		if (spawnX < 1 || spawnX >= ARENA_LENGTH - 1 || spawnY < 1 || spawnY >= ARENA_WIDTH - 1)
			continue;

		if (arena[spawnY][spawnX] == ' ' && !isBossTile(spawnX, spawnY)) {
			int** newWalkers = new int* [walkers + 1];
			int* newDirs = new int[walkers + 1];
			for (int w = 0; w < walkers; w++) {
				newWalkers[w] = walkersPositions[w];
				newDirs[w] = walkersDirections[w];
			}
			newWalkers[walkers] = new int[2] { spawnX, spawnY };
			newDirs[walkers] = rand() % 2;
			delete[] walkersPositions;
			delete[] walkersDirections;
			walkersPositions = newWalkers;
			walkersDirections = newDirs;

			drawChar(spawnX, spawnY, 'E', COLOR_WALKER);
			walkers++;
			totalEnemies++;
		}
	}
}

bool heroHitsBoss(int x, int y) {
	if (bossSplit) return false;
	for (int i = 0; i < BOSS_SIZE; i++)
		for (int j = 0; j < BOSS_SIZE; j++)
			if (x == BOSS_X + j && y == BOSS_Y + i)
				return true;
	return false;
}

void spawnFireball(int x, int y, int dx, int dy) {
	for (int i = 0; i < MAX_FIREBALLS; i++) {
		if (!fireballs[i].active) {
			fireballs[i] = { x, y, dx, dy, true, clock() };
			drawChar(x, y, '*');
			break;
		}
	}
}

void updateBossFireCooldown() {
	if (bossFireCooldown > 0)
		bossFireCooldown--;
}

void bossLaunchFireball() {
	if (bossFireCooldown > 0) return;
	bossFireCooldown = BOSS_FIRE_COOLDOWN_MAX;

	int bossCX = BOSS_X + 1;
	int bossCY = BOSS_Y + 1;

	int dx = HERO_X - bossCX;
	int dy = HERO_Y - bossCY;

	int dirX = (dx > 0) - (dx < 0);
	int dirY = (dy > 0) - (dy < 0);

	if (abs(dx) > abs(dy)) {
		int fireX = (dirX > 0) ? BOSS_X + 3 : BOSS_X - 1;
		for (int i = 0; i < 3; i++)
			spawnFireball(fireX, BOSS_Y + i, dirX, 0);
	}
	else {
		int fireY = (dirY > 0) ? BOSS_Y + 3 : BOSS_Y - 1;
		for (int i = 0; i < 3; i++)
			spawnFireball(BOSS_X + i, fireY, 0, dirY);
	}
}


void updateFireballs() {
	for (int i = 0; i < MAX_FIREBALLS; i++) {
		if (!fireballs[i].active) continue;

		double delta = double(clock() - fireballs[i].lastMove) / CLOCKS_PER_SEC;
		if (delta < FIREBALL_STEP_TIME) continue;

		fireballs[i].lastMove = clock();

		int oldX = fireballs[i].x;
		int oldY = fireballs[i].y;

		int nx = oldX + fireballs[i].dirX;
		int ny = oldY + fireballs[i].dirY;

		if (nx <= 0 || nx >= ARENA_LENGTH - 1 ||
			ny <= 0 || ny >= ARENA_WIDTH - 1 ||
			isSolid(nx, ny)) {

			drawChar(oldX, oldY, arena[oldY][oldX]);
			fireballs[i].active = false;
			continue;
		}

		if (isHeroAt(nx, ny)) {
			HERO_HP--;
			startKnockback(fireballs[i].dirX);
			drawChar(oldX, oldY, arena[oldY][oldX]);
			fireballs[i].active = false;
			continue;
		}

		drawChar(oldX, oldY, arena[oldY][oldX]);
		fireballs[i].x = nx;
		fireballs[i].y = ny;
		drawChar(nx, ny, '*');
	}
}


void damageBoss() {
	if (bossShield) {
		bossShield = false;
		return;
	}

	BOSS_HP--;

	if ((rand() % 100) < 20) {
		bossShield = true;
	}
}


void updateBoss() {
	if (!isBossSpawned || BOSS_HP <= 0) return;

	updateBossFireCooldown();
	bossLaunchFireball();
	updateFireballs();
	bossSummonEnemies();
}



//attack checks

bool isBossTile(int x, int y) {
	if (bossSplit || BOSS_HP <= 0) return false;
	return x >= BOSS_X && x < BOSS_X + BOSS_SIZE &&
		y >= BOSS_Y && y < BOSS_Y + BOSS_SIZE;
}


bool isSolid(int x, int y) {
	return arena[y][x] == '#' || arena[y][x] == '=';
}

bool attackHitsSolid(int x, int y) {
	return isSolid(x, y) ||
		isSolid(x - 1, y) ||
		isSolid(x + 1, y);
}
bool canAttackAt(int startX, int startY, int dirX, int dirY) {
	return !isSolid(startX + dirX, startY + dirY) &&
		!isSolid(startX, startY + dirY) &&
		!isSolid(startX - dirX, startY + dirY);
}

void drawChar(int x, int y, char c, int color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { (SHORT)x, (SHORT)y };

	SetConsoleCursorPosition(hConsole, pos);
	SetConsoleTextAttribute(hConsole, color);
	cout << c;
	SetConsoleTextAttribute(hConsole, COLOR_DEFAULT); 
}




//generate platforms logic

void generatePlatformByChance(int x, int y, double probability) {
	bool willBePlatform = rand() / (RAND_MAX + 1.0) < probability;
	if (willBePlatform) {
		arena[y][x] = '=';
		drawChar(x, y, '=');
	}
}

void generateRandomPlatform(int minX, int maxX, int minY, int maxY) {
	globalPlatformX = rand() % (maxX-minX + 1) + minX;
	globalPlatformY = rand() % (maxY-minY + 1) + minY;
	generatePlatformByChance(globalPlatformX, globalPlatformY, 1);
}

void generatePlatformLine(int length) {
	int left = globalPlatformX;
	int right = globalPlatformX;

	int remaining = length;

	while (remaining > 0) {
		bool expandRight = rand() % 2;

		if (expandRight) {
			int nx = right + 1;
			if (nx < ARENA_LENGTH - 1 && arena[globalPlatformY][nx] == ' ') {
				right = nx;
				arena[globalPlatformY][right] = '=';
				drawChar(right, globalPlatformY, '=');
				remaining--;
				continue;
			}
		}

		int nx = left - 1;
		if (nx > 0 && arena[globalPlatformY][nx] == ' ') {
			left = nx;
			arena[globalPlatformY][left] = '=';
			drawChar(left, globalPlatformY, '=');
			remaining--;
			continue;
		}

		break;
	}

	globalPlatformLength = right - left + 1;
}

void generateLayerPlatform(int fromX, int toX, int fromY, int toY) {
	generateRandomPlatform(fromX, toX, fromY, toY);
	int platformLength = rand() % (maxPlatformLength - minPlatformLength + 1) + minPlatformLength;
	generatePlatformLine(platformLength);
	globalPlatformLength = platformLength + 1;
}


void printArena() {
	for (int i = 0; i < ARENA_WIDTH; i++) {
		for (int j = 0; j < ARENA_LENGTH; j++) {
			cout << arena[i][j];
		}
		cout << endl;
	}
}

void generateArenaBoundaries() {
	for (int i = 0; i < ARENA_WIDTH; i++) {
		for (int j = 0; j < ARENA_LENGTH; j++) {
			if (i == 0 || i == ARENA_WIDTH - 1 || j == 0 || j == ARENA_LENGTH - 1) {
				arena[i][j] = '#';
			}
			else
				arena[i][j] = ' ';
		}
	}
}

int calculateBestPositionSecondLayer(int leftBorderline, int rightBorderline) {
	int neededX = 0;
	int leftBorder = globalPlatformX;
	for (int i = 0; i < globalPlatformLength; i++) {
		if (arena[globalPlatformY][leftBorder] == '=') {
			leftBorder--;
		}
		else {
			break;
		}
	}
	int rightBorder = globalPlatformLength + leftBorder - 1;
	bool right = rand() / (RAND_MAX + 1.0) < 0.5;
	bool left = !right;
	if (right) {
		int randGap = rand() % 6;
		if (rightBorder + randGap > rightBorderline - minPlatformLength - 1) {
			left = true;
			
		}
		else {
			neededX = rightBorder + randGap;
		}

	}
	if (left) {
		int randGap = rand() % 6;
		if (leftBorder - randGap > leftBorderline + minPlatformLength + 1) {
			neededX = leftBorder - randGap;
		}
	}

	return neededX;
}

void drawHP() {
	if (HERO_HP == lastDrawnHP) return;

	lastDrawnHP = HERO_HP;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { 0, (SHORT)ARENA_WIDTH };
	SetConsoleCursorPosition(hConsole, pos);

	cout << "HP: ";
	for (int i = 0; i < HERO_HP; i++)
		cout << "<3" << ' ';
	cout << "     ";
}

void drawBossHP() {
	if (!isBossSpawned || BOSS_HP <= 0) return;
	if (BOSS_HP == lastDrawnBossHP) return;

	lastDrawnBossHP = BOSS_HP;

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { 0, (SHORT)(ARENA_WIDTH + 1) };
	SetConsoleCursorPosition(hConsole, pos);

	cout << "BOSS: ";
	for (int i = 0; i < BOSS_HP; i++)
		cout << "<3" << ' ';
	cout << "     ";
}


void arenaSetup() {
	generateArenaBoundaries();
	printArena();
	lastDrawnHP = -1;
	drawHP();
	lastDrawnBossHP = -1;
	drawBossHP();

	drawChar(HERO_X, HERO_Y, '@');
	generateLayerPlatform(1, (ARENA_LENGTH - 1) / 2, ARENA_WIDTH - 9, ARENA_WIDTH - 4);
	int newPlatformX = calculateBestPositionSecondLayer(1, ARENA_WIDTH / 2);
	generateLayerPlatform(newPlatformX, newPlatformX + maxPlatformLength, globalPlatformY - 9, globalPlatformY - 4);
	generateLayerPlatform((ARENA_LENGTH - 1) / 2 + 3, ARENA_WIDTH - 1, ARENA_WIDTH - 9, ARENA_WIDTH - 4);
	newPlatformX = calculateBestPositionSecondLayer(ARENA_WIDTH / 2, ARENA_WIDTH);
	generateLayerPlatform(newPlatformX, newPlatformX + maxPlatformLength, globalPlatformY - 9, globalPlatformY - 4);
}

void setupWave(int wave) {
	clearScreen();

	totalEnemies = 0;
	arenaSetup();

	if (wave == 0) {
		collectWalkerSpots();
		setWalkers(3);
		collectCrawlerSpots();
		setCrawlers(2);
	}

	else if (wave == 1) {
		collectWalkerSpots();
		setWalkers(4);
		collectJumperSpots();
		setJumpers(2);
		collectCrawlerSpots();
		setCrawlers(3);
	}

	else if (wave == 2) {
		collectWalkerSpots();
		setWalkers(5);
		collectJumperSpots();
		setJumpers(3);
		setFliers(2);
	}

	else if (wave == 3) {
		BOSS_X = 60;
		BOSS_Y = 5;
		BOSS_HP = 10;
		bossShield = false;
		bossSplit = false;

		lastFireball = clock();
		lastSummon = clock();

		for (int i = 0; i < MAX_FIREBALLS; i++) {
			fireballs[i].active = false;
		}


		spawnBoss();
	}
}

bool anyEnemiesAlive() {
	return totalEnemies > 0 || BOSS_HP > 0;
}

bool isBossAlive() {
	return BOSS_HP>0;
}

void updateWaves() {
	if (spawningWave) {
		setupWave(currentWave);
		spawningWave = false;
		return;
	}

	if (!anyEnemiesAlive()) {
		currentWave++;

		spawningWave = true;
	}
	if (currentWave >= TOTAL_WAVES && !isBossAlive()) {
		gameOver = true; // victory
		return;
	}
}



//attack logic

void attackDirection(int dirX, int dirY) {
	int startX = HERO_X;
	int startY = HERO_Y;

	if (!canAttackAt(startX, startY, dirX, dirY)) return;

	attackActive = true;
	attackX = startX + dirX;
	attackY = startY + dirY;
	attackStep = 0;
	attackDir = dirY != 0 ? (dirY < 0 ? 0 : 2) : (dirX < 0 ? 1 : 3);
	attackStartTime = clock();
	attackLastStep = clock();
}

void restoreTile(int x, int y) {
	if (!isHeroAt(x, y))
		drawChar(x, y, arena[y][x]);
}

void drawCurrentAttack() {
	const char* pattern;

	if (attackDir == 0) pattern = attackUp;
	else if (attackDir == 1) pattern = attackLeft;
	else if (attackDir == 2) pattern = attackDown;
	else pattern = attackRight;

	if (attackDir == 0 || attackDir == 2) {
		drawChar(attackX - 1, attackY, pattern[0]);
		drawChar(attackX, attackY, pattern[1]);
		drawChar(attackX + 1, attackY, pattern[2]);
	}
	else {
		drawChar(attackX, attackY - 1, pattern[0]);
		drawChar(attackX, attackY, pattern[1]);
		drawChar(attackX, attackY + 1, pattern[2]);
	}
}

void eraseAttack() {
	const char* pattern;

	if (attackDir == 0) pattern = attackUp;
	else if (attackDir == 1) pattern = attackLeft;
	else if (attackDir == 2) pattern = attackDown;
	else pattern = attackRight;

	if (attackDir == 0 || attackDir == 2) {
		if (!isHeroAt(attackX - 1, attackY)) drawChar(attackX - 1, attackY, arena[attackY][attackX - 1]);
		if (!isHeroAt(attackX, attackY))     drawChar(attackX, attackY, arena[attackY][attackX]);
		if (!isHeroAt(attackX + 1, attackY)) drawChar(attackX + 1, attackY, arena[attackY][attackX + 1]);
	}
	else {
		if (!isHeroAt(attackX, attackY - 1)) drawChar(attackX, attackY - 1, arena[attackY - 1][attackX]);
		if (!isHeroAt(attackX, attackY))     drawChar(attackX, attackY, arena[attackY][attackX]);
		if (!isHeroAt(attackX, attackY + 1)) drawChar(attackX, attackY + 1, arena[attackY + 1][attackX]);
	}
}

void checkWalkerHit() {
	for (int i = 0; i < walkers; i++) {
		if (!walkersPositions[i]) continue;

		int wx = walkersPositions[i][0];
		int wy = walkersPositions[i][1];

		if ((attackX == wx && attackY == wy) ||
			(attackDir < 2 && attackX - 1 == wx && attackY == wy) ||
			(attackDir < 2 && attackX + 1 == wx && attackY == wy) ||
			(attackDir >= 2 && attackX == wx && attackY - 1 == wy) ||
			(attackDir >= 2 && attackX == wx && attackY + 1 == wy)) {
			removeWalker(i);
			totalEnemies--;
		}
	}
}

void checkFlierHit() {
	for (int i = 0; i < fliers; i++) {
		if (!fliersPositions[i]) continue;

		int fx = fliersPositions[i][0];
		int fy = fliersPositions[i][1];

		if ((attackX == fx && attackY == fy) ||
			(attackDir < 2 && attackX - 1 == fx && attackY == fy) ||
			(attackDir < 2 && attackX + 1 == fx && attackY == fy) ||
			(attackDir >= 2 && attackX == fx && attackY - 1 == fy) ||
			(attackDir >= 2 && attackX == fx && attackY + 1 == fy)) {
			removeFlier(i);
			totalEnemies--;
		}
	}
}

void checkCrawlerHit() {
	for (int i = 0; i < crawlers; i++) {
		if (!crawlersPositions[i]) continue;

		int cx = crawlersPositions[i][0];
		int cy = crawlersPositions[i][1];

		if ((attackX == cx && attackY == cy) ||
			(attackDir < 2 && attackX - 1 == cx && attackY == cy) ||
			(attackDir < 2 && attackX + 1 == cx && attackY == cy) ||
			(attackDir >= 2 && attackX == cx && attackY - 1 == cy) ||
			(attackDir >= 2 && attackX == cx && attackY + 1 == cy)) {

			drawChar(cx, cy, ' ');
			delete[] crawlersPositions[i];
			crawlersPositions[i] = nullptr;
			totalEnemies--;
		}
	}
}

void checkJumperHit() {
	for (int i = 0; i < jumpers; i++) {
		if (!jumpersPositions[i]) continue;

		int jx = jumpersPositions[i][0];
		int jy = jumpersPositions[i][1];

		if ((attackX == jx && attackY == jy) ||
			(attackDir < 2 && attackX - 1 == jx && attackY == jy) ||
			(attackDir < 2 && attackX + 1 == jx && attackY == jy) ||
			(attackDir >= 2 && attackX == jx && attackY - 1 == jy) ||
			(attackDir >= 2 && attackX == jx && attackY + 1 == jy)) {

			drawChar(jx, jy, ' ');
			delete[] jumpersPositions[i];
			jumpersPositions[i] = nullptr;
			totalEnemies--;
		}
	}
}

void updateAttack() {
	if (!attackActive) return;

	int nextX = attackX + (attackDir == 1 ? -1 : attackDir == 3 ? 1 : 0);
	int nextY = attackY + (attackDir == 0 ? -1 : attackDir == 2 ? 1 : 0);

	checkWalkerHit();
	checkFlierHit();
	checkCrawlerHit();
	checkJumperHit();

	bool bossHit = false;

	if (isBossTile(attackX, attackY)) bossHit = true;

	if (attackDir < 2) {
		if (isBossTile(attackX - 1, attackY)) bossHit = true;
		if (isBossTile(attackX + 1, attackY)) bossHit = true;
	}
	else {
		if (isBossTile(attackX, attackY - 1)) bossHit = true;
		if (isBossTile(attackX, attackY + 1)) bossHit = true;
	}

	if (bossHit) {
		damageBoss();
		eraseAttack();
		attackActive = false;
		return;
	}

	if (attackHitsSolid(nextX, nextY)) {
		eraseAttack();
		attackActive = false;
		return;
	}

	if (attackStep >= ATTACK_MAX_STEPS) {
		eraseAttack();
		attackActive = false;
		return;
	}

	double lifetime = double(clock() - attackStartTime) / CLOCKS_PER_SEC;
	if (lifetime >= ATTACK_DURATION) {
		eraseAttack();
		attackActive = false;
		return;
	}

	double stepDelta = double(clock() - attackLastStep) / CLOCKS_PER_SEC;
	if (stepDelta < ATTACK_STEP_TIME) return;

	eraseAttack();

	attackStep++;
	attackX = nextX;
	attackY = nextY;
	attackLastStep = clock();

	drawCurrentAttack();
}

//movement logic

void moveHorizontaly(int key) {
	int dx = (key == 'a') ? -1 : 1;
	int nx = HERO_X + dx;

	if (walkerAt(nx, HERO_Y)) {
		startKnockback(-dx);
		HERO_HP--;
		return;
	}

	if (crawlerAt(nx, HERO_Y)) {
		startKnockback(-dx);
		HERO_HP--;
		return;
	}

	if (jumperAt(nx, HERO_Y)) {
		startKnockback(-dx);
		HERO_HP--;
		return;
	}

	if (isBossTile(nx, HERO_Y)) {
		startKnockback(-dx);
		HERO_HP--;
		return;
	}

	if (isBlocked(nx, HERO_Y)) {
		return;
	}

	drawChar(HERO_X, HERO_Y, ' ');
	HERO_X = nx;
	drawChar(HERO_X, HERO_Y, '@');
}



void moveHero(int key) {
	switch (key) {
		case 'a':
		case 'd': moveHorizontaly(key); break;
		case 'w': heroJump(); break;
		case 'i': attackDirection(0, -1); break; 
		case 'k': attackDirection(0, 1);  break;
		case 'j': attackDirection(-1, 0); break; 
		case 'l': attackDirection(1, 0);  break;
	}
}

void action() {
	updateAttack();
	updateWalkers();
	updateFliers();
	updateJumpers();
	updateBoss();
	updateKnockback();

	drawHP();
	drawBossHP();

	if (isKnockback) return;

	if (_kbhit()) {
		int key = _getch();
		moveHero(key);
	}
}

void moveYAnimation(int jumpDiff = 0) {
	int heroYBeforeJump = HERO_Y;

	while (isJumping || HERO_Y < ARENA_WIDTH-2) {
		clock_t now = clock();
		double delta = double(now - lastFrame) / CLOCKS_PER_SEC;

		if (delta >= FRAME_TIME) {
			int heroYPrevious = HERO_Y;

			if (isJumping) {

				if (crawlerAt(HERO_X, HERO_Y)) {
					startKnockback(0);
					HERO_HP--;
					isJumping = false;
					continue; 
				}

				if (isBossTile(HERO_X, HERO_Y-1)) {
					startKnockback(0);
					HERO_HP--;
					isJumping = false;
					continue;
				}

				if (HERO_Y > 1 &&
					abs(heroYBeforeJump - HERO_Y) < HERO_JUMP_HEIGHT - jumpDiff &&
					arena[HERO_Y - 1][HERO_X] != '=') {
					HERO_Y--;
				}
				else {
					isJumping = false; 
				}
			}
			else if (HERO_Y < ARENA_WIDTH-2) {
				if (arena[HERO_Y + 1][HERO_X] == '=') {
					break;
				}
				HERO_Y += GRAVITY; 
			}

			drawChar(HERO_X, heroYPrevious, ' ');
			drawChar(HERO_X, HERO_Y, '@');

			lastFrame = now;
		}

		action(); 
	}
}


void heroJump(int jumpDiff) {
	if (jumpCount >= 2) return;
	isJumping = true;
	jumpCount++;
	moveYAnimation(jumpDiff);
	isJumping = false;
}


void gravityPull() {
	moveYAnimation();
}

void showEndGame(bool victory) {
	clearScreen();
	generateArenaBoundaries();

	for (int y = 0; y < ARENA_WIDTH; y++) {
		for (int x = 0; x < ARENA_LENGTH; x++) {
			drawChar(x, y, arena[y][x]);
		}
	}

	const char* message = victory ? "VICTORY!" : "GAME OVER";
	int msgLength = strlen(message);
	int centerX = ARENA_LENGTH / 2 - msgLength / 2;
	int centerY = ARENA_WIDTH / 2;

	for (int i = 0; i < msgLength; i++) {
		drawChar(centerX + i, centerY, message[i]);
	}

	_getch();

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD pos = { 0, (SHORT)(ARENA_WIDTH + 1) };
	SetConsoleCursorPosition(hConsole, pos);
}

int main()
{
	srand(static_cast<unsigned>(time(nullptr)));
	arenaSetup();
	do {
		drawHP();
		drawBossHP();
		action();
		updateAttack();
		updateWalkers();
		updateFliers();
		updateJumpers();
		if (isBossSpawned) {
			updateBoss();
		}

		updateWaves();

		updateKnockback();
		isPlayerDead();
		if (checkInBoundaries()) {
			gravityPull();
		}
		jumpCount = 0;
	} while (!gameOver);

	showEndGame(currentWave >= TOTAL_WAVES && HERO_HP > 0);

}