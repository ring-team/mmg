#ifndef _MMG3D_H
#define _MMG3D_H

/* Warning: never ever use assert() with a function,
   the option -DNDEBUG suppress all assert()*/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#if (defined(__APPLE__) && defined(__MACH__))
#include <sys/sysctl.h>
#elif defined(__unix__) || defined(__unix) || defined(unix)
#include <unistd.h>
#endif

#include "mmg3d5_redefine.h"
#include "libmmg3d5.h"
#include "libmesh5.h"

#define MG_VER   "5.2c"
#define MG_REL   "Jul. 6, 2012"
#define MG_CPY   "Copyright (c) IMB-LJLL, 2004-"
#define MG_STR   "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"

/* Macros */
#define MG_MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MG_MIN(a,b) (((a) < (b)) ? (a) : (b))

#define MG_SMSGN(a,b)  (((double)(a)*(double)(b) > (0.0)) ? (1) : (0))

/** Free allocated pointers of mesh and sol structure and return value val */
#ifdef SINGUL
#define RETURN_AND_FREE(mesh,met,sing,val)do    \
    {                                           \
      Free_all(mesh,met,sing);                  \
      return(val);                              \
    }while(0)
#else
#define RETURN_AND_FREE(mesh,met,sing,val)do    \
    {                                           \
      Free_all(mesh,met);                       \
      return(val);                              \
    }while(0)
#endif

/** Error message when lack of memory */
#define INCREASE_MEM_MESSAGE() do                         \
    {                                                     \
  printf("  ## Check the mesh size or increase maximal"); \
  printf(" authorized memory with the -m option.\n");     \
    } while(0)

/** Check if used memory overflow maximal authorized memory.
    Execute the command law if lack of memory. */
#define CHK_MEM(mesh,size,string,law) do                                \
    {                                                                   \
      if ( ((mesh)->memCur) > ((mesh)->memMax) ||                       \
           ((mesh)->memCur < 0 )) {                                     \
        fprintf(stdout,"  ## Error:");                                  \
        fprintf(stdout," unable to allocate %s.\n",string);             \
        fprintf(stdout,"  ## Check the mesh size or ");                 \
        fprintf(stdout,"increase the allocated memory with the -m option.\n"); \
        (mesh)->memCur -= (long long)(size);                            \
        law;                                                            \
      }                                                                 \
    }while(0)

/** Free pointer ptr of mesh structure and compute the new used memory.
    size is the size of the pointer */
#define DEL_MEM(mesh,ptr,size) do               \
    {                                           \
      (mesh)->memCur -= (long long)(size);      \
      free(ptr);                                \
      ptr = NULL;                               \
    }while(0)

/** Increment memory counter memCur and check if we don't overflow
    the maximum authorizied memory memMax. */
#define ADD_MEM(mesh,size,message,law) do       \
    {                                           \
      (mesh)->memCur += (long long)(size);      \
      CHK_MEM(mesh,size,message,law);           \
    }while(0)

/** Safe deallocation */
#define SAFE_FREE(ptr) do                       \
    {                                           \
      free(ptr);                                \
      ptr = NULL;                               \
    }while(0)


/** Safe allocation with calloc */
#define SAFE_CALLOC(ptr,size,type) do             \
    {                                             \
      ptr = (type *)calloc((size),sizeof(type));  \
      if ( !ptr ) {                               \
        perror("  ## Memory problem: calloc");    \
        exit(EXIT_FAILURE);                       \
      }                                           \
    }while(0)

/** Safe allocation with malloc */
#define SAFE_MALLOC(ptr,size,type) do             \
    {                                             \
      ptr = (type *)malloc((size)*sizeof(type));  \
      if ( !ptr ) {                               \
        perror("  ## Memory problem: malloc");    \
        exit(EXIT_FAILURE);                       \
      }                                           \
    }while(0)

