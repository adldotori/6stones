// Samsung Go Tournament Form C (g++-4.8.3)

/*
[AI 코드 작성 방법]

1. char info[]의 배열 안에               "TeamName:자신의 팀명,Department:자신의 소속"               순서로 작성합니다.
( 주의 ) Teamname:과 Department:는 꼭 들어가야 합니다.
"자신의 팀명", "자신의 소속"을 수정해야 합니다.

2. 아래의 myturn() 함수 안에 자신만의 AI 코드를 작성합니다.

3. AI 파일을 테스트 하실 때는 "육목 알고리즘대회 툴"을 사용합니다.

4. 육목 알고리즘 대회 툴의 연습하기에서 바둑돌을 누른 후, 자신의 "팀명" 이 들어간 알고리즘을 추가하여 테스트 합니다.



[변수 및 함수]
myturn(int cnt) : 자신의 AI 코드를 작성하는 메인 함수 입니다.
int cnt (myturn()함수의 파라미터) : 돌을 몇 수 둬야하는지 정하는 변수, cnt가 1이면 육목 시작 시  한 번만  두는 상황(한 번), cnt가 2이면 그 이후 돌을 두는 상황(두 번)
int  x[0], y[0] : 자신이 둘 첫 번 째 돌의 x좌표 , y좌표가 저장되어야 합니다.
int  x[1], y[1] : 자신이 둘 두 번 째 돌의 x좌표 , y좌표가 저장되어야 합니다.
void domymove(int x[], int y[], cnt) : 둘 돌들의 좌표를 저장해서 출력


//int board[BOARD_SIZE][BOARD_SIZE]; 바둑판 현재상황 담고 있어 바로사용 가능함. 단, 원본데이터로 수정 절대금지
// 놓을수 없는 위치에 바둑돌을 놓으면 실격패 처리.

boolean ifFree(int x, int y) : 현재 [x,y]좌표에 바둑돌이 있는지 확인하는 함수 (없으면 true, 있으면 false)
int showBoard(int x, int y) : [x, y] 좌표에 무슨 돌이 존재하는지 보여주는 함수 (1 = 자신의 돌, 2 = 상대의 돌, 3 = 블럭킹)


<-------AI를 작성하실 때, 같은 이름의 함수 및 변수 사용을 권장하지 않습니다----->
*/


// "샘플코드[C]"  -> 자신의 팀명 (수정)
// "AI부서[C]"  -> 자신의 소속 (수정)
// 제출시 실행파일은 반드시 팀명으로 제출!
char info[] = { "TeamName:5목할래요 ,Department:한가협" };

//timestamp 201708160009
#include <stdio.h>
#include <Windows.h>
#include <time.h>
#include <vector>
#include <utility>
#include <tuple>
#include <algorithm>
#include <stdlib.h>
#include <queue>
#include <chrono>
#include "Connect6Algo.h"

const int BOARD_SIZE = 19;
const int LENGTH = 6;
const int INF = 1000000;

const int COLOR_OURS = 1;
const int COLOR_OPPS = 2;
const int COLOR_BLOCK = 3;

int cand_size = 6;
int cand_point_size = 100;
const double weight = 0.9;

extern int limitTime;
std::chrono::system_clock::time_point start_time;
int time_threshold = 200;
bool isTimeExceeded = false;
int time_cnt = 0;

//if mode is 1, change depth
//if mode is 2, chagne cand_size
int mode = 1;

// x,y : the coordinates
// i : 0 if the stone is ours, 1 if the stone is opponent's, 2 if blocking
struct point {
	int x, y, c;
};

struct data {
	int z1, z2, score;
};

typedef struct stones {
	point p;
	int dir;
}stones_4;

// status of the board
int board[BOARD_SIZE][BOARD_SIZE];
int realboard[BOARD_SIZE][BOARD_SIZE];

// weight of adding our connected components
int myscore[LENGTH + 1] = { 0,1,6,12,45,30,0 };
int opscore[LENGTH + 1] = { 0,3,9,12,0,0,0 };


// directions of mok
int dx[4] = { 0, 1, 1, 1 };
int dy[4] = { 1, 1, 0, -1 };

const point UNDEFINED = { -1, -1 };

// cell_count[player][x][y][dir]
// total 2 * (19 * 14 + 14 * 14) == 924 winning moves exist
// player == 0 <=> our point
int cell_count[2][19][19][4];

