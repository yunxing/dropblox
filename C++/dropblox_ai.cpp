#include "dropblox_ai.h"

using namespace json;
using namespace std;

//----------------------------------
// Block implementation starts here!
//----------------------------------

Block::Block(Object& raw_block) {
  center.i = (int)(Number&)raw_block["center"]["i"];
  center.j = (int)(Number&)raw_block["center"]["j"];
  size = 0;

  Array& raw_offsets = raw_block["offsets"];
  for (Array::const_iterator it = raw_offsets.Begin(); it < raw_offsets.End(); it++) {
    size += 1;
  }
  for (int i = 0; i < size; i++) {
    offsets[i].i = (Number&)raw_offsets[i]["i"];
    offsets[i].j = (Number&)raw_offsets[i]["j"];
  }

  translation.i = 0;
  translation.j = 0;
  rotation = 0;
}

void Block::left() {
  translation.j -= 1;
}

void Block::right() {
  translation.j += 1;
}

void Block::up() {
  translation.i -= 1;
}

void Block::down() {
  translation.i += 1;
}

void Block::rotate() {
  rotation += 1;
}

void Block::unrotate() {
  rotation -= 1;
}

// The checked_* methods below perform an operation on the block
// only if it's a legal move on the passed in board.  They
// return true if the move succeeded.
//
// The block is still assumed to start in a legal position.
bool Block::checked_left(const Board& board) {
  left();
  if (board.check(*this)) {
    return true;
  }
  right();
  return false;
}

bool Block::checked_right(const Board& board) {
  right();
  if (board.check(*this)) {
    return true;
  }
  left();
  return false;
}

bool Block::checked_up(const Board& board) {
  up();
  if (board.check(*this)) {
    return true;
  }
  down();
  return false;
}

bool Block::checked_down(const Board& board) {
  down();
  if (board.check(*this)) {
    return true;
  }
  up();
  return false;
}

bool Block::checked_rotate(const Board& board) {
  rotate();
  if (board.check(*this)) {
    return true;
  }
  unrotate();
  return false;
}

void Block::do_command(const string& command) {
  if (command == "left") {
    left();
  } else if (command == "right") {
    right();
  } else if (command == "up") {
    up();
  } else if (command == "down") {
    down();
  } else if (command == "rotate") {
    rotate();
  } else {
    throw Exception("Invalid command " + command);
  }
}

void Block::do_commands(const vector<string>& commands) {
  for (int i = 0; i < commands.size(); i++) {
    do_command(commands[i]);
  }
}

void Block::reset_position() {
  translation.i = 0;
  translation.j = 0;
  rotation = 0;
}

//----------------------------------
// Board implementation starts here!
//----------------------------------

Board::Board() {
  rows = ROWS;
  cols = COLS;
}

Board::Board(Object& state) {
  rows = ROWS;
  cols = COLS;

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      bitmap[i][j] = ((int)(Number&)state["bitmap"][i][j] ? 1 : 0);
    }
  }

  // Note that these blocks are NEVER destructed! This is because calling
  // place() on a board will create new boards which share these objects.
  //
  // There's a memory leak here, but it's okay: blocks are only constructed
  // when you construct a board from a JSON Object, which should only happen
  // for the very first board. The total memory leaked will only be ~10 kb.
  block = new Block(state["block"]);
  for (int i = 0; i < PREVIEW_SIZE; i++) {
    preview.push_back(new Block(state["preview"][i]));
  }
}

// Returns true if the `query` block is in valid position - that is, if all of
// its squares are in bounds and are currently unoccupied.
bool Board::check(const Block& query) const {
  Point point;
  for (int i = 0; i < query.size; i++) {
    point.i = query.center.i + query.translation.i;
    point.j = query.center.j + query.translation.j;
    if (query.rotation % 2) {
      point.i += (2 - query.rotation)*query.offsets[i].j;
      point.j +=  -(2 - query.rotation)*query.offsets[i].i;
    } else {
      point.i += (1 - query.rotation)*query.offsets[i].i;
      point.j += (1 - query.rotation)*query.offsets[i].j;
    }
    if (point.i < 0 || point.i >= ROWS ||
        point.j < 0 || point.j >= COLS || bitmap[point.i][point.j]) {
      return false;
    }
  }
  return true;
}

