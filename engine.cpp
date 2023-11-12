#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <fstream>
#include <iostream>

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"

using namespace std;

void trial(Board& b) {
    BoardData board_data = b.data;
    cout << board_to_str(&board_data) << endl;
    cout << "YES board copied successfully" << endl;
}

bool isCheckMate(Board& currBoard) {
    if (currBoard.get_legal_moves().size() == 0) return true;
    return false;
}

unordered_set<U16> getCaptureMoves(unordered_set<U16> &moves, BoardData &bd) {
    unordered_set<U16> capture_moves;

    for (auto move : moves) {
        U8 p1 = getp1(move);
        for (int i = 0; i < 12; i++) {
            if (bd.board_0[i] == p1) {
                capture_moves.insert(move);
                break;
            }
        }
    }
    return capture_moves;
}

// First let us define a function that will tell us if a piece is under threat or not. There is no need to return a bool, We'll simply check if the 
// length of the vector returned is zero.
vector<U8> findAttackingPositions(U8 piece_pos, Board& b, PlayerColor color) {
    vector<U8> attacking_positions;
    // Board currBoard = *b.copy();
    b.flip_player_();
    auto legal_moves = b.get_legal_moves();
    b.flip_player_();

    for (auto move : legal_moves) 
    {
        U8 attacking_position = getp1(move);
        if (attacking_position == piece_pos) 
        {
            attacking_positions.push_back(getp0(move));
        }
    }
    return attacking_positions;   
}

U8 getPieceType(U8 piece_pos, Board& b){
    // Get the piece type and color at the given position for the current player
    U8 piece = b.data.board_0[piece_pos];

    // Check if the piece is a pawn, rook, bishop, or king
    U8 pieceType = 0;
    if (piece & PAWN) {
        pieceType = PAWN;
    } else if (piece & ROOK) {
        pieceType = ROOK;
    } else if (piece & BISHOP) {
        pieceType = BISHOP;
    } else if (piece & KING) {
        pieceType = KING;
    }

    return pieceType;
}

// A function to return the piece value given the piece type
int get_val(U8 pieceType){
    if (pieceType == PAWN) {
        return 10;
    }
    else if (pieceType == BISHOP) {
        return 30;
    }
    else if (pieceType == ROOK) {
        return 50;
    }
    else {
        return 100;
    }
}

// Find King Eval
double evalHelper(Board& b, PlayerColor color) {
    // First check if the white's king is under threat or not
    // use "find_possible_attackers" function
    if (color == BLACK) {
        if (b.get_legal_moves().size() == 0) return 1e9;
        double eval = 0.0;
        U8 w_king_pos = b.data.w_king;
        vector<U8> possible_attacking_positions = findAttackingPositions(w_king_pos, b, BLACK); // This will be the location of black's pieces that will try to attack the king
        if (possible_attacking_positions.size() != 0) {
            for (auto attacking_position : possible_attacking_positions) {
                int val = (abs(getx(attacking_position) - getx(w_king_pos)) + abs(gety(attacking_position) - gety(w_king_pos)));
                if (val <= 1) {
                    eval = -1000*(get_val(getPieceType(attacking_position, b)));
                    break;
                }
                else {
                    eval = 1000;
                }
            }
        }
        return eval;
    }

    if (b.get_legal_moves().size() == 0) return 1e9;
    int eval = 0;
    U8 b_king_pos = b.data.b_king;
    vector<U8> possibleAttackingPositions = findAttackingPositions(b_king_pos, b, WHITE);
    if (possibleAttackingPositions.size()!=0){
        for (auto attackingPosition : possibleAttackingPositions) {
            int val = (abs(getx(attackingPosition) - getx(b_king_pos)) + abs(gety(attackingPosition) - gety(b_king_pos)));
            if (val <= 1) {
                eval = -1000*(get_val(getPieceType(attackingPosition, b)));
                break;
            }
            else {
                eval = 1000;
            }
        }
    }
    return eval;
}

double distance(U8 p1, U8 p2){
    int x1 = getx(p1);
    int y1 = gety(p1);
    int x2 = getx(p2);
    int y2 = gety(p2);
    return sqrt(pow((x1-x2), 2) + pow((y1-y2), 2));
}

