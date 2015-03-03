/* =============================================================================
**  This file is part of the MMG3D 5 software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Inria - IMB (Université de Bordeaux) - LJLL (UPMC), 2004- .
**
**  MMG3D 5 is free software: you can redistribute it and/or modify it
**  under the terms of the GNU Lesser General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  MMG3D 5 is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License and of the GNU General Public License along with MMG3D 5 (in
**  files COPYING.LESSER and COPYING). If not, see
**  <http://www.gnu.org/licenses/>. Please read their terms carefully and
**  use this copy of the MMG3D 5 distribution only if you accept them.
** =============================================================================
*/

/**
 * \file mmg3d/bezier.c
 * \brief Functions for Bezier surface computation.
 * \author Charles Dapogny (LJLL, UPMC)
 * \author Cécile Dobrzynski (Inria / IMB, Université de Bordeaux)
 * \author Pascal Frey (LJLL, UPMC)
 * \author Algiane Froehly (Inria / IMB, Université de Bordeaux)
 * \version 5
 * \copyright GNU Lesser General Public License.
 * \todo doxygen documentation.
 */

#include "mmg3d.h"

extern char ddb;

/**
 * \param c1 coordinates of the first point of the curve.
 * \param c2 coordinates of the second point of the curve.
 * \param n1 normal at the first point of the curve.
 * \param n2 normal at the second point of the curve.
 * \param t1 computed normal at the first point of the curve.
 * \param t2 computed normal at the second point of the curve.
 * \return 0 if failed, 1 otherwise.
 *
 * Compute tangent to geometric support curve passing through c1,c2,
 * with normals n1,n2
 *
 */
inline int BezierTgt(double c1[3],double c2[3],double n1[3],double n2[3],double t1[3],double t2[3]) {
  double ux,uy,uz,b[3],n[3],dd;

  ux = c2[0] - c1[0];
  uy = c2[1] - c1[1];
  uz = c2[2] - c1[2];

  n[0] = 0.5*(n1[0]+n2[0]);
  n[1] = 0.5*(n1[1]+n2[1]);
  n[2] = 0.5*(n1[2]+n2[2]);

  b[0] = uy*n[2] - uz*n[1];
  b[1] = uz*n[0] - ux*n[2];
  b[2] = ux*n[1] - uy*n[0];

  t1[0] = n1[1]*b[2] - n1[2]*b[1];
  t1[1] = n1[2]*b[0] - n1[0]*b[2];
  t1[2] = n1[0]*b[1] - n1[1]*b[0];

  t2[0] = -( n2[1]*b[2] - n2[2]*b[1] );
  t2[1] = -( n2[2]*b[0] - n2[0]*b[2] );
  t2[2] = -( n2[0]*b[1] - n2[1]*b[0] );

  dd = t1[0]*t1[0] + t1[1]*t1[1] + t1[2]*t1[2];
  if ( dd < EPSD )
    return(0);
  else{
    dd = 1.0 / sqrt(dd);
    t1[0] *= dd;
    t1[1] *= dd;
    t1[2] *= dd;
  }

  dd = t2[0]*t2[0] + t2[1]*t2[1] + t2[2]*t2[2];
  if ( dd < EPSD )
    return(0);
  else{
    dd = 1.0 / sqrt(dd);
    t2[0] *= dd;
    t2[1] *= dd;
    t2[2] *= dd;
  }

  return(1);
}

/**
 * \param c1 coordinates of the first point of the curve.
 * \param c2 coordinates of the second point of the curve.
 * \param t1 normal at the first point of the curve.
 * \param t2 normal at the second point of the curve.
 * \return The parameter value.
 *
 * Compute value of the parameter that makes the underlying Bezier
 * curve with 'constant speed'
 *
 */
