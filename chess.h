#ifndef CHESS_H
#define CHESS_H

#include "core.h"
#include "chess_fen_validator/chess_validator.h"
#include <boost/asio.hpp>
#include <boost/process.hpp>

class Chess : public Component {

    REGISTER_AS_COMPONENT(Chess);

    private:

        // Process communication related ------------
        boost::process::child*                      child_process;
        boost::asio::io_service                     io_service;
        boost::process::async_pipe                 read_pipe;
        boost::process::async_pipe                 write_pipe;
        std::string                                 read_string;
        std::string                                 write_string;

        boost::asio::dynamic_string_buffer<char, std::char_traits<char>, std::allocator<char>>   d_buffer;
        std::function<void(const boost::system::error_code & ec, std::size_t n)> on_stdout;
        std::function<void(const boost::system::error_code & ec, std::size_t n)> on_stdin;
        // ------------------------------------------

        chess_validator::TableState     current_state;
        std::string     current_state_fen;
        std::string     last_engine_move;
        int             draw_reason;
        int             invalid_reason;
        bool            thinking;

    public:
        Chess();
        ~Chess();

        void            new_game( int level );
        std::string     get_current_state();
        bool            is_move_valid( std::string move );
        int             get_invalid_reason();
        void            apply_move( std::string move );
        void            start_thinking( int search_time_mseconds );
        bool            finish_thinking();
        std::string     get_current_winner();
        std::string     get_last_engine_move();
        bool            is_draw();
        int             get_draw_reason();

        static void bind_methods();

};

#endif