vector<int> board_data(BoardData &bd) {
    PlayerColor color = bd.player_to_play;

    vector<int> count_w(4, 0); // First index for pawn, second for rook and third for bishop
    vector<int> count_b(4, 0);
    if (color == BLACK) {
        count_b[0] += (bd.b_pawn_1 != DEAD) ? 1: 0;
        count_b[0] += (bd.b_pawn_2 != DEAD) ? 1: 0;
        count_b[0] += (bd.b_pawn_3 != DEAD) ? 1: 0;
        count_b[0] += (bd.b_pawn_4 != DEAD) ? 1: 0;
        count_b[1] += (bd.b_rook_1 != DEAD) ? 1: 0;
        count_b[1] += (bd.b_rook_2 != DEAD) ? 1: 0;
        count_b[2] += (bd.b_bishop != DEAD) ? 1 : 0;
        count_b[3] += (bd.b_knight_1 != DEAD) ? 1: 0;
        count_b[3] += (bd.b_knight_2 != DEAD) ? 1: 0;
        return count_b;
    }
    else {
        count_w[0] += (bd.w_pawn_1 != DEAD) ? 1: 0;
        count_w[0] += (bd.w_pawn_2 != DEAD) ? 1: 0;
        count_w[0] += (bd.w_pawn_3 != DEAD) ? 1: 0;
        count_w[0] += (bd.w_pawn_4 != DEAD) ? 1: 0;
        count_w[1] += (bd.w_rook_1 != DEAD) ? 1: 0;
        count_w[1] += (bd.w_rook_2 != DEAD) ? 1: 0;
        count_w[2] += (bd.w_bishop != DEAD) ? 1 : 0;
        count_w[3] += (bd.w_knight_1 != DEAD) ? 1: 0;
        count_w[3] += (bd.w_knight_2 != DEAD) ? 1: 0;
        return count_w;  
    }
}

// double pawn_promo(Board& b, U8 color) {
//     BoardData bd = b.data;
//     U8 p1=0, p2=0, p3=0, p4=0;
//     double result = 0;
//     if (color == WHITE){
//         if (bd.w_pawn_1!=DEAD) {p1 = bd.w_pawn_1;}
//         if (bd.w_pawn_2!=DEAD) {p2 = bd.w_pawn_2;}
//         if (bd.w_pawn_3!=DEAD) {p1 = bd.w_pawn_3;}
//         if (bd.w_pawn_4!=DEAD) {p2 = bd.w_pawn_4;}
//         vector<int> pieces_remaining = board_data(bd);
//         if (pieces_remaining[1]+pieces_remaining[2]<2) //Either one of these three pieces remains or none, we will start giving importance to pawn for promotion, such that it doesn't come under threat
//         {
//             U8 prom_pos_1 = pos(4, 5);
//             U8 prom_pos_2 = pos(4, 6);
//             double total_dist = 0;
//             if (p1) {total_dist += distance(p1, prom_pos_1) + distance(p1, prom_pos_2);}
//             if (p2) {total_dist += distance(p2, prom_pos_1) + distance(p2, prom_pos_2);}
            
//             result = 1000/total_dist;
//         }
//     }
//     else {
//         if (bd.b_pawn_1!=DEAD) {p1 = bd.b_pawn_1;}
//         if (bd.b_pawn_2!=DEAD) {p2 = bd.b_pawn_2;}
//         if (bd.b_pawn_3!=DEAD) {p1 = bd.b_pawn_3;}
//         if (bd.b_pawn_4!=DEAD) {p2 = bd.b_pawn_4;}
//         vector<int> pieces_remaining = board_data(bd);
//         if (pieces_remaining[1]+pieces_remaining[2]<2) //Either one of these three pieces remains or none, we will start giving importance to pawn for promotion, such that it doesn't come under threat
//         {
//             U8 prom_pos_1 = pos(2, 0);
//             U8 prom_pos_2 = pos(2, 1);

//             double total_dist = 0;
//             if (p1) {total_dist += distance(p1, prom_pos_1) + distance(p1, prom_pos_2);}
//             if (p2) {total_dist += distance(p2, prom_pos_1) + distance(p2, prom_pos_2);}
            
//             result = 1000/total_dist;
//         }
//     }
//     return result;
// }