/** Safe reallocation */
#define SAFE_REALLOC(ptr,size,type) do                  \
    {                                                   \
      type* tmp;                                        \
      tmp = (type *)realloc((ptr),(size)*sizeof(type)); \
      if ( !tmp ) {                                     \
        SAFE_FREE(ptr);                                 \
        perror(" ## Memory problem: realloc");          \
        exit(EXIT_FAILURE);                             \
      }                                                 \
      (ptr) = tmp;                                      \
    }while(0)

/** safe reallocation with memset at 0 for the new values of tab */
#define SAFE_RECALLOC(ptr,prevSize,newSize,type) do         \
    {                                                       \
      type* tmp;                                            \
      int k;                                                \
                                                            \
      tmp = (type *)realloc((ptr),(newSize)*sizeof(type));  \
      if ( !tmp ) {                                         \
        SAFE_FREE(ptr);                                     \
        perror(" ## Memory problem: realloc");              \
        exit(EXIT_FAILURE);                                 \
      }                                                     \
      (ptr) = tmp;                                          \
      for ( k=prevSize; k<newSize; k++) {                   \
        memset(&ptr[k],0,sizeof(type));                     \
      }                                                     \
    }while(0)

/** Reallocation of ptr of type type at size (initSize+wantedGap*initSize)
    if possible or at maximum available size if not. Execute the command law
    if reallocation failed. Memset to 0 for the new values of table. */
#define TAB_RECALLOC(mesh,ptr,initSize,wantedGap,type,message,law) do \
    {                                                                 \
      int gap;                                                        \
                                                                      \
      if ( (mesh->memMax-mesh->memCur) <                              \
           (long long) (wantedGap*initSize*sizeof(type)) ) {          \
        gap = (int)(mesh->memMax-mesh->memCur)/sizeof(type);          \
      }                                                               \
      else                                                            \
        gap = wantedGap*initSize;                                     \
                                                                      \
      ADD_MEM(mesh,gap*sizeof(type),message,law);                     \
      SAFE_RECALLOC((ptr),initSize+1,initSize+gap+1,type);            \
      initSize = initSize+gap;                                        \
                                                                      \
      if ( abs(mesh->info.imprim) > 4 || mesh->info.ddebug )          \
      fprintf(stdout,                                                 \
              "  ## Warning: %s:%d: %s reallocation (gap %d).\n",     \
              __FILE__,__LINE__,message,gap);                         \
    }while(0);


/** Reallocation of point table and sol table and creation
    of point ip with coordinates o and tag tag*/
#define POINT_REALLOC(mesh,sol,ip,wantedGap,law,o,tag ) do              \
    {                                                                   \
      int klink;                                                        \
                                                                        \
      TAB_RECALLOC(mesh,mesh->point,mesh->npmax,wantedGap,Point,        \
                   "larger point table",law);                           \
                                                                        \
      mesh->npnil = mesh->np+1;                                         \
      for (klink=mesh->npnil; klink<mesh->npmax-1; klink++)             \
        mesh->point[klink].tmp  = klink+1;                              \
                                                                        \
      /* solution */                                                    \
      if ( sol->m ) {                                                   \
        ADD_MEM(mesh,(mesh->npmax-sol->npmax)*sizeof(double),           \
                "larger solution",law);                                 \
        SAFE_REALLOC(sol->m,mesh->npmax+1,double);                      \
      }                                                                 \
      sol->npmax = mesh->npmax;                                         \
                                                                        \
      /* We try again to add the point */                               \
      ip = newPt(mesh,o,tag);                                           \
      if ( !ip ) {law;}                                                 \
    }while(0)

#ifndef PATTERN
/** Reallocation of point table, sol table and bucket table and creation
    of point ip with coordinates o and tag tag*/