inline double BezierGeod(double c1[3],double c2[3],double t1[3],double t2[3]) {
  double alpha,t[3],ll,ps,nt2,ux,uy,uz;

  ux = c2[0] - c1[0];
  uy = c2[1] - c1[1];
  uz = c2[2] - c1[2];

  ll = ux*ux + uy*uy + uz*uz;

  t[0] = ATHIRD*(t2[0]-t1[0]);
  t[1] = ATHIRD*(t2[1]-t1[1]);
  t[2] = ATHIRD*(t2[2]-t1[2]);

  nt2 = t[0]*t[0] + t[1]*t[1] + t[2]*t[2];
  nt2 = 16.0 - nt2;

  ps = t[0]*ux + t[1]*uy + t[2]*uz;

  alpha = ps + sqrt(ps*ps + ll*nt2);
  alpha *= (6.0 / nt2);

  return(ATHIRD*sqrt(ll));
}

/**
 * \param mesh pointer toward the mesh structure.
 * \param ip0 index of the first point of the curve.
 * \param ip1 index of the second point of the curve.
 * \param b0 the first computed extrapolated control point.
 * \param b1 the second computed extrapolated control point.
 * \param ised flag for special edge.
 * \param v direction for normal vectors.
 *
 * Compute control points associated to the underlying curve to
 * \f$[p0;p1]\f$. \a ised = 1 if \f$[p0;p1]\f must be considered as a
 * special edge. Provide a direction \a v which will be considered as
 * reference when dealing with choice of normal vectors.
 *
 */