double blackEvalFunction(Board& b, bool isInitial) {
    BoardData bd = b.data;
    double eval = 0;

    double pawn_value = 10;
    double bishop_value = 30;
    double knight_value = 30;
    double rook_value = 50;
    double king_value = 1000;

    int king = 0, bishop = 0, pawn = 0, rook = 0, knight = 0;

    bishop += (bd.b_bishop != DEAD) ? 2: 0;
    bishop -= (bd.w_bishop != DEAD) ? 2: 0;
    pawn -= (bd.w_pawn_1 != DEAD) ? 2: 0;
    pawn -= (bd.w_pawn_2 != DEAD) ? 2: 0;
    pawn -= (bd.w_pawn_3 != DEAD) ? 2: 0;
    pawn -= (bd.w_pawn_4 != DEAD) ? 2: 0;
    pawn += (bd.b_pawn_1 != DEAD) ? 2: 0;
    pawn += (bd.b_pawn_2 != DEAD) ? 2: 0;
    pawn += (bd.b_pawn_3 != DEAD) ? 2: 0;
    pawn += (bd.b_pawn_4 != DEAD) ? 2: 0;
    rook += (bd.b_rook_1 != DEAD) ? 2: 0;
    rook += (bd.b_rook_2 != DEAD) ? 2: 0;
    rook -= (bd.w_rook_1 != DEAD) ? 2: 0;
    rook -= (bd.w_rook_2 != DEAD) ? 2: 0;
    knight -= (bd.w_knight_1 != DEAD) ? 1: 0;
    knight -= (bd.w_knight_2 != DEAD) ? 1: 0;
    knight += (bd.b_knight_1 != DEAD) ? 1: 0;
    knight += (bd.b_knight_2 != DEAD) ? 1: 0;  

    if (isInitial) return (pawn_value*pawn + rook_value*rook + bishop*bishop_value + knight*knight_value);

    king -= (b.under_threat(bd.b_king)) ? 1: -1;
    bishop -= (b.under_threat(bd.b_bishop)) ? 1: -1;

    pawn -= (b.under_threat(bd.b_pawn_1)) ? 1: -1;
    pawn -= (b.under_threat(bd.b_pawn_2)) ? 1: -1;
    pawn -= (b.under_threat(bd.b_pawn_3)) ? 1: -1;
    pawn -= (b.under_threat(bd.b_pawn_4)) ? 1: -1;

    rook -= (b.under_threat(bd.b_rook_1)) ? 1: -1;
    rook -= (b.under_threat(bd.b_rook_2)) ? 1: -1;

    knight -= (b.under_threat(bd.b_knight_1)) ? 1: -1;
    knight -= (b.under_threat(bd.b_knight_2)) ? 1: -1;

    eval += (pawn_value*pawn + rook_value*rook + bishop_value*bishop + knight_value*knight + king_value*king);

    // Evaluate based on pawn advancement
    // eval += pawn_promo(b, BLACK);
    eval+= evalHelper(b, BLACK);

    auto white_moves = b.get_pseudolegal_moves_for_side(WHITE);
    auto black_moves = b.get_pseudolegal_moves_for_side(BLACK);

    int white_capture_val = getCaptureMoves(white_moves, bd).size();
    
    eval -= 60*white_capture_val;

    double white_mobility = white_moves.size();
    double black_mobility = black_moves.size();

    eval += 0.4*(black_mobility - white_mobility);

    return eval;
}