// Resets the block's position, moves it according to the given commands, then
// drops it onto the board. Returns a pointer to the new board state object.
//
// Throws an exception if the block is ever in an invalid position.
Board* Board::do_commands(const vector<string>& commands) {
  block->reset_position();
  if (!check(*block)) {
    throw Exception("Block started in an invalid position");
  }
  int row_removed;
  for (int i = 0; i < commands.size(); i++) {
    if (commands[i] == "drop") {
      return place(row_removed);
    } else {
      block->do_command(commands[i]);
      if (!check(*block)) {
        throw Exception("Block reached in an invalid position");
      }
    }
  }
  // If we've gotten here, there was no "drop" command. Drop anyway.
  return place(row_removed);
}

// Drops the block from whatever position it is currently at. Returns a
// pointer to the new board state object, with the next block drawn from the
// preview list.
//
// Assumes the block starts out in valid position.
// This method translates the current block downwards.
//
// If there are no blocks left in the preview list, this method will fail badly!
// This is okay because we don't expect to look ahead that far.
Board* Board::place(int &row_removed) {
  Board* new_board = new Board();

  while (check(*block)) {
    block->down();
  }
  block->up();

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      new_board->bitmap[i][j] = bitmap[i][j];
    }
  }

  Point point;
  for (int i = 0; i < block->size; i++) {
    point.i = block->center.i + block->translation.i;
    point.j = block->center.j + block->translation.j;
    if (block->rotation % 2) {
      point.i += (2 - block->rotation)*block->offsets[i].j;
      point.j +=  -(2 - block->rotation)*block->offsets[i].i;
    } else {
      point.i += (1 - block->rotation)*block->offsets[i].i;
      point.j += (1 - block->rotation)*block->offsets[i].j;
    }
    new_board->bitmap[point.i][point.j] = 1;
  }
  row_removed = Board::remove_rows(&(new_board->bitmap));

  new_board->block = preview[0];
  for (int i = 1; i < preview.size(); i++) {
    new_board->preview.push_back(preview[i]);
  }

  return new_board;
}

// A static method that takes in a new_bitmap and removes any full rows from it.
// Mutates the new_bitmap in place.
int Board::remove_rows(Bitmap* new_bitmap) {
  int rows_removed = 0;
  for (int i = ROWS - 1; i >= 0; i--) {
    bool full = true;
    for (int j = 0; j < COLS; j++) {
      if (!(*new_bitmap)[i][j]) {
        full = false;
        break;
      }
    }
    if (full) {
      rows_removed += 1;
    } else if (rows_removed) {
      for (int j = 0; j < COLS; j++) {
        (*new_bitmap)[i + rows_removed][j] = (*new_bitmap)[i][j];
      }
    }
  }
  for (int i = 0; i < rows_removed; i++) {
    for (int j = 0; j < COLS; j++) {
      (*new_bitmap)[i][j] = 0;
    }
  }
  return rows_removed;
}

// get the landing height
int get_landing_height(Block* block) {
    return block->center.i;
}

static int points_earned(int rows_cleared) {
  return (1 << rows_cleared) - 1;
}

// get the number of holes in the board
int get_number_of_holes(Board *board) {
    int holes = 0;
    for (int i = ROWS - 1; i >= 0; i--) {
        int row_holes = 0;
        for (int j = 0; j < COLS; j++) {
            if (board->bitmap[i][j] == 0) {
                row_holes++;
            }
        }
        if (row_holes == COLS) continue;
        holes += row_holes;
    }
    return holes;
}

// get row transitions
int get_row_transitions(Board *board) {
    int transitions = 0;
    int cell, last_cell = 1;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            cell = board->bitmap[i][j];
            if (cell != last_cell) {
                ++transitions;
            }
            last_cell = cell;
        }

        if (cell == 0) {
            ++transitions;
        }
        last_cell = 1;
    }
    return transitions;
}

