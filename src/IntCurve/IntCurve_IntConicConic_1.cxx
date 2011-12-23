// File:	IntCurve_IntConicConic_1.cxx
// Created:	Wed May  6 16:00:06 1992
// Author:	Laurent BUCHARD
//		<lbr@topsn3>
// a modifier le cas de 2 points confondus ( Insert a la place d'append ? ) 

#include <IntCurve_IntConicConic.jxx>

#include <IntCurve_IConicTool.hxx>
#include <IntCurve_PConic.hxx>
#include <IntRes2d_Domain.hxx>
#include <gp.hxx>
#include <IntCurve_IntConicConic_Tool.hxx>
#include <IntImpParGen.hxx>
#include <IntCurve_IntConicConic_1.hxx>
#include <ElCLib.hxx>
#include <Standard_ConstructionError.hxx>
#include <IntRes2d_IntersectionPoint.hxx>
#include <IntRes2d_IntersectionSegment.hxx>

#include <gp_Pnt2d.hxx>
#include <gp_Vec2d.hxx>
#include <Precision.hxx>
#include <IntRes2d_TypeTrans.hxx>

Standard_Boolean Affichage=Standard_False;
Standard_Boolean AffichageGraph=Standard_True;

//modified by NIZHNY-MKK  Tue Feb 15 10:53:34 2000.BEGIN
// #define TOLERANCE_ANGULAIRE 0.00000001
#define TOLERANCE_ANGULAIRE 1.e-15 //the reason is at least to make an accordance between transition and position computation.
//modified by NIZHNY-MKK  Tue Feb 15 10:53:45 2000.END

