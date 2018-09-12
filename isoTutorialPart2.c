/*
 *   Isometric Game Tutorial Part 2
 *   Author: Johan Forsblom
 *   Date: 27th October 2017
 *
 *   This tutorial covers the following:
 *
 *      * Converting between Isometric and Cartesian camera coordinates (both ways)
 *      * view culling - only draw the tiles visible on the screen
 *      * Zoom the map in and out with the mouse wheel
 *      * Center camera on point (object coordinates)
 *      * Center camera to tile under the mouse
 *      * Moving a character around in the game world (no collision detection in part 2)
 *      * Scrolling the map with the mouse
 *      * Toggle between game modes
 *
 *   NOTE: At the moment most of the code is in one file, this is due to this being a tutorial.
 *            When part 3 comes, two videos will be released at the same time, part 2.5 (refactoring code) and
 *            part 3 - Covering multi-layer map rendering, collision handling and some more stuff (not decided upon yet)
 *
 *   Usage:
 *   Space bar -  toggle between Overview mode / Object focus mode
 *   Move the character with w,a,s,d
 *   Zoom in and out with the mouse wheel
 *
 *   Overview mode:
 *   Left click - center map to tile under mouse
 *   Scroll map with the mouse close to the edges
 *
 *   Object focus mode:
 *   Left click on the map for "tile picking" (shows the selected tile up in the top left corner of the screen)
 *
 ******************************************************************************************************************
 *
 *   Copyright 2017 Johan Forsblom
 *
 *   You may use the code for whatever reason you want. If you do a commercial project and took inspiration from or
 *   copied this code, all i require is a place in your comment please do put my name in the credits section of your game :)
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 *   LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "initclose.h"
#include "renderer.h"
#include "texture.h"
#include "isoEngine.h"

#define PLAYER_DIR_UP_LEFT      0
#define PLAYER_DIR_UP           1
#define PLAYER_DIR_UP_RIGHT     2
#define PLAYER_DIR_RIGHT        3
#define PLAYER_DIR_DOWN_RIGHT   4
#define PLAYER_DIR_DOWN         5
#define PLAYER_DIR_DOWN_LEFT    6
#define PLAYER_DIR_LEFT         7

#define NUM_ISOMETRIC_TILES 5
#define NUM_CHARACTER_SPRITES 8
#define MAP_HEIGHT 64
#define MAP_WIDTH 64

#define GAME_MODE_OVERVIEW          0
#define GAME_MODE_OBJECT_FOCUS      1
#define NUM_GAME_MODES              2

int worldTest[MAP_HEIGHT][MAP_WIDTH];

typedef struct gameT
{
    SDL_Event event;
    int loopDone;
    SDL_Rect mouseRect;
    point2DT mousePoint;
    point2DT mapScroll2Dpos;
    int mapScrolllSpeed;
    isoEngineT isoEngine;
    int lastTileClicked;
    float zoomLevel;
    point2DT tilePos;
    point2DT charPoint;
    int charDirection;
    int gameMode;
}gameT;

gameT game;
textureT tilesTex;
textureT characterTex;
SDL_Rect tilesRects[NUM_ISOMETRIC_TILES];
SDL_Rect charRects[NUM_CHARACTER_SPRITES];

void scrollMapWithMouse();

void initTileClip()
{
    int x=0,y=0;
    int i;

    textureInit(&tilesTex,0,0,0,NULL,NULL,SDL_FLIP_NONE);
    for(i=0;i<NUM_ISOMETRIC_TILES;++i){
        setupRect(&tilesRects[i],x,y,64,80);
        x+=64;
    }
}

void initCharClip()
{
    int x=0,y=0;
    int i;
    textureInit(&characterTex,0,0,0,NULL,NULL,SDL_FLIP_NONE);
    for(i=0;i<NUM_CHARACTER_SPRITES;++i)
    {
        setupRect(&charRects[i],x,y,70,102);
        x+=70;
    }
}

void writeCoords()
{
    fprintf(stdout,"\rmap x:%d,map y:%d, iso x:%d, iso y:%d                                     ",
            (int)game.mapScroll2Dpos.x,(int)game.mapScroll2Dpos.y,(int)game.isoEngine.scrollX,(int)game.isoEngine.scrollY);
}

void generateMap()
{
    int x,y;
    int paintTile=0;
    for(y=0;y<MAP_HEIGHT;y+=2)
    {
        for(x=0;x<MAP_HEIGHT;x+=2)
        {
            worldTest[y][x] = 1;
            worldTest[y+1][x] = 1;
            worldTest[y][x+1] = 1;
            worldTest[y+1][x+1] = 1;
            paintTile = rand()%10;

            if(paintTile>8)
            {
                if(y<MAP_HEIGHT-4 && x<MAP_WIDTH-4){
                    worldTest[y][x] = 4;
                    worldTest[y+1][x] = 4;
                    worldTest[y][x+1] = 4;
                    worldTest[y+1][x+1] = 4;
                }
            }
            if(paintTile==7){
                if(y<MAP_HEIGHT-4 && x<MAP_WIDTH-4){
                    worldTest[y][x] = 3;
                    worldTest[y+1][x] = 3;
                    worldTest[y][x+1] = 3;
                    worldTest[y+1][x+1] = 3;
                }
            }
        }
    }
}

void init()
{
    int tileSize = 32;
    game.loopDone = 0;
    initTileClip();
    initCharClip();
    InitIsoEngine(&game.isoEngine,tileSize);
    IsoEngineSetMapSize(&game.isoEngine,16,16);
    generateMap();
    game.isoEngine.scrollX = 0;
    game.isoEngine.scrollY = 0;
    game.mapScroll2Dpos.x = 0;
    game.mapScroll2Dpos.y = 0;
    game.mapScrolllSpeed = 6;
    game.lastTileClicked = -1;
    game.zoomLevel = 1.0;
    game.charPoint.x = 0;
    game.charPoint.y = 0;
    game.charDirection = PLAYER_DIR_DOWN;
    game.gameMode = GAME_MODE_OVERVIEW;

    if(loadTexture(&tilesTex,"data/isotiles.png")==0){
        fprintf(stderr,"Error, could not load texture: data/isotiles.png\n");
        exit(1);
    }

    if(loadTexture(&characterTex,"data/character.png")==0){
        fprintf(stderr,"Error, could not load texture: data/character.png\n");
        exit(1);
    }
}

void drawIsoMouse()
{
    int modulusX = TILESIZE*game.zoomLevel;
    int modulusY = TILESIZE*game.zoomLevel;
    int correctX =(((int)game.mapScroll2Dpos.x)%modulusX)*2;
    int correctY = ((int)game.mapScroll2Dpos.y)%modulusY;

    game.mousePoint.x = (game.mouseRect.x/TILESIZE) * TILESIZE;
    game.mousePoint.y = (game.mouseRect.y/TILESIZE) * TILESIZE;

    //For every other x position on the map
    if(((int)game.mousePoint.x/TILESIZE)%2){
        //Move the mouse down by half a tile so we can
        //pick isometric tiles on that row as well.
        game.mousePoint.y+=TILESIZE*0.5;
    }
    textureRenderXYClipScale(&tilesTex,(game.zoomLevel*game.mousePoint.x)-correctX,
                             (game.zoomLevel*game.mousePoint.y)+correctY,&tilesRects[0],game.zoomLevel);
}

void drawIsoMap(isoEngineT *isoEngine)
{
    int i,j;
    int x,y;
    int tile = 4;
    point2DT point;

    int startX = -3/game.zoomLevel +(game.mapScroll2Dpos.x/game.zoomLevel/TILESIZE)*2;
    int startY = -20/game.zoomLevel + abs((game.mapScroll2Dpos.y/game.zoomLevel/TILESIZE))*2;
    int numTilesInWidth = ((WINDOW_WIDTH/TILESIZE)/game.zoomLevel);
    int numTilesInHeight = ((WINDOW_HEIGHT/TILESIZE)/game.zoomLevel)*2;

    for(i=startY;i<startY+numTilesInHeight+26;++i){
        for(j=startX;j<startX+numTilesInWidth+5;++j){

            //only draw when both x & y is equal, so we skip here
            if((j&1) != (i&1)){
                continue;
            }
            x = (i+j)/2;
            y = (i-j)/2;

            if(x>=0 && y>=0 && x<MAP_WIDTH && y<MAP_HEIGHT){
                tile = worldTest[y][x];
                point.x = ((x*game.zoomLevel *TILESIZE) + isoEngine->scrollX);
                point.y = ((y*game.zoomLevel *TILESIZE) + isoEngine->scrollY);
                Convert2dToIso(&point);
                textureRenderXYClipScale(&tilesTex,point.x,point.y,&tilesRects[tile],game.zoomLevel);
            }
        }
    }
    /*
    //loop through the map
    for(i=0;i<isoEngine->mapHeight;++i)
    {
        for(j=0;j<isoEngine->mapWidth;++j)
        {
            point.x = (j *game.zoomLevel *TILESIZE) + isoEngine->scrollX;
            point.y = (i *game.zoomLevel *TILESIZE) + isoEngine->scrollY;

            tile = worldTest[i][j];

            Convert2dToIso(&point);

            textureRenderXYClipScale(&tilesTex,point.x,point.y,&tilesRects[tile],game.zoomLevel);
        }
    }*/
}

