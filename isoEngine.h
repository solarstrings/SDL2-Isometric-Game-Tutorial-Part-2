#ifndef ISOENGINE_H_
#define ISOENGINE_H_
#include <SDL2/SDL.h>

unsigned int TILESIZE;

typedef struct isoEngineT
{
    int scrollX;
    int scrollY;
    int mapHeight;
    int mapWidth;
}isoEngineT;

typedef struct point2DT
{
    float x;
    float y;
}point2DT;

void setupRect(SDL_Rect *rect,int x,int y,int w,int h);
void InitIsoEngine(isoEngineT *isoEngine, int tileSizeInPixels);
void IsoEngineSetMapSize(isoEngineT *isoEngine,int width, int height);
void Convert2dToIso(point2DT *point);
void ConvertIsoTo2D(point2DT *point);
void GetTileCoordinates(point2DT *point,point2DT *point2DCoord);

void convertIsoCameraToCartesian(isoEngineT *isoEngine,point2DT *cartesianCamPos);
void convertCartesianCameraToIsometric(isoEngineT *isoEngine,point2DT *cartesianCamPos);
#endif // ISOENGINE_H_
