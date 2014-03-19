/* VMACHINE.CPP -- Copyright (c) 1999-2003 Julian Mensch
     Contains the virtual machine that executes compiled IncursionScript
   bytecode. This file includes the file "dispatch.h", which is generated
   by Incursion based on the IncursionScript file API.H. This is the core
   of the interface allowing IncursionScript code to call C++ functions.
     
     inline int32 VMachine::Value1(VCode *vc)
     inline int32 VMachine::Value2(VCode *vc)
     int32* VMachine::LValue1(VCode *vc)
     int32 VMachine::Execute(EventInfo *e, rID _xID, hCode CP)
     Object* VMachine::GetSystemObject(hObj h)

     (in dispatch.h)
     void VMachine::CallMemberFunc(int16 funcid, hObj h, int8 n)
     void VMachine::GetMemberVar(int16 varid, hObj h, int8 n)
     void VMachine::SetMemberVar(int16 varid, hObj h, int32 val)

*/
                                         
#include "Incursion.h"

#define CAST_RECT(r)  (*((Rect*)&r))
#define UNCAST_RECT(r)  (*((int32*)&r))

//#ifdef DEBUG
extern String & EventName(int16 Ev);
#define VERIFY(h,ty,funcname)                                         \
  if (ty != T_EVENTINFO && ty != T_TERM)                              \
    if (!oObject(h) || !(oObject(h)->isType(ty))) {                   \
      Error(Format("Invalid object referance (%s) calling %s in "     \
        "script routine $\"%s\"::%s.",                                \
        oObject(h) ? "object type mismatch" : "NULL object referance",\
        funcname,                                                     \
        xID ? ((const char*)NAME(xID)) : "<NULL>",                    \
        pe ? ((const char *)EventName(pe->Event)) : "???"));                          \
      return;                                                         \
      }
//#undef VERIFY
//#define VERIFY(h,ty,str) ;
#define MEMORY(a)       (*getMemorySafe(a))
#define STACK(a)        (*getStackSafe(Regs[63] - a))
#define VSTACK(a)       (*getStackSafe(max(0,Regs[63]-(a))))
#define MEMORY_ADDR(a)  (getMemorySafe(a))
#define STACK_ADDR(a)   (getStackSafe(Regs[63] - a))
#define GETSTR(ht)      (getStringSafe(ht))
#define REGS(a)         (*getRegSafe(a))
/*
#else
#define VERIFY(h,ty,str)
#define REGS(a)   (Regs[a])
#define MEMORY(a) (Memory[a])
#define STACK(a)  (Stack[Regs[63]-(a)])
#define VSTACK(a) (Stack[max(0,Regs[63]-(a))])
#define MEMORY_ADDR(a) (Memory + (a))
#define STACK_ADDR(a)  (Stack + (Regs[63]-(a))) 
#define GETSTR(ht)      (getStringSafe(ht))
//#define GETSTR(ht) ((ht) == 0 ? NULL : ( ht < 0 ? SRegs[max(0,-ht)] :       \
//                               (theGame->Modules[mn]->QTextSeg + (ht))))
#endif
*/
inline int16 LSCR()
  { return LastSkillCheckResult; }

inline void SystemBreak()
  { 
    BREAKOUT; 
  }

inline const char * SkillName(int16 sk)
  { for (int16 i=1;SkillInfo[i].name;i++)
      if (SkillInfo[i].sk == sk)
        return SkillInfo[i].name;
    return "[Invalid Skill]"; }
  
inline void SetEActor(EventInfo &e, Creature *a)
  { e.EActor = a; }
inline void SetETarget(EventInfo &e, Thing *t)
  { e.ETarget = t; }
inline void SetEVictim(EventInfo &e, Creature *v)
  { e.EVictim = v; }
inline void SetEItem(EventInfo &e, Item *it)
  { e.EItem = it; }
inline void SetEItem2(EventInfo &e, Item *it)
  { e.EItem2 = it; }

inline bool isResType(rID xID, int16 rt)
  { return (xID > 0x01000000) ? 
           (RES(xID)->isType(rt)) : false; }

inline void PrintRect(Rect r)
  { T1->Message(Format("Rect: x1 %d, y1 %d, x2 %d, y2 %d.",
                         r.x1, r.y1, r.x2, r.y2)); }
inline int16 PoisonDC(rID xID)
  { return TEFF(xID)->ef.sval; }
//inline int32 enFreaky()
//  { return enFreakFactor; }
  