void getMouseTilePos(isoEngineT *isoEngine, point2DT *mouseTilePos)
{
    point2DT point;
    point2DT tileShift, mouse2IsoPOint;

    if(isoEngine == NULL || mouseTilePos == NULL){
        return;
    }

    int modulusX = TILESIZE*game.zoomLevel;
    int modulusY = TILESIZE*game.zoomLevel;
    int correctX =(((int)game.mapScroll2Dpos.x)%modulusX)*2;
    int correctY = ((int)game.mapScroll2Dpos.y)%modulusY;

    //copy mouse point
    mouse2IsoPOint = game.mousePoint;
    ConvertIsoTo2D(&mouse2IsoPOint);

    //get tile coordinates
    GetTileCoordinates(&mouse2IsoPOint,&point);

    tileShift.x = correctX;
    tileShift.y = correctY;
    Convert2dToIso(&tileShift);

    //check for fixing tile position when the y position is larger than 0
    if(game.mapScroll2Dpos.y>0){
        point.y -= (((float)isoEngine->scrollY-tileShift.y)/(float)TILESIZE)/game.zoomLevel;
        point.y+=1;
    }
    else{
        point.y -= (((float)isoEngine->scrollY-tileShift.y)/(float)TILESIZE)/game.zoomLevel;
    }

    //check for fixing tile position when the x position is larger than 0
    if(game.mapScroll2Dpos.x>0)
    {
        point.x -= (((float)isoEngine->scrollX+(float)tileShift.x)/(float)TILESIZE)/game.zoomLevel;
        point.x+=1;
    }
    else{
        point.x -= (((float)isoEngine->scrollX+(float)tileShift.x)/(float)TILESIZE)/game.zoomLevel;
    }
    mouseTilePos->x = (int)point.x;
    mouseTilePos->y = (int)point.y;
}