inline int BezierEdge(pMesh mesh,int ip0,int ip1,double b0[3],double b1[3],char ised, double v[3]) {
  pPoint   p0,p1;
  pxPoint  pxp0,pxp1;
  double   ux,uy,uz,ps,ps1,ps2,*n1,*n2,np0[3],np1[3],t0[3],t1[3],il,ll,alpha;

  p0 = &mesh->point[ip0];
  p1 = &mesh->point[ip1];
  if ( !(p0->tag & MG_BDY) || !(p1->tag & MG_BDY) )  return(0);

  if ( !MG_SIN(p0->tag) ) {
    assert(p0->xp);
    pxp0 = &mesh->xpoint[p0->xp];
  }
  else
    pxp0 = 0;

  if ( !MG_SIN(p1->tag) ) {
    assert(p1->xp);
    pxp1 = &mesh->xpoint[p1->xp];
  }
  else
    pxp1 = 0;

  ux = p1->c[0] - p0->c[0];
  uy = p1->c[1] - p0->c[1];
  uz = p1->c[2] - p0->c[2];

  ll = ux*ux + uy*uy + uz*uz;
  if ( ll < EPSD2 ) {
    b0[0] = p0->c[0] + ATHIRD*ux;
    b0[1] = p0->c[1] + ATHIRD*uy;
    b0[2] = p0->c[2] + ATHIRD*uz;

    b1[0] = p1->c[0] - ATHIRD*ux;
    b1[1] = p1->c[1] - ATHIRD*uy;
    b1[2] = p1->c[2] - ATHIRD*uz;

    return(1);
  }
  il = 1.0 / sqrt(ll);

  if ( ised ) {
    if ( MG_SIN(p0->tag) ) {
      t0[0] = ux * il;
      t0[1] = uy * il;
      t0[2] = uz * il;
    }
    else {
      memcpy(t0,&(pxp0->t[0]),3*sizeof(double));
      ps = t0[0]*ux + t0[1]*uy + t0[2]*uz;
      if ( ps < 0.0 ) {
        t0[0] *= -1.0;
        t0[1] *= -1.0;
        t0[2] *= -1.0;
      }
    }
    if ( MG_SIN(p1->tag) ) {
      t1[0] = - ux * il;
      t1[1] = - uy * il;
      t1[2] = - uz * il;
    }
    else {
      memcpy(t1,&(pxp1->t[0]),3*sizeof(double));
      ps = -( t1[0]*ux + t1[1]*uy + t1[2]*uz );
      if ( ps < 0.0 ) {
        t1[0] *= -1.0;
        t1[1] *= -1.0;
        t1[2] *= -1.0;
      }
    }
  }

  else {
    if ( !MG_SIN(p0->tag) && !( p0->tag & MG_NOM ) ) {
      if ( p0->tag & MG_GEO ) {
        n1 = &(pxp0->n1[0]);
        n2 = &(pxp0->n2[0]);
        ps1 = v[0]*n1[0] + v[1]*n1[1] + v[2]*n1[2];
        ps2 = v[0]*n2[0] + v[1]*n2[1] + v[2]*n2[2];
        if ( fabs(ps2) > fabs(ps1) )
          memcpy(np0,&(pxp0->n2[0]),3*sizeof(double));
        else
          memcpy(np0,&(pxp0->n1[0]),3*sizeof(double));
      }
      else
        memcpy(np0,&(pxp0->n1[0]),3*sizeof(double));
    }

    if ( !MG_SIN(p1->tag) && !( p1->tag & MG_NOM )) {
      if ( p1->tag & MG_GEO ) {
        n1 = &(pxp1->n1[0]);
        n2 = &(pxp1->n2[0]);
        ps1 = -(v[0]*n1[0] + v[1]*n1[1] + v[2]*n1[2]);
        ps2 = -(v[0]*n2[0] + v[1]*n2[1] + v[2]*n2[2]);
        if ( fabs(ps2) > fabs(ps1) )
          memcpy(np1,&(pxp1->n2[0]),3*sizeof(double));
        else
          memcpy(np1,&(pxp1->n1[0]),3*sizeof(double));
      }
      else
        memcpy(np1,&(pxp1->n1[0]),3*sizeof(double));
    }
    if ( (MG_SIN(p0->tag) || ( p0->tag & MG_NOM )) &&
	 (MG_SIN(p1->tag) || ( p1->tag & MG_NOM )) ) {
      t0[0] = ux * il;
      t0[1] = uy * il;
      t0[2] = uz * il;

      t1[0] = -ux * il;
      t1[1] = -uy * il;
      t1[2] = -uz * il;
    }
    else if ( (!MG_SIN(p0->tag) && !( p0->tag & MG_NOM)) &&
	      ( MG_SIN(p1->tag) ||  ( p1->tag & MG_NOM ))) {
      if ( !BezierTgt(p0->c,p1->c,np0,np0,t0,t1) ) {
        t0[0] = ux * il;
        t0[1] = uy * il;
        t0[2] = uz * il;
      }
      t1[0] = -ux * il;
      t1[1] = -uy * il;
      t1[2] = -uz * il;
    }
    else if ( ( MG_SIN(p0->tag) || ( p0->tag & MG_NOM ) ) &&
	      (!MG_SIN(p1->tag) && !( p1->tag & MG_NOM ))) {
      if ( !BezierTgt(p0->c,p1->c,np1,np1,t0,t1) ) {
        t1[0] = - ux * il;
        t1[1] = - uy * il;
        t1[2] = - uz * il;
      }
      t0[0] = ux * il;
      t0[1] = uy * il;
      t0[2] = uz * il;
    }
    else {
      if ( !BezierTgt(p0->c,p1->c,np0,np1,t0,t1) ) {
        t0[0] = ux * il;
        t0[1] = uy * il;
        t0[2] = uz * il;

        t1[0] = - ux * il;
        t1[1] = - uy * il;
        t1[2] = - uz * il;
      }
    }
  }

  alpha = BezierGeod(p0->c,p1->c,t0,t1);
  b0[0] = p0->c[0] + alpha * t0[0];
  b0[1] = p0->c[1] + alpha * t0[1];
  b0[2] = p0->c[2] + alpha * t0[2];

  b1[0] = p1->c[0] + alpha * t1[0];
  b1[1] = p1->c[1] + alpha * t1[1];
  b1[2] = p1->c[2] + alpha * t1[2];

  return(1);
}

/**
 * \param mesh pointer toward the mesh structure.
 * \param pt pointer toward the triangle structure.
 * \param pb pointer toward the computed Bezier structure.
 * \param ori triangle orientation.
 * \return 1.
 *
 * Compute Bezier control points on triangle \a pt (cf. Vlachos)
 *
 */