inline bool mID_isMType(rID mID, uint32 mt)
  { return TMON(mID)->isMType(mID,mt); }
  
inline void SetPVal(EventInfo &e, int16 b, int16 n=0, int16 s=0)
  { if (e.EMagic) {
      e.EMagic->pval.Number = n;
      e.EMagic->pval.Sides = s;
      e.EMagic->pval.Bonus = b;
      }
  }

inline bool WithinRect(Rect r, int16 x, int16 y)
{
  return r.Within(x,y);
} 

/* We had to rename some functions in the script engine,
   to avoid conflicts with script engine keywords. Thus,
   the function Item::Weight() in the main program is the
   same as T_ITEM::ItemWeight() in the script engine. These
   defines kludge the autogenerated dispatch.h code accord-
   ingly. */
#define ItemWeight Weight
#define rnd random
#define ArmorPenalty Penalty
#define TFlags Flags
#define ieID eID
#define WriteXY Write
#define RIType IType
#define SetColor Color
#define MSize Size
#define CreateItem Item::Create
#define CreateMonster new Monster
#define CreateFeature new Feature
#define CreateDoor new Door
#define CreatePortal new Portal
#define CreateTrap new Trap

#define GenDungeonItem(fl,xID,depth,luck) Item::GenItem(fl,xID,depth,luck,DungeonItems)
#define GenChestItem(fl,xID,depth,luck) Item::GenItem(fl,xID,depth,luck,ChestItems)
#define DirX(a) DirX[a]
#define DirY(a) DirY[a]
#define eval EMagic->eval
#define dval EMagic->dval
#define aval EMagic->aval
#define tval EMagic->tval
#define qval EMagic->qval
#define sval EMagic->sval
#define lval EMagic->lval
#define cval EMagic->cval
#define xval EMagic->xval
#define yval EMagic->yval
#define rval EMagic->rval
#define pval EMagic->pval

/*
#define SetEFlag EMagic->SetFlag
#define UnsetEFlag EMagic->UnsetFlag
#define HasEFlag EMagic->HasFlag
*/
#define isEBlessed isBlessed
#define isECursed  isCursed
#define inField(fv) inField(fv) != NULL
#define DirToXY DirTo
#define TermUpdate Update

#define LastSkillCheckResult LSCR

#define AddCandidate(h) Candidates[nCandidates++] = h
#define GetCandidate(n) Candidates[n]
#define RandCandidate() Candidates[random(nCandidates)]
#define nCandidates() nCandidates

inline void ClearCandidates()
  { memset(Candidates,0,sizeof(hObj)*2048); nCandidates = 0; }

#ifdef WIN32
#include "dispatch.h"
#else
#include "dispatch.h"
#endif

#undef ItemWeight
#undef rnd
#undef ArmorPenalty
#undef TFlags
#undef isImmune
#undef isResist
#undef Ev
#undef ieID
#undef WriteXY
#undef SetColor
#undef CreateItem
#undef CreateMonster
#undef inField
#undef DirToXY
#undef DirX
#undef DirY
#undef TermUpdate

#ifndef  DISPATCH
void VMachine::CallMemberFunc(int16 funcid, hObj h, int8 n) { }
void VMachine::GetMemberVar(int16 varid, hObj h, int8 n)    { }
void VMachine::SetMemberVar(int16 varid, hObj h, int32 val) { }
#endif

int32  VMachine::Stack[8192];
int32  VMachine::Regs[64];
String VMachine::SRegs[64];
int32 *VMachine::Memory;
int32  VMachine::szMemory;
Breakpoint VMachine::Breakpoints[64];
int16  VMachine::nBreakpoints;

char CurrentRoutine[64];
extern String & EventName(int16 Ev);

int32* VMachine::getMemorySafe(int32 a)
  {
    static int32 scratch;
    if (a < 0)
      { Error(Format("%s: Invalid (negative) memory reference!",
                       CurrentRoutine));
        isTracing = true;
        return &scratch; }
    if (a >= szMemory)
      { Error(Format("%s: Invalid (out of bounds) memory reference!",
                       CurrentRoutine));
        isTracing = true;
        return &scratch; }
    return &(Memory[a]);
  }

int32* VMachine::getStackSafe(int32 a)
  {
    static int32 scratch;
    if (a < 0)
      { Error(Format("%s: Invalid (negative) stack reference!",
                       CurrentRoutine));
        isTracing = true;
        return &scratch; }
    if (a >= 8192)
      { Error(Format("%s: Invalid (out of bounds) stack reference!",
                       CurrentRoutine));
        isTracing = true;
        return &scratch; }
    return &(Stack[a]);  
  }