double evalFunction(Board& b, bool isInitial) {
    BoardData bd = b.data;
    double eval = 0.0;

    if (isCheckMate(b)) return 1e9;

    // Piecevalue
    int pawn_value = 10;
    int bishop_value = 30;
    int knight_value = 30;
    int rook_value = 50;
    int king_value = 1000;

    int king = 0, bishop = 0, pawn = 0, rook = 0, knight = 0;

    bishop += (bd.w_bishop != DEAD) ? 1: 0;
    bishop -= (bd.b_bishop != DEAD) ? 1: 0;
    pawn += (bd.w_pawn_1 != DEAD) ? 1: 0;
    pawn += (bd.w_pawn_2 != DEAD) ? 1: 0;
    pawn += (bd.w_pawn_3 != DEAD) ? 1: 0;
    pawn += (bd.w_pawn_4 != DEAD) ? 1: 0;
    pawn -= (bd.b_pawn_1 != DEAD) ? 1: 0;
    pawn -= (bd.b_pawn_2 != DEAD) ? 1: 0;
    pawn -= (bd.b_pawn_3 != DEAD) ? 1: 0;
    pawn -= (bd.b_pawn_4 != DEAD) ? 1: 0;
    rook += (bd.w_rook_1 != DEAD) ? 1: 0;
    rook += (bd.w_rook_2 != DEAD) ? 1: 0;
    rook -= (bd.b_rook_1 != DEAD) ? 1: 0;
    rook -= (bd.b_rook_2 != DEAD) ? 1: 0;
    knight += (bd.w_knight_1 != DEAD) ? 1: 0;
    knight += (bd.w_knight_2 != DEAD) ? 1: 0;
    knight -= (bd.b_knight_1 != DEAD) ? 1: 0;
    knight -= (bd.b_knight_2 != DEAD) ? 1: 0;

    if (isInitial) return (pawn_value*pawn + bishop_value*bishop + rook_value*rook + knight_value*knight);

    king -= (b.under_threat(bd.w_king)) ? 1: -1;
    bishop -= (b.under_threat(bd.w_bishop)) ? 1: -1;

    pawn -= (b.under_threat(bd.w_pawn_1)) ? 1: -1;
    pawn -= (b.under_threat(bd.w_pawn_2)) ? 1: -1;
    pawn -= (b.under_threat(bd.w_pawn_3)) ? 1: -1;
    pawn -= (b.under_threat(bd.w_pawn_4)) ? 1: -1;

    rook -= (b.under_threat(bd.w_rook_1)) ? 1: -1;
    rook -= (b.under_threat(bd.w_rook_2)) ? 1: -1;

    knight -= (b.under_threat(bd.w_knight_1)) ? 1: -1;
    knight -= (b.under_threat(bd.w_knight_2)) ? 1: -1;

    eval += (pawn_value*pawn + rook_value*rook + knight_value*knight + bishop_value*bishop + king_value*king);

    eval += (bd.w_pawn_1 + bd.w_pawn_2 + bd.w_pawn_3 + bd.w_pawn_4 - bd.b_pawn_1 - bd.b_pawn_2 - bd.b_pawn_3 - bd.b_pawn_4)*(0.6*rook_value + 0.4*bishop_value);

    auto white_moves = b.get_pseudolegal_moves_for_side(WHITE);
    auto black_moves = b.get_pseudolegal_moves_for_side(BLACK);

    int white_mobility = white_moves.size();
    int black_mobility = black_moves.size();

    eval += 0.4*(white_mobility - black_mobility);

    auto white_capture_moves = getCaptureMoves(white_moves, bd);
    auto black_capture_moves = getCaptureMoves(black_moves, bd);
    
    eval += 20*white_capture_moves.size();
    eval -= 40*black_capture_moves.size();

    return eval;
}

double alphaBetaSearch(Board& b, double alpha, double beta, int depth, bool is_max_player) {
    if (depth == 0) {
        if (b.data.player_to_play == WHITE) return blackEvalFunction(b, false);
        return evalFunction(b, false);
    }
    else {
        // PlayerColor curr_player = b.data.player_to_play;
        if (is_max_player) {
            double val = -1e9;
            const unordered_set<U16> legal_moves = b.get_legal_moves();
            if (legal_moves.size() == 0) return -1e9;

            for (const auto& move: legal_moves) {
                b.do_move_(move);

                double node_score = alphaBetaSearch(b, alpha, beta, depth-1, false);

                val = max(val, node_score);

                b.undo_last_move_without_flip_(move);
                b.flip_player_();

                alpha = max(alpha, val);

                // Beta cut off
                if (beta <= alpha) {
                    return val;
                }
            }
            return val;
        }
        else {
            double val = 1e9;
            const unordered_set<U16> legal_moves = b.get_legal_moves();

            for (const auto& move: legal_moves) {
                b.do_move_(move);

                double node_score = alphaBetaSearch(b, alpha, beta, depth-1, true);

                val = min(val, node_score);
                b.undo_last_move_without_flip_(move);
                b.flip_player_();

                beta = min(beta, val);

                // beta_cutoff
                if (beta <= alpha) {
                    return val;
                }
            }
            return val;
        }
    }
}

void printPlayer(PlayerColor player) {
    if (player == WHITE) {
        cout << "WHITE" << endl;
    }
    else if (player == BLACK) {
        cout << "BLACK" << endl;
    }
}

void printVector(vector<U16> moves, string color) {
    cout << color << " moves: ";
    for (U16 move: moves) {
        cout << move_to_str(move) << " ";
    }
    cout << endl;
}

vector<U16> global_move_vector_white;
vector<U16> global_move_vector_black;