const Standard_Real PIsur2 = 0.5*M_PI;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
IntRes2d_Position FindPositionLL(Standard_Real&,const IntRes2d_Domain&);
const IntRes2d_IntersectionPoint SegmentToPoint( const IntRes2d_IntersectionPoint& Pa
						,const IntRes2d_Transition& T1a
						,const IntRes2d_Transition& T2a
						,const IntRes2d_IntersectionPoint& Pb
						,const IntRes2d_Transition& T1b
						,const IntRes2d_Transition& T2b);  
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProjectOnC2AndIntersectWithC2Domain(const gp_Circ2d& Circle1
                                         ,const gp_Circ2d& Circle2
					 ,PeriodicInterval& C1DomainAndRes
					 ,PeriodicInterval& DomainC2
					 ,PeriodicInterval* SolutionC1
					 ,PeriodicInterval* SolutionC2
					 ,Standard_Integer &NbSolTotal
					 ,const Standard_Boolean IdentCircles)
{
  
  if(C1DomainAndRes.IsNull()) return;
  //-------------------------------------------------------------------------
  //--  On cherche l intervalle correspondant sur C2
  //--  Puis on intersecte l intervalle avec le domaine de C2
  //--  Enfin, on cherche l intervalle correspondant sur C1
  //--
  Standard_Real C2inf = 
    ElCLib::CircleParameter(Circle2.Axis()
			    ,ElCLib::CircleValue(C1DomainAndRes.Binf
						 ,Circle1.Axis(),Circle1.Radius()));
  Standard_Real C2sup = 
    ElCLib::CircleParameter(Circle2.Axis()
			    ,ElCLib::CircleValue(C1DomainAndRes.Bsup
						 ,Circle1.Axis(),Circle1.Radius()));

  PeriodicInterval C2Inter(C2inf,C2sup);

  if(!IdentCircles) {
    if(C2Inter.Length() > M_PI)
      C2Inter.Complement();
  }
  else {
    if(C2sup<=C2inf) C2sup+=PIpPI;
    if(C2inf>=PIpPI) {
      C2sup-=PIpPI;
      C2inf-=PIpPI;
    }
    C2Inter.Binf=C2inf;
    C2Inter.Bsup=C2sup; //--- Verifier la longueur de l'intervalle sur C2
    C2Inter.Bsup=C2inf+C1DomainAndRes.Bsup-C1DomainAndRes.Binf;
  }
  
  PeriodicInterval C2InterAndDomain[2];
  
  for(Standard_Integer i=0; i<2 ; i++) {
    C2InterAndDomain[i]=(i==0)?  DomainC2.FirstIntersection(C2Inter)
                               : DomainC2.SecondIntersection(C2Inter);
  
    if(!C2InterAndDomain[i].IsNull()) {

      Standard_Real C1inf = 
	ElCLib::CircleParameter(Circle1.Axis()
				,ElCLib::CircleValue(C2InterAndDomain[i].Binf
					,Circle2.Axis(),Circle2.Radius()));
      Standard_Real C1sup = 
	ElCLib::CircleParameter(Circle1.Axis()
				,ElCLib::CircleValue(C2InterAndDomain[i].Bsup
					,Circle2.Axis(),Circle2.Radius()));

      SolutionC1[NbSolTotal]=PeriodicInterval(C1inf,C1sup);
      if(!IdentCircles) {
	if(SolutionC1[NbSolTotal].Length() > M_PI)
	  SolutionC1[NbSolTotal].Complement();
      }
      else {
	if(SolutionC1[NbSolTotal].Bsup <= SolutionC1[NbSolTotal].Binf) {
	  SolutionC1[NbSolTotal].Bsup+=PIpPI;
	}
	if(SolutionC1[NbSolTotal].Binf>=PIpPI) {
	  SolutionC1[NbSolTotal].Binf-=PIpPI;
	  SolutionC1[NbSolTotal].Bsup-=PIpPI;	  
	}
      }
      SolutionC2[NbSolTotal]=C2InterAndDomain[i];
      NbSolTotal++;
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void CircleCircleGeometricIntersection(const gp_Circ2d& C1
				       ,const gp_Circ2d& C2
				       ,const Standard_Real Tol
				       ,const Standard_Real TolTang
				       ,PeriodicInterval& C1_Res1
				       ,PeriodicInterval& C1_Res2
				       ,Standard_Integer& nbsol) {
  
  Standard_Real C1_binf1,C1_binf2=0,C1_bsup1,C1_bsup2=0;
  Standard_Real dO1O2=(C1.Location()).Distance(C2.Location());
  Standard_Real R1=C1.Radius();
  Standard_Real R2=C2.Radius();
  Standard_Real AbsR1mR2=Abs(R1-R2);
  //---------------------------------------------------------------- 
  if(dO1O2 > (R1+R2+Tol)) {
    if(dO1O2 > (R1+R2+TolTang)) { 
      nbsol=0; 
      return; 
    }
    else { 
      C1_binf1 = 0.0;
      C1_bsup1 = 0.0;
      nbsol = 1;
    }
  }
  //---------------------------------------------------------------- 
  else if(dO1O2 <= Tol  &&  AbsR1mR2<=Tol)  { 
    nbsol=3; 
    return; 
  }
  else { 
    //---------------------------------------------------------------- 
    Standard_Real R1pR2=R1+R2;
    Standard_Real R1pTol=R1+Tol;
    Standard_Real R1mTol=R1-Tol;
//    Standard_Real R1R1=R1*R1;
    Standard_Real R2R2=R2*R2;
    Standard_Real R1pTolR1pTol=R1pTol*R1pTol;
    Standard_Real R1mTolR1mTol=R1mTol*R1mTol;
    Standard_Real dO1O2dO1O2=dO1O2*dO1O2;
    Standard_Real dAlpha1;
    //--------------------------------------------------------------- Cas 
    //-- C2 coupe le cercle C1+ (=C(x1,y1,R1+Tol))
    //--            1 seul segment donne par Inter C2 C1+
    //--
    if(dO1O2 > R1pR2-Tol) { 
      Standard_Real dx=(R1pTolR1pTol+dO1O2dO1O2-R2R2)/(dO1O2+dO1O2);
      Standard_Real dy=(R1pTolR1pTol-dx*dx);
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      dAlpha1=ATan2(dy,dx);
      
      C1_binf1=-dAlpha1;  
      C1_bsup1=dAlpha1;
      nbsol=1;
    }
    //--------------------------------------------------------------------
    //--           2 segments donnes par Inter C2 avec C1- C1 C1+
    //-- Seul le signe de dx change si dO1O2 < Max(R1,R2)
    //-- 
    else if(dO1O2 > AbsR1mR2-Tol) {  // -- + 
      //------------------- Intersection C2 C1+ --------------------------
      Standard_Real dx=(R1pTolR1pTol+dO1O2dO1O2-R2R2)/(dO1O2+dO1O2);
      Standard_Real dy=(R1pTolR1pTol-dx*dx); 
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      
      dAlpha1=ATan2(dy,dx);
      C1_binf1=-dAlpha1;  C1_bsup2=dAlpha1;  //--  |...?     ?...|   Sur C1
      
      //------------------ Intersection C2 C1- -------------------------
      dx=(R1mTolR1mTol+dO1O2dO1O2-R2R2)/(dO1O2+dO1O2);
      dy=(R1mTolR1mTol-dx*dx);
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      dAlpha1=ATan2(dy,dx);
      
      C1_binf2=dAlpha1;  C1_bsup1=-dAlpha1;  //--  |...x     x...|   Sur C1
      nbsol=2;    
      //------------------------------
      //-- Les 2 intervalles sont ils 
      //-- en fait un seul inter ? 
      //-- 
      if(dy==0) {    //-- Les 2 bornes internes sont identiques 
	C1_bsup1 = C1_bsup2; 
	nbsol = 1;
      }
      else { 
	if(C1_binf1>C1_bsup1) { 
	  dAlpha1 = C1_binf1; C1_binf1 = C1_bsup1; C1_bsup1 = dAlpha1; 
	}
	if(C1_binf2>C1_bsup2) { 
	  dAlpha1 = C1_binf2; C1_binf2 = C1_bsup2; C1_bsup2 = dAlpha1; 
	}
	if(   ((C1_binf1<=C1_bsup2) && (C1_binf1>=C1_binf2))
	   || ((C1_bsup1<=C1_bsup2) && (C1_bsup1>=C1_binf2))) { 
	  if(C1_binf1 > C1_binf2) C1_binf1 = C1_binf2;
	  if(C1_binf1 > C1_bsup2) C1_binf1 = C1_bsup2;
	  if(C1_bsup1 < C1_binf2) C1_bsup1 = C1_binf2;
	  if(C1_bsup1 < C1_bsup2) C1_bsup1 = C1_bsup2;
	  nbsol=1;
	}
      }
    }
    //--------------------------------------------------------------
    //--    1 seul segment donne par Inter C2 avec C1- ou C1+
    else if(dO1O2 > AbsR1mR2-Tol) {
      
      Standard_Real dx=(R1mTolR1mTol+dO1O2dO1O2-R2R2)/(dO1O2+dO1O2);
      Standard_Real dy=(R1mTolR1mTol-dx*dx);
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      dAlpha1=ATan2(dy,dx);
      
      dx=(R1pTolR1pTol+dO1O2dO1O2-R2R2)/(dO1O2+dO1O2);
      dy=(R1pTolR1pTol-dx*dx);
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      Standard_Real dAlpha2=ATan2(dy,dx);
      
      if(dAlpha2>dAlpha1) dAlpha1 = dAlpha2;
      C1_binf1=-dAlpha1;  C1_bsup1=dAlpha1;
      nbsol=1;
    }
    //--------------------------------------------------------------
    else {
      if((dO1O2 > AbsR1mR2-TolTang) && (AbsR1mR2-TolTang)>0.0) { 
	C1_binf1=0.0;  
	C1_bsup1=0.0;
	nbsol = 1;
      }
      else { 
	nbsol=0; return ;
      }
    }
  }

  //-- cout<<" C1_binf1:"<<C1_binf1;
  //-- cout<<" C1_bsup1:"<<C1_bsup1;
  //-- cout<<" C1_binf2:"<<C1_binf2;
  //-- cout<<" C1_bsup2:"<<C1_bsup2<<endl;
  //----------------------------------------------------------------
  //-- Mise en forme des resultats : 
  //--    Les calculs ont ete fait dans le repere x1,y1, (O1,O2)
  //--    On se ramene au repere propre a C1

  gp_Vec2d Axe1=C1.XAxis().Direction();
  gp_Vec2d AxeO1O2=gp_Vec2d(C1.Location(),C2.Location());
  
  Standard_Real dAngle1;
  if(AxeO1O2.Magnitude() <= gp::Resolution()) 
    dAngle1=Axe1.Angle(C2.XAxis().Direction());
  else
    dAngle1=Axe1.Angle(AxeO1O2);

  if(C1.IsDirect() == Standard_False) { 
    dAngle1 = -dAngle1; 
  }


  C1_binf1+=dAngle1;  C1_bsup1+=dAngle1;
  
  //-- par construction aucun des segments ne peut exceder PI
  //-- (permet de ne pas gerer trop de cas differents)

  C1_Res1.SetValues(C1_binf1,C1_bsup1);
  if(C1_Res1.Length() > M_PI) C1_Res1.Complement();

  if(nbsol==2) {
    C1_binf2+=dAngle1;  C1_bsup2+=dAngle1;
    C1_Res2.SetValues(C1_binf2,C1_bsup2);
    if(C1_Res2.Length() > M_PI) C1_Res2.Complement();
  }
  else {
    C1_Res2.SetNull(); 
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ProjectOnLAndIntersectWithLDomain(const gp_Circ2d& Circle
				       ,const gp_Lin2d& Line
				       ,PeriodicInterval& CDomainAndRes
				       ,Interval& LDomain
				       ,PeriodicInterval* CircleSolution
				       ,Interval* LineSolution
				       ,Standard_Integer &NbSolTotal
				       ,const IntRes2d_Domain& RefLineDomain
//				       ,const IntRes2d_Domain& )
				       ,const IntRes2d_Domain& )
{
  
  
  if(CDomainAndRes.IsNull()) return;
  //-------------------------------------------------------------------------
  //--  On cherche l intervalle correspondant sur C2
  //--  Puis on intersecte l intervalle avec le domaine de C2
  //--  Enfin, on cherche l intervalle correspondant sur C1
  //--

  Standard_Real Linf=ElCLib::Parameter(Line
			     ,ElCLib::CircleValue(CDomainAndRes.Binf
						 ,Circle.Axis()
						 ,Circle.Radius()));
  Standard_Real Lsup=ElCLib::Parameter(Line
			     ,ElCLib::CircleValue(CDomainAndRes.Bsup
						 ,Circle.Axis()
						 ,Circle.Radius()));

  Interval LInter(Linf,Lsup);   //-- Necessairement Borne 
  
  Interval LInterAndDomain=LDomain.IntersectionWithBounded(LInter);

  if(!LInterAndDomain.IsNull) {

    Standard_Real DomLinf = (RefLineDomain.HasFirstPoint())? RefLineDomain.FirstParameter() : -Precision::Infinite();
    Standard_Real DomLsup = (RefLineDomain.HasLastPoint())? RefLineDomain.LastParameter() : Precision::Infinite();
    
    Linf = LInterAndDomain.Binf;
    Lsup = LInterAndDomain.Bsup;

    if(Linf<DomLinf) {
      Linf = DomLinf; 
    }
    if(Lsup<DomLinf) { 
      Lsup = DomLinf; 
    }
    
    if(Linf>DomLsup) {
      Linf = DomLsup; 
    }
    if(Lsup>DomLsup) { 
      Lsup = DomLsup; 
    }
  
    LInterAndDomain.Binf = Linf;
    LInterAndDomain.Bsup = Lsup;

#if 0     
    Standard_Real Cinf = 
      ElCLib::CircleParameter(Circle.Axis()					       
			      ,ElCLib::LineValue(LInterAndDomain.Binf,
					Line.Position()));
    Standard_Real Csup = 
      ElCLib::CircleParameter(Circle.Axis()
			      ,ElCLib::LineValue(LInterAndDomain.Bsup
					,Line.Position()));

    if(Cinf<CDomainAndRes.Binf) Cinf = CDomainAndRes.Binf;
    if(Csup>CDomainAndRes.Bsup) Csup = CDomainAndRes.Bsup;
#else
    Standard_Real Cinf=CDomainAndRes.Binf;
    Standard_Real Csup=CDomainAndRes.Bsup;
#endif
    if(Cinf>=Csup) { Cinf = CDomainAndRes.Binf; Csup = CDomainAndRes.Bsup; } 
    CircleSolution[NbSolTotal]=PeriodicInterval(Cinf,Csup);
    if(CircleSolution[NbSolTotal].Length() > M_PI)
      CircleSolution[NbSolTotal].Complement();
    
    LineSolution[NbSolTotal]=LInterAndDomain;
    NbSolTotal++;
  }
}

//=======================================================================
//function : LineCircleGeometricIntersection
//purpose  : 
//~~ On cherche des segments d intersection dans le `tuyau` 
//~~   R+Tol   R-Tol  ( Tol est TolConf : Tolerance de confusion d arc)
//~~ On Cherche un point d intersection a une distance TolTang du cercle.   
//=======================================================================
void LineCircleGeometricIntersection(const gp_Lin2d& Line,
				     const gp_Circ2d& Circle,
				     const Standard_Real Tol,
				     const Standard_Real TolTang,
				     PeriodicInterval& CInt1,
				     PeriodicInterval& CInt2,
				     Standard_Integer& nbsol) 
{
  

  Standard_Real dO1O2=Line.Distance(Circle.Location());
  Standard_Real R=Circle.Radius();
  Standard_Real RmTol=R-Tol;
  Standard_Real binf1,binf2=0,bsup1,bsup2=0;
    
  //---------------------------------------------------------------- 
  if(dO1O2 > (R+Tol))  {  //-- pas d intersection avec le 'tuyau'
    if(dO1O2 > (R+TolTang)) {  
      nbsol=0; 
      return;
    }
    else { 
      binf1=0.0;  
      bsup1=0.0;
      nbsol=1;
    }
  }
  else { 
    //---------------------------------------------------------------- 
    Standard_Boolean b2Sol;
    Standard_Real dAlpha1;
    //---------------------------------------------------------------
    //-- Line coupe le cercle Circle+ (=C(x1,y1,R1+Tol))
    //modified by NIZNHY-PKV Thu May 12 12:25:17 2011f
    b2Sol=Standard_False;
    if (R>dO1O2+TolTang) {
      Standard_Real aX2, aTol2;
      //
      aTol2=Tol*Tol;
      aX2=4.*(R*R-dO1O2*dO1O2);
      if (aX2>aTol2) {
	b2Sol=!b2Sol;
      }
    }
    if(dO1O2 > RmTol && !b2Sol) { 
    //if(dO1O2 > RmTol) { 
    //modified by NIZNHY-PKV Thu May 12 12:25:20 2011t
      Standard_Real dx=dO1O2;
      Standard_Real dy=0.0;     //(RpTol*RpTol-dx*dx); //Patch !!!
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      dAlpha1=ATan2(dy,dx);
      
      binf1=-dAlpha1;  
      bsup1=dAlpha1;
      nbsol=1;
    }  
    //--------------------------------------------------------------------
    //--           2 segments donnes par Inter Line avec Circle-  Circle+
    //-- 
    else {
      //------------------- Intersection Line Circle+ --------------------------
      Standard_Real dx=dO1O2;
      Standard_Real dy=R*R-dx*dx;    //(RpTol*RpTol-dx*dx); //Patch !!!
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      
      dAlpha1=ATan2(dy,dx);
      binf1=-dAlpha1;  bsup2=dAlpha1;  //--  |...?     ?...|   Sur C1
      
      //------------------ Intersection Line Circle-  -------------------------
      dy=R*R-dx*dx;                  //(RmTol*RmTol-dx*dx); //Patch !!!
      dy=(dy>=0.0)? Sqrt(dy) : 0.0;
      dAlpha1=ATan2(dy,dx);
      
      binf2=dAlpha1;  bsup1=-dAlpha1;  //--  |...x     x...|   Sur C1

      if((dAlpha1*R)<(Max(Tol,TolTang))) { 
	bsup1 = bsup2; 
	nbsol = 1;
      }
      else { 
	nbsol=2;
      }
    }
  }
  //--------------------------------------------------------------
  //-- Mise en forme des resultats : 
  //--    Les calculs ont ete fait dans le repere x1,y1, (O1,O2)
  //--    On se ramene au repere propre a C1
  
  Standard_Real dAngle1=(Circle.XAxis().Direction()).Angle(Line.Direction());
  
#if 0 
  //---------------------------------------------
  //-- Si le cercle est indirect alors l origine
  //-- est vue en -dAngle1. 
  //--
  if(Circle.IsDirect() == Standard_False) { 
    dAngle1 = -dAngle1;
  }
#endif  
  
  
  Standard_Real a,b,c,d;
  Line.Coefficients(a,b,c);
  
  d = a*Circle.Location().X() + b*Circle.Location().Y() + c;
  
  if(d>0.0)  dAngle1+= PIsur2;
  else       dAngle1-= PIsur2;

     
  if(dAngle1<0.0) dAngle1+=PIpPI;
  else if(dAngle1>PIpPI) dAngle1-=PIpPI;
  
  
  binf1+=dAngle1;  bsup1+=dAngle1;
  
  //-- par construction aucun des segments ne peut exceder PI
  //-- (permet de ne pas gerer trop de cas differents)
  
  if(Circle.IsDirect() == Standard_False) {
    Standard_Real t=binf1; binf1=bsup1; bsup1=t;
    binf1 = -binf1;
    bsup1 = -bsup1;
  }


  CInt1.SetValues(binf1,bsup1);
  if(CInt1.Length() > M_PI) CInt1.Complement();
  

  if(nbsol==2) {
    binf2+=dAngle1;  bsup2+=dAngle1;

    if(Circle.IsDirect() == Standard_False) {
      Standard_Real t=binf2; binf2=bsup2; bsup2=t;
      binf2 = -binf2;
      bsup2 = -bsup2;
    }

    CInt2.SetValues(binf2,bsup2);
    if(CInt2.Length() > M_PI) CInt2.Complement();
  }
//  Modified by Sergey KHROMOV - Thu Oct 26 17:51:05 2000 Begin
  else {
    if (CInt1.Bsup > PIpPI && CInt1.Binf < PIpPI) {
      nbsol = 2;
      binf2 = CInt1.Binf;
      bsup2 = PIpPI;
      binf1 = 0.;
      CInt1.SetValues(binf1,CInt1.Bsup - PIpPI);
      if(CInt1.Length() > M_PI) CInt1.Complement();
      CInt2.SetValues(binf2,bsup2);
      if(CInt2.Length() > M_PI) CInt2.Complement();
    }
  }
//  Modified by Sergey KHROMOV - Thu Oct 26 17:51:13 2000 End
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DomainIntersection(const IntRes2d_Domain& Domain
			,const Standard_Real U1inf
			,const Standard_Real U1sup
			,Standard_Real& Res1inf
			,Standard_Real& Res1sup
			,IntRes2d_Position& PosInf
			,IntRes2d_Position& PosSup) {
  
  if(Domain.HasFirstPoint()) {
    if(U1sup < (Domain.FirstParameter()-Domain.FirstTolerance())) {
      Res1inf=1; Res1sup=-1; 
      return;
    }
    if(U1inf>(Domain.FirstParameter()+Domain.FirstTolerance())) {
      Res1inf=U1inf;
      PosInf=IntRes2d_Middle;
    }
    else {
      Res1inf=Domain.FirstParameter(); 
      PosInf=IntRes2d_Head;
    }
  }
  else {
    Res1inf=U1inf; 
    PosInf=IntRes2d_Middle;
  }
  
  if(Domain.HasLastPoint()) {
    if(U1inf >(Domain.LastParameter()+Domain.LastTolerance())) {
      Res1inf=1; Res1sup=-1;
      return;
    }
    if(U1sup<(Domain.LastParameter()-Domain.LastTolerance())) {
      Res1sup=U1sup; 
      PosSup=IntRes2d_Middle;
    }
    else {
      Res1sup=Domain.LastParameter();
      PosSup=IntRes2d_End;
    }
  }
  else {
    Res1sup=U1sup;
    PosSup=IntRes2d_Middle;
  }
  //-- Si un des points est en bout ,
  //-- on s assure que les parametres sont corrects
  if(Res1inf>Res1sup) { 
    if(PosSup==IntRes2d_Middle) {
      Res1sup=Res1inf;
    }
    else {
      Res1inf=Res1sup;
    }
  }
  //--- Traitement des cas ou une intersection vraie est dans la tolerance
  //--  d un des bouts
  /*if(PosInf==IntRes2d_Head) {
    if(Res1sup <= (Res1inf+Domain.FirstTolerance())) {
      Res1sup=Res1inf;
      PosSup=IntRes2d_Head;
    }
  }
  if(PosSup==IntRes2d_End) {
    if(Res1inf >= (Res1sup-Domain.LastTolerance())) {
      Res1inf=Res1sup;
      PosInf=IntRes2d_End;
    }
  }*/
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void LineLineGeometricIntersection(const gp_Lin2d& L1
				   ,const gp_Lin2d& L2
				   ,const Standard_Real Tol
				   ,Standard_Real& U1
				   ,Standard_Real& U2
				   ,Standard_Real& SinDemiAngle
				   ,Standard_Integer& nbsol) {
  
  Standard_Real U1x=L1.Direction().X();
  Standard_Real U1y=L1.Direction().Y();
  Standard_Real U2x=L2.Direction().X();
  Standard_Real U2y=L2.Direction().Y();
  Standard_Real Uo21x = L2.Location().X() - L1.Location().X();
  Standard_Real Uo21y = L2.Location().Y() - L1.Location().Y();
  
  Standard_Real D=U1y*U2x-U1x*U2y;

//modified by NIZHNY-MKK  Tue Feb 15 10:54:04 2000.BEGIN
//   if(Abs(D)<1e-15) { //-- Droites //
  if(Abs(D) < TOLERANCE_ANGULAIRE) {
//modified by NIZHNY-MKK  Tue Feb 15 10:54:11 2000.END
    D=U1y*Uo21x - U1x*Uo21y;
    nbsol=(Abs(D)<=Tol)? 2 : 0;
  }
  else {
    U1=(Uo21y * U2x - Uo21x * U2y)/D;
    U2=(Uo21y * U1x - Uo21x * U1y)/D;
    
    //------------------- Calcul du Sin du demi angle  entre L1 et L2
    //---- 
    if(D<0.0) D=-D;
    if(D>1.0) D=1.0;                      //-- Deja vu !
    SinDemiAngle=Sin(0.5*ASin(D));
    nbsol=1;
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*IntCurve_IntConicConic::IntCurve_IntConicConic(const gp_Lin2d& L1
					       ,const IntRes2d_Domain& D1
					       ,const gp_Lin2d& L2
					       ,const IntRes2d_Domain& D2
					       ,const Standard_Real TolConf
					       ,const Standard_Real Tol)  {
  Perform(L1,D1,L2,D2,TolConf,Tol);
}


IntCurve_IntConicConic::IntCurve_IntConicConic(const gp_Lin2d& L1
					       ,const IntRes2d_Domain& D1
					       ,const gp_Circ2d& C2
					       ,const IntRes2d_Domain& D2
					       ,const Standard_Real TolConf
					       ,const Standard_Real Tol) {
  
  Perform(L1,D1,C2,D2,TolConf,Tol);
}


IntCurve_IntConicConic::IntCurve_IntConicConic(const gp_Circ2d& C1
                                              ,const IntRes2d_Domain& D1
                                              ,const gp_Circ2d& C2
                                              ,const IntRes2d_Domain& D2
					      ,const Standard_Real TolConf
                                              ,const Standard_Real Tol) {
  SetReversedParameters(Standard_False);
  Perform(C1,D1,C2,D2,TolConf,Tol);
}*/ //amv OCC12547
//----------------------------------------------------------------------
void IntCurve_IntConicConic::Perform(const gp_Circ2d& Circle1
				,const IntRes2d_Domain& DomainCirc1
				,const gp_Circ2d& _Circle2
				,const IntRes2d_Domain& _DomainCirc2
				,const Standard_Real TolConf,const Standard_Real Tol) {


//-- TRES TRES MAL FAIT    A REPRENDRE UN JOUR ....   (lbr Octobre 98) 
  gp_Circ2d Circle2=_Circle2;
  IntRes2d_Domain  DomainCirc2=_DomainCirc2;
  Standard_Boolean IndirectCircles=Standard_False;
  if(Circle1.IsDirect() != _Circle2.IsDirect()) { 
    IndirectCircles=Standard_True;
    Circle2=_Circle2.Reversed();
    DomainCirc2.SetValues(_DomainCirc2.LastPoint(),
			  PIpPI-_DomainCirc2.LastParameter(),
			  _DomainCirc2.LastTolerance(),
			  _DomainCirc2.FirstPoint(),
			  PIpPI-_DomainCirc2.FirstParameter(),
			  _DomainCirc2.FirstTolerance());
    DomainCirc2.SetEquivalentParameters(0.0,PIpPI);
  }
  
  this->ResetFields();
  Standard_Integer nbsol=0;
  PeriodicInterval C1_Int1,C1_Int2;

  //------- Intersection sans tenir compte du domaine  ----> nbsol=0,1,2,3 
  CircleCircleGeometricIntersection(Circle1,Circle2,TolConf,Tol,C1_Int1,C1_Int2,nbsol);
  done=Standard_True;

  if(nbsol==0) { //-- Pas de solutions 
    return;
  }

  PeriodicInterval C1Domain(DomainCirc1); 
  //-- On se ramene entre 0 et 2PI
  Standard_Real deltat = C1Domain.Bsup-C1Domain.Binf;
  if(deltat>=PIpPI) { deltat=PIpPI-1e-14; } 
  
  while(C1Domain.Binf >= PIpPI) C1Domain.Binf-=PIpPI;
  while(C1Domain.Binf <  0.0)   C1Domain.Binf+=PIpPI;
  C1Domain.Bsup=C1Domain.Binf+deltat;

  PeriodicInterval C2Domain(DomainCirc2); 
  deltat = C2Domain.Bsup-C2Domain.Binf;
  if(deltat>=PIpPI) { deltat=PIpPI-1e-14; } 

  while(C2Domain.Binf >= PIpPI) C2Domain.Binf-=PIpPI;
  while(C2Domain.Binf <  0.0)   C2Domain.Binf+=PIpPI;
  C2Domain.Bsup=C2Domain.Binf+deltat;

  Standard_Boolean IdentCircles=Standard_False;

  if(nbsol>2) {       
    //-- Les 2 cercles sont confondus a Tol pres
    C1_Int1.SetValues(0,PIpPI);
    C1_Int2.SetNull(); 
    //---------------------------------------------------------------
    //-- Flag utilise pour specifier que les intervalles manipules
    //--   peuvent etre de longueur superieure a pi. 
    //-- Pour des cercles non identiques, on a necessairement cette
    //--   condition sur les resultats de l intersection geometrique
    //--   ce qui permet de normaliser rapidement les intervalles.
    //--   ex: -1 4 -> longueur > PI  
    //--        donc -1 4 devient  4 , 2*pi-1
    //---------------------------------------------------------------
    IdentCircles=Standard_True;
  }

  Standard_Integer NbSolTotal=0;
  PeriodicInterval SolutionC1[4];
  PeriodicInterval SolutionC2[4];
  
  //----------------------------------------------------------------------
  //----------- Traitement du premier intervalle Geometrique  C1_Int1 ----
  //----------------------------------------------------------------------
  //-- NbSolTotal est incremente a chaque Intervalle solution.
  //-- On stocke les intervalles dans les tableaux : SolutionC1(C2)
  //-- Dimensionnes a 4 elements.
  //-- des Exemples faciles donnent 3 Intersections
  //-- des Problemes numeriques peuvent en donner 4 ??????
  //--
  PeriodicInterval C1DomainAndRes=C1Domain.FirstIntersection(C1_Int1);
  
  ProjectOnC2AndIntersectWithC2Domain(Circle1,Circle2
				      ,C1DomainAndRes
				      ,C2Domain
				      ,SolutionC1,SolutionC2
				      ,NbSolTotal
				      ,IdentCircles);
  //----------------------------------------------------------------------
  //-- Seconde Intersection :  Par exemple :     2*PI-1  2*PI+1
  //--                         Intersecte avec     0.5   2*PI-0.5
  //--     Donne les intervalles : 0.5,1    et  2*PI-1,2*PI-0.5
  //--
  C1DomainAndRes=C1Domain.SecondIntersection(C1_Int1);
  
  ProjectOnC2AndIntersectWithC2Domain(Circle1,Circle2
				      ,C1DomainAndRes
				      ,C2Domain
				      ,SolutionC1,SolutionC2
				      ,NbSolTotal
				      ,IdentCircles);

  //----------------------------------------------------------------------
  //----------- Traitement du second intervalle Geometrique   C1_Int2 ----
  //----------------------------------------------------------------------
  if(nbsol==2) {
    C1DomainAndRes=C1Domain.FirstIntersection(C1_Int2);
    
    ProjectOnC2AndIntersectWithC2Domain(Circle1,Circle2
					,C1DomainAndRes
					,C2Domain
					,SolutionC1,SolutionC2
					,NbSolTotal
					,IdentCircles);
    //--------------------------------------------------------------------
    C1DomainAndRes=C1Domain.SecondIntersection(C1_Int2);
    
    ProjectOnC2AndIntersectWithC2Domain(Circle1,Circle2
					,C1DomainAndRes
					,C2Domain
					,SolutionC1,SolutionC2
					,NbSolTotal
					,IdentCircles);
  }
  //----------------------------------------------------------------------
  //-- Calcul de toutes les transitions et Positions.
  //--
  //----------------------------------------------------------------------
  //-- On determine si des intervalles sont reduit a des points 
  //--      ( Rayon * Intervalle.Length()    <    Tol   )
  //--
  Standard_Real R1=Circle1.Radius();
  Standard_Real R2=Circle2.Radius();
  Standard_Real Tol2=Tol+Tol;     //---- Pour eviter de toujours retourner
                                  //des segments
  Standard_Integer i ;
  if(Tol < (1e-10)) Tol2 = 1e-10; 
  for( i=0; i<NbSolTotal ; i++) { 
    if(((R1 * SolutionC1[i].Length()))<=Tol2 
       && ((R2 * SolutionC2[i].Length()))<=Tol2) {
      
      Standard_Real t=(SolutionC1[i].Binf+SolutionC1[i].Bsup)*0.5;
      SolutionC1[i].Binf=SolutionC1[i].Bsup=t;
      
      t=(SolutionC2[i].Binf+SolutionC2[i].Bsup)*0.5;
      SolutionC2[i].Binf=SolutionC2[i].Bsup=t;
    }
  }

  //----------------------------------------------------------------------
  //-- Traitement des intervalles (ou des points obtenus)
  //-- 
  gp_Ax22d Axis2C1=Circle1.Axis();
  gp_Ax22d Axis2C2=Circle2.Axis();
  gp_Pnt2d P1a,P1b,P2a,P2b;
  gp_Vec2d Tan1,Tan2,Norm1,Norm2;
  IntRes2d_Transition T1a,T1b,T2a,T2b;
  IntRes2d_Position Pos1a,Pos1b,Pos2a,Pos2b;

  Standard_Boolean Opposite=((Circle1.Location().SquareDistance(Circle2.Location()))
		   >(R1*R1+R2*R2))? Standard_True : Standard_False;

  //if(Circle1.IsDirect()) { cout<<" C1 Direct"<<endl; } else { cout<<" C1 INDirect"<<endl; }
  //if(Circle2.IsDirect()) { cout<<" C2 Direct"<<endl; } else { cout<<" C2 INDirect"<<endl; }

  for(i=0; i<NbSolTotal; i++) {
    Standard_Real C2inf=(Opposite)? SolutionC2[i].Bsup : SolutionC2[i].Binf;
    Standard_Real C2sup=(Opposite)? SolutionC2[i].Binf : SolutionC2[i].Bsup;

    Standard_Real C1inf=NormalizeOnCircleDomain(SolutionC1[i].Binf,DomainCirc1);
    C2inf=NormalizeOnCircleDomain(C2inf,DomainCirc2);

    if(IndirectCircles) { 
      
      ElCLib::CircleD2(C1inf,Axis2C1,R1,P1a,Tan1,Norm1); 
      ElCLib::CircleD2(C2inf,Axis2C2,R2,P2a,Tan2,Norm2);
      Tan2.Reverse();
      
      IntImpParGen::DeterminePosition(Pos1a,DomainCirc1,P1a,C1inf);
      IntImpParGen::DeterminePosition(Pos2a,_DomainCirc2,P2a,PIpPI-C2inf);
      Determine_Transition_LC(Pos1a,Tan1,Norm1,T1a , Pos2a,Tan2,Norm2,T2a, Tol);
      
      
      IntRes2d_IntersectionPoint NewPoint1(P1a,C1inf,PIpPI-C2inf,T1a,T2a,Standard_False);
      
      if((SolutionC1[i].Length()>0.0 ) || (SolutionC2[i].Length() >0.0)) {
	//-- On traite un intervalle non reduit a un point
	Standard_Real C1sup=NormalizeOnCircleDomain(SolutionC1[i].Bsup,DomainCirc1);
	if(C1sup<C1inf) C1sup+=PIpPI;
	C2sup=NormalizeOnCircleDomain(C2sup,DomainCirc2);
	
	ElCLib::CircleD2(C1sup,Axis2C1,R1,P1b,Tan1,Norm1); 
	ElCLib::CircleD2(C2sup,Axis2C2,R2,P2b,Tan2,Norm2);
	Tan2.Reverse();

	IntImpParGen::DeterminePosition(Pos1b,DomainCirc1,P1b,C1sup);
	IntImpParGen::DeterminePosition(Pos2b,_DomainCirc2,P2b,PIpPI-C2sup);
	Determine_Transition_LC(Pos1b,Tan1,Norm1,T1b , Pos2b,Tan2,Norm2,T2b, Tol);
	
	//--------------------------------------------------
	
	if(Opposite) {
	  if(nbsol!=3) { 
	    if(C2inf<C2sup) C2inf+=PIpPI;
	  }
	}
	else {
	  if(nbsol!=3) { 
	    if(C2sup<C2inf) C2sup+=PIpPI;
	  }
	}
	
	IntRes2d_IntersectionPoint NewPoint2(P1b,C1sup,PIpPI-C2sup,T1b,T2b,Standard_False);
	IntRes2d_IntersectionSegment NewSeg(NewPoint1,NewPoint2,
					    (Opposite==Standard_True)? Standard_False : Standard_True,
					    Standard_False);
	Append(NewSeg);
	
      }
      else {
	Append(NewPoint1);
      }
      
    }
    else { 
      
      ElCLib::CircleD2(C1inf,Axis2C1,R1,P1a,Tan1,Norm1); 
      ElCLib::CircleD2(C2inf,Axis2C2,R2,P2a,Tan2,Norm2);
      
      IntImpParGen::DeterminePosition(Pos1a,DomainCirc1,P1a,C1inf);
      IntImpParGen::DeterminePosition(Pos2a,DomainCirc2,P2a,C2inf);
      Determine_Transition_LC(Pos1a,Tan1,Norm1,T1a , Pos2a,Tan2,Norm2,T2a, Tol);
      
      
      IntRes2d_IntersectionPoint NewPoint1(P1a,C1inf,C2inf,T1a,T2a,Standard_False);
      
      if((SolutionC1[i].Length()>0.0 ) || (SolutionC2[i].Length() >0.0)) {
	//-- On traite un intervalle non reduit a un point
	Standard_Real C1sup=NormalizeOnCircleDomain(SolutionC1[i].Bsup,DomainCirc1);
	if(C1sup<C1inf) C1sup+=PIpPI;
	C2sup=NormalizeOnCircleDomain(C2sup,DomainCirc2);
	
	ElCLib::CircleD2(C1sup,Axis2C1,R1,P1b,Tan1,Norm1); 
	ElCLib::CircleD2(C2sup,Axis2C2,R2,P2b,Tan2,Norm2);
	
	IntImpParGen::DeterminePosition(Pos1b,DomainCirc1,P1b,C1sup);
	IntImpParGen::DeterminePosition(Pos2b,DomainCirc2,P2b,C2sup);
	Determine_Transition_LC(Pos1b,Tan1,Norm1,T1b , Pos2b,Tan2,Norm2,T2b, Tol);
	
	//--------------------------------------------------
	
	if(Opposite) {
	  if(nbsol!=3) { 
	    if(C2inf<C2sup) C2inf+=PIpPI;
	  }
	}
	else {
	  if(nbsol!=3) { 
	    if(C2sup<C2inf) C2sup+=PIpPI;
	  }
	}
	
	IntRes2d_IntersectionPoint NewPoint2(P1b,C1sup,C2sup,T1b,T2b,Standard_False);
	IntRes2d_IntersectionSegment NewSeg(NewPoint1,NewPoint2,Opposite,Standard_False);
	Append(NewSeg);
	
      }
      else {
	Append(NewPoint1);
      }
    }
  }
}
//----------------------------------------------------------------------
IntRes2d_Position FindPositionLL(Standard_Real &Param
				 ,const IntRes2d_Domain& Domain) 
{
  Standard_Real aDPar = Precision::Infinite();
  IntRes2d_Position aPos = IntRes2d_Middle; 
  Standard_Real aResPar = Param;
  if(Domain.HasFirstPoint()) {
    aDPar = Abs(Param-Domain.FirstParameter());
    if( aDPar <= Domain.FirstTolerance()) {
      aResPar=Domain.FirstParameter();
      aPos = IntRes2d_Head;
     
    }
  }
  if(Domain.HasLastPoint()) {
    Standard_Real aD2 = Abs(Param-Domain.LastParameter());
    if( aD2 <= Domain.LastTolerance() && (aPos == IntRes2d_Middle || aD2 < aDPar )) 
    {
      aResPar=Domain.LastParameter();
      aPos = IntRes2d_End;
    }
  }
  Param = aResPar;
  return aPos;
}
//--------------------------------------------------------------------
//gka 0022833
// Method to compute of point of intersection for case
//when specified domain less than specified tolerance for intersection
static inline void getDomainParametrs(const IntRes2d_Domain& theDomain,
                                      Standard_Real& theFirst,
                                      Standard_Real& theLast,
                                      Standard_Real& theTol1,
                                      Standard_Real& theTol2)
{
  theFirst = (theDomain.HasFirstPoint() ? theDomain.FirstParameter() : -Precision::Infinite());
  theLast = (theDomain.HasLastPoint() ? theDomain.LastParameter() : Precision::Infinite()); 
  theTol1 = (theDomain.HasFirstPoint() ? theDomain.FirstTolerance() : 0.);
  theTol2 = (theDomain.HasLastPoint() ? theDomain.LastTolerance() : 0.);
}


static Standard_Boolean computeIntPoint(const IntRes2d_Domain& theCurDomain,
                                                         const IntRes2d_Domain& theDomainOther,
                                                         const gp_Lin2d& theCurLin,
                                                         const gp_Lin2d& theOtherLin,   
                                                         Standard_Real theCosT1T2,
                                                         Standard_Real theParCur, Standard_Real theParOther,
                                                         Standard_Real& theResInf, Standard_Real& theResSup,
                                                         Standard_Integer theNum,
                                                         IntRes2d_TypeTrans theCurTrans,    
                                                         IntRes2d_IntersectionPoint& theNewPoint)
{
  if(fabs(theResSup-theParCur) > fabs(theResInf-theParCur))
    theResSup = theResInf;

  Standard_Real aRes2 = theParOther + (theResSup - theParCur) * theCosT1T2;

  Standard_Real aFirst2, aLast2, aTol1, aTol2;
  getDomainParametrs(theDomainOther,aFirst2, aLast2, aTol1, aTol2);
  if( aRes2  < aFirst2 - aTol1 || aRes2  > aLast2 + aTol2 ) 
	  return Standard_False;
	
  //------ compute parameters of intersection point --
  IntRes2d_Transition aT1,aT2;
  IntRes2d_Position aPos1a = FindPositionLL(theResSup,theCurDomain);
  IntRes2d_Position aPos2a = FindPositionLL(aRes2,theDomainOther);
  IntRes2d_TypeTrans anOtherTrans = ( theCurTrans == IntRes2d_Out ? 
      IntRes2d_In : ( theCurTrans == IntRes2d_In ? IntRes2d_Out : IntRes2d_Undecided ) );

  if( theCurTrans != IntRes2d_Undecided )
  {
    aT1.SetValue(Standard_False, aPos1a, theCurTrans);
    aT2.SetValue(Standard_False,  aPos2a, anOtherTrans);
  }
  else  
  { 
    Standard_Boolean anOpposite = theCosT1T2 < 0.;
    aT1.SetValue(Standard_False,aPos1a,IntRes2d_Unknown,anOpposite);
    aT2.SetValue(Standard_False,aPos2a,IntRes2d_Unknown,anOpposite);
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //--------------------------------------------------
  //gka bug 0022833 
  Standard_Real aResU1 = theParCur;
  Standard_Real aResU2 = theParOther;

  Standard_Real aFirst1, aLast1;
  getDomainParametrs(theCurDomain,aFirst1, aLast1, aTol1, aTol2);
  
  Standard_Boolean isInside1 = (theParCur >= aFirst1 && theParCur <= aLast1);
  Standard_Boolean isInside2 = (theParOther >= aFirst2 && theParOther <= aLast2);

  if(!isInside1 || !isInside2)
  {
    if(isInside1) 
    {
      gp_Pnt2d Pt1=ElCLib::Value(aRes2,theOtherLin);
      aResU2 = aRes2;
      Standard_Real aPar1 = ElCLib::Parameter(theCurLin,Pt1);
      aResU1 =((aPar1 >= aFirst1 && aPar1<= aLast1) ?  aPar1 : theResSup);
      
    }
    else if(isInside2)
    {
      gp_Pnt2d aPt1=ElCLib::Value(theResSup,theCurLin);
      aResU1 = theResSup;
      Standard_Real aPar2 = ElCLib::Parameter(theOtherLin,aPt1);
      aResU2= ((aPar2 >= aFirst2 && aPar2<= aLast2) ? aPar2 : aRes2);
    }
    else 
    {
      aResU1 = theResSup;
      aResU2= aRes2;
    }
  }
  gp_Pnt2d aPres((ElCLib::Value(aResU1,theCurLin).XY() + ElCLib::Value(aResU2,theOtherLin).XY()) * 0.5 );
  if(theNum == 1 )
    theNewPoint.SetValues(aPres, aResU1, aResU2 ,aT1, aT2, Standard_False);
  else
    theNewPoint.SetValues(aPres, aResU2, aResU1 ,aT2, aT1, Standard_False);
  return Standard_True;
}

//----------------------------------------------------------------------
void IntCurve_IntConicConic::Perform(const gp_Lin2d& L1
				      ,const IntRes2d_Domain& Domain1
				     ,const gp_Lin2d& L2
				     ,const IntRes2d_Domain& Domain2
				     ,const Standard_Real,const Standard_Real TolR) {  
  this->ResetFields();

  //-- Coordonnees du point d intersection sur chacune des 2 droites
  Standard_Real U1,U2; 
  //-- Nombre de points solution : 1 : Intersection
  //--                             0 : Non Confondues
  //--                             2 : Confondues a la tolerance pres
  Standard_Integer nbsol;
  IntRes2d_IntersectionPoint PtSeg1,PtSeg2;
  Standard_Real SINL1L2;
  Standard_Real Tol = TolR;
  if(TolR< 1e-10) Tol = 1e-10;
  

  LineLineGeometricIntersection(L1,L2,Tol,U1,U2,SINL1L2,nbsol);

  gp_Vec2d Tan1=L1.Direction();
  gp_Vec2d Tan2=L2.Direction();
 
  Standard_Real aCosT1T2 = Tan1.Dot(Tan2);
  Standard_Boolean Opposite=(aCosT1T2 < 0.0)? Standard_True : Standard_False;

  done=Standard_True;

  if(nbsol==1) {
    //---------------------------------------------------
    //-- d: distance du point I a partir de laquelle  les 
    //--  points de parametre U1+d et U2+-d sont ecartes
    //--  d une distance superieure a Tol.
    //---------------------------------------------------
    IntRes2d_Position Pos1a,Pos2a,Pos1b,Pos2b;
    Standard_Real d=Tol/(SINL1L2);
    Standard_Real U1inf=U1-d;
    Standard_Real U1sup=U1+d;
    Standard_Real U1mU2=U1-U2;
    Standard_Real U1pU2=U1+U2;
    Standard_Real Res1inf,Res1sup;
    Standard_Real ProdVectTan;
    

    //---------------------------------------------------
    //-- On agrandit la zone U1inf U1sup pour tenir compte 
    //-- des tolerances des points en bout
    //--
    if(Domain1.HasFirstPoint()) { 
      if(L2.Distance(Domain1.FirstPoint()) < Domain1.FirstTolerance()) { 
	if(U1inf > Domain1.FirstParameter()) { 
	  U1inf = Domain1.FirstParameter();
	}
	if(U1sup < Domain1.FirstParameter()) { 
	  U1sup = Domain1.FirstParameter();
	}
      }
    }
    if(Domain1.HasLastPoint()) { 
      if(L2.Distance(Domain1.LastPoint()) < Domain1.LastTolerance()) { 
	if(U1inf > Domain1.LastParameter()) { 
	  U1inf = Domain1.LastParameter();
	}
	if(U1sup < Domain1.LastParameter()) { 
	  U1sup = Domain1.LastParameter();
	}
      }
    }      
    if(Domain2.HasFirstPoint()) { 
      if(L1.Distance(Domain2.FirstPoint()) < Domain2.FirstTolerance()) { 
	Standard_Real p = ElCLib::Parameter(L1,Domain2.FirstPoint());
	if(U1inf > p) { 
	  U1inf = p;
	}
	if(U1sup < p) { 
	  U1sup = p;
	}
      }
    }
    if(Domain2.HasLastPoint()) { 
      if(L1.Distance(Domain2.LastPoint()) < Domain2.LastTolerance()) { 
	Standard_Real p = ElCLib::Parameter(L1,Domain2.LastPoint());
	if(U1inf > p) { 
	  U1inf = p;
	}
	if(U1sup < p) { 
	  U1sup = p;
	}
      }
    }
    //-----------------------------------------------------------------

    DomainIntersection(Domain1,U1inf,U1sup,Res1inf,Res1sup,Pos1a,Pos1b);
    
    if((Res1sup-Res1inf)<0.0) {
      //-- Si l intersection est vide       
      //-- 
    }
    else { //-- (Domain1  INTER   Zone Intersection)    non vide

      ProdVectTan=Tan1.Crossed(Tan2);
      
      Standard_Boolean IsInDomain=Standard_True;

      //#####################################################################
      //##  Longueur Minimale d un segment    Sur Courbe 1 
      //##################################################################### 

      Standard_Real LongMiniSeg=Tol;


      if(((Res1sup-Res1inf)<=LongMiniSeg)
        || ((Pos1a==Pos1b)&&(Pos1a!=IntRes2d_Middle)))     
      {
        //-------------------------------  Un seul Point -------------------
        //--- lorsque la longueur du segment est inferieure a ??
        //--- ou si deux points designent le meme bout
        //gka #0022833
        IntRes2d_TypeTrans aCurTrans = ( ProdVectTan >= TOLERANCE_ANGULAIRE ? 
             IntRes2d_Out : ( ProdVectTan <= -TOLERANCE_ANGULAIRE ? IntRes2d_In : IntRes2d_Undecided ) );

        IntRes2d_IntersectionPoint NewPoint1;
        if( computeIntPoint(Domain1, Domain2, L1, L2, aCosT1T2, U1, U2, Res1inf, Res1sup, 1, aCurTrans, NewPoint1 ) )
          Append(NewPoint1);

        //------------------------------------------------------
      
    
    }  //---------------   Fin du cas  :   1 seul point --------------------
      
      else {
	//-- Intersection AND Domain1  --------> Segment ---------------------
	Standard_Real U2inf,U2sup;
	Standard_Real Res2inf,Res2sup;
	
	if(Opposite) { U2inf = U1pU2 -Res1sup;  U2sup= U1pU2-Res1inf;  }
	else         { U2inf = Res1inf-U1mU2;   U2sup= Res1sup-U1mU2;  }
	
	DomainIntersection(Domain2,U2inf,U2sup,Res2inf,Res2sup,Pos2a,Pos2b);

	//####################################################################
	//##  Test sur la longueur minimale d un segment sur Ligne2
	//####################################################################
	Standard_Real Res2sup_m_Res2inf = Res2sup-Res2inf;
	if(Res2sup_m_Res2inf < 0.0) {
	  //-- Pas de solutions On retourne Vide
	}
	else if(((Res2sup-Res2inf) > LongMiniSeg)  
		|| ((Pos2a==Pos2b)&&(Pos2a!=IntRes2d_Middle)))     {
	  //----------- Calcul des attributs du segment --------------
	  //-- Attention, les bornes Res1inf(sup) bougent donc il faut
	  //--  eventuellement recalculer les attributs
	  
	  if(Opposite) { Res1inf=U1pU2-Res2sup; Res1sup=U1pU2-Res2inf; 
			 Standard_Real Tampon=Res2inf; Res2inf=Res2sup; Res2sup=Tampon;
			 IntRes2d_Position Pos=Pos2a; Pos2a=Pos2b; Pos2b=Pos;
		       }
	  else         { Res1inf=U1mU2+Res2inf; Res1sup=U1mU2+Res2sup; }
	  
	  Pos1a=FindPositionLL(Res1inf,Domain1);
	  Pos1b=FindPositionLL(Res1sup,Domain1);	  
	  
	  IntRes2d_Transition T1a,T2a,T1b,T2b;
	  
	  if(ProdVectTan>=TOLERANCE_ANGULAIRE) {  // &&&&&&&&&&&&&&&
	    T1a.SetValue(Standard_False,Pos1a,IntRes2d_Out);      
	    T2a.SetValue(Standard_False,Pos2a,IntRes2d_In);
	  }
	  else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) {
	    T1a.SetValue(Standard_False,Pos1a,IntRes2d_In);      
	    T2a.SetValue(Standard_False,Pos2a,IntRes2d_Out);
	  }
	  else {
	    T1a.SetValue(Standard_False,Pos1a,IntRes2d_Unknown,Opposite);
	    T2a.SetValue(Standard_False,Pos2a,IntRes2d_Unknown,Opposite);
	  }
	  

	  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  //~~~~~~~  C O N V E N T I O N    -    S E G M E N T     ~~~~~~~
	  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  //~~ On Renvoie un segment dans les cas suivants :            ~~
	  //~~   (1) Extremite L1 L2   ------>    Extremite L1 L2       ~~
	  //~~   (2) Extremite L1 L2   ------>    Intersection          ~~
	  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	  
	  Standard_Boolean ResultIsAPoint=Standard_False;

	  if(((Res1sup-Res1inf)<=LongMiniSeg) 
	     || (Abs(Res2sup-Res2inf)<=LongMiniSeg)) {
	    //-- On force la creation d un point
	    ResultIsAPoint=Standard_True;
	  }
	  else {
	    //------------------------------------------------------------
	    //-- On traite les cas ou l intersection est situee du 
	    //-- Mauvais cote du domaine 
	    //-- Attention : Res2inf <-> Pos2a        Res2sup <-> Pos2b
	    //--  et         Res1inf <-> Pos1a        Res1sup <-> Pos1b
	    //--             avec Res1inf <= Res1sup
	    //------------------------------------------------------------
	    //-- Le point sera : Res1inf,Res2inf,T1a(Pos1a),T2a(Pos2a)
	    //------------------------------------------------------------

	    if(Pos1a==IntRes2d_Head) { 
	      if(Pos1b!=IntRes2d_End && U1<Res1inf)    { ResultIsAPoint=Standard_True; U1=Res1inf; U2=Res2inf; }
	    }
	    if(Pos1b==IntRes2d_End)  { 
	      if(Pos1a!=IntRes2d_Head && U1>Res1sup)   { ResultIsAPoint=Standard_True; U1=Res1sup; U2=Res2sup; }
	    }
	
            if(Pos2a==IntRes2d_Head) { 
	      if(Pos2b!=IntRes2d_End && U2<Res2inf)    { ResultIsAPoint=Standard_True; U2=Res2inf; U1=Res1inf; }
	    }
	    else {
	      if(Pos2a==IntRes2d_End)  { 
		if(Pos2b!=IntRes2d_Head && U2>Res2inf) { ResultIsAPoint=Standard_True; U2=Res2inf; U1=Res1inf; }
	      }
	    }
	    if(Pos2b==IntRes2d_Head) { 
	      if(Pos2a!=IntRes2d_End && U2<Res2sup)    { ResultIsAPoint=Standard_True; U2=Res2sup; U1=Res1sup; }
	    }
	    else {
	      if(Pos2b==IntRes2d_End) {
		if(Pos2a!=IntRes2d_Head && U2>Res2sup) { ResultIsAPoint=Standard_True; U2=Res2sup; U1=Res1sup; }
	      }
	    }
	  }
	    
	  

	  if((!ResultIsAPoint) && (Pos1a!=IntRes2d_Middle || Pos2a!=IntRes2d_Middle)) {
	    IntRes2d_Transition T1b,T2b;
	    if(ProdVectTan>=TOLERANCE_ANGULAIRE) { //&&&&&&&&&&&&&&
	      T1b.SetValue(Standard_False,Pos1b,IntRes2d_Out);
	      T2b.SetValue(Standard_False,Pos2b,IntRes2d_In);
	    }
	    else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) { //&&&&&&&&&&&&&&
	      T1b.SetValue(Standard_False,Pos1b,IntRes2d_In);
	      T2b.SetValue(Standard_False,Pos2b,IntRes2d_Out);
	    }
	    else {
	      T1b.SetValue(Standard_False,Pos1b,IntRes2d_Unknown,Opposite);
	      T2b.SetValue(Standard_False,Pos2b,IntRes2d_Unknown,Opposite);
	    }
	    gp_Pnt2d Ptdebut;
	    if(Pos1a==IntRes2d_Middle) { 
	      Standard_Real t3;
	      if(Opposite) {
		t3 = (Pos2a == IntRes2d_Head)? Res2sup : Res2inf;
	      }
	      else {
		t3 = (Pos2a == IntRes2d_Head)? Res2inf : Res2sup;
	      }
	      Ptdebut=ElCLib::Value(t3,L2);
	      Res1inf=ElCLib::Parameter(L1,Ptdebut);
	    }
	    else {
	      Standard_Real t4 = (Pos1a == IntRes2d_Head)? Res1inf : Res1sup;
	      Ptdebut=ElCLib::Value(t4,L1);
	      Res2inf=ElCLib::Parameter(L2,Ptdebut);
	    }
	    PtSeg1.SetValues(Ptdebut,Res1inf,Res2inf,T1a,T2a,Standard_False);
	    if(Pos1b!=IntRes2d_Middle || Pos2b!=IntRes2d_Middle) {
	      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	      //~~ Ajustement des parametres et du point renvoye
	      gp_Pnt2d Ptfin;
	      if(Pos1b==IntRes2d_Middle) { 
		Ptfin=ElCLib::Value(Res2sup,L2);
		Res1sup=ElCLib::Parameter(L1,Ptfin);
	      }
	      else {
		Ptfin=ElCLib::Value(Res1sup,L1);
		Res2sup=ElCLib::Parameter(L2,Ptfin);
	      }
	      PtSeg2.SetValues(Ptfin,Res1sup,Res2sup,T1b,T2b,Standard_False);
	      IntRes2d_IntersectionSegment Segment(PtSeg1,PtSeg2
						   ,Opposite,Standard_False);
	      Append(Segment);  
	    } 
	    else { //-- Extremite(L1 ou L2)  ------>   Point Middle(L1 et L2)

	      Pos1b=FindPositionLL(U1,Domain1);
	      Pos2b=FindPositionLL(U2,Domain2);	  	
	      if(ProdVectTan>=TOLERANCE_ANGULAIRE) {
		T1b.SetValue(Standard_False,Pos1b,IntRes2d_Out);
		T2b.SetValue(Standard_False,Pos2b,IntRes2d_In);
	      }
	      else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) {
		T1b.SetValue(Standard_False,Pos1b,IntRes2d_In);
		T2b.SetValue(Standard_False,Pos2b,IntRes2d_Out);
	      }
	      else {
		T1b.SetValue(Standard_False,Pos1b,IntRes2d_Unknown,Opposite);
		T2b.SetValue(Standard_False,Pos2b,IntRes2d_Unknown,Opposite);
	      }
	      
	      PtSeg2.SetValues(ElCLib::Value(U2,L2),U1,U2,T1b,T2b,Standard_False);

	      if((Abs(Res1inf-U1) >LongMiniSeg) && (Abs(Res2inf-U2) >LongMiniSeg)) {
		IntRes2d_IntersectionSegment Segment(PtSeg1,PtSeg2,Opposite,Standard_False);
		Append(Segment);  
	      }
	      else {
		Append(SegmentToPoint(PtSeg1,T1a,T2a,PtSeg2,T1b,T2b));
	      }
	    }

	  } //-- (Pos1a!=IntRes2d_Middle || Pos2a!=IntRes2d_Middle) --
	  else {  //-- Pos1a == Pos2a == Middle
	    if(Pos1b==IntRes2d_Middle) Pos1b=Pos1a;
	    if(Pos2b==IntRes2d_Middle) Pos2b=Pos2a;
	    if(ResultIsAPoint) {
	      //-- Middle sur le segment A 
	      //-- 
	      if(Pos1b!=IntRes2d_Middle || Pos2b!=IntRes2d_Middle) {
		gp_Pnt2d Ptfin;
		if(Pos1b==IntRes2d_Middle) {
		  Standard_Real t2;
		  if(Opposite) { 
		    t2 = (Pos2b == IntRes2d_Head)? Res2sup : Res2inf;
		  }
		  else {
		    t2 = (Pos2b == IntRes2d_Head)? Res2inf : Res2sup;
		  }
		  Ptfin=ElCLib::Value(t2,L2);
		  Res1sup=ElCLib::Parameter(L1,Ptfin);
//modified by NIZHNY-MKK  Tue Feb 15 10:54:51 2000.BEGIN
		  Pos1b=FindPositionLL(Res1sup,Domain1);
//modified by NIZHNY-MKK  Tue Feb 15 10:54:55 2000.END

		}
		else {
		  Standard_Real t1 = (Pos1b == IntRes2d_Head)? Res1inf : Res1sup;
		  Ptfin=ElCLib::Value(t1,L1);
		  Res2sup=ElCLib::Parameter(L2,Ptfin);
//modified by NIZHNY-MKK  Tue Feb 15 10:55:08 2000.BEGIN
		  Pos2b=FindPositionLL(Res2sup,Domain2);
//modified by NIZHNY-MKK  Tue Feb 15 10:55:11 2000.END
		}
		if(ProdVectTan>=TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Out);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_In);
		}
		else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_In);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Out);
		}
		else {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Unknown,Opposite);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Unknown,Opposite);
		}
	        PtSeg2.SetValues(Ptfin,Res1sup,Res2sup,T1b,T2b,Standard_False);
		Append(PtSeg2);
	      }
	      else {
		Pos1b=FindPositionLL(U1,Domain1);
		Pos2b=FindPositionLL(U2,Domain2);	  	
		
		if(ProdVectTan>=TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Out);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_In);
		}
		else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_In);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Out);
		}
		else {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Unknown,Opposite);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Unknown,Opposite);
		}
		PtSeg1.SetValues(ElCLib::Value(U2,L2),U1,U2,T1b,T2b,Standard_False);
		Append(PtSeg1); 
	      }
	    }
	    else {
	      PtSeg1.SetValues(ElCLib::Value(U2,L2),U1,U2,T1a,T2a,Standard_False);
	      
	      if((Pos1b!=IntRes2d_Middle || Pos2b!=IntRes2d_Middle)) {
		IntRes2d_Transition T1b,T2b;
		if(ProdVectTan>=TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Out);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_In);
		}
		else if(ProdVectTan<=-TOLERANCE_ANGULAIRE) {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_In);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Out);
		}
		else {
		  T1b.SetValue(Standard_False,Pos1b,IntRes2d_Unknown,Opposite);
		  T2b.SetValue(Standard_False,Pos2b,IntRes2d_Unknown,Opposite);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//~~ Ajustement des parametres et du point renvoye
		gp_Pnt2d Ptfin;             
		if(Pos1b==IntRes2d_Middle) {
		  Ptfin=ElCLib::Value(Res2sup,L2);
		  Res1sup=ElCLib::Parameter(L1,Ptfin);
		}
		else {
		  Ptfin=ElCLib::Value(Res1sup,L1);
		  Res2sup=ElCLib::Parameter(L2,Ptfin);
		}

	        PtSeg2.SetValues(Ptfin,Res1sup,Res2sup,T1b,T2b,Standard_False);
		
		if((Abs(U1-Res1sup)>LongMiniSeg)
		   ||(Abs(U2-Res2sup)>LongMiniSeg)) { 
		  //-- Modif du 1er Octobre 92 (Pour Composites)
		  
		  IntRes2d_IntersectionSegment Segment(PtSeg1,PtSeg2
						       ,Opposite,Standard_False);
		  Append(Segment);  
		}
		else {
		  Append(SegmentToPoint(PtSeg1,T1a,T2a,PtSeg2,T1b,T2b));
		}
	      }
	      else {
		Append(PtSeg1); 
	      }
	    }
	  }
	} //----- Fin Creation Segment ----(Res2sup-Res2inf>Tol)-------------
	else {
	  //------ (Intersection And Domain1)  AND  Domain2  --> Point ------
	  //-- Attention Res1sup peut etre  different de  U2
	  //--   Mais on a Res1sup-Res1inf < Tol 

    //gka #0022833
    IntRes2d_TypeTrans aCurTrans = ( ProdVectTan >= TOLERANCE_ANGULAIRE ? 
           IntRes2d_In : ( ProdVectTan <= -TOLERANCE_ANGULAIRE ? IntRes2d_Out : IntRes2d_Undecided ) );

    IntRes2d_IntersectionPoint NewPoint1;
    if( computeIntPoint(Domain2, Domain1, L2, L1, aCosT1T2, U2, U1, Res2inf, Res2sup, 2, aCurTrans, NewPoint1 ) )
      Append(NewPoint1);
	
	}
      }
    }
  }
  else {
    if(nbsol==2) {  //== Droites confondues a la tolerance pres 
      //--On traite ici le cas de segments resultats non neccess. bornes
      //-- 
      //--On prend la droite D1 comme reference ( pour le sens positif )
      //--
      Standard_Integer ResHasFirstPoint=0;    
      Standard_Integer ResHasLastPoint=0;
      Standard_Real ParamStart,ParamStart2,ParamEnd,ParamEnd2;
      Standard_Real Org2SurL1=ElCLib::Parameter(L1,L2.Location());
      //== 3 : L1 et L2 bornent
      //== 2 :       L2 borne
      //== 1 : L1 borne
      if(Domain1.HasFirstPoint()) ResHasFirstPoint=1;
      if(Domain1.HasLastPoint())   ResHasLastPoint=1;
      if(Opposite) {
	if(Domain2.HasLastPoint())     ResHasFirstPoint+=2;
	if(Domain2.HasFirstPoint())   ResHasLastPoint+=2;
      }
      else {
	if(Domain2.HasLastPoint())     ResHasLastPoint+=2;
	if(Domain2.HasFirstPoint())   ResHasFirstPoint+=2;
      }
      if(ResHasFirstPoint==0 && ResHasLastPoint==0) {
	//~~~~ Creation d un segment infini avec Opposite
	Append(IntRes2d_IntersectionSegment(Opposite));
      }
      else {  //-- On obtient au pire une demi-droite
	switch(ResHasFirstPoint) {
	case 1: 
	  ParamStart=Domain1.FirstParameter(); 
	  ParamStart2=(Opposite)? (Org2SurL1-ParamStart) 
	                         :(ParamStart-Org2SurL1); 
	  break;
	case 2:
	  if(Opposite) {
	    ParamStart2=Domain2.LastParameter();
	    ParamStart=Org2SurL1 - ParamStart2; 
	  }
	  else {
	    ParamStart2=Domain2.FirstParameter();
	    ParamStart=Org2SurL1 + ParamStart2;
	  }
	  break;
	case 3:
	  if(Opposite) {
	    ParamStart2=Domain2.LastParameter();
	    ParamStart=Org2SurL1 - ParamStart2; 
	    if(ParamStart < Domain1.FirstParameter()) {
	      ParamStart=Domain1.FirstParameter();
	      ParamStart2=Org2SurL1 -  ParamStart;
	    }
	  }
	  else {
	    ParamStart2=Domain2.FirstParameter();
	    ParamStart=Org2SurL1 + ParamStart2;
	    if(ParamStart < Domain1.FirstParameter()) {
	      ParamStart=Domain1.FirstParameter();
	      ParamStart2=ParamStart - Org2SurL1;
	    }
	  }
	  break;
	default:  //~~~ Segment Infini a gauche
	  break;
	}
	
	switch(ResHasLastPoint) {
	case 1: 
	  ParamEnd=Domain1.LastParameter(); 
	  ParamEnd2=(Opposite)? (Org2SurL1-ParamEnd) 
	                         :(ParamEnd-Org2SurL1); 
	  break;
	case 2:
	  if(Opposite) {
	    ParamEnd2=Domain2.FirstParameter();
	    ParamEnd=Org2SurL1 - ParamEnd2; 
	  }
	  else {
	    ParamEnd2=Domain2.LastParameter();
	    ParamEnd=Org2SurL1 + ParamEnd2;
	  }
	  break;
	case 3:
	  if(Opposite) {
	    ParamEnd2=Domain2.FirstParameter();
	    ParamEnd=Org2SurL1 - ParamEnd2; 
	    if(ParamEnd > Domain1.LastParameter()) {
	      ParamEnd=Domain1.LastParameter();
	      ParamEnd2=Org2SurL1 -  ParamEnd;
	    }
	  }
	  else {
	    ParamEnd2=Domain2.LastParameter();
	    ParamEnd=Org2SurL1 + ParamEnd2;
	    if(ParamEnd > Domain1.LastParameter()) {
	      ParamEnd=Domain1.LastParameter();
	      ParamEnd2=ParamEnd - Org2SurL1;
	    }
	  }
	default:  //~~~ Segment Infini a droite
	  break;
	}
	
	IntRes2d_Transition Tinf,Tsup;

	if(ResHasFirstPoint) {
	  if(ResHasLastPoint) {
	    //~~~ Creation de la borne superieure
	    //~~~ L1 :     |------------->       ou          |-------------->
	    //~~~ L2 : <------------|            ou  <----|
	    if(ParamEnd >= (ParamStart-Tol)) { 
	      //~~~ Creation d un segment
	      IntRes2d_Position Pos1,Pos2;
	      Pos1=FindPositionLL(ParamStart,Domain1);
	      Pos2=FindPositionLL(ParamStart2,Domain2);
	      Tinf.SetValue(Standard_True,Pos1,IntRes2d_Unknown,Opposite);
	      Tsup.SetValue(Standard_True,Pos2,IntRes2d_Unknown,Opposite);
	      IntRes2d_IntersectionPoint P1(ElCLib::Value(ParamStart,L1)
					    ,ParamStart,ParamStart2
					    ,Tinf,Tsup,Standard_False);
	      if(ParamEnd > (ParamStart+Tol)) {
		//~~~ Le segment est assez long
		Pos1=FindPositionLL(ParamEnd,Domain1);
		Pos2=FindPositionLL(ParamEnd2,Domain2);
		Tinf.SetValue(Standard_True,Pos1,IntRes2d_Unknown,Opposite);
		Tsup.SetValue(Standard_True,Pos2,IntRes2d_Unknown,Opposite);

		IntRes2d_IntersectionPoint P2(ElCLib::Value(ParamEnd,L1)
					      ,ParamEnd,ParamEnd2
					      ,Tinf,Tsup,Standard_False);
		IntRes2d_IntersectionSegment Seg(P1,P2,Opposite,Standard_False);
		Append(Seg);
	      }
	      else {   //~~~~ le segment est de longueur inferieure a Tol
		Append(P1);
	      }
	    } //-- if( ParamEnd >= ...)
	  }   //-- if(ResHasLastPoint)
	  else { 
	    //~~~ Creation de la demi droite   |----------->
	    IntRes2d_Position Pos1=FindPositionLL(ParamStart,Domain1);
	    IntRes2d_Position Pos2=FindPositionLL(ParamStart2,Domain2);
	    Tinf.SetValue(Standard_True,Pos1,IntRes2d_Unknown,Opposite);
	    Tsup.SetValue(Standard_True,Pos2,IntRes2d_Unknown,Opposite);

	    IntRes2d_IntersectionPoint P(ElCLib::Value(ParamStart,L1)
					  ,ParamStart,ParamStart2
					  ,Tinf,Tsup,Standard_False);
	    IntRes2d_IntersectionSegment Seg(P,Standard_True,Opposite,Standard_False);
	    Append(Seg);
	  }
	}
	else {
	  IntRes2d_Position Pos1=FindPositionLL(ParamEnd,Domain1);
	  IntRes2d_Position Pos2=FindPositionLL(ParamEnd2,Domain2);
	  Tinf.SetValue(Standard_True,Pos1,IntRes2d_Unknown,Opposite);
	  Tsup.SetValue(Standard_True,Pos2,IntRes2d_Unknown,Opposite);

	  IntRes2d_IntersectionPoint P2(ElCLib::Value(ParamEnd,L1)
					,ParamEnd,ParamEnd2
					,Tinf,Tsup,Standard_False);
	  IntRes2d_IntersectionSegment Seg(P2,Standard_False,Opposite,Standard_False);
	  Append(Seg);
	  //~~~ Creation de la demi droite   <-----------|
	}
      }
    }
  }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void IntCurve_IntConicConic::Perform(const gp_Lin2d& Line
				     ,const IntRes2d_Domain& LIG_Domain
				     ,const gp_Circ2d& Circle
				     ,const IntRes2d_Domain& CIRC_Domain
				     ,const Standard_Real TolConf,const Standard_Real Tol) {

//--  if(! CIRC_Domain.IsClosed()) {
//--    Standard_ConstructionError::Raise("Domaine incorrect");
//--  }

  Standard_Boolean TheReversedParameters=ReversedParameters();
  this->ResetFields();
  this->SetReversedParameters(TheReversedParameters);

  Standard_Integer nbsol=0;
  PeriodicInterval CInt1,CInt2;  
  
  LineCircleGeometricIntersection(Line,Circle,TolConf,Tol
				  ,CInt1,CInt2
				  ,nbsol);

  done=Standard_True;

  if(nbsol==0) { //-- Pas de solutions 
    return;
  }
  
//  Modified by Sergey KHROMOV - Mon Dec 18 11:13:18 2000 Begin
  if (nbsol == 2 && CInt2.Bsup == CInt1.Binf + PIpPI) {
    Standard_Real FirstBound = CIRC_Domain.FirstParameter();
    Standard_Real LastBound = CIRC_Domain.LastParameter();
    Standard_Real FirstTol = CIRC_Domain.FirstTolerance();
    Standard_Real LastTol = CIRC_Domain.LastTolerance();
    if (CInt1.Binf == 0 && FirstBound - FirstTol > CInt1.Bsup) {
      nbsol = 1;
      CInt1.SetValues(CInt2.Binf, CInt2.Bsup);
    } else if (CInt2.Bsup == PIpPI && LastBound + LastTol < CInt2.Binf)
      nbsol = 1;
  }
//  Modified by Sergey KHROMOV - Mon Dec 18 11:13:20 2000 End

  PeriodicInterval CDomain(CIRC_Domain);
  Standard_Real deltat = CDomain.Bsup-CDomain.Binf;
  while(CDomain.Binf >= PIpPI) CDomain.Binf-=PIpPI;
  while(CDomain.Binf <  0.0)   CDomain.Binf+=PIpPI;
  CDomain.Bsup=CDomain.Binf+deltat;

  //------------------------------------------------------------
  //-- Ajout : Jeudi 28 mars 96 
  //-- On agrandit artificiellement les domaines
  Standard_Real BinfModif = CDomain.Binf;
  Standard_Real BsupModif = CDomain.Bsup;
  BinfModif-=CIRC_Domain.FirstTolerance() / Circle.Radius();
  BsupModif+=CIRC_Domain.LastTolerance() / Circle.Radius();
  deltat = BsupModif-BinfModif;
  if(deltat<=PIpPI) { 
    CDomain.Binf = BinfModif;
    CDomain.Bsup = BsupModif;
  }
  else { 
    Standard_Real t=PIpPI-deltat;
    t*=0.5;
    CDomain.Binf = BinfModif+t;
    CDomain.Bsup = BsupModif-t; 
  }
  deltat = CDomain.Bsup-CDomain.Binf;
  while(CDomain.Binf >= PIpPI) CDomain.Binf-=PIpPI;
  while(CDomain.Binf <  0.0)   CDomain.Binf+=PIpPI;
  CDomain.Bsup=CDomain.Binf+deltat;
  //-- ------------------------------------------------------------

  Interval LDomain(LIG_Domain);
  
  Standard_Integer NbSolTotal=0;
  
  PeriodicInterval SolutionCircle[4];
  Interval SolutionLine[4];
  
  //----------------------------------------------------------------------
  //----------- Traitement du premier intervalle Geometrique  CInt1   ----
  //----------------------------------------------------------------------
  //-- NbSolTotal est incremente a chaque Intervalle solution.
  //-- On stocke les intervalles dans les tableaux : SolutionCircle[4] 
  //--                                            et SolutionLine[4]
  //-- des Exemples faciles donnent 3 Intersections
  //-- des Problemes numeriques peuvent peut etre en donner 4 ??????
  //--
  PeriodicInterval CDomainAndRes=CDomain.FirstIntersection(CInt1);
  
  ProjectOnLAndIntersectWithLDomain(Circle,Line
				    ,CDomainAndRes
				    ,LDomain
				    ,SolutionCircle
				    ,SolutionLine
				    ,NbSolTotal
				    ,LIG_Domain
				    ,CIRC_Domain);
  
  CDomainAndRes=CDomain.SecondIntersection(CInt1);
  
  ProjectOnLAndIntersectWithLDomain(Circle,Line
				    ,CDomainAndRes
				    ,LDomain
				    ,SolutionCircle
				    ,SolutionLine
				    ,NbSolTotal
				    ,LIG_Domain
				    ,CIRC_Domain);
  
  //----------------------------------------------------------------------
  //----------- Traitement du second intervalle Geometrique   C1_Int2 ----
  //----------------------------------------------------------------------
  if(nbsol==2) {
    CDomainAndRes=CDomain.FirstIntersection(CInt2);
    
    ProjectOnLAndIntersectWithLDomain(Circle,Line
				      ,CDomainAndRes
				      ,LDomain
				      ,SolutionCircle
				      ,SolutionLine
				      ,NbSolTotal
				      ,LIG_Domain
				      ,CIRC_Domain);
    
    //--------------------------------------------------------------------
    CDomainAndRes=CDomain.SecondIntersection(CInt2);
    
    
    ProjectOnLAndIntersectWithLDomain(Circle,Line
				      ,CDomainAndRes
				      ,LDomain
				      ,SolutionCircle
				      ,SolutionLine
				      ,NbSolTotal
				      ,LIG_Domain
				      ,CIRC_Domain);
  }









  //----------------------------------------------------------------------
  //-- Calcul de toutes les transitions et Positions.
  //--
  //-- On determine si des intervalles sont reduit a des points 
  //--      ( Rayon * Intervalle.Length()    <    TolConf   )   ### Modif 19 Nov Tol-->TolConf
  //--
  Standard_Real R=Circle.Radius();
  Standard_Integer i ;
  Standard_Real MaxTol = TolConf;
  if(MaxTol<Tol) MaxTol = Tol;
  if(MaxTol<1.0e-10) MaxTol = 1.0e-10; 

  for( i=0; i<NbSolTotal ; i++) { 
    if((R * SolutionCircle[i].Length())<MaxTol 
       && (SolutionLine[i].Length())<MaxTol) {
      
      Standard_Real t=(SolutionCircle[i].Binf+SolutionCircle[i].Bsup)*0.5;
      SolutionCircle[i].Binf=SolutionCircle[i].Bsup=t;
      
      t=(SolutionLine[i].Binf+SolutionLine[i].Bsup)*0.5;
      SolutionLine[i].Binf=SolutionLine[i].Bsup=t;
    }
  }
#if 0 
  if(NbSolTotal == 2) { 
    if(SolutionLine[0].Binf==SolutionLine[0].BSup) { 
      if(SolutionLine[1].Binf==SolutionLine[1].BSup) {
	if(Abs(SolutionLine[0].Binf-SolutionLine[1].Binf)<TolConf) { 
	  SolutionLine[0].Binf=0.5*(SolutionLine[0].BSup+SolutionLine[1].BSup);
	  SolutionLine[0].BSup=SolutionLine[0].Binf;
	  NbSolTotal = 1;
	}
      }
    }
  }
#endif
  //----------------------------------------------------------------------
  //-- Traitement des intervalles (ou des points obtenus)
  //-- 
  if(NbSolTotal) { 
    gp_Ax22d CircleAxis=Circle.Axis();
    gp_Ax2d LineAxis=Line.Position();
    gp_Pnt2d P1a,P2a,P1b,P2b;
    gp_Vec2d Tan1,Tan2,Norm1;
    gp_Vec2d Norm2(0.0,0.0);
    IntRes2d_Transition T1a,T2a,T1b,T2b;
    IntRes2d_Position Pos1a,Pos1b,Pos2a,Pos2b;
    
    ElCLib::CircleD1(SolutionCircle[0].Binf,CircleAxis,R,P1a,Tan1);
    ElCLib::LineD1(SolutionLine[0].Binf,LineAxis,P2a,Tan2);
    
    Standard_Boolean Opposite=((Tan1.Dot(Tan2))<0.0)? Standard_True : Standard_False;
    
    
    for(i=0; i<NbSolTotal; i++ ) {


      //-- 7 aout 97 
      //-- On recentre Bin et Bsup de facon a avoir une portion commune avec CIRC_Domain
      Standard_Real p1=SolutionCircle[i].Binf;
      Standard_Real p2=SolutionCircle[i].Bsup;
      Standard_Real q1=CIRC_Domain.FirstParameter();
      Standard_Real q2=CIRC_Domain.LastParameter();
      //--          |------ CircDomain ------|   [-- Sol --]
      if(p1>q2) { 	
	do { 
	  p1-=PIpPI; 
	  p2-=PIpPI;
	}
	while( (p1>q2) );
      }
      else if(p2<q1) { 
	do { 
	  p1+=PIpPI; 
	  p2+=PIpPI;
	}
	while( (p2<q1) );	
      }
      if(p1<q1 && p2>q1) { 
	p1=q1;
      }
      if(p1<q2 && p2>q2) { 
	p2=q2;
      }
      
#if 0
      if(SolutionCircle[i].Binf!=p1 || SolutionCircle[i].Bsup!=p2) { 
	printf("\n IntCurve_IntConicConic_1.cxx : (%g , %g) --> (%g , %g)\n",
	       SolutionCircle[i].Binf,SolutionCircle[i].Bsup,p1,p2); 
      } 
#endif
      SolutionCircle[i].Binf=p1;
      SolutionCircle[i].Bsup=p2;
      
//-- Fin 7 aout 97

      
      Standard_Real Linf=(Opposite)? SolutionLine[i].Bsup : SolutionLine[i].Binf;
      Standard_Real Lsup=(Opposite)? SolutionLine[i].Binf : SolutionLine[i].Bsup;
      
      //---------------------------------------------------------------
      //-- Si les parametres sur le cercle sont en premier 
      //-- On doit retourner ces parametres dans l ordre croissant
      //---------------------------------------------------------------
      if(Linf > Lsup) {
	Standard_Real T=SolutionCircle[i].Binf;
	SolutionCircle[i].Binf=SolutionCircle[i].Bsup;
	SolutionCircle[i].Bsup=T;
	
	T=Linf; Linf=Lsup; Lsup=T;
      }
      
      
      ElCLib::CircleD2(SolutionCircle[i].Binf,CircleAxis,R,P1a,Tan1,Norm1); 
      ElCLib::LineD1(Linf,LineAxis,P2a,Tan2);
      
      IntImpParGen::DeterminePosition(Pos1a,CIRC_Domain,P1a,SolutionCircle[i].Binf);
      IntImpParGen::DeterminePosition(Pos2a,LIG_Domain,P2a,Linf); 
      Determine_Transition_LC(Pos1a,Tan1,Norm1,T1a , Pos2a,Tan2,Norm2,T2a, Tol);
      Standard_Real Cinf;
      if(Pos1a==IntRes2d_End) {
	Cinf = CIRC_Domain.LastParameter();
	P1a  = CIRC_Domain.LastPoint();
	Linf = ElCLib::Parameter(Line,P1a);
	
	ElCLib::CircleD2(Cinf,CircleAxis,R,P1a,Tan1,Norm1); 
	ElCLib::LineD1(Linf,LineAxis,P2a,Tan2);	
	IntImpParGen::DeterminePosition(Pos1a,CIRC_Domain,P1a,Cinf);
	IntImpParGen::DeterminePosition(Pos2a,LIG_Domain,P2a,Linf); 
	Determine_Transition_LC(Pos1a,Tan1,Norm1,T1a , Pos2a,Tan2,Norm2,T2a, Tol);
      }
      else if(Pos1a==IntRes2d_Head) { 
	Cinf = CIRC_Domain.FirstParameter();
	P1a  = CIRC_Domain.FirstPoint();
	Linf = ElCLib::Parameter(Line,P1a);
	
	ElCLib::CircleD2(Cinf,CircleAxis,R,P1a,Tan1,Norm1); 
	ElCLib::LineD1(Linf,LineAxis,P2a,Tan2);	
	IntImpParGen::DeterminePosition(Pos1a,CIRC_Domain,P1a,Cinf);
	IntImpParGen::DeterminePosition(Pos2a,LIG_Domain,P2a,Linf); 
	Determine_Transition_LC(Pos1a,Tan1,Norm1,T1a , Pos2a,Tan2,Norm2,T2a, Tol);
      }
      else { 
	Cinf=NormalizeOnCircleDomain(SolutionCircle[i].Binf,CIRC_Domain);
      }

      IntRes2d_IntersectionPoint NewPoint1(P1a,Linf,Cinf,T2a,T1a,ReversedParameters());
      
      if((SolutionLine[i].Length()+SolutionCircle[i].Length()) >0.0) {
	
	ElCLib::CircleD2(SolutionCircle[i].Bsup,CircleAxis,R,P1b,Tan1,Norm1); 
        ElCLib::LineD1(Lsup,LineAxis,P2b,Tan2);
	  
	IntImpParGen::DeterminePosition(Pos1b,CIRC_Domain,P1b,SolutionCircle[i].Bsup);
	IntImpParGen::DeterminePosition(Pos2b,LIG_Domain,P2b,Lsup);
	Determine_Transition_LC(Pos1b,Tan1,Norm1,T1b , Pos2b,Tan2,Norm2,T2b, Tol);
	Standard_Real Csup;
	if(Pos1b==IntRes2d_End) {
	  Csup = CIRC_Domain.LastParameter();
	  P1b  = CIRC_Domain.LastPoint();
	  Lsup = ElCLib::Parameter(Line,P1b);
	  ElCLib::CircleD2(Csup,CircleAxis,R,P1b,Tan1,Norm1); 
	  ElCLib::LineD1(Lsup,LineAxis,P2b,Tan2);
	  
	  IntImpParGen::DeterminePosition(Pos1b,CIRC_Domain,P1b,Csup);
	  IntImpParGen::DeterminePosition(Pos2b,LIG_Domain,P2b,Lsup);
	  Determine_Transition_LC(Pos1b,Tan1,Norm1,T1b , Pos2b,Tan2,Norm2,T2b, Tol);	  
	}
	else if(Pos1b==IntRes2d_Head) { 
	  Csup = CIRC_Domain.FirstParameter();
	  P1b  = CIRC_Domain.FirstPoint();
	  Lsup = ElCLib::Parameter(Line,P1b);
	  ElCLib::CircleD2(Csup,CircleAxis,R,P1b,Tan1,Norm1); 
	  ElCLib::LineD1(Lsup,LineAxis,P2b,Tan2);
	  
	  IntImpParGen::DeterminePosition(Pos1b,CIRC_Domain,P1b,Csup);
	  IntImpParGen::DeterminePosition(Pos2b,LIG_Domain,P2b,Lsup);
	  Determine_Transition_LC(Pos1b,Tan1,Norm1,T1b , Pos2b,Tan2,Norm2,T2b, Tol);	
	}
	else { 
	  Csup=NormalizeOnCircleDomain(SolutionCircle[i].Bsup,CIRC_Domain);
	}

	IntRes2d_IntersectionPoint NewPoint2(P1b,Lsup,Csup,T2b,T1b,ReversedParameters());
	
	if(((Abs(Csup-Cinf)*R >  MaxTol) && (Abs(Lsup-Linf) > MaxTol))
	   || (T1a.TransitionType() != T2a.TransitionType())) {  
	  //-- Verifier egalement les transitions 
	  
	  IntRes2d_IntersectionSegment NewSeg(NewPoint1,NewPoint2
					      ,Opposite,ReversedParameters());
	  Append(NewSeg);
	}
	else { 
	  if(Pos1a!=IntRes2d_Middle ||  Pos2a!=IntRes2d_Middle) { 
	    Insert(NewPoint1);
	  }
	  if(Pos1b!=IntRes2d_Middle ||  Pos2b!=IntRes2d_Middle) { 
	    Insert(NewPoint2);
	  }

	}
      }
      else {
        //--Standard_Real Cmid=NormalizeOnCircleDomain(0.5*(SolutionCircle[i].Bsup+SolutionCircle[i].Binf)
	//--					   ,CIRC_Domain);        
	//--IntRes2d_IntersectionPoint NewPoint(P2a,0.5*(Linf+Lsup)
	//--				    ,Cmid
	//--				    ,T2a,T1a,ReversedParameters());
	Insert(NewPoint1);
      }
    }
  }
}




