C
C  NUMERICAL MATHEMATICS AND COMPUTING, CHENEY/KINCAID, (c) 1985
C
C FILE: rk4sys.f
C
C RUNGE-KUTTA METHOD OF ORDER 4 FOR A SYSTEM OF ODE'S (RK4SYS,XPSYS)
C
      DOUBLE PRECISION X,T,H
      DIMENSION  X(3) 
      DATA  N/3/, T/0.0/, X/8.5,3.1,1.2/
      DATA  H/1E-10/, NSTEP/20000000/ 
      CALL RK4SYS(N,T,X,H,NSTEP)
      STOP
      END 
  
      SUBROUTINE XPSYS(X,F)
      DOUBLE PRECISION X,F
      DIMENSION  X(3),F(3)  
      F(1) = 10.0 * ( X(2) - X(1) )
      F(2) = 28.0 * X(1) - X(2) - X(1) * X(3)
      F(3) = X(1)*X(2) - (8.0/3.0) * X(3)
      RETURN
      END 

      SUBROUTINE RK4SYS(N,T,X,H,NSTEP)
      DOUBLE PRECISION X,Y,T,H,F1,F2,F3,F4
      DIMENSION  X(N),Y(N),F1(N),F2(N),F3(N),F4(N)  
      PRINT 7,T,(X(I),I=1,N)
      H2 = 0.5*H  
      START = T   
      DO 6 K = 1,NSTEP      
        CALL XPSYS(X,F1)    
        DO 2 I = 1,N
          Y(I) = X(I) + H2*F1(I)      
   2    CONTINUE
        CALL XPSYS(Y,F2)    
        DO 3 I = 1,N
          Y(I) = X(I) + H2*F2(I)      
   3    CONTINUE
        CALL XPSYS(Y,F3)    
        DO 4 I = 1,N
          Y(I) = X(I) + H*F3(I)       
   4    CONTINUE
        CALL XPSYS(Y,F4)    
        DO 5 I = 1,N
          X(I) = X(I) + H*(F1(I) + 2.0*(F2(I) + F3(I)) + F4(I))/6.0 
   5    CONTINUE
   6  CONTINUE
      PRINT 7,T,(X(I),I = 1,N)
   7  FORMAT(2X,'T,X:',E10.3,5(2X,E22.14))
      RETURN      
      END 