//store the distance from the center of stones
//sort by this array when make new order
double distFromMid[BOARD_SIZE][BOARD_SIZE];

//save prev points
std::vector<std::pair<point, point>> prev;
std::vector<std::pair<point, point>> realprev;

//4stones
int count;
std::vector<stones_4> sol[3];//0->opp first stone, 1->opp second stone,2->our stone

							 //number of stones placed prev turn
int prevOppStones = 0, prevOurStones = 0;

point p1, p2, res1, res2;

class CompData {
public:
	bool operator()(const data &a, const data & b) {
		return a.score > b.score;
	}
};

// whether the position is valid
bool is_valid(int x, int y)
{
	return 0 <= x && x < BOARD_SIZE && 0 <= y && y < BOARD_SIZE;
}
// initializing the score and each cells' count
// is this method really necessary?
// currently this method is not used
void init_score()
{
	for (int player = 0; player < 2; player++)
	{
		for (int x = 0; x < BOARD_SIZE; x++)
		{
			for (int y = 0; y < BOARD_SIZE; y++)
			{
				for (int dir = 0; dir < 4; dir++)
				{
					int nx = x + dx[dir];
					int ny = y + dy[dir];
					if (is_valid(nx, ny))
					{
						int& cnt = cell_count[player][x][y][dir];
						for (int loc = 0; loc < LENGTH; loc++)
						{
							if (board[x + dx[dir] * loc][y + dy[dir] * loc] == player + 1)
							{
								cnt++;
							}
							else if (board[x + dx[dir] * loc][y + dy[dir] * loc] != 0)
							{
								cnt = 0;
								break;
							}
						}
					}
				}
			}
		}
	}
}

// consider whether seven-mok is inevitable if we place the stone on line[6]
int isSevenMok(int line[2 * LENGTH + 1], int cur_color)
{
	int seven_count = 0;
	int our_stone = 0;
	int prevent = 0; //prevent 
	int first_stone, last_stone; //oo...oo
								 // assume empty cells are our colors
	first_stone = line[0];
	last_stone = line[LENGTH];
	for (int pos = 0; pos <= LENGTH; pos++)
	{
		if (line[pos] == cur_color || line[pos] == COLOR_BLOCK) our_stone++;
		if (line[pos] == 3 - cur_color) prevent++;
	}
	if (our_stone == LENGTH + 1) return 1;
	if ((first_stone == cur_color || first_stone == COLOR_BLOCK) && (last_stone == cur_color || last_stone == COLOR_BLOCK) && prevent == 0)
		return 2;

	for (int pos = 0; pos <= LENGTH - 1; pos++)
	{
		first_stone = line[pos + 1];
		last_stone = line[pos + LENGTH + 1];
		if (line[pos] == cur_color || line[pos] == COLOR_BLOCK) our_stone--;
		if (line[pos + LENGTH + 1] == cur_color || line[pos + LENGTH + 1] == COLOR_BLOCK) our_stone++;
		if (line[pos] == 3 - cur_color) prevent--;
		if (line[pos + LENGTH + 1] == 3 - cur_color) prevent++;
		if (our_stone == LENGTH + 1) return 1;
		if ((first_stone == cur_color || first_stone == COLOR_BLOCK) && (last_stone == cur_color || last_stone == COLOR_BLOCK) && prevent == 0)
			return 2;
	}
	return 0;
}
int compute_score(point p)
{
	int diff = 0; // (our score) - (opp score)

	int* my_score = myscore;
	int* op_score = opscore;

	int x = p.x;
	int y = p.y;

	for (int dir = 0; dir < 4; dir++)
	{
		int our_count = 0; // the number of 'our' (subjective) stones
		int opp_count = 0; // the number of opponent's stones
		int blk_count = 0; // the number of block's stones
		int out_count = 0; // the number of map's outside
		int our_count_prev = 0; // the number of 'our' stones when board[x][y] == 0
		int line[2 * LENGTH + 1] = {}; // the board status of the line segment

									   // computing the line segment
		for (int k = -LENGTH; k <= LENGTH; k++)
		{
			int nx = x + dx[dir] * k;
			int ny = y + dy[dir] * k;
			int& cur_color = line[LENGTH - k];

			// we assume to be a blocking cell
			// if the cell is invalid
			cur_color = 4;
			if (is_valid(nx, ny))
				cur_color = board[nx][ny];
		}

		// just pass the seven mok for now
		if (isSevenMok(line, p.c) == 1)
		{
			diff -= 10000;
			continue;
		}
		else if (isSevenMok(line, p.c) == 2)
			continue;
		// calculate the difference
		for (int k = -LENGTH + 1; k <= LENGTH - 1; k++)
		{
			int& cur_color = line[LENGTH + k];
			if (cur_color == p.c)
			{
				our_count++;
				if (k) our_count_prev++;
			}
			else if (cur_color == 3 - p.c) // k != 0
			{
				opp_count++;
			}
			else if (cur_color == COLOR_BLOCK)
			{
				blk_count++;
			}
			//out of map
			else if (cur_color == 4)
			{
				out_count++;
			}

			if (k >= 0)
			{
				// If there are currently no stones, calculate the difference
				if (out_count == 0 && opp_count == 0)
				{
					diff = diff + my_score[our_count + blk_count];
					diff = diff - my_score[our_count_prev + blk_count];
				}
				// note that our_count > 0
				if (out_count == 0 && our_count_prev == 0 && opp_count)
				{
					diff = diff + op_score[opp_count + blk_count];
					diff = diff - op_score[opp_count + blk_count - 1];
				}
				// erase the leftmost stone (similar to deque)
				int prev_color = line[k + 1];
				if (prev_color == p.c)
				{
					our_count--;
					our_count_prev--;
				}
				else if (prev_color == 3 - p.c)
				{
					opp_count--;
				}
				else if (prev_color == COLOR_BLOCK)
				{
					blk_count--;
				}
				else if (prev_color == 4)
				{
					out_count--;
				}
			}
		}
	}
	return diff;
}