#define POINT_AND_BUCKET_REALLOC(mesh,sol,ip,wantedGap,law,o,tag ) do   \
    {                                                                   \
      int klink,gap;                                                    \
                                                                        \
      if ( (mesh->memMax-mesh->memCur) <                                \
           (long long) (wantedGap*mesh->npmax*                          \
                        (sizeof(Point)+sizeof(int))) ) {                \
        gap = (int)(mesh->memMax-mesh->memCur)/                         \
          (sizeof(Point)+sizeof(int));                                  \
      }                                                                 \
      else                                                              \
        gap = wantedGap*mesh->npmax;                                    \
                                                                        \
      ADD_MEM(mesh,gap*(sizeof(Point)+sizeof(int)),\
              "point and bucket",law);                                  \
      SAFE_RECALLOC(mesh->point,mesh->npmax+1,mesh->npmax+gap+1,Point); \
      SAFE_RECALLOC(bucket->link,mesh->npmax+1,mesh->npmax+gap+1,int);  \
      mesh->npmax = mesh->npmax+gap;                                    \
                                                                        \
      if ( abs(mesh->info.imprim) > 4 || mesh->info.ddebug )            \
        fprintf(stdout,                                                 \
                "  ## Warning: %s:%d: point and bucket reallocation (gap %d).\n", \
                __FILE__,__LINE__,gap);                                 \
                                                                        \
      mesh->npnil = mesh->np+1;                                         \
      for (klink=mesh->npnil; klink<mesh->npmax-1; klink++)             \
        mesh->point[klink].tmp  = klink+1;                              \
                                                                        \
      /* solution */                                                    \
      if ( sol->m ) {                                                   \
        ADD_MEM(mesh,(mesh->npmax-sol->npmax)*sizeof(double),           \
                "larger solution",law);                                 \
        SAFE_REALLOC(sol->m,mesh->npmax+1,double);                      \
      }                                                                 \
      sol->npmax = mesh->npmax;                                         \
                                                                        \
      /* We try again to add the point */                               \
      ip = newPt(mesh,o,tag);                                           \
      if ( !ip ) {law;}                                                 \
    }while(0)
#endif

/** Reallocation of tetra table and creation
    of tetra jel */
#define TETRA_REALLOC(mesh,jel,wantedGap,law ) do                       \
    {                                                                   \
      int klink,oldSiz;                                                 \
                                                                        \
      oldSiz = mesh->nemax;                                             \
      TAB_RECALLOC(mesh,mesh->tetra,mesh->nemax,wantedGap,Tetra,        \
                   "larger tetra table",law);                           \
                                                                        \
      mesh->nenil = mesh->ne+1;                                         \
      for (klink=mesh->nenil; klink<mesh->nemax-1; klink++)             \
        mesh->tetra[klink].v[3]  = klink+1;                             \
                                                                        \
      if ( mesh->adja ) {                                               \
        /* adja table */                                                \
        ADD_MEM(mesh,4*(mesh->nemax-oldSiz)*sizeof(int),                \
                "larger adja table",law);                               \
        SAFE_RECALLOC(mesh->adja,4*mesh->ne+5,4*mesh->nemax+5,int);     \
      }                                                                 \
                                                                        \
      /* We try again to add the point */                               \
      jel = newElt(mesh);                                               \
      if ( !jel ) {law;}                                                \
    }while(0)

          /* numerical accuracy */
#define ALPHAD    20.7846096908265    //0.04811252243247      /* 12*sqrt(3) */
#define LLONG     2.5//2.0   // 1.414213562373
#define LSHRT     0.3  // 0.707106781186
#define LOPTL     1.3
#define LOPTS     0.6
#define ANGEDG    0.707106781186548   /*0.573576436351046 */
#define ANGLIM   -0.999999
#define SQR32     0.866025403784
#define ATHIRD    0.333333333333
#define EPSD      1.e-30
#define EPSD2     1.0e-200
#define EPS       1.e-06
#define LMAX      10240
#define BADKAL    0.2
#define NULKAL    1.e-30
#ifdef SINGUL
#define EPS2      1.e-12
#endif

#define NPMAX  1000000 //200000
#define NAMAX   200000 //40000
#define NTMAX  2000000 //400000
#define NEMAX  6000000 //1200000

#define MEMMAX  800 //160
#define BOXSIZE 500

#ifndef M_PI
#define M_PI            3.14159265358979323846   /**< pi   */
#define M_PI_2          1.57079632679489661923   /**< pi/2 */
#endif

          /* tags */
