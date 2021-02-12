#include "chess.h"
#include "chess_validator.h"



Chess::Chess(): d_buffer(boost::asio::dynamic_buffer(read_string)) ,
                read_pipe(boost::process::async_pipe(io_service)) ,
                write_pipe(boost::process::async_pipe(io_service))
{
    thinking = false;
    d_buffer.grow( 8000 );
    child_process = new boost::process::child(  "stockfish_12_win_x64/stockfish_20090216_x64.exe" , 
                                                boost::process::std_in  <  write_pipe ,
                                                boost::process::std_out >  read_pipe  );
    
    on_stdout = [&](const boost::system::error_code & ec, size_t n){
        saucer_print("on stdout done");
        // if(!ec) boost::asio::async_read( *read_pipe , boost::asio::buffer(vread_buffer) , on_stdout );
    };
    on_stdin = [&]( const boost::system::error_code & ec, std::size_t n ){
        saucer_print("on stdin done");
    };
}
Chess::~Chess(){
    child_process->terminate();
    delete child_process;
}
void            Chess::new_game( int level ){
    if( level > 20 ) level = 20;
    if( level < 0 ) level = 0;
    
    // current_state_fen = "8/6q1/4K3/1q6/k7/8/1ppp4/7q w - - 3 10";
    current_state_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    chess_validator::get_table_state( current_state_fen.c_str() , &current_state );
    
    write_string = "setoption name Skill Level value " + std::to_string(level) + "\n";
    boost::asio::async_write( write_pipe , boost::process::buffer(write_string) , on_stdin );
    io_service.poll();
    io_service.restart();

}
std::string     Chess::get_current_state(){
    return current_state_fen;
}
bool            Chess::is_move_valid( std::string move ){
    invalid_reason = chess_validator::is_move_invalid( &current_state , move.c_str() );
    return invalid_reason == chess_validator::INVALID_REASON::VALID;
}
int             Chess::get_invalid_reason(){
    return invalid_reason;
}
void            Chess::apply_move( std::string move ){
    if( is_move_valid(move) ){
        std::string old_state = current_state_fen;

        current_state = chess_validator::apply_move( &current_state , move.c_str() );
        current_state_fen = chess_validator::to_fen_string(&current_state);

        #ifndef NDEBUG
            std::fstream fs;
            fs.open("stockfish_log.txt",std::fstream::app);
            fs << "__halfmove__" << std::endl;
            fs << old_state << " ---> " << move << " ---> " << current_state_fen << std::endl;
            fs.close();
        #endif

    }
    else saucer_err("Trying to apply a invalid move!?" , current_state_fen , "->" , move );
}
void            Chess::start_thinking( int search_time_mseconds ){
    SAUCER_ASSERT( thinking == false , "Can't start thinking while thinking already!" );
    thinking = true;
    
    saucer_print("IA: I'll start to think about " , current_state_fen );
    
    write_string = "position fen " + current_state_fen + "\ngo movetime " + std::to_string(search_time_mseconds) + " \n";
    boost::asio::async_write( write_pipe , boost::process::buffer(write_string) , on_stdin );

}
bool            Chess::finish_thinking(){
    if( !thinking ) return false;
    
    d_buffer.consume(d_buffer.size());
    boost::asio::async_read_until( read_pipe , d_buffer , '\n' , on_stdout );
    io_service.poll();
    io_service.restart();
    size_t bestmove_pos = read_string.find("bestmove");
    if( bestmove_pos != std::string::npos ){
        std::string bestmove_line = read_string.substr( bestmove_pos  );
        size_t move_begin = bestmove_line.find(' ');
        size_t move_end  = bestmove_line.find(' ',move_begin+1) - 1;
        std::string move = bestmove_line.substr(move_begin+1,move_end-move_begin);
        if( move.find("(none)") != std::string::npos ){
            // probably checkmate or stalemate
            thinking = false;
            return true;
        }
        else if( is_move_valid(move) ) {
            apply_move( move );
            thinking = false;
            return true;
        }
        else {
            saucer_err("IA trying invalid move?" )
            saucer_err(current_state_fen);
            saucer_err(move);
        }
    }
    return false;
}
std::string     Chess::get_current_winner(){
    if( chess_validator::is_check_mate(&current_state) ){
        if( current_state.next_color_to_play == chess_validator::COLOR_BLACK )
            return "white";
        else return "black";
    } else return "";
}
bool            Chess::is_draw(){
    draw_reason = chess_validator::get_draw_reason(&current_state); 
    return draw_reason != chess_validator::DRAW_REASON::NO_DRAW;
}
int             Chess::get_draw_reason(){
    return draw_reason;
}
void            Chess::bind_methods(){

    REGISTER_COMPONENT_HELPERS(Chess,"chess")

    REGISTER_LUA_CONSTANT(ChessInvalidReason, VALID,                chess_validator::VALID );
    REGISTER_LUA_CONSTANT(ChessInvalidReason, INVALID_FEN_STRING,   chess_validator::INVALID_FEN_STRING ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, NOT_PLAYER_TURN,      chess_validator::NOT_PLAYER_TURN ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, NO_UNIT,              chess_validator::NO_UNIT ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, SAME_PLACE,           chess_validator::SAME_PLACE ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, CAPTURING_SAME_COLOR, chess_validator::CAPTURING_SAME_COLOR ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, INVALID_UNIT_MOVE,    chess_validator::INVALID_UNIT_MOVE ); 
    REGISTER_LUA_CONSTANT(ChessInvalidReason, KING_IN_CHECK,        chess_validator::KING_IN_CHECK ); 

    REGISTER_LUA_CONSTANT(ChessDrawReason, NO_DRAW,                chess_validator::NO_DRAW );
    REGISTER_LUA_CONSTANT(ChessDrawReason, STALEMATE,              chess_validator::STALEMATE );    
    REGISTER_LUA_CONSTANT(ChessDrawReason, INSUFFICIENT_MATERIAL,  chess_validator::INSUFFICIENT_MATERIAL );                
    REGISTER_LUA_CONSTANT(ChessDrawReason, SEVENTYFIVE_MOVES,      chess_validator::SEVENTYFIVE_MOVES );            


    REGISTER_LUA_MEMBER_FUNCTION( Chess , new_game );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , get_current_state );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , is_move_valid );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , get_invalid_reason );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , apply_move );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , start_thinking );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , finish_thinking );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , get_current_winner );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , is_draw );
    REGISTER_LUA_MEMBER_FUNCTION( Chess , get_draw_reason );

}