String & VMachine::getStringSafe(hText ht)
  {
    const char* scratch;
    if (ht >= 0)
      { 
        if (ht > theGame->Modules[0]->szTextSeg)
          { Error(Format("%s: Out of bounds text segment reference!",
                                CurrentRoutine));
            isTracing = true;
            scratch = "<badtext>";
            return *tmpstr(scratch); }
        scratch = (theGame->Modules[0]->QTextSeg + ht); 
        return *tmpstr(scratch);
      }
    if (ht <= -64)
      { Error(Format("%s: Invalid string register reference!",
                       CurrentRoutine));
        isTracing = true;
        scratch = "<badstr>";
        return *tmpstr(scratch); }
    return SRegs[-ht];
  }
  
int32* VMachine::getRegSafe(int32 n)
  {
    static int32 badReg;
    if (n < 0)
      { Error(Format("%s: Invalid (negative) register number!",
                       CurrentRoutine));
        isTracing = true;
        badReg = 0;
        return &badReg; }
    if (n >= 64)
      { Error(Format("%s: Invalid (above 63) register number!",
                       CurrentRoutine));
        isTracing = true;
        return &badReg; }
    return &(Regs[n]);  
  }
  
  
inline int32 VMachine::Value1(VCode *vc)
  {
    int32 pv;
    if (vc->P1Type & RT_EXTENDED)
      pv = *((int32*)(vc+1));
    else
      pv = vc->Param1;

    if((vc->P1Type & 3) == (RT_REGISTER + RT_MEMORY)) 
      return REGS(pv) >= 0 ? 
        MEMORY(REGS(pv)) :
        STACK(-REGS(pv));
    else if ((vc->P1Type & 3) == RT_REGISTER)
      return REGS(pv);
    else if ((vc->P1Type & 3) == RT_MEMORY)
      return pv >= 0 ? 
        MEMORY(pv) :
        STACK(-pv);
    else
      return pv;
  }

inline int32 VMachine::Value2(VCode *vc)
  {
    int32 pv;
    if (vc->P2Type & RT_EXTENDED) {
      if (vc->P1Type & RT_EXTENDED)
        pv = *((int32*)(vc+2));
      else
        pv = *((int32*)(vc+1));
      }
    else
      pv = vc->Param2;

    if((vc->P2Type & 3) == (RT_REGISTER + RT_MEMORY))
      return REGS(pv) >= 0 ? 
        MEMORY(REGS(pv)) :
        STACK(-REGS(pv));
    else if ((vc->P2Type & 3) == RT_REGISTER)
      return REGS(pv);
    else if ((vc->P2Type & 3) == RT_MEMORY)
      return pv >= 0 ? 
        MEMORY(pv) :
        STACK(-pv);
    else
      return pv;
  }

int32* VMachine::LValue1(VCode *vc)
  {
    int32 pv; 
    if (vc->P1Type & RT_EXTENDED)
      pv = *((int32*)(vc+1));
    else
      pv = vc->Param1;

    if((vc->P1Type & 3) == (RT_REGISTER + RT_MEMORY))
      return REGS(pv) >= 0 ? 
        MEMORY_ADDR(REGS(pv)) :
        STACK_ADDR(-REGS(pv));
    else if ((vc->P1Type & 3) == RT_REGISTER) {
      return &(REGS(pv));
    } else if ((vc->P1Type & 3) == RT_MEMORY)
      return pv >= 0 ? 
        MEMORY_ADDR(pv) :
        STACK_ADDR(-pv);
    else
      return MEMORY_ADDR(0);
  }

/*

String * VMachine::oString(hObj h)
  {                                 
    ASSERT(h > 64 && h < 128);
    return VStrings[h-64];
  }

*/

