/*
 *  This file was autogen'd by xtractprotos, v. 1.4
 */
#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

LEPT_DLL LEPTONICA_EXTERN PIXCMAP * pixcmapCreate ( l_int32 depth );
LEPT_DLL LEPTONICA_EXTERN PIXCMAP * pixcmapCopy ( PIXCMAP *cmaps );
LEPT_DLL LEPTONICA_EXTERN void pixcmapDestroy ( PIXCMAP **pcmap );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixcmapAddColor ( PIXCMAP *cmap, l_int32 rval, l_int32 gval, l_int32 bval );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixcmapGetCount ( PIXCMAP *cmap );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixcmapGetColor ( PIXCMAP *cmap, l_int32 index, l_int32 *prval, l_int32 *pgval, l_int32 *pbval );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixcmapHasColor ( PIXCMAP *cmap, l_int32 *pcolor );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixcmapToArrays ( PIXCMAP *cmap, l_int32 **prmap, l_int32 **pgmap, l_int32 **pbmap );
LEPT_DLL extern PIX * pixThresholdToBinary ( PIX *pixs, l_int32 thresh );
LEPT_DLL LEPTONICA_EXTERN void thresholdToBinaryLow ( l_uint32 *datad, l_int32 w, l_int32 h, l_int32 wpld, l_uint32 *datas, l_int32 d, l_int32 wpls, l_int32 thresh );
LEPT_DLL LEPTONICA_EXTERN void thresholdToBinaryLineLow ( l_uint32 *lined, l_int32 w, l_uint32 *lines, l_int32 d, l_int32 thresh );
LEPT_DLL extern JBCLASSER * jbCorrelationInitWithoutComponents ( l_int32 components, l_int32 maxwidth, l_int32 maxheight, l_float32 thresh, l_float32 weightfactor );
LEPT_DLL extern l_int32 jbAddPage ( JBCLASSER *classer, PIX *pixs );
LEPT_DLL extern void jbClasserDestroy ( JBCLASSER **pclasser );
LEPT_DLL extern l_int32 jbGetLLCorners ( JBCLASSER *classer );
LEPT_DLL extern l_int32 numaGetIValue ( NUMA *na, l_int32 index, l_int32 *pival );
LEPT_DLL LEPTONICA_EXTERN PIX * pixCreate ( l_int32 width, l_int32 height, l_int32 depth );
LEPT_DLL LEPTONICA_EXTERN PIX * pixCreateNoInit ( l_int32 width, l_int32 height, l_int32 depth );
LEPT_DLL LEPTONICA_EXTERN PIX * pixCreateTemplate ( PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN PIX * pixCreateTemplateNoInit ( PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN PIX * pixCreateHeader ( l_int32 width, l_int32 height, l_int32 depth );
LEPT_DLL extern PIX * pixClone ( PIX *pixs );
LEPT_DLL extern void pixDestroy ( PIX **ppix );
LEPT_DLL extern PIX * pixCopy ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixResizeImageData ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixCopyColormap ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSizesEqual ( PIX *pix1, PIX *pix2 );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetWidth ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetWidth ( PIX *pix, l_int32 width );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetHeight ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetHeight ( PIX *pix, l_int32 height );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetDepth ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetDepth ( PIX *pix, l_int32 depth );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetDimensions ( PIX *pix, l_int32 *pw, l_int32 *ph, l_int32 *pd );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetWpl ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetWpl ( PIX *pix, l_int32 wpl );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetRefcount ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixChangeRefcount ( PIX *pix, l_int32 delta );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetXRes ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetXRes ( PIX *pix, l_int32 res );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetYRes ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetYRes ( PIX *pix, l_int32 res );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixCopyResolution ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixScaleResolution ( PIX *pix, l_float32 xscale, l_float32 yscale );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixGetInputFormat ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetInputFormat ( PIX *pix, l_int32 informat );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixCopyInputFormat ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN char * pixGetText ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetText ( PIX *pix, const char *textstring );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixCopyText ( PIX *pixd, PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN PIXCMAP * pixGetColormap ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetColormap ( PIX *pix, PIXCMAP *colormap );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixDestroyColormap ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_uint32 * pixGetData ( PIX *pix );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetData ( PIX *pix, l_uint32 *data );
LEPT_DLL LEPTONICA_EXTERN l_int32 pixSetPixel ( PIX *pix, l_int32 x, l_int32 y, l_uint32 val );
LEPT_DLL extern l_int32 pixSetPadBits ( PIX *pix, l_int32 val );
LEPT_DLL extern PIX * pixRemoveBorder ( PIX *pixs, l_int32 npix );
LEPT_DLL LEPTONICA_EXTERN l_int32 composeRGBPixel ( l_int32 rval, l_int32 gval, l_int32 bval, l_uint32 *ppixel );
LEPT_DLL LEPTONICA_EXTERN PIX * pixInvert ( PIX *pixd, PIX *pixs );
LEPT_DLL extern l_int32 pixCountPixels ( PIX *pix, l_int32 *pcount, l_int32 *tab8 );
LEPT_DLL extern void pixaDestroy ( PIXA **ppixa );
LEPT_DLL extern PIX * pixRemoveColormap ( PIX *pixs, l_int32 type );
LEPT_DLL extern PIX * pixConvertRGBToGrayFast ( PIX *pixs );
LEPT_DLL LEPTONICA_EXTERN PIX * pixReadStreamPng ( FILE *fp );
LEPT_DLL LEPTONICA_EXTERN PIX * pixReadStreamPnm ( FILE *fp );
LEPT_DLL LEPTONICA_EXTERN l_int32 freadHeaderPnm ( FILE *fp, PIX **ppix, l_int32 *pwidth, l_int32 *pheight, l_int32 *pdepth, l_int32 *ptype, l_int32 *pbps, l_int32 *pspp );
LEPT_DLL extern PTA * ptaCreate ( l_int32 n );
LEPT_DLL extern void ptaDestroy ( PTA **ppta );
LEPT_DLL extern l_int32 ptaAddPt ( PTA *pta, l_float32 x, l_float32 y );
LEPT_DLL extern PIX * pixRead ( const char *filename );
LEPT_DLL LEPTONICA_EXTERN PIX * pixReadStream ( FILE *fp, l_int32 hint );
LEPT_DLL extern l_int32 findFileFormatStream ( FILE *fp, l_int32 *pformat );
LEPT_DLL LEPTONICA_EXTERN l_int32 findFileFormatBuffer ( const l_uint8 *buf, l_int32 *pformat );
LEPT_DLL extern l_int32 pixRasterop ( PIX *pixd, l_int32 dx, l_int32 dy, l_int32 dw, l_int32 dh, l_int32 op, PIX *pixs, l_int32 sx, l_int32 sy );
LEPT_DLL LEPTONICA_EXTERN void rasteropUniLow ( l_uint32 *datad, l_int32 dpixw, l_int32 dpixh, l_int32 depth, l_int32 dwpl, l_int32 dx, l_int32 dy, l_int32 dw, l_int32 dh, l_int32 op );
LEPT_DLL extern PIX * pixScaleGray2xLIThresh ( PIX *pixs, l_int32 thresh );
LEPT_DLL extern PIX * pixScaleGray4xLIThresh ( PIX *pixs, l_int32 thresh );
LEPT_DLL LEPTONICA_EXTERN void scaleGray2xLILineLow ( l_uint32 *lined, l_int32 wpld, l_uint32 *lines, l_int32 ws, l_int32 wpls, l_int32 lastlineflag );
LEPT_DLL LEPTONICA_EXTERN void scaleGray4xLILineLow ( l_uint32 *lined, l_int32 wpld, l_uint32 *lines, l_int32 ws, l_int32 wpls, l_int32 lastlineflag );
LEPT_DLL LEPTONICA_EXTERN l_int32 returnErrorInt ( const char *msg, const char *procname, l_int32 ival );
LEPT_DLL LEPTONICA_EXTERN void * returnErrorPtr ( const char *msg, const char *procname, void *pval );
LEPT_DLL LEPTONICA_EXTERN void l_error ( const char *msg, const char *procname );
LEPT_DLL LEPTONICA_EXTERN void l_warning ( const char *msg, const char *procname );
LEPT_DLL LEPTONICA_EXTERN void l_warningInt ( const char *msg, const char *procname, l_int32 ival );
LEPT_DLL LEPTONICA_EXTERN char * stringNew ( const char *src );
LEPT_DLL LEPTONICA_EXTERN l_int32 stringCopy ( char *dest, const char *src, l_int32 n );
LEPT_DLL LEPTONICA_EXTERN l_int32 stringReplace ( char **pdest, const char *src );
LEPT_DLL LEPTONICA_EXTERN size_t fnbytesInFile ( FILE *fp );
LEPT_DLL LEPTONICA_EXTERN l_uint16 convertOnBigEnd16 ( l_uint16 shortin );
LEPT_DLL LEPTONICA_EXTERN FILE * fopenReadStream ( const char *filename );
LEPT_DLL LEPTONICA_EXTERN char * genPathname ( const char *dir, const char *fname );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