// get column transitions
int get_col_transitions(Board *board) {
    int transitions = 0;
    int cell, last_cell = 1;
    for (int j = 0; j < COLS; j++) {
        for (int i = 0; i < ROWS; i++) {
            cell = board->bitmap[i][j];
            if (cell != last_cell) {
                ++transitions;
            }
            last_cell = cell;
        }
        if (cell == 0) {
            ++transitions;
        }
        last_cell = 1;
    }
    return transitions;
}

// get well sum
int get_well_sum(Board *board) {
    int well_sum = 0;
    for (int col = 0; col < COLS; col++) {
        int has_a_roof = false;
        int found_well = false;
        for (int row = 0; row < ROWS; row++) {
            if (board->bitmap[row][col]) {
                has_a_roof = true;
            }
            if (!has_a_roof) {
                bool leftcol = (col== 0) || board->bitmap[row][col - 1];
                bool rightcol = (col == COLS - 1) || board->bitmap[row][col + 1];
                if (!board->bitmap[row][col] && leftcol && rightcol) {
                    if (!found_well) {
                        found_well = true;
                        for (int i = row; i < ROWS; i++) {
                            if (!board->bitmap[i][col]) {
                                well_sum++;
                            }
                        }
                    }
                }
            }
        }
    }
    return well_sum;
}
#define ROWS_REMOVED 0.378565931393
#define ROW_TRANSITIONS -0.548886169599
#define LANDING_HEIGHT -0.71240334146
#define POINTS_EARNED 0.378565931393
#define HOLES -1.99902287016
#define WELL_SUMS -0.151923526632
#define COL_TRANSITIONS -0.793256698244
float calc_score(Board board) {
  Block* block = board.block;
  Point prev_translation = block->translation;
  int prev_rotation = block->rotation;
  int row_removed = 0;
  Board* new_board = board.place(row_removed);
  // calculate score
  float score = 0;
  int landing_height = get_landing_height(block);
  int number_of_holes = get_number_of_holes(new_board);
  int row_transitions = get_row_transitions(new_board);
  int col_transitions = get_col_transitions(new_board);
  int well_sum = get_well_sum(new_board);
  int points = points_earned(row_removed);
  score = row_removed * ROWS_REMOVED + landing_height * LANDING_HEIGHT
    + number_of_holes * HOLES + row_transitions * ROW_TRANSITIONS
    + col_transitions * COL_TRANSITIONS + well_sum * WELL_SUMS
    + points * POINTS_EARNED;

  block->translation = prev_translation;
  block->rotation = prev_rotation;
  return score;
}

// only calculates left/right, rotation
vector<string> get_moves(Block* block) {
  vector<string> moves;
  for (int i = 0; i < block->rotation - 1; ++i) {
    moves.push_back("rotate");
  }
  if (block->translation.j > 0) {
    for (int i = 0; i < block->translation.j; ++i) {
      moves.push_back("right");
    }
  }
  else if (block->translation.j < 0) {
    for (int i = 0; i < -block->translation.j; ++i) {
      moves.push_back("left");
    }
  }
  return moves;
}

vector<string> pick_move(Board board) {
  Block* block = board.block;
  float max_score = 0.0f;
  vector<string> best_moves;
  for (int i = 0; i < 4; ++i) {
    block->rotate();
    while (board.check(*block)) {
      block->left();
      float score = calc_score(board);
      if (score > max_score) {
        max_score = score;
        best_moves = get_moves(block);
      }
    }

    block->translation.i = 0;
    block->translation.j = 0;

    while (board.check(*block)) {
      block->right();
      float score = calc_score(board);
      if (score > max_score) {
        max_score = score;
        best_moves = get_moves(block);
      }
    }
  }
  return best_moves;
}

int main(int argc, char** argv) {
  // Construct a JSON Object with the given game state.
  istringstream raw_state(argv[1]);
  Object state;
  Reader::Read(state, raw_state);

  // Construct a board from this Object.
  Board board(state);

  // Make some moves!
  vector<string> moves;
  moves = pick_move(board);
  // Ignore the last move, because it moved the block into invalid
  // position. Make all the rest.
  for (int i = 0; i < moves.size() - 1; i++) {
    cout << moves[i] << endl;
  }
}
