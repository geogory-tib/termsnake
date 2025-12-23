#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#define GRIDROW 10
#define GRIDCOL 50
#define PANIC(Msg)\
  printf("PANIC in %s: %s\n",__func__,Msg);\
  abort();
#define ANSI_CLEAR_SCREEN "\x1B[2J"
#define ANSI_MOVE_TO_ORIGIN "\x1b[H\x1b[2J"
#define ANSI_MOVE_DOWN_ONE "\x1B[1B"
#define ANSI_MOVE_TO_COL_ZERO "\x1B[0G"

struct Vector2I{
  int x,y ;
  void operator+=(Vector2I other){
	this->x += other.x;
	this->y += other.y;
  }
  Vector2I operator+(Vector2I other){
	return {(this->x + other.x),(this->y + other.y)};
  }
  bool operator==(Vector2I other){
	return ((this->x == other.x) && (this->y == other.y));
  }
};

struct snake_t{
  int body_len;
  Vector2I *body;
  Vector2I dir;
};
struct gamestate{
  snake_t snake;
  Vector2I food;
  bool dead;
};

char Grid [GRIDROW * GRIDCOL];

Vector2I DirTable[4] = {
  {1,0},
  {-1,0},
  {0,1},
  {0,-1}
};
char async_get_char(){
  pollfd stdin_poll;
  stdin_poll.events = POLLIN;
  stdin_poll.fd = STDIN_FILENO;
  int ready = poll(&stdin_poll, 1, 80);
  if(!ready){
	return 0;
  }
  return getchar();
}
termios init_terminal(void){
  termios prev_term_attrs;
  termios new_term_attrs{};
  int code = 0;
  code = tcgetattr(STDIN_FILENO, &prev_term_attrs);
  if(code < 0){
	PANIC("Error Getting Term Info");
  }
  new_term_attrs = prev_term_attrs;
  new_term_attrs.c_lflag &= ~(ICANON | ECHO);
  new_term_attrs.c_cc[VMIN] = 1;
  new_term_attrs.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term_attrs);
  setvbuf(stdout, NULL, _IONBF, 0); 
  return prev_term_attrs;
}

Vector2I random_vec(){
  Vector2I ret;
  srand(time(0));
  ret.x = rand() % GRIDCOL;
  ret.y = rand() % GRIDROW;
  return ret;
}

void render_grid(void){
  write(STDOUT_FILENO,ANSI_MOVE_TO_ORIGIN,sizeof(ANSI_MOVE_TO_ORIGIN));
  for(int y = 0; y <= GRIDROW;y++){
	for(int x = 0;x <= GRIDCOL;x++ ){
	  int index = (y * GRIDCOL) + x;
	  putchar(Grid[index]);
	}
	write(STDOUT_FILENO,ANSI_MOVE_DOWN_ONE,sizeof(ANSI_MOVE_DOWN_ONE));
	write(STDOUT_FILENO,ANSI_MOVE_TO_COL_ZERO,sizeof(ANSI_MOVE_TO_COL_ZERO));
  }
}


void write_to_gird_buf(Vector2I cell,char ch){
  int index = (cell.y * GRIDCOL) + cell.x;
  if(index > 100){
	//asm("int3");
  }
  Grid[index] = ch;
}
void handle_snake_movement(gamestate *state){
  snake_t *snake = &state->snake;
  Vector2I *PrevSnake = (Vector2I *)calloc(snake->body_len,sizeof(Vector2I));
  for(int i = 0;i < snake->body_len;i++){
	PrevSnake[i] = snake->body[i];
	write_to_gird_buf(PrevSnake[i], ' ');
  }
  Vector2I *SnakeHead = &snake->body[0];
  if((*SnakeHead + snake->dir) == state->food){
	//asm("int3");
	write_to_gird_buf(state->food, ' ');
	snake->body = (Vector2I *)realloc(snake->body, (snake->body_len + 1) * sizeof(Vector2I));
	snake->body_len += 1;
	snake->body[0] = PrevSnake[0];
	snake->body[0] += snake->dir;
	write_to_gird_buf(snake->body[0], '#');
	for(int i = 0; i < snake->body_len - 1; i++){
	  snake->body[i + 1] = PrevSnake[i];
	  write_to_gird_buf(snake->body[i + 1], '#');
	}
	state->food = random_vec();
	write_to_gird_buf(state->food, '@');
	goto cleanup;
  }
  *SnakeHead += snake->dir;
  write_to_gird_buf(*SnakeHead, '#');
  // if(SnakeHead->x == 10){
  // 	asm("int3");
  // }
  
  for(int i = 1;i < snake->body_len;i++){
	snake->body[i] = PrevSnake[i - 1];
	write_to_gird_buf(snake->body[i], '#');
  }
 cleanup:
  free(PrevSnake);
}

void init_game_state(gamestate *state){
  state->snake.body = (Vector2I *)calloc(1,sizeof(Vector2I));
  state->snake.body_len = 1;
  state->snake.dir = DirTable[0];
  state->snake.body[0] = {0,0};
  state->food = random_vec();
}

void handle_input(gamestate *state,char playerin){
  switch (playerin){
  case 'd':
	state->snake.dir = DirTable[0];
	break;
  case 'a':
	state->snake.dir = DirTable[1];
	break;
  case 'w':
	state->snake.dir = DirTable[3];
	break;
  case 's':
	state->snake.dir = DirTable[2];
	break;
  default:
	break;
  }
}

int main(void){
  gamestate state{};
  termios prev_state = init_terminal();
  char playerin = 0;
  memset(Grid,' ',sizeof(Grid));
  init_game_state(&state);
  write_to_gird_buf(state.food, '@');
  while(playerin != 'q' && !state.dead){
	render_grid();
	//playerin = getchar();
	playerin = async_get_char();
	handle_input(&state, playerin);
	handle_snake_movement(&state);
  }
  tcsetattr(STDIN_FILENO, TCSANOW, &prev_state);
  return 0;
}