void getMouseTileClick(isoEngineT *isoEngine)
{
    point2DT point;
    getMouseTilePos(isoEngine,&point);
    if(point.x>=0 && point.y>=0 && point.x<MAP_WIDTH && point.y<MAP_HEIGHT)
    {
        game.lastTileClicked = worldTest[(int)point.y][(int)point.x];
    }
}

void CenterMapToTileUnderMouse(isoEngineT *isoEngine)
{
    point2DT mouseIsoTilePos;

    //calculate the offset of the center of the screen
    int offsetX = WINDOW_WIDTH/game.zoomLevel/2;
    int offsetY = WINDOW_HEIGHT/game.zoomLevel/2;

    //get the tile under the mouse
    getMouseTilePos(isoEngine,&mouseIsoTilePos);

    game.tilePos.x = mouseIsoTilePos.x*TILESIZE;
    game.tilePos.y = mouseIsoTilePos.y*TILESIZE;

    //convert to isometric coordinates
    Convert2dToIso(&mouseIsoTilePos);

    //move the x position
    game.mapScroll2Dpos.x = ((mouseIsoTilePos.x*TILESIZE)*game.zoomLevel)/2;
    game.mapScroll2Dpos.x -= (offsetX*game.zoomLevel)/2;

    //move the y position
    game.mapScroll2Dpos.y = -((mouseIsoTilePos.y*TILESIZE)*game.zoomLevel);
    game.mapScroll2Dpos.y += offsetY*game.zoomLevel;

    //convert the map 2d camera to isometric camera
    convertCartesianCameraToIsometric(isoEngine,&game.mapScroll2Dpos);
}