void isFourExist(int player) { //mode=1-> my four stones, mode=2-> opp four stones
	sol[0].clear();
	sol[1].clear();
	sol[2].clear();
	count = 0;
	stones_4 var;
	var.p = { -1, -1, -1 };
	var.dir = -1;
	int prev_size = prev.size();
	for (int mode = 0; mode < 2; mode++) { //cnt==0->my four stones, cnt==1->opp four stones
		bool flag = false;
		int cand_x[2] = { prev[prev_size - 2 + mode].first.x, prev[prev_size - 2 + mode].second.x };
		int cand_y[2] = { prev[prev_size - 2 + mode].first.y, prev[prev_size - 2 + mode].second.y };
		int x, y, nx, ny;
		for (int i = 0; i < 2; i++) {
			if (flag) break;
			for (int dir = 0; dir < 4; dir++) {
				if (flag) break;
				for (int idx = 0; idx < 6; idx++) {
					if (flag) break;
					x = cand_x[i] - dx[dir] * idx;
					y = cand_y[i] - dy[dir] * idx;
					nx = x + 5 * dx[dir];
					ny = y + 5 * dy[dir];
					if (!is_valid(x, y)) continue;
					if (!is_valid(nx, ny)) continue;
					int cnt = 0;
					for (int k = 0; k < 6; k++) {
						int color = board[x + k * dx[dir]][y + k * dy[dir]];
						if (color == 2 - (player + mode) % 2) cnt++; //COLOR_OURS
						else if (color == 1 + (player + mode) % 2) { cnt = 0; break; } //COLOR_OPPS
						else if (color == COLOR_BLOCK) cnt++;
					}
					/*if (is_valid(x + (-1) * dx[dir], y + (-1) * dy[dir]) && board[x + (-1) * dx[dir]][y + (-1) * dy[dir]] == ((player + mode) % 2 + 2))
					cnt = 0;
					if (is_valid(x + 6 * dx[dir], y + 6 * dy[dir]) && board[x + 6 * dx[dir]][y + 6 * dy[dir]] == ((player + mode) % 2 + 2)) // prevent 7 stones
					cnt = 0;*/
					//printf("%d (%d,%d) (%d,%d) %d\n", i, x, y, nx, ny, cnt);
					if (cnt >= 4) {
						var.p = { x,y,2 - (player + mode) % 2 };
						var.dir = dir;
						if (mode == 0) {
							sol[2].push_back(var);
							flag = true;
						}
						else {
							if (i == 0) {
								sol[0].push_back(var);
							}
							else {
								sol[1].push_back(var);
							}
						}
					}
				}
			}
		}
		if (sol[2].size() == 0) sol[2].push_back(var);
	}
}