const IntRes2d_IntersectionPoint SegmentToPoint( const IntRes2d_IntersectionPoint& Pa
						,const IntRes2d_Transition& T1a
						,const IntRes2d_Transition& T2a
						,const IntRes2d_IntersectionPoint& Pb
						,const IntRes2d_Transition& T1b
						,const IntRes2d_Transition& T2b) {  
  
  if((T1b.PositionOnCurve() == IntRes2d_Middle) 
     && (T2b.PositionOnCurve() == IntRes2d_Middle)) { 
    return(Pa);
  }
  if((T1a.PositionOnCurve() == IntRes2d_Middle) 
     && (T2a.PositionOnCurve() == IntRes2d_Middle)) { 
    return(Pb);
  }
  
  IntRes2d_Transition t1 = T1a;
  IntRes2d_Transition t2 = T2a;
  Standard_Real u1 = Pa.ParamOnFirst();
  Standard_Real u2 = Pa.ParamOnSecond();
  
  
  if(t1.PositionOnCurve() == IntRes2d_Middle) { 
    t1.SetPosition(T1b.PositionOnCurve());
    u1 = Pb.ParamOnFirst();
  }
  if(t2.PositionOnCurve() == IntRes2d_Middle) {
    t2.SetPosition(T2b.PositionOnCurve());
    u2 = Pb.ParamOnSecond();
  } 
  return(IntRes2d_IntersectionPoint(Pa.Value(),u1,u2,t1,t2,Standard_False));
}
