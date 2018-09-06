// Samsung Go Tournament Form C (g++-4.8.3)

/*
[AI �ڵ� �ۼ� ���]

1. char info[]�� �迭 �ȿ�               "TeamName:�ڽ��� ����,Department:�ڽ��� �Ҽ�"               ������ �ۼ��մϴ�.
( ���� ) Teamname:�� Department:�� �� ���� �մϴ�.
"�ڽ��� ����", "�ڽ��� �Ҽ�"�� �����ؾ� �մϴ�.

2. �Ʒ��� myturn() �Լ� �ȿ� �ڽŸ��� AI �ڵ带 �ۼ��մϴ�.

3. AI ������ �׽�Ʈ �Ͻ� ���� "���� �˰����ȸ ��"�� ����մϴ�.

4. ���� �˰��� ��ȸ ���� �����ϱ⿡�� �ٵϵ��� ���� ��, �ڽ��� "����" �� �� �˰����� �߰��Ͽ� �׽�Ʈ �մϴ�.



[���� �� �Լ�]
myturn(int cnt) : �ڽ��� AI �ڵ带 �ۼ��ϴ� ���� �Լ� �Դϴ�.
int cnt (myturn()�Լ��� �Ķ����) : ���� �� �� �־��ϴ��� ���ϴ� ����, cnt�� 1�̸� ���� ���� ��  �� ����  �δ� ��Ȳ(�� ��), cnt�� 2�̸� �� ���� ���� �δ� ��Ȳ(�� ��)
int  x[0], y[0] : �ڽ��� �� ù �� ° ���� x��ǥ , y��ǥ�� ����Ǿ�� �մϴ�.
int  x[1], y[1] : �ڽ��� �� �� �� ° ���� x��ǥ , y��ǥ�� ����Ǿ�� �մϴ�.
void domymove(int x[], int y[], cnt) : �� ������ ��ǥ�� �����ؼ� ���


//int board[BOARD_SIZE][BOARD_SIZE]; �ٵ��� �����Ȳ ��� �־� �ٷλ�� ������. ��, ���������ͷ� ���� �������
// ������ ���� ��ġ�� �ٵϵ��� ������ �ǰ��� ó��.

boolean ifFree(int x, int y) : ���� [x,y]��ǥ�� �ٵϵ��� �ִ��� Ȯ���ϴ� �Լ� (������ true, ������ false)
int showBoard(int x, int y) : [x, y] ��ǥ�� ���� ���� �����ϴ��� �����ִ� �Լ� (1 = �ڽ��� ��, 2 = ����� ��, 3 = ��ŷ)


<-------AI�� �ۼ��Ͻ� ��, ���� �̸��� �Լ� �� ���� ����� �������� �ʽ��ϴ�----->
*/


// "�����ڵ�[C]"  -> �ڽ��� ���� (����)
// "AI�μ�[C]"  -> �ڽ��� �Ҽ� (����)
// ����� ���������� �ݵ�� �������� ����!
char info[] = { "TeamName:5���ҷ��� ,Department:�Ѱ���" };

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

const int MAX_DEPTH = 6;
int cand_size = 8;

extern int limitTime;
std::chrono::system_clock::time_point start_time;
int time_threshold = 200;
bool isTimeExceeded = false;
int time_cnt = 0;

// x,y : the coordinates
// i : 0 if the stone is ours, 1 if the stone is opponent's, 2 if blocking
struct point {
	int x, y, c;
};

struct data {
	int z1, z2, score;
};

// status of the boardA
int board[BOARD_SIZE][BOARD_SIZE];
int realboard[BOARD_SIZE][BOARD_SIZE];
// weight of adding our connected components
int myscore[LENGTH + 1] = { 0,1,6,10,100,0,0 };
int opscore[LENGTH + 1] = { 0,0,4,6,0,0,0 };


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

point p1, p2, res1, res2;