#define  MG_NOTAG     (0)
#define  MG_REF       (1 << 0)        /**< 1  edge reference  */
#define  MG_GEO       (1 << 1)        /**< 2  geometric ridge */
#define  MG_REQ       (1 << 2)        /**< 4  required entity */
#define  MG_NOM       (1 << 3)        /**< 8  non manifold    */
#define  MG_BDY       (1 << 4)        /**< 16  boundary entity */
#define  MG_CRN       (1 << 5)        /**< 32  corner         */
#define  MG_NUL       (1 << 6)        /**< 64  vertex removed */
#ifdef SINGUL
#define  MG_SGL       (1 << 7)        /**< 128 inserted singularity */
#endif

#define MG_PLUS    2
#define MG_MINUS   3
#define MG_ISO    10

#define MG_VOK(ppt)      (ppt && ((ppt)->tag < MG_NUL)) /**< Vertex OK */
#define MG_EOK(pt)       (pt && ((pt)->v[0] > 0))       /**< Element OK */
#define MG_EDG(tag)      ((tag & MG_GEO) || (tag & MG_REF)) /**< Edge or Ridge */
#define MG_SIN(tag)      ((tag & MG_CRN) || (tag & MG_REQ)) /**< Corner or Required */

#define MG_SET(flag,bit) ((flag) |= (1 << (bit)))  /**< bit number bit is set to 1 */
#define MG_CLR(flag,bit) ((flag) &= ~(1 << (bit))) /**< bit number bit is set to 0 */
#define MG_GET(flag,bit) ((flag) & (1 << (bit)))   /**< return bit number bit value */

extern unsigned char inxt2[3];   /**< next vertex of triangle: {1,2,0} */
extern unsigned char iprv2[3];   /**< previous vertex of triangle: {2,0,1} */
extern unsigned char inxt3[7];   /**< next vertex of tetra: {1,2,3,0,1,2,3} */
extern unsigned char iprv3[7];   /**< previous vertex of tetra: {3,0,1,2,3,0,1} */
extern unsigned char idir[4][3]; /**< idir[i] : vertices of face opposite to vertex i */
extern          char idirinv[4][4]; /**< idirinv[i][j] : num of the jth point in the ith face */
extern unsigned char iarf[4][3]; /**< iarf[i]: edges of face opposite to vertex i */
extern unsigned char iarfinv[4][6]; /**< num of the j^th edge in the i^th face */
extern unsigned char iare[6][2]; /**< vertices of extremities of the edges of the tetra */
extern unsigned char ifar[6][2]; /**< ifar[i][]: faces sharing the ith edge of the tetra */
extern unsigned char isar[6][2]; /**< isar[i][]: vertices of extremities of the edge opposite to the ith edge */
extern unsigned char arpt[4][3]; /**< arpt[i]: edges passing through vertex i */


typedef struct {
  double  b[10][3]; /**< Bezier basis functions */
  double  n[6][3],t[6][3]; /**< normals and tangents at points */
  pPoint  p[3];
} Bezier;
typedef Bezier * pBezier;

/** used to hash edges */
typedef struct {
  int   a,b,nxt;
  int   s,k; /** k = point along edge a b */
} hedge;

typedef struct {
  int     siz,max,nxt;
  hedge  *item;
} Hash;

typedef struct {
  int     size;
  int    *head;
  int    *link;
} Bucket;
typedef Bucket * pBucket;

#ifdef SINGUL
typedef struct {
  double   c[3],cb[4]; /**< c: coor of entrance/exit point, cb: bary coor of entrance/exit */
  int      kel,key; /**< kel: elt in which we travel, key: location of entrance or exit */
  int      np; /**< global indice of entrance point */
  char     tag; /**< tag of edge */

} Travel;
#endif

