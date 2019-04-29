/*
 * Copyright (C) 2019  Nitu Robert-Georgian
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This is a C + SDL2 implementation of Conway's Game Of Life */
/* Keybindings:
   - minus             -> slow down simulation
   - plus              -> fasten simulation
   - p                 -> pause / resume
   - left mouse click  -> change cells color
   - right mouse click -> change background color
   - escape / q        -> quit the simulation
   - F11               -> fullscreen
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <SDL2/SDL.h>

#define ARRAY_SIZE( name, type ) ( sizeof( name ) / sizeof ( type ) )

struct SDL_Color gameColors = {
    .r = 255,
    .g = 127,
    .b = 0,
    .a = 255
};

struct SDL_Color backgroundColor = {
    .r = 0,
    .g = 0,
    .b = 0,
    .a = 255
};

const uint8_t  DEFAULT_DELTA_TIME = 60;
const uint16_t BOARD_SIDE         = 200;
const uint8_t  PIXEL_SIZE         = 5;
uint8_t        gFullscreen        = 0;

struct gameOfLife_t {
    uint8_t  deltaTime;
    SDL_bool simulationPaused;
    uint8_t  board[BOARD_SIDE][BOARD_SIDE]; /* the board to display */
    uint8_t  workBoard[BOARD_SIDE][BOARD_SIDE]; /* the working board */
};

SDL_Window *   gWindow   = NULL;
SDL_Renderer * gRenderer = NULL;

void Abort( const char *, ... );
void InitializeGraphics( void );
void InitializeSimulation( struct gameOfLife_t * );
void SimulationLoop( struct gameOfLife_t * );
void CleanUp( void );

int GetRowByIndex( int );
int GetColumnByIndex( int );

void UpdateBoard( struct gameOfLife_t * );
void EvaluateKey( SDL_Event *, struct gameOfLife_t * );
void ApplyLifeRule( struct gameOfLife_t *, int, int );

int main( void ) {
    struct gameOfLife_t gameOfLife = {
        .deltaTime = DEFAULT_DELTA_TIME,
        .simulationPaused = SDL_FALSE
    };

    InitializeGraphics();
    atexit( CleanUp );
    InitializeSimulation( &gameOfLife );
    SimulationLoop( &gameOfLife );
}

void Abort( const char * errorMessage, ... ) {
    va_list stackArguments;

    for ( int c = 0; errorMessage[c] != '\0'; c++ ) {
        if ( errorMessage[c] == '{' && errorMessage[c+1] == '}' ) {
            fprintf( stderr, "%s", va_arg( stackArguments, const char * ) );
            c++;
        } else {
            fputc( errorMessage[c], stderr );
        }
    }

    fputc( '\n', stderr );
    exit( 1 );
}

void InitializeGraphics( void ) {
    if ( SDL_Init( SDL_INIT_VIDEO ) ) {
        Abort( "Cannot initialize SDL2: {}", SDL_GetError() );
    }

    gWindow = SDL_CreateWindow( "Game of Life",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                PIXEL_SIZE * BOARD_SIDE, PIXEL_SIZE * BOARD_SIDE,
                                SDL_WINDOW_SHOWN );

    if ( gWindow == NULL ) {
        Abort( "[-] Cannot create window: {}", SDL_GetError() );
    }

    gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED );

    if ( gRenderer == NULL ) {
        Abort( "[-] Cannot create renderer: {}", SDL_GetError() );
    }

    SDL_SetRenderDrawColor( gRenderer, gameColors.r, gameColors.g, gameColors.b, gameColors.a );
    SDL_RenderClear( gRenderer );
}

void InitializeSimulation( struct gameOfLife_t * gameOfLife ) {
    uint8_t randomNumber;

    srand( time( 0 ) );
    for ( uint16_t i = 0; i < BOARD_SIDE * BOARD_SIDE; i++ ) {
        randomNumber = random() % 10;
        if ( randomNumber == 0 ) {
            gameOfLife->board[GetColumnByIndex( i )][GetRowByIndex( i )] = 1;
        } else {
            gameOfLife->board[GetColumnByIndex( i )][GetRowByIndex( i )] = 0;
        }

        if ( (i + 1) % BOARD_SIDE == 0 ) {
            printf( "\n" );
        }
    }
}

void SimulationLoop( struct gameOfLife_t * gameOfLife ) {
    SDL_Event event;
    struct SDL_Color * colorPointer;

    for ( ;; ) {
        if ( !gameOfLife->simulationPaused ) {
            UpdateBoard( gameOfLife );
        }

        while ( SDL_PollEvent( &event ) ) {
            switch ( event.type ) {
            case SDL_QUIT:
                puts( "Arrivederci" );
                exit( 0 );

            case SDL_KEYDOWN:
                EvaluateKey( &event, gameOfLife );
                break;

            case SDL_MOUSEBUTTONDOWN:
                if ( event.button.button == SDL_BUTTON_LEFT ) {
                    colorPointer = &gameColors;
                } else {
                    colorPointer = &backgroundColor;
                }
                
                colorPointer->r = random() % 256;
                colorPointer->g = random() % 256;
                colorPointer->b = random() % 256;
                UpdateBoard( gameOfLife );
                break;
            }
        }

        SDL_Delay( 1000 / gameOfLife->deltaTime );
    }
}