// update board status
void update_board()
{
	prevOppStones = 0;
	prevOurStones = 0;
	realprev.clear();
	realprev.resize(2);
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			int newData = showBoard(i, j);
			if (realboard[i][j] != showBoard(i, j) && showBoard(i, j) == COLOR_OPPS) {
				if (prevOppStones == 0) {
					realprev[1].first.x = i;
					realprev[1].first.y = j;
					realprev[1].first.c = COLOR_OPPS;
					prevOppStones++;
				}
				else {
					realprev[1].second.x = i;
					realprev[1].second.y = j;
					realprev[1].second.c = COLOR_OPPS;
					prevOppStones++;
				}
			}
			if (realboard[i][j] != showBoard(i, j) && showBoard(i, j) == COLOR_OURS) {
				if (prevOurStones == 0) {
					realprev[0].first.x = i;
					realprev[0].first.y = j;
					realprev[0].first.c = COLOR_OURS;
					prevOurStones++;
				}
				else {
					realprev[0].second.x = i;
					realprev[0].second.y = j;
					realprev[0].second.c = COLOR_OURS;
					prevOurStones++;
				}
			}
			realboard[i][j] = showBoard(i, j);
		}
	}
	if (prevOppStones == 1) {
		realprev[1].second.x = realprev[1].first.x;
		realprev[1].second.y = realprev[1].first.y;
		realprev[1].second.c = realprev[1].first.c;
	}
	if (prevOurStones == 1) {
		realprev[0].second.x = realprev[0].first.x;
		realprev[0].second.y = realprev[0].first.y;
		realprev[0].second.c = realprev[0].first.c;
	}
}