/* bucket */
pBucket newBucket(pMesh ,int );
int     addBucket(pMesh ,pBucket ,int );
int     delBucket(pMesh ,pBucket ,int );
int buckin_iso(pMesh mesh,pSol sol,pBucket bucket,int ip);
/* prototypes */
void rotmatrix(double n[3],double r[3][3]);
double det3pt1vec(double c0[3],double c1[3],double c2[3],double v[3]);
double det4pt(double c0[3],double c1[3],double c2[3],double c3[3]);
double orvol(pPoint point,int *v);
int directsurfball(pMesh mesh, int ip, int *list, int ilist, double n[3]);

int  newPt(pMesh mesh,double c[3],char tag);
int  newElt(pMesh mesh);
void delElt(pMesh mesh,int iel);
void delPt(pMesh mesh,int ip);
int  zaldy(pMesh mesh);
void freeXTets(pMesh mesh);
char chkedg(pMesh mesh,pTria pt,char ori);
int  chkNumberOfTri(pMesh mesh);
void tet2tri(pMesh mesh,int k,char ie,Tria *ptt);
int  bezierCP(pMesh mesh,Tria *pt,pBezier pb,char ori);
int  BezierTgt(double c1[3],double c2[3],double n1[3],double n2[3],double t1[3],double t2[3]);
double BezierGeod(double c1[3], double c2[3], double t1[3], double t2[3]);
int  bezierInt(pBezier pb,double uv[2],double o[3],double no[3],double to[3]);
int  BezierReg(pMesh mesh,int ip0, int ip1, double s, double v[3], double *o, double *no);
int  BezierRef(pMesh mesh,int ip0, int ip1, double s, double *o, double *no, double *to);
int  BezierEdge(pMesh mesh,int ip0, int ip1, double b0[3], double b1[3],char isrid, double v[3]);
int  BezierRidge(pMesh mesh,int ip0, int ip1, double s, double *o, double *no1, double *no2, double *to);
int  BezierNom(pMesh mesh,int ip0,int ip1,double s,double *o,double *no,double *to);
int  norface(pMesh mesh ,int k, int iface, double v[3]);
int  boulen(pMesh mesh,int start,int ip,double *nn);
int  bouler(pMesh mesh,int start,int ip,int *list,int *ng,int *nr);
int  boulenm(pMesh mesh, int start, int ip, int iface, double n[3],double t[3]);
int  boulec(pMesh mesh,int start,int ip,double *tt);
int  boulevolp(pMesh mesh, int start, int ip, int * list);
int  boulesurfvolp(pMesh mesh,int start,int ip,int iface,int *listv,int *ilistv,int *lists,int*ilists);
int  startedgsurfball(pMesh mesh,int nump,int numq,int *list,int ilist);
int  srcbdy(pMesh mesh,int start,int ia);
int  coquil(pMesh mesh, int start, int ia, int * list);
int  coquilface(pMesh mesh, int start, int ia, int * list, int * it1, int *it2);
int  gettag(pMesh,int,int,int *,int *);
int  settag(pMesh,int,int,int,int);
int  chkcol_int(pMesh ,pSol met,int,char,char,int *,char typchk);
int  chkcol_bdy(pMesh,int,char,char,int *);
int  chkmanicoll(pMesh mesh,int k,int iface,int iedg,int ndepmin,int ndepplus,char isminp,char isplp);
int  chkmani(pMesh mesh);
int  colver(pMesh,int *,int,char);
int  analys(pMesh mesh);
int  hashTetra(pMesh mesh, int pack);
int  hashTria(pMesh mesh);
int  hashEdge(pMesh mesh,Hash *hash,int a,int b,int k);
int  hashGet(Hash *hash,int a,int b);
int  hashPop(Hash *hash,int a,int b);
int  hashNew(pMesh mesh, Hash *hash,int hsiz,int hmax);
int  hPop(HGeom *hash,int a,int b,int *ref,char *tag);
int  hTag(HGeom *hash,int a,int b,int ref,char tag);
int  hGet(HGeom *hash,int a,int b,int *ref,char *tag);
void hEdge(pMesh mesh,int a,int b,int ref,char tag);
int  hNew(HGeom *hash,int hsiz,int hmax,int secure);
int  hGeom(pMesh mesh);
int  bdryTria(pMesh );
int  bdryIso(pMesh );
int  bdrySet(pMesh );
int  bdryUpdate(pMesh );
int  bdryPerm(pMesh );
int  chkmsh(pMesh,int,int);
int  chkfemtopo(pMesh mesh);
int  cntbdypt(pMesh mesh, int nump);
long long memSize(void);
void memOption(pMesh mesh);
int  mmg3d1(pMesh ,pSol );
int  mmg3d1_delone(pMesh ,pSol );
int  mmg3d2(pMesh ,pSol );
int  split1_sim(pMesh mesh,pSol met,int k,int vx[6]);
void split1(pMesh mesh,pSol met,int k,int vx[6]);
int  split1b(pMesh mesh,pSol met,int *list,int ret,int ip,int cas);
int  split2sf_sim(pMesh mesh,pSol met,int k,int vx[6]);
void split2sf(pMesh mesh,pSol met,int k,int vx[6]);
void split2(pMesh mesh,pSol met,int k,int vx[6]);
int  split3_sim(pMesh mesh,pSol met,int k,int vx[6]);
void split3(pMesh mesh,pSol met,int k,int vx[6]);
void split3cone(pMesh mesh,pSol met,int k,int vx[6]);
void split3op(pMesh mesh, pSol met, int k, int vx[6]);
void split4sf(pMesh mesh,pSol met,int k,int vx[6]);
void split4op(pMesh mesh,pSol met,int k,int vx[6]);
void split5(pMesh mesh,pSol met,int k,int vx[6]);
void split6(pMesh mesh,pSol met,int k,int vx[6]);
int  split4bar(pMesh mesh,pSol met,int k);
int  simbulgept(pMesh mesh, int *list, int ilist, double o[3]);
int  dichoto1b(pMesh mesh,int *list,int ret,double o[3],double ro[3]);
void nsort(int ,double *,char *);
int  nortri(pMesh mesh,pTria pt,double *n);
double orcal(pMesh mesh,int iel);
int  movintpt(pMesh mesh, int *list, int ilist, int improve);
int  movbdyregpt(pMesh mesh, int *listv, int ilistv, int *lists, int ilists);
int  movbdyrefpt(pMesh mesh, int *listv, int ilistv, int *lists, int ilists);
int  movbdynompt(pMesh mesh, int *listv, int ilistv, int *lists, int ilists);
int  movbdyridpt(pMesh mesh, int *listv, int ilistv, int *lists, int ilists);
double caltri(pMesh mesh,pTria ptt);
int  scaleMesh(pMesh mesh,pSol met,pSingul sin);
int  unscaleMesh(pMesh mesh,pSol met);
int  chkswpbdy(pMesh mesh,int *list,int ilist,int it1,int it2);
#ifdef PATTERN
int  swpbdy(pMesh mesh,pSol met,int *list,int ret,int it1);
int  swpgen(pMesh mesh,pSol met,int nconf, int ilist, int *list);
#else
int  swpbdy(pMesh mesh,pSol met,int *list,int ret,int it1,pBucket bucket);
int  swpgen(pMesh mesh,pSol met,int nconf, int ilist, int *list,pBucket bucket);
#endif
int  chkswpgen(pMesh mesh, int start, int ia, int *ilist, int *list,double crit);
int  srcface(pMesh mesh,int n0,int n1,int n2);
int  bouleext(pMesh mesh, int start, int ip, int iface, int *listv, int *ilistv, int *lists, int*ilists);
int chkptonbdy(pMesh,int);
int norpts(pMesh,int,int,int,double *);
double orcal_poi(double a[3],double b[3],double c[3],double d[3]);
int trydisp(pMesh,double *,short);
int dichodisp(pMesh,double *);
int lapantilap(pMesh,double *);
int ppgdisp(pMesh,double *);
int denoisbdy(pMesh);
int eigensym(double m[3],double lambda[2],double vp[2][2]);
int sys33sym(double a[6], double b[3], double r[3]);
void outqua(pMesh mesh,pSol met);
int  badelt(pMesh mesh,pSol met);
int prilen(pMesh mesh,pSol met);
int DoSol(pMesh mesh,pSol met);
/* useful functions to debug */
int  indElt(pMesh mesh,int kel);
int  indPt(pMesh mesh,int kp);
void printTria(pMesh mesh,char* fileName);
void printTetra(pMesh mesh,char* fileName);
int  saveMesh(pMesh mesh);
int  saveLibraryMesh(pMesh mesh);