int bezierCP(pMesh mesh,Tria *pt,pBezier pb,char ori) {
  pPoint    p[3];
  xPoint   *pxp;
  double   *n1,*n2,nt[3],t1[3],t2[3],ps,ps2,dd,ux,uy,uz,l,ll,alpha;
  int       ia,ib,ic;
  char      i,i1,i2,im,isnm;

  ia   = pt->v[0];
  ib   = pt->v[1];
  ic   = pt->v[2];
  p[0] = &mesh->point[ia];
  p[1] = &mesh->point[ib];
  p[2] = &mesh->point[ic];

  memset(pb,0,sizeof(Bezier));

  /* first 3 CP = vertices, normals */
  for (i=0; i<3; i++) {
    memcpy(&pb->b[i],p[i]->c,3*sizeof(double));
    pb->p[i] = p[i];

    if ( MG_SIN(p[i]->tag) ) {
      nortri(mesh,pt,pb->n[i]);
      if ( !ori ) {
        pb->n[i][0] *= -1.0;
        pb->n[i][1] *= -1.0;
        pb->n[i][2] *= -1.0;
      }
    }
    else if( p[i]->tag & MG_NOM){
      nortri(mesh,pt,pb->n[i]);
      if ( !ori ) {
        pb->n[i][0] *= -1.0;
        pb->n[i][1] *= -1.0;
        pb->n[i][2] *= -1.0;
      }
      assert(p[i]->xp);
      pxp = &mesh->xpoint[p[i]->xp];
      memcpy(&pb->t[i],pxp->t,3*sizeof(double));
    }
    else {
      assert(p[i]->xp);
      pxp = &mesh->xpoint[p[i]->xp];
      if ( MG_EDG(p[i]->tag) ) {
        nortri(mesh,pt,nt);
        if ( !ori ) {
          nt[0] *= -1.0;
          nt[1] *= -1.0;
          nt[2] *= -1.0;
        }
        ps  = pxp->n1[0]*nt[0] + pxp->n1[1]*nt[1] + pxp->n1[2]*nt[2];
        ps2 = pxp->n2[0]*nt[0] + pxp->n2[1]*nt[1] + pxp->n2[2]*nt[2];
        if ( fabs(ps) > fabs(ps2) )
          memcpy(&pb->n[i],pxp->n1,3*sizeof(double));
        else
          memcpy(&pb->n[i],pxp->n2,3*sizeof(double));
        memcpy(&pb->t[i],pxp->t,3*sizeof(double));
      }
      else
        memcpy(&pb->n[i],pxp->n1,3*sizeof(double));
    }
  }

  im = -1;
  isnm = 0;
  for (i=0; i<3; i++) {
    if ( p[i]->tag & MG_NOM )
      isnm++;
    else if ( im == -1 )
      im = i;
  }

  if ( isnm ) {
    if ( im != -1 ) {
      for (i=0; i<3; i++) {
        if ( p[i]->tag & MG_NOM ) {
          ps = pb->n[i][0]*pb->n[im][0] + pb->n[i][1]*pb->n[im][1] + pb->n[i][2]*pb->n[im][2];
          if ( ps < 0.0 ) {
            pb->n[i][0] *= -1.0;
            pb->n[i][1] *= -1.0;
            pb->n[i][2] *= -1.0;
          }
        }
      }
    }
    else {
      for (i=1; i<3; i++) {
        if ( p[i]->tag & MG_NOM ) {
          ps = pb->n[i][0]*pb->n[0][0] + pb->n[i][1]*pb->n[0][1] + pb->n[i][2]*pb->n[0][2];
          if ( ps < 0.0 ) {
            pb->n[i][0] *= -1.0;
            pb->n[i][1] *= -1.0;
            pb->n[i][2] *= -1.0;
          }
        }
      }
    }
  }

  /* compute control points along edges of face */
  for (i=0; i<3; i++) {
    i1 = inxt2[i];
    i2 = iprv2[i];

    ux = p[i2]->c[0] - p[i1]->c[0];
    uy = p[i2]->c[1] - p[i1]->c[1];
    uz = p[i2]->c[2] - p[i1]->c[2];

    ll = ux*ux + uy*uy + uz*uz;   // A PROTEGER !!!!
    l = sqrt(ll);

    /* choose normals */
    n1 = pb->n[i1];
    n2 = pb->n[i2];

    /* check for boundary curve */
    if ( MG_EDG(pt->tag[i]) || (pt->tag[i] & MG_NOM)) {
      if ( MG_SIN(p[i1]->tag) ) {
        t1[0] = ux / l;
        t1[1] = uy / l;
        t1[2] = uz / l;
      }
      else {
        memcpy(t1,&pb->t[i1],3*sizeof(double));
        ps = t1[0]*ux + t1[1]*uy + t1[2]*uz;
        if(ps < 0.0){
          t1[0] *= -1.0;
          t1[1] *= -1.0;
          t1[2] *= -1.0;
        }
      }
      if ( MG_SIN(p[i2]->tag) ) {
        t2[0] = - ux / l;
        t2[1] = - uy / l;
        t2[2] = - uz / l;
      }
      else {
        memcpy(t2,&pb->t[i2],3*sizeof(double));
        ps = -(t2[0]*ux + t2[1]*uy + t2[2]*uz);
        if(ps < 0.0){
          t2[0] *= -1.0;
          t2[1] *= -1.0;
          t2[2] *= -1.0;
        }
      }

      /* tangent evaluation */
      ps = ux*(pb->t[i1][0]+pb->t[i2][0]) + uy*(pb->t[i1][1]+pb->t[i2][1]) + uz*(pb->t[i1][2]+pb->t[i2][2]);
      ps = 2.0 * ps / ll;
      pb->t[i+3][0] = pb->t[i1][0] + pb->t[i2][0] - ps*ux;
      pb->t[i+3][1] = pb->t[i1][1] + pb->t[i2][1] - ps*uy;
      pb->t[i+3][2] = pb->t[i1][2] + pb->t[i2][2] - ps*uz;
      dd = pb->t[i+3][0]*pb->t[i+3][0] + pb->t[i+3][1]*pb->t[i+3][1] + pb->t[i+3][2]*pb->t[i+3][2];
      if ( dd > EPSD2 ) {
        dd = 1.0 / sqrt(dd);
        pb->t[i+3][0] *= dd;
        pb->t[i+3][1] *= dd;
        pb->t[i+3][2] *= dd;
      }
    }

    else { /* internal edge */
      if ( !BezierTgt(p[i1]->c,p[i2]->c,n1,n2,t1,t2) ) {
        t1[0] = ux / l;
        t1[1] = uy / l;
        t1[2] = uz / l;

        t2[0] = - ux / l;
        t2[1] = - uy / l;
        t2[2] = - uz / l;
      }
    }

    alpha = BezierGeod(p[i1]->c,p[i2]->c,t1,t2);

    pb->b[2*i+3][0] = p[i1]->c[0] + alpha * t1[0];
    pb->b[2*i+3][1] = p[i1]->c[1] + alpha * t1[1];
    pb->b[2*i+3][2] = p[i1]->c[2] + alpha * t1[2];

    pb->b[2*i+4][0] = p[i2]->c[0] + alpha * t2[0];
    pb->b[2*i+4][1] = p[i2]->c[1] + alpha * t2[1];
    pb->b[2*i+4][2] = p[i2]->c[2] + alpha * t2[2];

    /* normal evaluation */
    ps = ux*(n1[0]+n2[0]) + uy*(n1[1]+n2[1]) + uz*(n1[2]+n2[2]);
    ps = 2.0*ps / ll;
    pb->n[i+3][0] = n1[0] + n2[0] - ps*ux;
    pb->n[i+3][1] = n1[1] + n2[1] - ps*uy;
    pb->n[i+3][2] = n1[2] + n2[2] - ps*uz;
    dd = pb->n[i+3][0]*pb->n[i+3][0] + pb->n[i+3][1]*pb->n[i+3][1] + pb->n[i+3][2]*pb->n[i+3][2];
    if ( dd > EPSD2 ) {
      dd = 1.0 / sqrt(dd);
      pb->n[i+3][0] *= dd;
      pb->n[i+3][1] *= dd;
      pb->n[i+3][2] *= dd;
    }
  }

  /* Central Bezier coefficient */
  for (i=0; i<3; i++) {
    dd = 0.5 / 3.0;
    pb->b[9][0] -= dd * pb->b[i][0];
    pb->b[9][1] -= dd * pb->b[i][1];
    pb->b[9][2] -= dd * pb->b[i][2];
  }
  for (i=0; i<3; i++) {
    pb->b[9][0] += 0.25 * (pb->b[2*i+3][0] + pb->b[2*i+4][0]);
    pb->b[9][1] += 0.25 * (pb->b[2*i+3][1] + pb->b[2*i+4][1]);
    pb->b[9][2] += 0.25 * (pb->b[2*i+3][2] + pb->b[2*i+4][2]);
  }

  return(1);
}