void copy_board() {
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			board[i][j] = realboard[i][j];
		}
	}
	prev.clear();
	prev.resize(2);
	for (int i = 0; i < 2; i++) {
		prev[i].first.x = realprev[i].first.x;
		prev[i].first.y = realprev[i].first.y;
		prev[i].first.c = realprev[i].first.c;
		prev[i].second.x = realprev[i].second.x;
		prev[i].second.y = realprev[i].second.y;
		prev[i].second.c = realprev[i].second.c;
	}
}
//test
std::vector<point> order[20];
void updateOrder(int depth);
// alpha-beta by the difference and a bit of greedy
int alphabeta(int depth, const int player, const int player_cnt, int score, int alpha, int beta, const bool feedback)
{
	//if left time is less then time_threshold, set isValid as 0 and return
	if (time_cnt++ > 100 && (std::chrono::system_clock::now() - start_time) > std::chrono::milliseconds(1000 * limitTime - time_threshold)) {
		isTimeExceeded = true;
		time_cnt = 0;
		return 0;
	}

	if (depth == 0)
	{
		return 0;
	}
	updateOrder(depth);
	//use this code when play the first stone
	if (player_cnt == 1)
	{
		int ret = -INF;
		for (int z = 0; z < BOARD_SIZE * BOARD_SIZE; z++)
		{
			int x = order[depth][z].x;
			int y = order[depth][z].y;
			if (board[x][y]) continue;

			board[x][y] = player;

			int offset = compute_score({ x, y, player });
			if (x <= 1 || x >= 17 || y <= 1 || y >= 17) offset = 0;

			board[x][y] = 0;
			//should make maximal cost,, ret is v in psuedo code
			ret = max(ret, offset);

			if (feedback && alpha < ret)
			{
				p1 = { x, y };
			}
			alpha = max(alpha, ret);

			if (beta <= alpha + score)
			{
				break;
			}
		}
		return ret;
	}
	//2 stones
	else
	{
		// investigate O(N^2) cases is too costly
		// currently the program investigates some of them 
		// the amount of the candidate can be calibrated
		//printf("depth:%d,player:%d,score:%d,alpha,beta:%d,%d\n", depth, player, score, alpha, beta);
		//printf("%d\n---prev---\n", prev.size());
		for (unsigned int i = 0; i < prev.size(); i++) {
			//printf("(%d,%d) (%d,%d)\n", prev[i].first.x, prev[i].first.y, prev[i].second.x, prev[i].second.y);
		}
		int color = (player == COLOR_OURS) ? 1 : -1;
		int ret = -color * INF;

		//stores the candidates
		std::priority_queue<data, std::vector<data>, CompData> pq;

		isFourExist(player);
		/*for (int i = 0; i < 3; i++) {
		printf("%d : ", i);
		for (unsigned int j = 0; j<sol[i].size(); j++)
		printf("%d %d %d ///", sol[i][j].p.x, sol[i][j].p.y, sol[i][j].dir);
		printf("\n");
		}*/
		if (sol[2][0].dir != -1) {
			point p[2];
			int cnt = 0;
			for (int j = 0; j < LENGTH; j++) {
				int nx = sol[2][0].p.x + dx[sol[2][0].dir] * j;
				int ny = sol[2][0].p.y + dy[sol[2][0].dir] * j;
				if (board[nx][ny] == 0) {
					p[cnt++] = { nx,ny,sol[2][0].p.c };
				}
			}
			if (cnt == 1) {
				for (int x = 0; x < BOARD_SIZE; x++) {
					for (int y = 0; y < BOARD_SIZE; y++) {
						if (board[x][y] == 0) {
							p[cnt] = { x,y,sol[2][0].p.c };
						}
					}
				}
			}
			//printf("i win\n");
			ret = color * 10000;
			//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, p[0].x, p[0].y, p[1].x, p[1].y, 10000, alpha, beta);

			if (feedback)
			{
				p1 = { p[0].x, p[0].y };
				p2 = { p[1].x, p[1].y };
			}
			//printf("ret = %d\n", ret);
			return ret;
		}

		for (int num = 0; num < 2; num++) { // prevent with two stones now or lose
											//printf("num=%d\n", num);
			int cnt_dir[3] = { 0,0,0 };
			int cnt = -1;
			int dir = -1;
			int three = -1;
			int last = 0;
			for (unsigned int i = 0; i < sol[num].size(); i++) {
				int newdir = sol[num][i].dir;
				if (dir != newdir)
					cnt++;
				cnt_dir[cnt]++;
				dir = newdir;
			}
			for (int i = 0; i < 3; i++) {
				if (cnt_dir[i] >= 3) {
					three = i;
					for (int j = 0; j < three; j++) {
						last += cnt_dir[j];
					}
				}
			}
			//printf("cnt_dir %d %d %d\n", cnt_dir[0], cnt_dir[1], cnt_dir[2]);
			//printf("%d %d %d %d\n", cnt + 1, dir, three, last);
			if (three != -1) { //opp can make 6 stones
							   //printf("three");
				point p = sol[num][last].p;
				int dir = sol[num][last].dir;
				//printf("%d %d %d\n", p.x, p.y, dir);
				point Must1[2];
				point Must2[2];
				int cnt1 = 0;
				int cnt2 = 0;
				int pos[4] = { -2,-1,4,5 };
				for (int i = 0; i < 4; i++) {
					int nx = p.x + dx[dir] * pos[i];
					int ny = p.y + dy[dir] * pos[i];
					if (i < 2) {
						if (is_valid(nx, ny) && board[nx][ny] == 0)
							Must1[cnt1++] = { nx,ny,p.c };
					}
					else {
						if (is_valid(nx, ny) && board[nx][ny] == 0)
							Must2[cnt2++] = { nx,ny,p.c };
					}
				}
				//printf("%d %d", cnt1, cnt2);
				for (int z1 = 0; z1 < cnt1; z1++)
				{
					int x1 = Must1[z1].x;
					int y1 = Must1[z1].y;
					if (board[x1][y1]) continue;

					board[x1][y1] = player;
					int offset1 = compute_score({ x1, y1, player });

					for (int z2 = 0; z2 < cnt2; z2++)
					{
						int x2 = Must2[z2].x;
						int y2 = Must2[z2].y;
						if (board[x2][y2]) continue;

						board[x2][y2] = player;
						int offset2 = compute_score({ x2, y2, player });
						if (z1 == 0 && z2 == cnt2 - 1) offset2 = -1000;
						pq.push({ z1, z2, offset1 + offset2 });
						board[x2][y2] = 0;
					}
					board[x1][y1] = 0;
				}

				for (int z = 0; z < cnt1*cnt2; z++)
				{
					//printf("one 4 stones prevent from 2 stones\n");
					data dat = pq.top();
					pq.pop();
					int x1 = Must1[dat.z1].x;
					int y1 = Must1[dat.z1].y;
					board[x1][y1] = player;

					int x2 = Must2[dat.z2].x;
					int y2 = Must2[dat.z2].y;
					board[x2][y2] = player;
					//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, x1, y1, x2, y2, color*dat.score, alpha, beta);
					prev.push_back(std::pair<point, point>({ x1, y1, player }, { x2, y2, player }));
					int val = alphabeta(depth - 2, 3 - player, 2, color*dat.score, alpha, beta, false);
					if (isTimeExceeded) return 0;
					prev.pop_back();
					board[x1][y1] = 0;
					board[x2][y2] = 0;
					if (player == COLOR_OURS)
						ret = max(ret, dat.score + (int)(weight * val));
					else
						ret = min(ret, -dat.score + (int)(weight * val));

					//printf("ret = %d\n", ret);
					if (feedback && alpha < ret)
					{
						p1 = { x1, y1 };
						p2 = { x2, y2 };
					}

					if (player == COLOR_OURS) {
						alpha = max(alpha, ret);
						if (beta <= alpha + score) break;
					}
					else {
						beta = min(beta, ret);
						if (beta + score <= alpha) break;
					}
				}
				return ret;
			}
			else if (cnt + 1 >= 2) {
				int x[2], y[2], cnt = 0;
				for (int i = 0; i < 2; i++) {
					for (int j = 0; j < LENGTH; j++) {
						int nx = sol[num][cnt].p.x + dx[sol[num][cnt].dir] * j;
						int ny = sol[num][cnt].p.y + dy[sol[num][cnt].dir] * j;
						if (is_valid(nx, ny) && board[nx][ny] == 0) {
							x[i] = nx;
							y[i] = ny;
							//printf("%d %d \n", x[i], y[i]);
							break;
						}
					}
					board[x[i]][y[i]] = player;
					cnt += cnt_dir[0];
				}
				//printf("2 each 4 stones prevent from 1 stone\n");
				int offset = compute_score({ x[0], y[0], player }) + compute_score({ x[1],y[1],player });
				//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, x[0], y[0], x[1], y[1], color*offset, alpha, beta);
				prev.push_back(std::pair<point, point>({ x[0], y[0], player }, { x[1], y[1], player }));
				int val = alphabeta(depth - 2, 3 - player, 2, color*offset, alpha, beta, false);
				if (isTimeExceeded) return 0;
				prev.pop_back();
				board[x[0]][y[0]] = 0;
				board[x[1]][y[1]] = 0;
				if (player == COLOR_OURS)
					ret = max(ret, offset + (int)(weight * val));
				else
					ret = min(ret, -offset + (int)(weight * val));

				//printf("ret = %d\n", ret);
				if (feedback && alpha < ret)
				{
					p1 = { x[0], y[0] };
					p2 = { x[1], y[1] };
				}
				return ret;
			}
		}

		if (sol[0].size() >= 1 && sol[1].size() >= 1 && !(sol[0][0].p.x == sol[1][0].p.x && sol[0][0].p.y == sol[1][0].p.y)) {
			int x[2], y[2];
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < LENGTH; j++) {
					int nx = sol[i][0].p.x + dx[sol[i][0].dir] * j;
					int ny = sol[i][0].p.y + dy[sol[i][0].dir] * j;
					if (is_valid(nx, ny) && board[nx][ny] == 0) {
						x[i] = nx;
						y[i] = ny;
						//printf("%d %d \n", x[i], y[i]);
						break;
					}
				}

				board[x[i]][y[i]] = player;
			}
			//printf("2 4 stones prevent from 2 stones\n");
			int offset = compute_score({ x[0], y[0], player }) + compute_score({ x[1],y[1],player });
			//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, x[0], y[0], x[1], y[1], color*offset, alpha, beta);
			prev.push_back(std::pair<point, point>({ x[0], y[0], player }, { x[1], y[1], player }));
			int val = alphabeta(depth - 2, 3 - player, 2, color*offset, alpha, beta, false);
			if (isTimeExceeded) return 0;
			prev.pop_back();
			board[x[0]][y[0]] = 0;
			board[x[1]][y[1]] = 0;
			if (player == COLOR_OURS)
				ret = max(ret, offset + (int)(weight * val));
			else
				ret = min(ret, -offset + (int)(weight * val));

			//printf("ret = %d\n", ret);
			if (feedback && alpha < ret)
			{
				p1 = { x[0], y[0] };
				p2 = { x[1], y[1] };
			}
			return ret;
		}
		for (int i = 0; i<2; i++) { // prevent with one stone now or lose
			if (sol[i].size() == 1 || sol[i].size() == 2) {
				point Must[5];
				int cnt = 0;
				for (int j = 0; j < LENGTH; j++) {
					int nx = sol[i][0].p.x + dx[sol[i][0].dir] * j;
					int ny = sol[i][0].p.y + dy[sol[i][0].dir] * j;
					if (is_valid(nx, ny) && board[nx][ny] == 0) {
						Must[cnt++] = { nx,ny,sol[i][0].p.c };
					}
				}
				int repo[5][2] = { 0, };
				//printf("%d %d", cnt1, cnt2);
				for (int z1 = 0; z1 < cnt; z1++)
				{
					int x1 = Must[z1].x;
					int y1 = Must[z1].y;
					if (board[x1][y1]) continue;

					board[x1][y1] = player;
					int offset1 = compute_score({ x1, y1, player });

					for (int z2 = 0; z2 < cand_point_size; z2++)
					{
						int x2 = order[depth][z2].x;
						int y2 = order[depth][z2].y;
						if (board[x2][y2]) continue;
						for (int t = 0; t < cnt; t++) {
							if (Must[t].x == x2 && Must[t].y == y2) continue;
						}
						board[x2][y2] = player;
						int offset2 = compute_score({ x2, y2, player });
						if (repo[z1][1] < offset1 + offset2) {
							repo[z1][0] = z2;
							repo[z1][1] = offset1 + offset2;
						}
						if (pq.size() < (unsigned int)cand_size)
							pq.push({ z1, z2, offset1 + offset2 });

						else if ((pq.top()).score < offset1 + offset2) {
							pq.pop();
							pq.push({ z1, z2, offset1 + offset2 });
						}
						board[x2][y2] = 0;
					}
					board[x1][y1] = 0;
				}
				std::vector<data> repodata;
				for (int t = 0; t < cand_size; t++) {
					repodata.push_back(pq.top());
					pq.pop();
				}
				for (int t = 0; t < cnt; t++) {
					bool  check = true;
					for (int j = 0; j < repodata.size(); j++) {
						if (t == repodata[j].z1) {
							check = false;
							break;
						}
					}
					if (check) {
						repodata.push_back({ t,repo[t][0],repo[t][1] });
					}
				}
				for (int t = 0; t < repodata.size(); t++)
					pq.push(repodata[t]);
				for (int z = 0; z < repodata.size(); z++)
				{
					//printf("one 4 stones prevent from 1 stone\n");
					data dat = pq.top();
					pq.pop();
					int x1 = Must[dat.z1].x;
					int y1 = Must[dat.z1].y;
					board[x1][y1] = player;

					int x2 = order[depth][dat.z2].x;
					int y2 = order[depth][dat.z2].y;
					board[x2][y2] = player;
					//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, x1, y1, x2, y2, color*dat.score, alpha, beta);
					prev.push_back(std::pair<point, point>({ x1, y1, player }, { x2, y2, player }));
					int val = alphabeta(depth - 2, 3 - player, 2, color*dat.score, alpha, beta, false);
					if (isTimeExceeded) return 0;
					prev.pop_back();
					board[x1][y1] = 0;
					board[x2][y2] = 0;
					if (player == COLOR_OURS)
						ret = max(ret, dat.score + (int)(weight * val));
					else
						ret = min(ret, -dat.score + (int)(weight * val));

					//printf("ret = %d\n", ret);
					if (feedback && alpha < ret)
					{
						p1 = { x1, y1 };
						p2 = { x2, y2 };
					}

					if (player == COLOR_OURS) {
						alpha = max(alpha, ret);
						if (beta <= alpha + score) break;
					}
					else {
						beta = min(beta, ret);
						if (beta + score <= alpha) break;
					}
				}
				return ret;
			}
		}

		for (int z1 = 0; z1 < cand_point_size; z1++)
		{
			int x1 = order[depth][z1].x;
			int y1 = order[depth][z1].y;
			if (board[x1][y1]) continue;

			board[x1][y1] = player;
			int offset1 = compute_score({ x1, y1, player });

			for (int z2 = z1 + 1; z2 < cand_point_size; z2++)
			{
				int x2 = order[depth][z2].x;
				int y2 = order[depth][z2].y;
				if (board[x2][y2]) continue;

				board[x2][y2] = player;
				int offset2 = compute_score({ x2, y2, player });

				if (pq.size() < (unsigned int)cand_size)
					pq.push({ z1, z2, offset1 + offset2 });

				else if ((pq.top()).score < offset1 + offset2) {
					pq.pop();
					pq.push({ z1, z2, offset1 + offset2 });
				}
				board[x2][y2] = 0;
			}
			board[x1][y1] = 0;
		}

		for (int z = 0; z < (unsigned int)cand_size; z++)
		{
			//printf("nothing\n");
			data dat = pq.top();
			pq.pop();
			int x1 = order[depth][dat.z1].x;
			int y1 = order[depth][dat.z1].y;
			board[x1][y1] = player;

			int x2 = order[depth][dat.z2].x;
			int y2 = order[depth][dat.z2].y;
			board[x2][y2] = player;
			//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, order[dat.z1].x, order[dat.z1].y, order[dat.z2].x, order[dat.z2].y, color*dat.score, alpha, beta);
			//printf("data.score = %d\n", dat.score);
			prev.push_back(std::pair<point, point>({ x1, y1, player }, { x2, y2, player }));
			int val = alphabeta(depth - 2, 3 - player, 2, color*dat.score, alpha, beta, false);
			if (isTimeExceeded) return 0;
			prev.pop_back();

			board[x1][y1] = 0;
			board[x2][y2] = 0;
			if (player == COLOR_OURS)
				ret = max(ret, dat.score + (int)(weight * val));
			else
				ret = min(ret, -dat.score + (int)(weight * val));

			//printf("ret = %d\n", ret);
			if (feedback && alpha < ret)
			{
				p1 = { x1, y1 };
				p2 = { x2, y2 };
			}

			if (player == COLOR_OURS) {
				alpha = max(alpha, ret);
				if (beta <= alpha + score) {
					break;
				}
			}
			else {
				beta = min(beta, ret);
				if (beta + score <= alpha) {
					break;
				}
			}
		}
		return ret;
	}
}