#ifdef USE_SCOTCH
int renumbering(int vertBoxNbr, pMesh mesh, pSol sol);
#endif

#ifdef SINGUL
int  inserSingul(pMesh mesh, pSol met, pSingul singul);
int  creaEdge(pMesh mesh, pSol met, Travel *trav, char tag);
int  creaPoint(pMesh mesh, pSol met,int iel, double c[3], double cb[4], char tag);
int  colSing(pMesh mesh,pSol met);
int  seekEdge(pMesh mesh, pSol met, psPoint ppt0, psPoint ppt1,
              Travel *trav, int *lastet,int*,int*);
int  seekPoint(pMesh mesh, psPoint ppt, double cb[4],int*,int*);
int  solveUnsignedTet(pMesh mesh,pSol met);
int  split3cb(pMesh mesh, pSol met, int k, int ifac, double o[3],double cb[4], int *ip);
int  split4cb(pMesh mesh, pSol met, int k, double o[3], double cb[4],int *ip);
int  swap23(pMesh mesh,int k, int ip);
#endif

int meancur(pMesh mesh,int np,double c[3],int ilist,int *list,double h[3]);
double surftri(pMesh,int,int);
double timestepMCF(pMesh,double);
int bdyMCF(pMesh);
double volint(pMesh);

/* Delaunay functions*/
int delone(pMesh mesh,pSol sol,int ip,int *list,int ilist);
int cavity(pMesh mesh,pSol sol,int iel,int ip,int *list,int lon);
int cenrad_iso(pMesh mesh,double *ct,double *c,double *rad);

