// WL_PLANE.C

#include "version.h"

#ifdef USE_FLOORCEILINGTEX

#include "wl_def.h"
#include "wl_shade.h"

byte    *ceilingsource,*floorsource;

#ifndef USE_MULTIFLATS
void GetFlatTextures (void)
{
#ifdef USE_FEATUREFLAGS
    ceilingsource = PM_GetPage(ffDataBottomRight >> 8);
    floorsource = PM_GetPage(ffDataBottomRight & 0xff);
#else
    ceilingsource = PM_GetPage(0);
    floorsource = PM_GetPage(1);
#endif
}
#endif

/*
===================
=
= DrawSpan
=
= Height ranges from 0 (infinity) to [centery] (nearest)
=
= With multi-textured floors and ceilings stored in lower and upper bytes of
= according tilex/tiley in the third mapplane, respectively
=
===================
*/

void DrawSpan (int x1, int x2, int height)
{
    int      tilex,tiley;
    byte     *dest;
    byte     *shade;
    int      texture,spot;
    uint32_t rowofs;
    int      count,prestep;
    int      xpix,ypix,spanlength;
    fixed    basedist,absstep;
    fixed    xpartial,ypartial;
    fixed    xfrac,yfrac;
    fixed    xstep,ystep;

    count = x2 - x1;

    if (!count)
        return;                                                 // nothing to draw

	if (gamestate.gameflags & GM_SHADE)
    shade = shadetable[GetShade(height << 3)];
    dest = vbuf + ylookup[centery - 1 - height] + x1;
    rowofs = ylookup[(height << 1) + 1];                        // toprow to bottomrow delta

    prestep = centerx - x1 + 1;
    basedist = FixedDiv(scale,height + 1) >> 1;                 // distance to row projection

    xstep = (viewsin >> 1) / (height + 1);
    ystep = (viewcos >> 1) / (height + 1);

    xfrac = (viewx + FixedMul(basedist,viewcos)) - (xstep * prestep);
    yfrac = (viewy - FixedMul(basedist,viewsin)) - (ystep * prestep);

    xpix = ypix = 0x7fff;

//
// draw two spans simultaneously
//
    while (count > 0)
    {
        //
        // get tile coords of texture
        //
        tilex = xfrac >> TILESHIFT;
        tiley = yfrac >> TILESHIFT;

        //
        // get floor & ceiling textures
        //
        spot = MAPSPOT(tilex,tiley,2);

        ceilingsource = PM_GetPage(spot >> 8 );
        floorsource = PM_GetPage(spot & 0xff);

        if (xstep)
        {
            absstep = xstep;
            xpartial = xfrac & (TILEGLOBAL - 1);

            if (xstep > 0)
                xpartial = (xpartial ^ (TILEGLOBAL - 1)) + 1;
            else
                absstep = -absstep;

            if (xpartial <= absstep)
                xpix = 1;
            else
            {
                xpix = xpartial / absstep;

                if (xpartial % absstep)
                    xpix++;
            }
        }

        if (ystep)
        {
            absstep = ystep;
            ypartial = yfrac & (TILEGLOBAL - 1);

            if (ystep > 0)
                ypartial = (ypartial ^ (TILEGLOBAL - 1)) + 1;
            else
                absstep = -absstep;

            if (ypartial <= absstep)
                ypix = 1;
            else
            {
                ypix = ypartial / absstep;

                if (ypartial % absstep)
                    ypix++;
            }
        }

        spanlength = (xpix < ypix) ? xpix : ypix;

        if (count < spanlength)
        {
            spanlength = count;
            count = 0;
        }
        else
            count -= spanlength;

        while (spanlength--)
        {
            texture = ((xfrac >> FIXED2TEXSHIFT) & TEXTUREMASK) + ((yfrac >> (FRACBITS - TEXTURESHIFT)) & (TEXTURESIZE - 1));

            //
            // write ceiling pixel
            //

            if (gamestate.gameflags & GM_SHADE) *dest = shade[ceilingsource[texture]];
            else *dest = ceilingsource[texture];
            //
            // write floor pixel
            //

            if (gamestate.gameflags & GM_SHADE)  dest[rowofs] = shade[floorsource[texture]];
            else dest[rowofs] = floorsource[texture];

            dest++;

            xfrac += xstep;
            yfrac += ystep;
        }
    }
}

/*
===================
=
= DrawPlanes
=
===================
*/

void DrawPlanes (void)
{
    int     x,y;
    int16_t	height;

//
// loop over all columns
//
    y = centery;

    for (x = 0; x < viewwidth; x++)
    {
        height = wallheight[x] >> 3;

        if (height < y)
        {
            //
            // more starts
            //
            while (y > height)
                spanstart[--y] = x;
        }
        else if (height > y)
        {
            //
            // draw clipped spans
            //
            if (height > centery)
                height = centery;

            while (y < height)
            {
                if (y > 0)
                    DrawSpan (spanstart[y],x,y);

                y++;
            }
        }
    }

    //
    // draw spans
    //
    height = centery;

    while (y < height)
    {
        if (y > 0)
            DrawSpan (spanstart[y],viewwidth,y);

        y++;
    }
}

#endif