bool compDist(point p1, point p2) {
	return distFromMid[p1.x][p1.y] < distFromMid[p2.x][p2.y];
}

void updateOrder(int depth) {
	double mid_y = 0, mid_x = 0;
	int stone_number = 0;
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (board[i][j] != 0) {
				stone_number++;
				mid_x += i;
				mid_y += j;
			}
		}
	}
	mid_y /= stone_number;
	mid_x /= stone_number;

	for (int i = 0; i < BOARD_SIZE; i++)
		for (int j = 0; j < BOARD_SIZE; j++) {
			if (board[i][j]) distFromMid[i][j] = INF;
			else distFromMid[i][j] = abs(mid_x - i) + abs(mid_y - j);
		}

	sort(order[depth].begin(), order[depth].end(), compDist);
}

void myturn(int cnt) {
	start_time = std::chrono::system_clock::now();
	isTimeExceeded = false;

	static bool isFirst = true;
	if (isFirst)
	{
		srand((time_t)time(NULL));
		for (int i = 0; i < BOARD_SIZE; i++)
			for (int j = 0; j < BOARD_SIZE; j++)
				for (int k = 0; k<20; k++)
					order[k].push_back({ i, j });
	}

	// maybe bfs among the previous stones might be better

	update_board();

	if (mode == 1) {
		for (int depth = 4; depth <= 8; depth += 2) {
			copy_board();
			alphabeta(cnt + depth, COLOR_OURS, cnt, 0, -INF, INF, true);
			if (isTimeExceeded) break;
			res1 = p1;
			res2 = p2;
		}
	}
	else if (mode == 2) {
		for (int i = 6; i <= 10; i += 2) {
			cand_size = i;
			copy_board();
			alphabeta(cnt + 6, COLOR_OURS, cnt, 0, -INF, INF, true);
			if (isTimeExceeded) break;
			res1 = p1;
			res2 = p2;
		}
	}

	copy_board();

	int x[2] = { res1.x, res2.x };
	int y[2] = { res1.y, res2.y };

	domymove(x, y, cnt);

	isFirst = false;
}