/*mmg3d1.c*/
void tet2tri(pMesh mesh,int k,char ie,Tria *ptt);
int dichoto(pMesh mesh,pSol met,int k,int *vx);
/** Find acceptable position for split1b, passing the shell of considered edge, starting from o */
int dichoto1b(pMesh mesh,int *list,int ret,double o[3],double ro[3]);
/** return edges of (virtual) triangle pt that need to be split w/r Hausdorff criterion */
char chkedg(pMesh mesh,Tria *pt,char ori);
int swpmsh(pMesh mesh,pSol met
#ifndef PATTERN
	   ,pBucket bucket
#endif
);
int anatet(pMesh mesh,pSol met,char typchk) ;
int anatet4(pMesh mesh, pSol met);
int anatets(pMesh mesh,pSol met,char typchk);
int coltet(pMesh mesh,pSol met,char typchk) ;

/* pointers */
double caltet_ani(pMesh mesh,pSol met,int ia,int ib,int ic,int id);
double caltet_iso(pMesh mesh,pSol met,int ia,int ib,int ic,int id);
double lenedg_ani(pMesh ,pSol ,int ,int );
double lenedg_iso(pMesh ,pSol ,int ,int );
double lenedgCoor_ani(double*, double*, double*, double*);
double lenedgCoor_iso(double*, double*, double*, double*);
int    defsiz_iso(pMesh,pSol );
int    defsiz_ani(pMesh ,pSol );
int    gradsiz_iso(pMesh ,pSol );
int    gradsiz_ani(pMesh ,pSol );

double (*caltet)(pMesh mesh,pSol met,int ia,int ib,int ic,int id);
int    (*defsiz)(pMesh ,pSol );
double (*lenedg)(MMG5_pMesh mesh,MMG5_pSol sol ,int ,int );
int    (*gradsiz)(pMesh ,pSol );

#endif