void CenterMap(isoEngineT *isoEngine,point2DT *objectPoint)
{
    point2DT pointPos = *objectPoint;

    //calculate the offset of the center of the screen
    int offsetX = WINDOW_WIDTH/game.zoomLevel/2;
    int offsetY = WINDOW_HEIGHT/game.zoomLevel/2;

    game.tilePos.x = objectPoint->x;
    game.tilePos.y = objectPoint->y;

    Convert2dToIso(&pointPos);

    game.mapScroll2Dpos.x = floor((pointPos.x)*game.zoomLevel)/2;
    game.mapScroll2Dpos.x -= offsetX*game.zoomLevel/2;

    if(game.gameMode == GAME_MODE_OBJECT_FOCUS){
        game.mapScroll2Dpos.x +=45*game.zoomLevel/2;
    }

    game.mapScroll2Dpos.y = -floor((pointPos.y)*game.zoomLevel);
    game.mapScroll2Dpos.y += offsetY*game.zoomLevel;

    if(game.gameMode == GAME_MODE_OBJECT_FOCUS){
        game.mapScroll2Dpos.y -= 51*game.zoomLevel;
    }

    convertCartesianCameraToIsometric(isoEngine,&game.mapScroll2Dpos);
}


void drawCharacter(isoEngineT *isoEngine)
{
    point2DT point;
    point.x = (int)(game.charPoint.x*game.zoomLevel)+ isoEngine->scrollX;
    point.y = (int)(game.charPoint.y*game.zoomLevel)+ isoEngine->scrollY;
    Convert2dToIso(&point);
    textureRenderXYClipScale(&characterTex,point.x,point.y,&charRects[game.charDirection],game.zoomLevel);
}

void draw()
{
    SDL_SetRenderDrawColor(getRenderer(),0x3b,0x3b,0x3b,0x00);
    SDL_RenderClear(getRenderer());

    drawIsoMap(&game.isoEngine);
    drawCharacter(&game.isoEngine);
    drawIsoMouse();

    if(game.lastTileClicked!=-1){
        textureRenderXYClip(&tilesTex,0,0,&tilesRects[game.lastTileClicked]);
    }

    SDL_RenderPresent(getRenderer());
    //Don't be a CPU HOG!! :D
    SDL_Delay(10);
}

void update()
{
    SDL_GetMouseState(&game.mouseRect.x,&game.mouseRect.y);
    game.mouseRect.x = game.mouseRect.x/game.zoomLevel;
    game.mouseRect.y = game.mouseRect.y/game.zoomLevel;

    if(game.gameMode == GAME_MODE_OBJECT_FOCUS)
    {
        CenterMap(&game.isoEngine,&game.charPoint);
    }
    else if(game.gameMode == GAME_MODE_OVERVIEW){
        scrollMapWithMouse();
    }

}

