#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "isoEngine.h"

void setupRect(SDL_Rect *rect,int x,int y,int w,int h)
{
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
}

void InitIsoEngine(isoEngineT *isoEngine, int tileSizeInPixels)
{
    if(isoEngine == NULL)
    {
        fprintf(stderr,"Error in InitIsoEngine(...): isoEngine parameter is NULL!\n");
        return;
    }

    if(tileSizeInPixels<=0){
        TILESIZE = 32;
    }
    else{
        TILESIZE = tileSizeInPixels;
    }

    isoEngine->mapHeight = 0;
    isoEngine->mapWidth = 0;
    isoEngine->scrollX = 0;
    isoEngine->scrollY = 0;
}

void IsoEngineSetMapSize(isoEngineT *isoEngine,int width, int height)
{
    if(isoEngine == NULL)
    {
        return;
    }
    isoEngine->mapHeight = height;
    isoEngine->mapWidth = width;
}

void Convert2dToIso(point2DT *point)
{
    int tmpX = point->x - point->y;
    int tmpY = (point->x + point->y)*0.5;
    point->x = tmpX;
    point->y = tmpY;
}

void ConvertIsoTo2D(point2DT *point)
{
    int tmpX = (2 * point->y + point->x)*0.5;
    int tmpY = (2 * point->y - point->x)*0.5;
    point->x = tmpX;
    point->y = tmpY;
}

void GetTileCoordinates(point2DT *point,point2DT *point2DCoord)
{
    float tempX = (float)point->x / (float)TILESIZE;
    float tempY = (float)point->y / (float)TILESIZE;

    //convert back to integer
    point2DCoord->x = (int)tempX;
    point2DCoord->y = (int)tempY;
}

void convertIsoCameraToCartesian(isoEngineT *isoEngine,point2DT *cartesianCamPos)
{
    point2DT tmpPoint;

    if(isoEngine == NULL || cartesianCamPos == NULL)
    {
        return;
    }

    tmpPoint.x = isoEngine->scrollX;
    tmpPoint.y = isoEngine->scrollY;

    Convert2dToIso(&tmpPoint);

    tmpPoint.x = tmpPoint.x/2;

    if(tmpPoint.x<0){
        tmpPoint.x = abs(tmpPoint.x);
    }
    else if(tmpPoint.x>0)
    {
        tmpPoint.x = -abs(tmpPoint.x);
    }
    *cartesianCamPos = tmpPoint;
}

void convertCartesianCameraToIsometric(isoEngineT *isoEngine,point2DT *cartesianCamPos)
{
    point2DT tmpPoint;

    if(isoEngine == NULL || cartesianCamPos == NULL)
    {
        return;
    }
    tmpPoint = *cartesianCamPos;

    tmpPoint.x = (int)tmpPoint.x*2;

    if(tmpPoint.x<0){
        tmpPoint.x = abs((int)tmpPoint.x);
    }
    else if(tmpPoint.x>0)
    {
        tmpPoint.x = -abs((int)tmpPoint.x);
    }

    ConvertIsoTo2D(&tmpPoint);

    isoEngine->scrollX = (int)tmpPoint.x;
    isoEngine->scrollY = (int)tmpPoint.y;

}