void CleanUp( void ) {
    SDL_DestroyRenderer( gRenderer );
    SDL_DestroyWindow( gWindow );
    SDL_Quit();
}

int GetRowByIndex( int index ) {
    int row = index / BOARD_SIDE;
    return row;
}

int GetColumnByIndex( int index ) {
    int column = index % BOARD_SIDE;
    return column;
}

void UpdateBoard( struct gameOfLife_t * gameOfLife ) {
    struct SDL_Rect pixel = {
                             .w = PIXEL_SIZE,
                             .h = PIXEL_SIZE,
                             .x = 0,
                             .y = 0
    };

    uint16_t lineX = 0;

    /* Clear the screen */
    SDL_SetRenderDrawColor( gRenderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a );
    SDL_RenderClear( gRenderer );

    /* Draw the board */
    SDL_SetRenderDrawColor( gRenderer, gameColors.r, gameColors.g, gameColors.b, gameColors.a );
    while ( lineX < PIXEL_SIZE * BOARD_SIDE ) {
        SDL_RenderDrawLine( gRenderer, 0, lineX, PIXEL_SIZE * BOARD_SIDE, lineX );
        SDL_RenderDrawLine( gRenderer, lineX, 0, lineX, PIXEL_SIZE * BOARD_SIDE );
        lineX += PIXEL_SIZE;
    }

    /* work on the new arena */
    /* work on the board */
    for ( int i = BOARD_SIDE; i < BOARD_SIDE * BOARD_SIDE; i++ ) {
        ApplyLifeRule( gameOfLife, GetRowByIndex( i ), GetColumnByIndex( i ) );
    }
    
    /* set the new board to the working board */
    memcpy( gameOfLife->board, gameOfLife->workBoard, sizeof ( gameOfLife->workBoard ) );

    /* draw the life cells */
    for ( int i = 0; i < BOARD_SIDE * BOARD_SIDE; i++ ) {
        if ( gameOfLife->board[GetRowByIndex( i )][GetColumnByIndex( i )] == 1 ) {
            pixel.x = GetRowByIndex( i ) * PIXEL_SIZE;
            pixel.y = GetColumnByIndex( i ) * PIXEL_SIZE;
            SDL_RenderFillRect( gRenderer, &pixel );
        }
    }

    /* show the changes to the screen */
    SDL_RenderPresent( gRenderer );
}

void EvaluateKey( SDL_Event * event, struct gameOfLife_t * gameOfLife ) {
    switch ( event->key.keysym.sym ) {
    case SDLK_ESCAPE:
    case SDLK_q:
        puts( "Arrivederci" );
        exit( 0 );

    case SDLK_p:
        gameOfLife->simulationPaused = ( gameOfLife->simulationPaused ) ? SDL_FALSE : SDL_TRUE;
        printf( "Pause: %d\n", gameOfLife->simulationPaused );
        break;

    case SDLK_MINUS:
        gameOfLife->deltaTime--;
        break;

    case SDLK_PLUS:
        gameOfLife->deltaTime++;
        break;

    case SDLK_F11:
        if ( gFullscreen ) {
            SDL_SetWindowFullscreen( gWindow, SDL_WINDOW_SHOWN );
            gFullscreen = 0;
        } else {
            SDL_SetWindowFullscreen( gWindow, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP );
            gFullscreen = 1;
        }
    }
}

void ApplyLifeRule( struct gameOfLife_t * gameOfLife, int row, int col ) {
    /* A rewrite would be beneficial, this sucks on eyes! */
    if ( row > 0 && row < BOARD_SIDE - 1 ) {
        /* not the bottom or top row */
        if ( col > 0 && col < BOARD_SIDE - 1 ) {
            /* middle board */
            int lifeForms = 0;
            for ( int neighbourRow = row - 1; neighbourRow < row + 2; neighbourRow++ ) {
                for ( int neighbourCol = col - 1; neighbourCol < col + 2; neighbourCol++ ) {
                    if ( gameOfLife->board[neighbourRow][neighbourCol] ) {
                        lifeForms++;
                    }
                }
            }
            lifeForms--;

            /* rule 1 and 3 */
            if ( lifeForms < 2 || lifeForms > 3 ) {
                gameOfLife->workBoard[row][col] = 0;
            } else {
                /* rule 2 and 4 */
                gameOfLife->workBoard[row][col] = 1;
            }
        } else {
            /* left or right column */
            /* to implement */
        }
    } else {
        /* top or bottom row */
        /* to implement */
    }
}