void updateInput()
{
    const Uint8 *keystate = SDL_GetKeyboardState(NULL);

    while(SDL_PollEvent(&game.event) != 0)
    {
        switch(game.event.type)
        {
            case SDL_QUIT:
                game.loopDone=1;
            break;

            case SDL_KEYUP:
                switch(game.event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        game.loopDone=1;
                    break;

                    case SDLK_SPACE:
                        game.gameMode++;
                        if(game.gameMode>=NUM_GAME_MODES)
                        {
                            game.gameMode = GAME_MODE_OVERVIEW;
                        }
                    break;

                    default:break;
                }
            break;

            case SDL_MOUSEBUTTONDOWN:
                if(game.event.button.button == SDL_BUTTON_LEFT)
                {
                    if(game.gameMode==GAME_MODE_OVERVIEW){
                        CenterMapToTileUnderMouse(&game.isoEngine);

                    }
                    if(game.gameMode == GAME_MODE_OBJECT_FOCUS){
                        getMouseTileClick(&game.isoEngine);
                    }
                }
            break;

            case SDL_MOUSEWHEEL:
                //If the user scrolled the mouse wheel up
                if(game.event.wheel.y>=1)
                {
                    if(game.zoomLevel<3.0){
                        game.zoomLevel+=0.25;
                        if(game.gameMode==GAME_MODE_OVERVIEW)
                        {
                            CenterMap(&game.isoEngine,&game.tilePos);
                        }
                        if(game.gameMode == GAME_MODE_OBJECT_FOCUS){
                            CenterMap(&game.isoEngine,&game.charPoint);
                        }
                    }
                }
                //If the user scrolled the mouse wheel down
                else{
                    if(game.zoomLevel>1.0){
                        game.zoomLevel-=0.25;
                        if(game.gameMode==GAME_MODE_OVERVIEW)
                        {
                            CenterMap(&game.isoEngine,&game.tilePos);
                        }
                        if(game.gameMode == GAME_MODE_OBJECT_FOCUS){
                            CenterMap(&game.isoEngine,&game.charPoint);
                        }
                    }
                }
            break;

            default:break;
        }
    }

    if(keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x+=5;
        game.charPoint.y+=5;
        game.charDirection = PLAYER_DIR_DOWN;
    }
    else if(!keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A] && keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x-=5;
        game.charPoint.y-=5;
        game.charDirection = PLAYER_DIR_UP;
    }
    else if(!keystate[SDL_SCANCODE_S] && keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A] && keystate[SDL_SCANCODE_W])
    {
        game.charPoint.y-=5;
        game.charDirection = PLAYER_DIR_UP_RIGHT;
    }
    else if(!keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_D] && keystate[SDL_SCANCODE_A] && keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x-=5;
        game.charDirection = PLAYER_DIR_UP_LEFT;
    }
    else if(!keystate[SDL_SCANCODE_S] && keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x+=3;
        game.charPoint.y-=3;
        game.charDirection = PLAYER_DIR_RIGHT;
    }
    else if(!keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_D] && keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x-=3;
        game.charPoint.y+=3;
        game.charDirection = PLAYER_DIR_LEFT;
    }
    else if(keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_D] && keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_W])
    {
        game.charPoint.y+=5;
        game.charDirection = PLAYER_DIR_DOWN_LEFT;
    }
    else if(keystate[SDL_SCANCODE_S] && keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_W])
    {
        game.charPoint.x+=5;
        game.charDirection = PLAYER_DIR_DOWN_RIGHT;
    }
/*
    if(keystate[SDL_SCANCODE_W]){

        game.mapScroll2Dpos.y+=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);
    }
    if(keystate[SDL_SCANCODE_A]){

        game.mapScroll2Dpos.x-=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);
    }
    if(keystate[SDL_SCANCODE_S]){

        game.mapScroll2Dpos.y-=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);

    }
    if(keystate[SDL_SCANCODE_D]){

        game.mapScroll2Dpos.x+=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);
    }
*/
}

void scrollMapWithMouse()
{
    int zoomEdgeX = (WINDOW_WIDTH*game.zoomLevel)-(WINDOW_WIDTH);
    int zoomEdgeY = (WINDOW_HEIGHT*game.zoomLevel)-(WINDOW_HEIGHT);

    if(game.mouseRect.x<2){
        game.mapScroll2Dpos.x-=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);
    }
    if(game.mouseRect.x>WINDOW_WIDTH-(zoomEdgeX/game.zoomLevel)-2){
        game.mapScroll2Dpos.x+=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);

    }
    if(game.mouseRect.y<2){
        game.mapScroll2Dpos.y+=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);

    }
    if(game.mouseRect.y>WINDOW_HEIGHT-(zoomEdgeY/game.zoomLevel)-2){
        game.mapScroll2Dpos.y-=game.mapScrolllSpeed;
        convertCartesianCameraToIsometric(&game.isoEngine,&game.mapScroll2Dpos);

    }

}

int main(int argc, char *argv[])
{
    initSDL("Isometric Game Tutorial - Part 2 - By Johan Forsblom");
    init();

    SDL_ShowCursor(0);
    SDL_SetWindowGrab(getWindow(),SDL_TRUE);
    SDL_WarpMouseInWindow(getWindow(),WINDOW_WIDTH/2,WINDOW_HEIGHT/2);

    while(!game.loopDone){
        update();
        updateInput();
        draw();
    }

    closeDownSDL();
    return 0;
}