/**
 * \param pb pointer toward the Bezier structure.
 * \param uv coordinates of the point in the parametric space.
 * \param o computed coordinates of the point in the real space.
 * \param no computed normal.
 * \param to computed tangent.
 * \return 1.
 *
 * Compute \a o, \a no and \a to at \f$(u,v)\f$ in Bezier patch.
 *
 */
int bezierInt(pBezier pb,double uv[2],double o[3],double no[3],double to[3]) {
  double    dd,u,v,w,ps,ux,uy,uz;
  char      i;

  memset(to,0,3*sizeof(double));
  u = uv[0];
  v = uv[1];
  w = 1 - u - v;

  for (i=0; i<3; i++) {
    o[i]  = pb->b[0][i]*w*w*w + pb->b[1][i]*u*u*u + pb->b[2][i]*v*v*v \
      + 3.0 * (pb->b[3][i]*u*u*v + pb->b[4][i]*u*v*v + pb->b[5][i]*w*v*v \
               + pb->b[6][i]*w*w*v + pb->b[7][i]*w*w*u + pb->b[8][i]*w*u*u)\
      + 6.0 * pb->b[9][i]*u*v*w;

    /* quadratic interpolation of normals */
    no[i] =        pb->n[0][i]*w*w + pb->n[1][i]*u*u + pb->n[2][i]*v*v \
      + 2.0 * (pb->n[3][i]*u*v + pb->n[4][i]*v*w + pb->n[5][i]*u*w);

    /* linear interpolation, not used here
       no[i] = pb->n[0][i]*w + pb->n[1][i]*u + pb->n[2][i]*v; */
  }

  /* tangent */
  if ( w < EPSD2 ) {
    ux = pb->b[2][0] - pb->b[1][0];
    uy = pb->b[2][1] - pb->b[1][1];
    uz = pb->b[2][2] - pb->b[1][2];
    dd = ux*ux + uy*uy + uz*uz;
    if ( dd > EPSD2 ) {
      dd = 1.0 / sqrt(dd);
      ux *= dd;
      uy *= dd;
      uz *= dd;
    }

    /* corners */
    if ( MG_SIN(pb->p[1]->tag) ) {
      pb->t[1][0] = ux;
      pb->t[1][1] = uy;
      pb->t[1][2] = uz;
    }
    if ( MG_SIN(pb->p[2]->tag) ) {
      pb->t[2][0] = ux;
      pb->t[2][1] = uy;
      pb->t[2][2] = uz;
    }

    ps = pb->t[1][0]* pb->t[2][0] + pb->t[1][1]* pb->t[2][1] + pb->t[1][2]* pb->t[2][2];
    if ( ps > 0.0 ) {
      to[0] = pb->t[1][0]*u + pb->t[2][0]*v;
      to[1] = pb->t[1][1]*u + pb->t[2][1]*v;
      to[2] = pb->t[1][2]*u + pb->t[2][2]*v;
    }
    else {
      to[0] = -pb->t[1][0]*u + pb->t[2][0]*v;
      to[1] = -pb->t[1][1]*u + pb->t[2][1]*v;
      to[2] = -pb->t[1][2]*u + pb->t[2][2]*v;
    }
  }

  if ( u < EPSD2 ) {
    ux = pb->b[2][0] - pb->b[0][0];
    uy = pb->b[2][1] - pb->b[0][1];
    uz = pb->b[2][2] - pb->b[0][2];
    dd = ux*ux + uy*uy + uz*uz;
    if ( dd > EPSD2 ) {
      dd = 1.0 / sqrt(dd);
      ux *= dd;
      uy *= dd;
      uz *= dd;
    }

    /* corners */
    if ( MG_SIN(pb->p[0]->tag) ) {
      pb->t[0][0] = ux;
      pb->t[0][1] = uy;
      pb->t[0][2] = uz;
    }
    if ( MG_SIN(pb->p[2]->tag) ) {
      pb->t[2][0] = ux;
      pb->t[2][1] = uy;
      pb->t[2][2] = uz;
    }

    ps = pb->t[0][0]* pb->t[2][0] + pb->t[0][1]* pb->t[2][1] + pb->t[0][2]* pb->t[2][2];
    if ( ps > 0.0 ) {
      to[0] = pb->t[0][0]*w + pb->t[2][0]*v;
      to[1] = pb->t[0][1]*w + pb->t[2][1]*v;
      to[2] = pb->t[0][2]*w + pb->t[2][2]*v;
    }
    else {
      to[0] = -pb->t[0][0]*w + pb->t[2][0]*v;
      to[1] = -pb->t[0][1]*w + pb->t[2][1]*v;
      to[2] = -pb->t[0][2]*w + pb->t[2][2]*v;
    }
  }

  if ( v < EPSD2 ) {
    ux = pb->b[1][0] - pb->b[0][0];
    uy = pb->b[1][1] - pb->b[0][1];
    uz = pb->b[1][2] - pb->b[0][2];
    dd = ux*ux + uy*uy + uz*uz;
    if ( dd > EPSD2 ) {
      dd = 1.0 / sqrt(dd);
      ux *= dd;
      uy *= dd;
      uz *= dd;
    }

    /* corners */
    if ( MG_SIN(pb->p[0]->tag) ) {
      pb->t[0][0] = ux;
      pb->t[0][1] = uy;
      pb->t[0][2] = uz;
    }
    if ( MG_SIN(pb->p[1]->tag) ) {
      pb->t[1][0] = ux;
      pb->t[1][1] = uy;
      pb->t[1][2] = uz;
    }

    ps = pb->t[0][0]* pb->t[1][0] + pb->t[0][1]* pb->t[1][1] + pb->t[0][2]* pb->t[1][2];
    if ( ps > 0.0 ) {
      to[0] = pb->t[0][0]*w + pb->t[1][0]*u;
      to[1] = pb->t[0][1]*w + pb->t[1][1]*u;
      to[2] = pb->t[0][2]*w + pb->t[1][2]*u;
    }
    else {
      to[0] = -pb->t[0][0]*w + pb->t[1][0]*u;
      to[1] = -pb->t[0][1]*w + pb->t[1][1]*u;
      to[2] = -pb->t[0][2]*w + pb->t[1][2]*u;
    }
  }

  dd = no[0]*no[0] + no[1]*no[1] + no[2]*no[2];
  if ( dd > EPSD2 ) {
    dd = 1.0 / sqrt(dd);
    no[0] *= dd;
    no[1] *= dd;
    no[2] *= dd;
  }

  dd = to[0]*to[0] + to[1]*to[1] + to[2]*to[2];
  if ( dd > EPSD2 ) {
    dd = 1.0 / sqrt(dd);
    to[0] *= dd;
    to[1] *= dd;
    to[2] *= dd;
  }

  return(1);
}