pair<U16, double> getBestMove(const Board &curr_board, int depth) {
    BoardData bd = curr_board.data;
    Board b = Board(bd);
    double best_score = -1e9;
    U8 color = b.data.player_to_play;
    U16 best_move = 0;
    double val1 = 0, val2 = 0;

    auto move_set = b.get_legal_moves();
    cout << "legal_move_set size: " << move_set.size() << endl;
    if (move_set.size() == 0) {
        return make_pair(best_move, best_score);
    }
    else {
        double alpha = -1e9;
        double beta = 1e9;
        for (auto &legal_move : move_set) {
            if (color == BLACK) val1 = blackEvalFunction(b, true);
            else val1 = evalFunction(b, true);
            b.do_move_(legal_move);
            if (color == BLACK) val2 = blackEvalFunction(b, false);
            else val2 = evalFunction(b, false);
            double move_score = alphaBetaSearch(b, alpha, beta, depth, false) + val2 - (2*val1);
            b.undo_last_move_without_flip_(legal_move);
            b.flip_player_();
            cout << "MOVE: " << move_to_str(legal_move) << " " << move_score << endl;
            if (best_score < move_score) {
                if (color == WHITE) {
                    cout << "checking for 3-fold repitition. WHITE" << endl;
                    int size = global_move_vector_white.size();
                    cout << global_move_vector_white.size() << endl;
                    if ((size >= 4) && (global_move_vector_white[size-2] == global_move_vector_white[size-4]) && (global_move_vector_white[size-2] == legal_move)) {cout << "three-flod repetition found." << endl;continue;}
                    else {
                        best_score = move_score;
                        best_move = legal_move;
                    }
                }
                else {
                    cout << "Checking for 3-fold repition. BLACK" << endl;
                    int size = global_move_vector_black.size();
                    if ((size >= 4) && (global_move_vector_black[size-2] == global_move_vector_black[size-4]) && (global_move_vector_black[size-2] == legal_move)) {cout << "three-fold repition found." << endl;continue;}
                    else {
                        best_score = move_score;
                        best_move = legal_move;
                    }
                }
            }
        }
    }

    return make_pair(best_move, best_score);
}

int moves_taken_white = 0, moves_taken_black = 0;

pair<int, int> piecesLeft(const Board b) {
    BoardData bd = b.data;
    int count_w=0, count_b=0;

    count_w += (bd.w_bishop != DEAD) ? 1: 0;
    count_b += (bd.b_bishop != DEAD) ? 1: 0;
    count_w += (bd.w_pawn_1 != DEAD) ? 1: 0;
    count_w += (bd.w_pawn_2 != DEAD) ? 1: 0;
    count_w += (bd.w_pawn_3 != DEAD) ? 1: 0;
    count_w += (bd.w_pawn_4 != DEAD) ? 1: 0;
    count_b += (bd.b_pawn_1 != DEAD) ? 1: 0;
    count_b += (bd.b_pawn_2 != DEAD) ? 1: 0;
    count_b += (bd.b_pawn_3 != DEAD) ? 1: 0;
    count_b += (bd.b_pawn_4 != DEAD) ? 1: 0;
    count_w += (bd.w_rook_1 != DEAD) ? 1: 0;
    count_w += (bd.w_rook_2 != DEAD) ? 1: 0;
    count_b += (bd.b_rook_1 != DEAD) ? 1: 0;
    count_b += (bd.b_rook_2 != DEAD) ? 1: 0;
    count_w += (bd.w_knight_1 != DEAD) ? 1: 0;
    count_w += (bd.w_knight_2 != DEAD) ? 1: 0;
    count_b += (bd.b_knight_1 != DEAD) ? 1: 0;
    count_b += (bd.b_knight_2 != DEAD) ? 1: 0;

    return make_pair(count_w, count_b);

}