int32 VMachine::Execute(EventInfo *e, rID _xID, hCode CP)
  {
    int8 table_size, i; int32 val;


    // weimer: paranoia! -- incursion code seems to slowly reg[63] (the
    // stack pointer) as time goes by, eventually causing corruption. 
    // You can see the assertion marked (!!!) below eventually fail as
    // reg[63] gets up to 8192. Note that this will only happedn if you
    // comment out the assertions maked (1). 
    // 
    // Currently to prevent this from happening I reset reg_63 to whatever
    // it was when we came in. 
    int32 incoming_reg_63 = REGS(63); 
    
    if (theGame->InPlay())
      switch (theGame->Opt(OPT_NO_SCRIPTS))
        {
          case 0:
            break;
          case 1:
            return NOTHING;
          case 2:
            if (theGame->GetPlayer(0)) {
              if (e->EActor == theGame->GetPlayer(0))
                break;
              if (e->EVictim == theGame->GetPlayer(0))
                break;
              }
            return NOTHING;
        }
              
            
    
    #ifdef DEBUG
    /* We're doing this so when a script causes an access violation, we
       can quickly discover WHAT script did it. */
    if (e->Event != EV_ISTARGET && e->Event != EV_CRITERIA) {
      strcpy(CurrentRoutine,NAME(_xID));
      strcat(CurrentRoutine,"::");
      strcat(CurrentRoutine,EventName(e->Event));
      }
    #endif
    

    ASSERT(sizeof(int32) <= sizeof(VCode));
    xID = _xID;
    mn = RES(xID)->ModNum; 

    Memory   = (int32*)(theGame->MDataSeg[mn]);
    szMemory = theGame->MDataSegSize[mn];
    VCode *ovc, *vc = theGame->Modules[mn]->QCodeSeg + CP, *cs = vc;
    VCode *BreakStack[16]; int8 bsp = 0;
    vc--; isTracing = false;
    while ((++vc)->Opcode) {
      // weimer: paranoia! I've had crashes where the previous four
      // instrucitons were pushes and Reg[63] was 2.  
      ASSERT(REGS(63) >= 0); 
      ASSERT(REGS(63) < (sizeof(Stack)/sizeof(Stack[0]))); // !!!
      pe = e;

      for(i=0;i!=nBreakpoints;i++)
        if (Breakpoints[i].Location ==
              (vc - theGame->Modules[mn]->QCodeSeg))
          isTracing = true;
          
      if (isTracing)
        T1->TraceWindow(_xID,e->Event,this,
          (vc - theGame->Modules[mn]->QCodeSeg),true);

      switch(vc->Opcode)
        {
          case JUMP: vc += (Value1(vc)-1);              goto NextOpcode;
          case HALT: case LAST:
            REGS(63) = incoming_reg_63;
            ASSERT(REGS(63) == incoming_reg_63); /// (1)
            return NOTHING;
          case RET: 
          {
            int32 retval = Value1(vc);
            REGS(63) = incoming_reg_63;
            ASSERT(REGS(63) == incoming_reg_63); /// (1) 
            return retval; 
          }
          case ADD: REGS(0) = Value1(vc) + Value2(vc);  break;
          case SUB: REGS(0) = Value1(vc) - Value2(vc);  break;
          case MULT: REGS(0) = Value1(vc) * Value2(vc);  break;
          case DIV: REGS(0) = Value1(vc) / Value2(vc);  break;
          case MOD: REGS(0) = Value1(vc) % Value2(vc);  break;
          case INC: *LValue1(vc) += Value2(vc); break;
          case DEC: *LValue1(vc) -= Value2(vc); break;
          case ROLL: REGS(0) = Dice::Roll(Value1(vc),Value2(vc)); break;
          case MIN:  REGS(0) = min(Value1(vc),Value2(vc)); break;
          case MAX:  REGS(0) = max(Value1(vc),Value2(vc)); break;
          case MOV: *LValue1(vc) = Value2(vc);         break;
          case BSHL: REGS(0) = Value1(vc) >> Value2(vc); break;
          case BSHR: REGS(0) = Value1(vc) << Value2(vc); break;
          case JTRU: if (Value1(vc))  { vc += (Value2(vc)-1); goto NextOpcode; } break;
          case JFAL: if (!Value1(vc)) { vc += (Value2(vc)-1); goto NextOpcode; } break;
          case CMEQ: REGS(0) = (Value1(vc) == Value2(vc)); break;
          case CMNE: REGS(0) = (Value1(vc) != Value2(vc)); break;
          case CMGT: REGS(0) = (Value1(vc) > Value2(vc)); break;
          case CMLT: REGS(0) = (Value1(vc) < Value2(vc)); break;
          case CMLE: REGS(0) = (Value1(vc) <= Value2(vc)); break;
          case CMGE: REGS(0) = (Value1(vc) >= Value2(vc)); break;
          case CMEM: CallMemberFunc(Value2(vc),Value1(vc),0); xID = _xID; break;
          case GVAR: GetMemberVar(Value2(vc),Value1(vc),0); break;
          case SVAR: SetMemberVar(Value2(vc),Value1(vc),REGS(0)); break;
          case NOT: REGS(0) = !Value1(vc);          break;
          case NEG: REGS(0) = ~Value1(vc);          break;
          case BAND: REGS(0) = Value1(vc) & Value2(vc); break;
          case BOR:  REGS(0) = Value1(vc) | Value2(vc); break;
          case LAND: REGS(0) = Value1(vc) && Value2(vc); break;
          case LOR:  REGS(0) = Value1(vc) || Value2(vc); break;
          case RUN: /* Execute(vc+Value1); */   break;
          case PUSH: ASSERT(REGS(63) >= 0); 
                     ASSERT(REGS(63) < (sizeof(Stack)/sizeof(Stack[0]))); // !!!
                     Stack[REGS(63)++] = Value1(vc);   break;
          case POP:  REGS(63) -= Value1(vc);  break;
          case SBRK: BreakStack[bsp++] = vc + Value1(vc); ASSERT(bsp < 16); break;
          /* Note -- on Apr 6 2007 FJM commented out the assert(bsp) in
             FBRK (i.e., the break; statement) because it was triggering
             in the fountain script for reasons I don't have time to
             determine. We need to fix this later; I think it's an off-
             by-one bytecode error. On second thought, it may not be a
             bug but might instead relate to dropping through case statement
             leading to the default. The ASSERT was triggered by drinking
             from a fountain. */
          case FBRK: /*ASSERT(bsp);*/ if (bsp > 0) bsp--; break;
          case JBRK: if (bsp) vc = BreakStack[--bsp]; break;
          case JTAB: 
            val        = Value1(vc);
            table_size = Value2(vc);
            ovc = vc; vc++;
            //if (xID == 16778068 && e->Event == EV_EFFECT)
            //  __asm int 3;
            for(i=0;i!=table_size;i++,vc+=2)
              if (val == *((int32*)vc))
                {
                  vc = ovc + *((int32*)(vc+1));
                  goto NextOpcode;
                }
            vc = ovc + *((int32*)vc);
            goto NextOpcode;
           break;
          case ESTR: 
           ASSERT(Value1(vc) <= 0); 
           GETSTR(Value1(vc)).Empty(); break;
          case WSTR: 
           ASSERT(Value1(vc) <= 0); 
           GETSTR(Value1(vc)) = GETSTR(Value2(vc)); break;
          case ASTR: GETSTR(-1) =  GETSTR(Value1(vc));
                     GETSTR(-1) += GETSTR(Value2(vc)); break;
          case CSTR: REGS(0) = stricmp(GETSTR(Value1(vc)), 
                                 GETSTR(Value2(vc))); break;            
          default: 
            Error("Illegal opcode at address %d in routine $\"%s\"::%s!",
              vc-cs,xID ? NAME(xID) : "<NULL>", pe ? Lookup(EV_CONSTNAMES,
              pe->Event) : "???");   
           break;
        }
      if (vc->P1Type & RT_EXTENDED && vc->P2Type & RT_EXTENDED)
        vc += 2;
      else if (vc->P1Type & RT_EXTENDED || vc->P2Type & RT_EXTENDED)
        vc++;
      NextOpcode:;
      }
    Error("HALT opcode at address %d in routine $\"%s\"::%s!",
             vc-cs,xID ? NAME(xID) : "<NULL>", pe ? Lookup(EV_CONSTNAMES,
             pe->Event) : "???");   
    return DONE;
  }

Object* VMachine::GetSystemObject(hObj h)
  {
    switch(h) {
      case SOBJ_E:
        return (Object*)pe;
      case SOBJ_ETERM:
        return (Object*)(&T1);
      case SOBJ_EACTOR:
        return pe ? pe->EActor : Subject;
      case SOBJ_ETARGET:
      case SOBJ_EVICTIM:
        return pe ? pe->EVictim : Subject;
      case SOBJ_EITEM:
        return pe ? pe->EItem : Subject;
      case SOBJ_EITEM2:
        return pe ? pe->EItem2 : Subject;
      case SOBJ_EMAP:
        return pe ? (pe->EMap ? pe->EMap : pe->EActor->m) : 
          ((Thing*)Subject)->m;
      case SOBJ_GAME:
        return theGame;
      default:
        Error("Illegal system object number (%d)!",h);
        return pe ? pe->EActor : Subject;
      }
  }
          


