// Samsung Go Tournament Form C (g++-4.8.3)

/*
[AI 코드 작성 방법]

1. char info[]의 배열 안에					"TeamName:자신의 팀명,Department:자신의 소속"					순서로 작성합니다.
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
char info[] = { "TeamName:5목할래요 ,Department:abc" };

//timestamp 201708160009
#include <stdio.h>
#include <Windows.h>
#include <time.h>
#include <vector>
#include <utility>
#include <tuple>
#include <algorithm>
#include <stdlib.h>

// uncomment the below macro to debug via console or user input
//#define SECRET_AGENCY_DEBUG_MODE

#ifndef SECRET_AGENCY_DEBUG_MODE
#include "Connect6Algo.h"
#endif

const int BOARD_SIZE = 19;
const int LENGTH = 6;
const int INF = 1000000;

const int COLOR_OURS = 1;
const int COLOR_OPPS = 2;
const int COLOR_BLOCK = 3;

const int MAX_DEPTH = 4;

// x,y : the coordinates
// i : 0 if the stone is ours, 1 if the stone is opponent's, 2 if blocking
struct point {
	int x, y, c;
};

struct data {
	int z1, z2, score;
};

// status of the board
int board[BOARD_SIZE][BOARD_SIZE];

// weight of adding our connected components
int myscore[LENGTH + 1] = { 0,1,8,20,300,0,10000 };

// weight of blocking opponent's components
int opscore[LENGTH + 1] = { 0,0,6,15,1000,2000,5000 };

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

point p1, p2;

#ifdef SECRET_AGENCY_DEBUG_MODE
// define these debug methods
void domymove(int x[2], int y[2], int cnt)
{
	for (int i = 0; i<cnt; i++)
	{
		printf("%d %d\n", x[i], y[i]);
	}
}

int showBoard(int x, int y)
{
	return board[x][y];
}
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

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
bool isSevenMok(int line[2 * LENGTH + 1], int cur_color)
{
	int our_count = 0;
	bool seven_count = false;
	// assume empty cells are our colors
	for (int pos = -LENGTH + 1; pos < 0; pos++)
	{
		if (line[pos + LENGTH] == cur_color || line[pos + LENGTH] == 0) our_count++;
	}

	for (int pos = 0; pos < LENGTH; pos++)
	{
		if (line[pos + LENGTH] == cur_color || line[pos + LENGTH] == 0) our_count++;

		if (our_count == 6)
		{
			// it's a mere six-mok
			if (line[pos] != cur_color && line[pos + LENGTH + 1] != cur_color)
				return false;

			// else it's seven or more
			seven_count = true;
		}

		if (line[pos + 1] == cur_color || line[pos + 1] == 0) our_count--;
	}
	return seven_count;
}
// compute the score of the given point
// the most tricky part : calibration required
int compute_score(point p)
{
	int diff = 0; // (our score) - (opp score)

	int* my_score = myscore;	
	int* op_score = opscore;
	if (p.c == COLOR_OPPS)
		std::swap(my_score, op_score);

	int x = p.x;
	int y = p.y;

	int parity = p.c == COLOR_OURS ? 1 : -1;

	for (int dir = 0; dir < 4; dir++)
	{
		int our_count = 0; // the number of 'our' (subjective) stones
		int opp_count = 0; // the number of opponent's stones
		int blk_count = 0; // the number of block's stones
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
		if (isSevenMok(line, p.c))
		{
			diff -= 10000;
			continue;
		}
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

			if (k >= 0)
			{
				// If there are currently no stones, calculate the difference
				if (opp_count == 0)
				{
					diff = diff + my_score[our_count+blk_count] * parity;
					diff = diff - my_score[our_count_prev+blk_count] * parity;
				}
				// note that our_count > 0
				if (our_count_prev == 0 && opp_count)
				{
					diff = diff + op_score[opp_count+blk_count] * parity;
					diff = diff - op_score[opp_count + blk_count - 1] * parity;
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
			}
		}
	}
	return diff;
}

// update board status
void update_board()
{
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			board[i][j] = showBoard(i, j);
		}
	}
}

//test

std::vector<point> order;	
// alpha-beta by the difference and a bit of greedy
int alphabeta(int depth, const int player, const int player_cnt, int alpha, int beta, const bool feedback)
{
	if (depth == 0)
	{
		return 0;
	}
	if (player == COLOR_OURS)
	{
		int ret = -INF;
		if (player_cnt == 1) // first stone, investigate all possibilities (pure alpha-beta)
		{
			for (int z = 0; z < BOARD_SIZE * BOARD_SIZE; z++)
			{
				int x = order[z].x;
				int y = order[z].y;
				if (board[x][y]) continue;

				board[x][y] = player;

				int offset = compute_score({ x, y, player });
				int val = alphabeta(depth - 1, COLOR_OPPS, 2, alpha, beta, false);

				board[x][y] = 0;
				ret = max(ret, offset + val);

				if (feedback && alpha < ret)
				{
					p1 = { x, y };
				}
				alpha = max(alpha, ret);

				if (beta <= alpha)
				{
					break;
				}
			}
		}
		else // our stones
		{
			// investigate O(N^2) cases is too costly
			// currently the program investigates only top 361 greedy methods
			// the amount of the candidate can be calibrated

			//stores the candidates
			std::vector<data> data_vector;
			for (int z1 = 0; z1 < BOARD_SIZE * BOARD_SIZE; z1++)
			{
				int x1 = order[z1].x;
				int y1 = order[z1].y;
				if (board[x1][y1]) continue;

				board[x1][y1] = player;
				int offset1 = compute_score({ x1, y1, player });

				for (int z2 = z1 + 1; z2 < BOARD_SIZE * BOARD_SIZE; z2++)
				{
					int x2 = order[z2].x;
					int y2 = order[z2].y;
					if (board[x2][y2]) continue;

					board[x2][y2] = player;
					int offset2 = compute_score({ x2, y2, player });

					data_vector.push_back({ z1, z2, offset1 + offset2 });

					board[x2][y2] = 0;
				}
				board[x1][y1] = 0;
			}

			// stores the score in decreasing order, to easily access the candidates
			std::sort(data_vector.begin(), data_vector.end(), [&](data X, data Y) {
				return X.score > Y.score;
			});

			int num = min(BOARD_SIZE * BOARD_SIZE, data_vector.size());

			// sort the most-valued candidates in increasing order
			std::sort(data_vector.begin(), data_vector.begin() + num, [&](data X, data Y) {
				return X.score < Y.score;
			});

			for (int z = 0; z < num; z++)
			{
				data dat = data_vector[z];
				int x1 = order[dat.z1].x;
				int y1 = order[dat.z1].y;
				board[x1][y1] = player;

				int x2 = order[dat.z2].x;
				int y2 = order[dat.z2].y;
				board[x2][y2] = player;
				int val = alphabeta(depth - 2, COLOR_OPPS, 2, alpha, beta, false);
				//printf("%d %d %d %d %d %d\n",x1,y1,x2,y2,dat.score,val);

				board[x1][y1] = 0;
				board[x2][y2] = 0;

				ret = max(ret, dat.score + val);

				// alpha < ret or alpha <= ret?
				if (feedback && alpha <= ret)
				{
					p1 = { x1, y1 };
					p2 = { x2, y2 };
				}

				alpha = max(alpha, ret);
				if (beta <= alpha)
				{
					break;
				}
			}
		}
		return ret;
	}
	else // pure alpha-beta to the opponent
	{
		int ret = INF;
		for (int z1 = 0; z1 < BOARD_SIZE * BOARD_SIZE; z1++)
		{
			int x1 = order[z1].x;
			int y1 = order[z1].y;
			if (board[x1][y1]) continue;

			board[x1][y1] = player;
			int offset1 = compute_score({ x1, y1, player });

			for (int z2 = z1 + 1; z2 < BOARD_SIZE * BOARD_SIZE; z2++)
			{
				int x2 = order[z2].x;
				int y2 = order[z2].y;
				if (board[x2][y2]) continue;

				board[x2][y2] = player;

				int offset2 = compute_score({ x2, y2, player });
				int val = alphabeta(depth - 2, COLOR_OURS, 2, alpha, beta, false);

				board[x2][y2] = 0;
				ret = min(ret, offset1 + offset2 + val);

				beta = min(beta, ret);

				if (beta <= alpha)
				{
					break;
				}
			}
			board[x1][y1] = 0;
		}
		return ret;
	}
}

bool compDist(point p1, point p2) {
	return distFromMid[p1.x][p1.y] < distFromMid[p2.x][p2.y];
}

void updateOrder() {
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
		for (int j = 0; j < BOARD_SIZE; j++)
			distFromMid[i][j] = abs(mid_x - i) + abs(mid_y - j);

	sort(order.begin(), order.end(), compDist);
}

void myturn(int cnt) {
	static bool isFirst = true;
	if (isFirst)
	{
		srand((time_t)time(NULL));
		for (int i = 0; i < BOARD_SIZE; i++)
			for (int j = 0; j < BOARD_SIZE; j++)
				order.push_back({ i, j });
	}

	// maybe bfs among the previous stones might be better

	update_board();

	updateOrder();

	alphabeta(cnt + MAX_DEPTH, COLOR_OURS, cnt, -INF, INF, true);

	int x[2] = { p1.x, p2.x };
	int y[2] = { p1.y, p2.y };

	domymove(x, y, cnt);

	isFirst = false;
}

#ifdef SECRET_AGENCY_DEBUG_MODE
int main()
{
	// testing the first 'two-stone' turn
	board[9][9] = 2;
	myturn(2);
}
#endif