void storeRecords(const string winner, const PlayerColor curr_player) {
    int moves_played_white = 0, moves_played_black = 0;

    if (curr_player == WHITE) {
        moves_played_black = moves_taken_white;
        moves_played_white = moves_taken_white;
    }
    else {
        moves_played_black = moves_taken_black;
        moves_played_white = moves_taken_black + 1;
    }

    string player = (curr_player == WHITE) ? "WHITE": "BLACK";

    const char* white_file = "LogBook/white_data.txt";
    const char* black_file = "LogBook/black_data.txt";

    const char* data_file = "LogBook/records.dat";

    ifstream white_moves(white_file);
    ifstream black_moves(black_file);

    ofstream out_file(data_file, ios::out | ios::app);

    if (!out_file.is_open()) {
        cerr << "Error opening the file." << endl;
        return;
    }

    out_file << "<----------------------------------------NEW GAME STARTS HERE------------------------------------------>\n";

    out_file << "Winner is: " << winner << " player" << endl;
    out_file << endl;

    out_file << "Total moves taken by WHITE player: " << moves_played_white << endl;
    out_file << "Moves taken by WHITE Player: " << endl;

    out_file << white_moves.rdbuf();

    out_file << "\n\n";

    out_file << "Total moves taken by BLACK player: " << moves_played_black << endl;
    out_file << "Moves taken by BLACK Player: " << endl;

    out_file << black_moves.rdbuf();

    out_file << "\n\n";

    out_file << "Game completed. \n\n\n";
    out_file.close();
    white_moves.close();
    black_moves.close();

    cout << "Data record completed" << endl;

    ofstream white_moves_truncate(white_file, ios::trunc);
    ofstream black_moves_truncate(black_file, ios::trunc);

    white_moves_truncate.close();
    black_moves_truncate.close();

    cout << "Data recorded and temporary files created." << endl; 
}

bool isFileEmpty(const string& filename) {
    ifstream file(filename);
    return file.peek() == ifstream::traits_type::eof();
}

// Now, the function is done and dusted. It remains, to choose the correct move.
void Engine::find_best_move(const Board& b) {
    BoardData bd = b.data;
    Board copy = Board(bd);
    PlayerColor curr_player = bd.player_to_play;
    U16 move_to_take = 0;
    double best_score = 0;
    int max_depth = 4;
    if (isFileEmpty("LogBook/white_data.txt")) {
        moves_taken_white = 0;
        global_move_vector_white.clear();
    }
    if (isFileEmpty("LogBook/black_data.txt")) {
        moves_taken_black = 0;
        global_move_vector_black.clear();
    }
    if (curr_player == WHITE && global_move_vector_white.size() == 0) {
        cout << "moves_taken_white: " << moves_taken_white << endl;
        cout << "moves_taken_black: " << moves_taken_black << endl;
        printVector(global_move_vector_white, "WHITE");
        printVector(global_move_vector_black, "BLACK");
        auto move_set = copy.get_legal_moves();
        vector<U16> moves;
        sample(move_set.begin(), move_set.end(), back_inserter(moves), 1, mt19937{random_device{}()});
        move_to_take = moves[0];
        // global_move_vector_white.push_back(move_to_take);
        cout << "Initial Move Taken Randomly by WHITE." << endl;
        cout << "move choosen: " << move_to_str(move_to_take) << endl;
        // moves_taken_white++;
    }
    else {
        for (int i = 1; i <= max_depth; i++) {      
            cout << "iteration: " << i << endl;
            // if (this->time_left <= std::chrono::milliseconds(0)) {break;}
            pair<U16, double> result = getBestMove(copy, i);
            move_to_take = result.first;
            best_score = result.second;
            if (best_score <= -1e9 || best_score >= 1e9) {break;}
        }
    } 
    this->best_move = move_to_take;
    cout << "move choosen: " << move_to_str(move_to_take) << "utility value: " << best_score << endl;
    if (curr_player == WHITE) {
        moves_taken_white++;
        global_move_vector_white.push_back(move_to_take);
        ofstream outfile("LogBook/white_data.txt", ios::out | ios::app);
        outfile << move_to_str(move_to_take) << " ";
        outfile.close();
    }
    else {
        moves_taken_black++;
        global_move_vector_black.push_back(move_to_take);
        ofstream outfile("LogBook/black_data.txt", ios::out | ios::app);
        outfile << move_to_str(move_to_take) << " ";
        outfile.close();
    }
    if (move_to_take == 0) {
        string winner = (curr_player == WHITE) ?"BLACK": "WHITE";
        cout << "WINNER: " << winner << endl;
        cout << "moves taken white: " << moves_taken_white << " " << global_move_vector_white.size() << endl;
        cout << "moves taken black: " << moves_taken_black << " " << global_move_vector_black.size() << endl;
        printVector(global_move_vector_black, "BLACK");
        printVector(global_move_vector_white, "WHITE");
        storeRecords(winner, curr_player);
    }
}