#ifdef SECRET_AGENCY_DEBUG_MODE
// define these debug methods
void domymove(int x[2], int y[2], int cnt)
{
	for (int i = 0; i < cnt; i++)
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
std::pair<point, int> isOurFourExist(int player) { //start_point & dir ; if there aren't four stones, dir = -1
	bool check = false;
	int ret_dir = -1;
	point stones = { -1,-1,-1 };
	for (int dir = 0; dir < 4; dir++) {
		for (int x = 0; x < BOARD_SIZE; x++) {
			for (int y = 0; y < BOARD_SIZE; y++) {
				int nx = x + 5 * dx[dir];
				int ny = y + 5 * dy[dir];
				if (!is_valid(nx, ny)) continue;
				int cnt = 0;
				for (int k = 0; k < 6; k++) {
					int color = board[x + k * dx[dir]][y + k * dy[dir]];
					if (color == player) cnt++;
					else if (color == 3 - player) { cnt = 0; break; }
					else if (color == COLOR_BLOCK) cnt++;
				}
				if (board[x + (-1) * dx[dir]][y + (-1) * dy[dir]] == player || board[x + 6 * dx[dir]][y + 6 * dy[dir]] == player) // prevent 7 stones
					cnt = 0;
				if (cnt >= 4) {
					check = true;
					stones.x = x;
					stones.y = y;
					stones.c = COLOR_OURS;
					ret_dir = dir;
				}
			}
		}
	}
	return std::make_pair(stones, ret_dir);
}
std::pair<int, std::pair<point, int>> isOppFourExist(point p) { //mystone +, opstone -
	int x = p.x, y = p.y;
	int count=-1, pos = 0;
	int ret_dir = -1;
	bool check = false;
	point stones = { -1,-1,-1 };
	for (int dir = 0; dir < 4; dir++)
	{
		int cnt = 0;
		int our_count = 0;
		int opp_count = 0;
		int blk_count = 0;
		int out_count = 0;
		int line[2 * LENGTH + 1] = {}; // the board status of the line segment
		for (int k = -LENGTH; k <= LENGTH; k++)
		{
			int nx = x + dx[dir] * k;
			int ny = y + dy[dir] * k;
			int& cur_color = line[LENGTH + k];

			// we assume to be a blocking cell
			// if the cell is invalid
			cur_color = 4;
			if (is_valid(nx, ny))
				cur_color = board[nx][ny];
		}
		for (int k = -LENGTH + 1; k <= LENGTH - 1; k++)
		{
			int& cur_color = line[LENGTH + k];
			if (cur_color == 3 - p.c)
				our_count++;
			if (cur_color == p.c)
				opp_count++;
			if (cur_color == COLOR_BLOCK)
				blk_count++;
			//out of map
			else if (cur_color == 4)
				out_count++;

			if (k >= 0)
			{
				if (opp_count + blk_count >= 4 && out_count == 0 && our_count == 0) {
					cnt++;
					pos = k;
				}
				// erase the leftmost stone (similar to deque)
				int prev_color = line[k + 1];
				if (prev_color == 3 - p.c)
				{
					our_count--;
				}
				else if (prev_color == p.c)
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
		if (cnt != 0) {
			check = true;
			stones.x = x + dx[dir] * (pos + 1 - LENGTH);
			stones.y = y + dy[dir] * (pos + 1 - LENGTH);
			stones.c = line[pos + 1];
			ret_dir = dir;
			count = cnt;
		}
	}
	return std::make_pair(count, std::make_pair(stones, ret_dir));
}

// update board status
void update_board()
{
	int cnt = 0;
	realprev.clear();
	realprev.resize(1);
	for (int i = 0; i < BOARD_SIZE; i++)
	{
		for (int j = 0; j < BOARD_SIZE; j++)
		{
			int newData = showBoard(i, j);
			if (realboard[i][j] != showBoard(i, j) && showBoard(i, j) == COLOR_OPPS) {
				if (cnt == 0) {
					realprev[0].first.x = i;
					realprev[0].first.y = j;
					realprev[0].first.c = COLOR_OPPS;
					cnt++;
				}
				else {





					realprev[0].second.x = i;
					realprev[0].second.y = j;
					realprev[0].second.c = COLOR_OPPS;
				}
			}
			realboard[i][j] = showBoard(i, j);
		}
	}
}

void copy_board() {
	for (int i = 0; i < BOARD_SIZE; i++) {
		for (int j = 0; j < BOARD_SIZE; j++) {
			board[i][j] = realboard[i][j];
		}
	}
	prev.clear();
	prev.resize(1);
	prev[0].first.x = realprev[0].first.x;
	prev[0].first.y = realprev[0].first.y;
	prev[0].first.c = realprev[0].first.c;
	prev[0].second.x = realprev[0].second.x;
	prev[0].second.y = realprev[0].second.y;
	prev[0].second.c = realprev[0].second.c;
}

//test
std::vector<point> order;
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
	//use this code when play the first stone
	if (player_cnt == 1)
	{
		int ret = -INF;
		for (int z = 0; z < BOARD_SIZE * BOARD_SIZE; z++)
		{
			int x = order[z].x;
			int y = order[z].y;
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

		int color = (player == COLOR_OURS) ? 1 : -1;
		int ret = -color * INF;

		//stores the candidates
		std::priority_queue<data, std::vector<data>, CompData> pq;

		std::pair<point, int> OurFour = isOurFourExist(player);
		if (OurFour.second != -1) {
			point p[2];
			int cnt = 0;
			for (int j = 0; j < LENGTH; j++) {
				int nx = OurFour.first.x + dx[OurFour.second] * j;
				int ny = OurFour.first.y + dy[OurFour.second] * j;
				if (board[nx][ny] == 0) {
					p[cnt++] = { nx,ny,OurFour.first.c };
				}
			}
			if (cnt == 1) {
				for (int x = 0; x < BOARD_SIZE; x++) {
					for (int y = 0; y < BOARD_SIZE; y++) {
						if (board[x][y] == 0) {
							p[cnt] = { x,y,OurFour.first.c };
						}
					}
				}
			}
			ret = color * 10000;
			//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, p[0].x, p[0].y, p[1].x, p[1].y, 10000, alpha, beta);

			if (feedback)
			{
				p1 = { p[0].x, p[0].y };
				p2 = { p[1].x, p[1].y };
			}
			return ret;
		}
		std::pair<int, std::pair<point, int>> OppFour[2];
		OppFour[0] = isOppFourExist(prev.front().first);
		OppFour[1] = isOppFourExist(prev.front().second);
		for (int i = 0; i < 2; i++) { // prevent with two stones now or lose
			if (OppFour[i].first >= 3) { //opp can make 6 stones
				point p = OppFour[i].second.first;
				int dir = OppFour[i].second.second;
				point Must1[2];
				point Must2[2];
				int cnt1 = 0;
				int cnt2 = 0;
				int pos[4] = { -2,-1,4,5 };
				for (int i = 0; i < 4; i++) {
					if (i < 2) {
						if (board[p.x + dx[dir] * pos[i]][p.y + dy[dir] * pos[i]] == 0)
							Must1[cnt1++] = { p.x + dx[dir] * pos[i],p.y + dy[dir] * pos[i],p.c };
					}
					else {
						if (board[p.x + dx[dir] * pos[i]][p.y + dy[dir] * pos[i]] == 0)
							Must2[cnt2++] = { p.x + dx[dir] * pos[i],p.y + dy[dir] * pos[i],p.c };
					}
				}
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
					data dat = pq.top();
					pq.pop();
					int x1 = Must1[dat.z1].x;
					int y1 = Must1[dat.z1].y;
					board[x1][y1] = player;

					int x2 = Must2[dat.z2].x;
					int y2 = Must2[dat.z2].y;
					board[x2][y2] = player;
					//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, x1, y1, x2, y2, color*dat.score, alpha, beta);
					if (isTimeExceeded) return 0;
					board[x1][y1] = 0;
					board[x2][y2] = 0;
					if (player == COLOR_OURS)
						ret = max(ret, dat.score);
					else
						ret = min(ret, -dat.score);
					//printf("ret = %d\n", ret);

					if (feedback)
					{
						p1 = { x1, y1 };
						p2 = { x2, y2 };
					}
				}
				return ret;
			}
		}
		if (OppFour[0].first >= 1 && OppFour[1].first >= 1 && (OppFour[0].second.second != OppFour[1].second.second)) {
			int x[2], y[2];
			for (int i = 0; i < 2; i++) {
				point p;
				for (int j = 0; j < LENGTH; j++) {
					if (board[OppFour[i].second.first.x + dx[OppFour[i].second.second] * j][OppFour[i].second.first.y + dy[OppFour[i].second.second] * j] == 0) {
						p = { OppFour[i].second.first.x + dx[OppFour[i].second.second] * j,OppFour[i].second.first.y + dy[OppFour[i].second.second] * j,OppFour[i].second.first.c };
						break;
					}
				}
				x[i] = p.x;
				y[i] = p.y;
				if (board[x[i]][y[i]]) continue;

				board[x[i]][y[i]] = player;
				int offset1 = compute_score({ x[i], y[i], player });
			}
			if (feedback && alpha < ret)
			{
				p1 = { x[0], y[0] };
				p2 = { x[1], y[1] };
			}
			return 0;
		}
		for (int i = 0; i<2; i++) { // prevent with one stone now or lose
			if (OppFour[i].first == 1 || OppFour[i].first == 2) {
				point p;
				for (int j = 0; j < LENGTH; j++) {
					if (board[OppFour[i].second.first.x + dx[OppFour[i].second.second] * j][OppFour[i].second.first.y + dy[OppFour[i].second.second] * j] == 0) {
						p = { OppFour[i].second.first.x + dx[OppFour[i].second.second] * j,OppFour[i].second.first.y + dy[OppFour[i].second.second] * j,OppFour[i].second.first.c };
						break;
					}
				}
				int x1 = p.x;
				int y1 = p.y;
				if (board[x1][y1]) continue;

				board[x1][y1] = player;
				int offset1 = compute_score({ x1, y1, player });

				for (int z2 = 0; z2 < BOARD_SIZE * BOARD_SIZE; z2++)
				{
					int x2 = order[z2].x;
					int y2 = order[z2].y;
					if (board[x2][y2]) continue;

					board[x2][y2] = player;
					int offset2 = compute_score({ x2, y2, player });

					if (pq.size() < cand_size)
						pq.push({ 1, z2, offset1 + offset2 });

					if ((pq.top()).score < offset1 + offset2) {
						pq.pop();
						pq.push({ 1, z2, offset1 + offset2 });
					}
					board[x2][y2] = 0;
				}
				board[x1][y1] = 0;

				for (int z = 0; z < cand_size; z++)
				{
					data dat = pq.top();
					pq.pop();
					int x1 = p.x;
					int y1 = p.y;
					board[x1][y1] = player;

					int x2 = order[dat.z2].x;
					int y2 = order[dat.z2].y;
					board[x2][y2] = player;
					//printf("depth:%d,(%d,%d) (%d,%d) %d (%d,%d)\n", depth, p.x, p.y, x2, y2, color*dat.score, alpha, beta);

					prev.push_back(std::pair<point, point>({ x1, y1, player }, { x2, y2, player }));
					int val = alphabeta(depth - 2, 3 - player, 2, color*dat.score, alpha, beta, false);
					if (isTimeExceeded) return 0;
					prev.pop_back();

					board[x1][y1] = 0;
					board[x2][y2] = 0;
					if (player == COLOR_OURS)
						ret = max(ret, dat.score + val);
					else
						ret = min(ret, -dat.score + val);
					//printf("ret = %d\n", color*dat.score + val);

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

				if (pq.size() < cand_size)
					pq.push({ z1, z2, offset1 + offset2 });

				if ((pq.top()).score < offset1 + offset2) {
					pq.pop();
					pq.push({ z1, z2, offset1 + offset2 });
				}
				board[x2][y2] = 0;
			}
			board[x1][y1] = 0;
		}

		for (int z = 0; z < cand_size; z++)
		{
			data dat = pq.top();
			pq.pop();
			int x1 = order[dat.z1].x;
			int y1 = order[dat.z1].y;
			board[x1][y1] = player;

			int x2 = order[dat.z2].x;
			int y2 = order[dat.z2].y;
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
				ret = max(ret, dat.score + val);
			else
				ret = min(ret, -dat.score + val);

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
	start_time = std::chrono::system_clock::now();
	isTimeExceeded = false;

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

	for (int depth = 0; depth <= 10; depth += 2) {
		copy_board();
		alphabeta(cnt + depth, COLOR_OURS, cnt, 0, -INF, INF, true);
		if (isTimeExceeded) break;
		res1 = p1;
		res2 = p2;
	}

	int x[2] = { res1.x, res2.x };
	int y[2] = { res1.y, res2.y };

	domymove(x, y, cnt);

	isFirst = false;
}