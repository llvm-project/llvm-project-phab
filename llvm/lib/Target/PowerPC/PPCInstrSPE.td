//=======-- PPCInstrSPE.td - The PowerPC SPE Extension -*- tablegen -*-=======//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the Signal Processing Engine extension to
// the PowerPC instruction set.
//
//===----------------------------------------------------------------------===//

class EFXForm_1<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               I<4, OOL, IOL, asmstr, itin> {
  bits<5> RT;
  bits<5> RA;
  bits<5> RB;

  let Pattern = pattern;
  
  let Inst{6-10}  = RT;
  let Inst{11-15} = RA;
  let Inst{16-20} = RB;
  let Inst{21-31} = xo;
}

class EFXForm_2<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               EFXForm_1<xo, OOL, IOL, asmstr, itin, pattern> {
  let RB = 0;
}

class EFXForm_2a<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               EFXForm_1<xo, OOL, IOL, asmstr, itin, pattern> {
  let RA = 0;
}

class EFXForm_3<bits<11> xo, dag OOL, dag IOL, string asmstr,
              InstrItinClass itin> :
              I<4, OOL, IOL, asmstr, itin> {
  bits<3> crD;
  bits<5> RA;
  bits<5> RB;

  let Inst{6-8}  = crD;
  let Inst{9-10}  = 0;
  let Inst{11-15} = RA;
  let Inst{16-20} = RB;
  let Inst{21-31} = xo;
}

class EVXForm_1<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               I<4, OOL, IOL, asmstr, itin> {
  bits<5> RT;
  bits<5> RA;
  bits<5> RB;

  let Pattern = pattern;
  
  let Inst{6-10}  = RT;
  let Inst{11-15} = RA;
  let Inst{16-20} = RB;
  let Inst{21-31} = xo;
}

class EVXForm_2<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               EVXForm_1<xo, OOL, IOL, asmstr, itin, pattern> {
  let RB = 0;
}

class EVXForm_2a<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               EVXForm_1<xo, OOL, IOL, asmstr, itin, pattern> {
  let RA = 0;
}

class EVXForm_3<bits<11> xo, dag OOL, dag IOL, string asmstr,
              InstrItinClass itin, list<dag> pattern> :
              I<4, OOL, IOL, asmstr, itin> {
  bits<3> crD;
  bits<5> RA;
  bits<5> RB;

  let Pattern = pattern;
  
  let Inst{6-8}  = crD;
  let Inst{9-10}  = 0;
  let Inst{11-15} = RA;
  let Inst{16-20} = RB;
  let Inst{21-31} = xo;
}

class EVXForm_4<bits<8> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               I<4, OOL, IOL, asmstr, itin> {
  bits<3> crD;
  bits<5> RA;
  bits<5> RB;
  bits<5> RT;

  let Pattern = pattern;

  let Inst{6-10}  = RT;
  let Inst{11-15} = RA;
  let Inst{16-20} = RB;
  let Inst{21-28} = xo;
  let Inst{29-31} = crD;
}

class EVXForm_D<bits<11> xo, dag OOL, dag IOL, string asmstr,
               InstrItinClass itin, list<dag> pattern> :
               I<4, OOL, IOL, asmstr, itin> {
  bits<5> RT;
  bits<21> D;

  let Pattern = pattern;

  let Inst{6-10}  = RT;
  let Inst{20} = D{0};
  let Inst{19} = D{1};
  let Inst{18} = D{2};
  let Inst{17} = D{3};
  let Inst{16} = D{4};
  let Inst{15} = D{5};
  let Inst{14} = D{6};
  let Inst{13} = D{7};
  let Inst{12} = D{8};
  let Inst{11} = D{9};
  let Inst{11-20} = D{0-9};
  let Inst{21-31} = xo;
}

let DecoderNamespace = "SPE", Predicates = [HasSPE] in {

def BRINC          : EVXForm_1<527, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "brinc $RT, $RA, $RB", IIC_VecFP, []>;

// Double-precision floating point
def EFDABS         : EFXForm_2<740, (outs sperc:$RT), (ins sperc:$RA),
                                "efdabs $RT, $RA", IIC_FPGeneral,
                                [(set f64:$RT, (fabs f64:$RA))]>;

def EFDADD         : EFXForm_1<736, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                                "efdadd $RT, $RA, $RB", IIC_FPAddSub,
                                [(set f64:$RT, (fadd f64:$RA, f64:$RB))]>;

def EFDCFS         : EFXForm_2a<751, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efdcfs $RT, $RB", IIC_FPGeneral,
                                [(set f64:$RT, (fpextend f32:$RB))]>;

def EFDCFSF        : EFXForm_2a<755, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efdcfsf $RT, $RB", IIC_FPGeneral, []>;

def EFDCFSI        : EFXForm_2a<753, (outs sperc:$RT), (ins gprc:$RB),
                                "efdcfsi $RT, $RB", IIC_FPGeneral,
                                [(set f64:$RT, (fpextend (sint_to_fp i32:$RB)))]>;

def EFDCFUF        : EFXForm_2a<754, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efdcfuf $RT, $RB", IIC_FPGeneral, []>;

def EFDCFUI        : EFXForm_2a<752, (outs sperc:$RT), (ins gprc:$RB),
                                "efdctui $RT, $RB", IIC_FPGeneral,
                                [(set f64:$RT, (fpextend (uint_to_fp i32:$RB)))]>;

def EFDCMPEQ       : EFXForm_3<750, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdcmpeq $crD, $RA, $RB", IIC_FPCompare>;
def EFDCMPGT       : EFXForm_3<748, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdcmpgt $crD, $RA, $RB", IIC_FPCompare>;
def EFDCMPLT       : EFXForm_3<749, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdcmplt $crD, $RA, $RB", IIC_FPCompare>;

def EFDCTSF        : EFXForm_2a<759, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efdctsf $RT, $RB", IIC_FPGeneral, []>;

def EFDCTSI        : EFXForm_2a<757, (outs gprc:$RT), (ins sperc:$RB),
                                "efdctsi $RT, $RB", IIC_FPGeneral,
                                [(set i32:$RT, (fp_to_sint f64:$RB))]>;

def EFDCTSIZ       : EFXForm_2a<762, (outs gprc:$RT), (ins sperc:$RB),
                                "efdctsiz $RT, $RB", IIC_FPGeneral,
                                []>;

def EFDCTUF        : EFXForm_2a<758, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efdctuf $RT, $RB", IIC_FPGeneral, []>;

def EFDCTUI        : EFXForm_2a<756, (outs gprc:$RT), (ins sperc:$RB),
                                "efdctui $RT, $RB", IIC_FPGeneral,
                                [(set i32:$RT, (fp_to_uint f64:$RB))]>;

def EFDCTUIZ       : EFXForm_2a<760, (outs gprc:$RT), (ins sperc:$RB),
                                "efdctuiz $RT, $RB", IIC_FPGeneral,
                                []>;

def EFDDIV         : EFXForm_1<745, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "efddiv $RT, $RA, $RB", IIC_FPDivD,
                               [(set f64:$RT, (fdiv f64:$RA, f64:$RB))]>;

def EFDMUL         : EFXForm_1<744, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "efdmul $RT, $RA, $RB", IIC_FPGeneral,
                               [(set f64:$RT, (fmul f64:$RA, f64:$RB))]>;

def EFDNABS        : EFXForm_2<741, (outs sperc:$RT), (ins sperc:$RA),
                               "efsnabs $RT, $RA", IIC_FPGeneral,
                               [(set f64:$RT, (fneg (fabs f64:$RA)))]>;

def EFDNEG         : EFXForm_2<742, (outs sperc:$RT), (ins sperc:$RA),
                               "efdneg $RT, $RA", IIC_FPGeneral,
                               [(set f64:$RT, (fneg f64:$RA))]>;

def EFDSUB         : EFXForm_1<737, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "efdsub $RT, $RA, $RB", IIC_FPAddSub,
                               [(set f64:$RT, (fsub f64:$RA, f64:$RB))]>;

def EFDTSTEQ       : EFXForm_3<766, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdtsteq $crD, $RA, $RB", IIC_FPCompare>;
def EFDTSTGT       : EFXForm_3<764, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdtstgt $crD, $RA, $RB", IIC_FPCompare>;
def EFDTSTLT       : EFXForm_3<765, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efdtstlt $crD, $RA, $RB", IIC_FPCompare>;

// Single-precision floating point
def EFSABS         : EFXForm_2<708, (outs spe4rc:$RT), (ins spe4rc:$RA),
                                "efsabs $RT, $RA", IIC_FPGeneral,
                                [(set f32:$RT, (fabs f32:$RA))]>;

def EFSADD         : EFXForm_1<704, (outs spe4rc:$RT), (ins spe4rc:$RA, spe4rc:$RB),
                                "efsadd $RT, $RA, $RB", IIC_FPAddSub,
                                [(set f32:$RT, (fadd f32:$RA, f32:$RB))]>;

def EFSCFD         : EFXForm_2a<719, (outs spe4rc:$RT), (ins sperc:$RB),
                                "efscfd $RT, $RB", IIC_FPGeneral,
                                [(set f32:$RT, (fpround f64:$RB))]>;

def EFSCFSF        : EFXForm_2a<723, (outs spe4rc:$RT), (ins spe4rc:$RB),
                                "efscfsf $RT, $RB", IIC_FPGeneral, []>;

def EFSCFSI        : EFXForm_2a<721, (outs spe4rc:$RT), (ins gprc:$RB),
                                "efscfsi $RT, $RB", IIC_FPGeneral,
                                [(set f32:$RT, (sint_to_fp i32:$RB))]>;

def EFSCFUF        : EFXForm_2a<722, (outs spe4rc:$RT), (ins spe4rc:$RB),
                                "efscfuf $RT, $RB", IIC_FPGeneral, []>;

def EFSCFUI        : EFXForm_2a<720, (outs spe4rc:$RT), (ins gprc:$RB),
                                "efscfui $RT, $RB", IIC_FPGeneral,
                                [(set f32:$RT, (uint_to_fp i32:$RB))]>;

def EFSCMPEQ       : EFXForm_3<718, (outs crrc:$crD), (ins spe4rc:$RA, spe4rc:$RB),
                               "efscmpeq $crD, $RA, $RB", IIC_FPCompare>;
def EFSCMPGT       : EFXForm_3<716, (outs crrc:$crD), (ins spe4rc:$RA, spe4rc:$RB),
                               "efscmpgt $crD, $RA, $RB", IIC_FPCompare>;
def EFSCMPLT       : EFXForm_3<717, (outs crrc:$crD), (ins spe4rc:$RA, spe4rc:$RB),
                               "efscmplt $crD, $RA, $RB", IIC_FPCompare>;

def EFSCTSF        : EFXForm_2a<727, (outs spe4rc:$RT), (ins spe4rc:$RB),
                                "efsctsf $RT, $RB", IIC_FPGeneral, []>;

def EFSCTSI        : EFXForm_2a<725, (outs gprc:$RT), (ins spe4rc:$RB),
                                "efsctsi $RT, $RB", IIC_FPGeneral,
                                [(set i32:$RT, (fp_to_sint f32:$RB))]>;

def EFSCTSIZ       : EFXForm_2a<730, (outs gprc:$RT), (ins spe4rc:$RB),
                                "efsctsiz $RT, $RB", IIC_FPGeneral,
                                []>;

def EFSCTUF        : EFXForm_2a<726, (outs sperc:$RT), (ins spe4rc:$RB),
                                "efsctsf $RT, $RB", IIC_FPGeneral, []>;

def EFSCTUI        : EFXForm_2a<724, (outs gprc:$RT), (ins spe4rc:$RB),
                                "efsctui $RT, $RB", IIC_FPGeneral,
                                [(set i32:$RT, (fp_to_uint f32:$RB))]>;

def EFSCTUIZ       : EFXForm_2a<728, (outs gprc:$RT), (ins spe4rc:$RB),
                                "efsctsiz $RT, $RB", IIC_FPGeneral,
                                []>;

def EFSDIV         : EFXForm_1<713, (outs spe4rc:$RT), (ins spe4rc:$RA, spe4rc:$RB),
                               "efsdiv $RT, $RA, $RB", IIC_FPDivD,
                               [(set f32:$RT, (fdiv f32:$RA, f32:$RB))]>;

def EFSMUL         : EFXForm_1<712, (outs spe4rc:$RT), (ins spe4rc:$RA, spe4rc:$RB),
                               "efsmul $RT, $RA, $RB", IIC_FPGeneral,
                               [(set f32:$RT, (fmul f32:$RA, f32:$RB))]>;

def EFSNABS        : EFXForm_2<709, (outs spe4rc:$RT), (ins spe4rc:$RA),
                               "efsnabs $RT, $RA", IIC_FPGeneral,
                               [(set f32:$RT, (fneg (fabs f32:$RA)))]>;

def EFSNEG         : EFXForm_2<710, (outs spe4rc:$RT), (ins spe4rc:$RA),
                               "efsneg $RT, $RA", IIC_FPGeneral,
                               [(set f32:$RT, (fneg f32:$RA))]>;

def EFSSUB         : EFXForm_1<705, (outs spe4rc:$RT), (ins spe4rc:$RA, spe4rc:$RB),
                               "efssub $RT, $RA, $RB", IIC_FPAddSub,
                               [(set f32:$RT, (fsub f32:$RA, f32:$RB))]>;

def EFSTSTEQ       : EFXForm_3<734, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efststeq $crD, $RA, $RB", IIC_FPCompare>;
def EFSTSTGT       : EFXForm_3<732, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efststgt $crD, $RA, $RB", IIC_FPCompare>;
def EFSTSTLT       : EFXForm_3<735, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "efststlt $crD, $RA, $RB", IIC_FPCompare>;

// SPE Vector operations

def EVABS          : EVXForm_2<520, (outs sperc:$RT), (ins sperc:$RA),
                               "evabs $RT, $RA", IIC_VecFP,
                               [(set v2i32:$RT, (abs v2i32:$RA))]>;

def EVADDIW        : EVXForm_1<514, (outs sperc:$RT), (ins sperc:$RA, u5imm:$RB),
                               "evaddiw $RT, $RB, $RA", IIC_VecFP, []>;
def EVADDSMIAAW    : EVXForm_2<1225, (outs sperc:$RT), (ins sperc:$RA),
                               "evaddsmiaaw $RT, $RA", IIC_VecFP, []>;
def EVADDSSIAAW    : EVXForm_2<1217, (outs sperc:$RT), (ins sperc:$RA),
                               "evaddssiaaw $RT, $RA", IIC_VecFP, []>;
def EVADDUSIAAW    : EVXForm_2<1216, (outs sperc:$RT), (ins sperc:$RA),
                               "evaddusiaaw $RT, $RA", IIC_VecFP, []>;
def EVADDUMIAAW    : EVXForm_2<1224, (outs sperc:$RT), (ins sperc:$RA),
                               "evaddumiaaw $RT, $RA", IIC_VecFP, []>;
def EVADDW         : EVXForm_1<512, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evaddw $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (add v2i32:$RA, v2i32:$RB))]>;

def EVAND          : EVXForm_1<529, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evand $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (and v2i32:$RA, v2i32:$RB))]>;
def EVANDC         : EVXForm_1<530, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evandc $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (and v2i32:$RA, (not v2i32:$RB)))]>;

def EVCMPEQ        : EVXForm_3<564, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evcmpeq $crD, $RA, $RB", IIC_VecFP, []>;
def EVCMPGTS       : EVXForm_3<561, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evcmpgts $crD, $RA, $RB", IIC_VecFP, []>;
def EVCMPGTU       : EVXForm_3<560, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evcmpgtu $crD, $RA, $RB", IIC_VecFP, []>;
def EVCMPLTS       : EVXForm_3<563, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evcmplts $crD, $RA, $RB", IIC_VecFP, []>;
def EVCMPLTU       : EVXForm_3<562, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evcmpltu $crD, $RA, $RB", IIC_VecFP, []>;

def EVCNTLSW       : EVXForm_2<526, (outs sperc:$RT), (ins sperc:$RA),
                               "evcntlsw $RT, $RA", IIC_VecFP, []>;
def EVCNTLZW       : EVXForm_2<525, (outs sperc:$RT), (ins sperc:$RA),
                               "evcntlzw $RT, $RA", IIC_VecFP,
                               [(set v2i32:$RT, (ctlz v2i32:$RA))]>;

def EVDIVWS        : EVXForm_1<1222, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evdivws $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (sdiv v2i32:$RA, v2i32:$RB))]>;
def EVDIVWU        : EVXForm_1<1223, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evdivwu $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (udiv v2i32:$RA, v2i32:$RB))]>;

def EVEQV          : EVXForm_1<537, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "eveqv $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT,
                                 (not (xor v2i32:$RA, v2i32:$RB)))]>;

def EVEXTSB        : EVXForm_2<522, (outs sperc:$RT), (ins sperc:$RA),
                               "evextsb $RT, $RA", IIC_VecFP,
                               []>;
def EVEXTSH        : EVXForm_2<523, (outs sperc:$RT), (ins sperc:$RA),
                               "evextsh $RT, $RA", IIC_VecFP,
                               []>;

def EVFSABS        : EVXForm_2<644, (outs sperc:$RT), (ins sperc:$RA),
                               "evfsabs $RT, $RA", IIC_VecFP,
                               [(set v2f32:$RT, (fabs v2f32:$RA))]>;
def EVFSADD        : EVXForm_1<640, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evfsadd $RT, $RA, $RB", IIC_VecFP,
                               [(set v2f32:$RT, (fadd v2f32:$RA, v2f32:$RB))]>;
def EVFSCFSF       : EVXForm_2a<659, (outs sperc:$RT), (ins sperc:$RB),
                                "evfscfsf $RT, $RB", IIC_VecFP, []>;
def EVFSCFSI       : EVXForm_2a<657, (outs sperc:$RT), (ins sperc:$RB),
                                "evfscfsi $RT, $RB", IIC_VecFP,
                                [(set v2f32:$RT, (sint_to_fp v2i32:$RB))]>;
def EVFSCFUF       : EVXForm_2a<658, (outs sperc:$RT), (ins sperc:$RB),
                                "evfscfuf $RT, $RB", IIC_VecFP, []>;
def EVFSCFUI       : EVXForm_2a<650, (outs sperc:$RT), (ins sperc:$RB),
                                "evfscfui $RT, $RB", IIC_VecFP,
                                [(set v2f32:$RT, (uint_to_fp v2i32:$RB))]>;
def EVFSCMPEQ      : EVXForm_3<654, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfscmpeq $crD, $RA, $RB", IIC_FPCompare, []>;
def EVFSCMPGT      : EVXForm_3<652, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfscmpgt $crD, $RA, $RB", IIC_FPCompare, []>;
def EVFSCMPLT      : EVXForm_3<653, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfscmplt $crD, $RA, $RB", IIC_FPCompare, []>;
def EVFSCTSF        : EVXForm_2a<663, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctsf $RT, $RB", IIC_FPGeneral, []>;
def EVFSCTSI        : EVXForm_2a<661, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctsi $RT, $RB", IIC_FPGeneral,
                                [(set v2i32:$RT, (fp_to_sint v2f32:$RB))]>;
def EVFSCTSIZ       : EVXForm_2a<666, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctsiz $RT, $RB", IIC_FPGeneral,
                                []>;
def EVFSCTUF        : EVXForm_2a<662, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctsf $RT, $RB", IIC_FPGeneral, []>;
def EVFSCTUI        : EVXForm_2a<660, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctui $RT, $RB", IIC_FPGeneral,
                                [(set v2i32:$RT, (fp_to_uint v2f32:$RB))]>;
def EVFSCTUIZ       : EVXForm_2a<664, (outs sperc:$RT), (ins sperc:$RB),
                                "evfsctsiz $RT, $RB", IIC_FPGeneral,
                                []>;
def EVFSDIV         : EVXForm_1<649, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evfsdiv $RT, $RA, $RB", IIC_FPDivD,
                               [(set v2f32:$RT, (fdiv v2f32:$RA, v2f32:$RB))]>;
def EVFSMUL         : EVXForm_1<648, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evfsmul $RT, $RA, $RB", IIC_FPGeneral,
                               [(set v2f32:$RT, (fmul v2f32:$RA, v2f32:$RB))]>;
def EVFSNABS        : EVXForm_2<645, (outs sperc:$RT), (ins sperc:$RA),
                               "evfsnabs $RT, $RA", IIC_FPGeneral,
                               [(set v2f32:$RT, (fneg (fabs v2f32:$RA)))]>;
def EVFSNEG         : EVXForm_2<646, (outs sperc:$RT), (ins sperc:$RA),
                               "evfsneg $RT, $RA", IIC_FPGeneral,
                               [(set v2f32:$RT, (fneg v2f32:$RA))]>;
def EVFSSUB         : EVXForm_1<641, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evfssub $RT, $RA, $RB", IIC_FPAddSub,
                               [(set v2f32:$RT, (fsub v2f32:$RA, v2f32:$RB))]>;
def EVFSTSTEQ       : EVXForm_3<670, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfststeq $crD, $RA, $RB", IIC_FPCompare, []>;
def EVFSTSTGT       : EVXForm_3<668, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfststgt $crD, $RA, $RB", IIC_FPCompare, []>;
def EVFSTSTLT       : EVXForm_3<671, (outs crrc:$crD), (ins sperc:$RA, sperc:$RB),
                               "evfststlt $crD, $RA, $RB", IIC_FPCompare, []>;

def EVLDD          : EVXForm_D<769, (outs sperc:$RT), (ins spe8dis:$dst),
                               "evldd $RT, $dst", IIC_VecFP,
                               [(set f64:$RT, (load ixaddr:$dst))]>;
def EVLDDX         : EVXForm_1<768, (outs sperc:$RT), (ins memrr:$src),
                               "evlddx $RT, $src", IIC_VecFP,
                               [(set f64:$RT, (load xaddr:$src))]>;
def EVLDH          : EVXForm_D<773, (outs sperc:$RT), (ins spe8dis:$dst),
                               "evldh $RT, $dst", IIC_VecFP, []>;
def EVLDHX         : EVXForm_1<772, (outs sperc:$RT), (ins memrr:$src),
                               "evldhx $RT, $src", IIC_VecFP, []>;
def EVLDW          : EVXForm_D<771, (outs sperc:$RT), (ins spe8dis:$dst),
                               "evldw $RT, $dst", IIC_VecFP, []>;
def EVLDWX         : EVXForm_1<770, (outs sperc:$RT), (ins memrr:$src),
                               "evldwx $RT, $src", IIC_VecFP, []>;
def EVLHHESPLAT    : EVXForm_D<777, (outs sperc:$RT), (ins spe2dis:$dst),
                               "evlhhesplat $RT, $dst", IIC_VecFP, []>;
def EVLHHESPLATX   : EVXForm_1<776, (outs sperc:$RT), (ins memrr:$src),
                               "evlhhesplatx $RT, $src", IIC_VecFP, []>;
def EVLHHOUSPLAT   : EVXForm_D<781, (outs sperc:$RT), (ins spe2dis:$dst),
                               "evlhhousplat $RT, $dst", IIC_VecFP, []>;
def EVLHHOUSPLATX  : EVXForm_1<780, (outs sperc:$RT), (ins memrr:$src),
                               "evlhhousplatx $RT, $src", IIC_VecFP, []>;
def EVLHHOSSPLAT   : EVXForm_D<783, (outs sperc:$RT), (ins spe2dis:$dst),
                               "evlhhossplat $RT, $dst", IIC_VecFP, []>;
def EVLHHOSSPLATX  : EVXForm_1<782, (outs sperc:$RT), (ins memrr:$src),
                               "evlhhossplatx $RT, $src", IIC_VecFP, []>;
def EVLWHE         : EVXForm_D<785, (outs sperc:$RT), (ins spe4dis:$dst),
                               "evlwhe $RT, $dst", IIC_VecFP, []>;
def EVLWHEX        : EVXForm_1<784, (outs sperc:$RT), (ins memrr:$src),
                               "evlwhex $RT, $src", IIC_VecFP, []>;
def EVLWHOS        : EVXForm_D<791, (outs sperc:$RT), (ins spe4dis:$dst),
                               "evlwhos $RT, $dst", IIC_VecFP, []>;
def EVLWHOSX       : EVXForm_1<790, (outs sperc:$RT), (ins memrr:$src),
                               "evlwhosx $RT, $src", IIC_VecFP, []>;
def EVLWHOU        : EVXForm_D<789, (outs sperc:$RT), (ins spe4dis:$dst),
                               "evlwhou $RT, $dst", IIC_VecFP, []>;
def EVLWHOUX       : EVXForm_1<788, (outs sperc:$RT), (ins memrr:$src),
                               "evlwhoux $RT, $src", IIC_VecFP, []>;
def EVLWHSPLAT     : EVXForm_D<797, (outs sperc:$RT), (ins spe4dis:$dst),
                               "evlwhsplat $RT, $dst", IIC_VecFP, []>;
def EVLWHSPLATX    : EVXForm_1<796, (outs sperc:$RT), (ins memrr:$src),
                               "evlwhsplatx $RT, $src", IIC_VecFP, []>;
def EVLWWSPLAT     : EVXForm_D<793, (outs sperc:$RT), (ins spe4dis:$dst),
                               "evlwwsplat $RT, $dst", IIC_VecFP, []>;
def EVLWWSPLATX    : EVXForm_1<792, (outs sperc:$RT), (ins memrr:$src),
                               "evlwwsplatx $RT, $src", IIC_VecFP, []>;

def EVMERGEHI      : EVXForm_1<556, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmergehi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMERGELO      : EVXForm_1<557, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmergelo $RT, $RA, $RB", IIC_VecFP, []>;
def EVMERGEHILO    : EVXForm_1<558, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmergehilo $RT, $RA, $RB", IIC_VecFP, []>;
def EVMERGELOHI    : EVXForm_1<559, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmergelohi $RT, $RA, $RB", IIC_VecFP, []>;

def EVMHEGSMFAA    : EVXForm_1<1323, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegsmfaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEGSMFAN    : EVXForm_1<1451, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegsmfan $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEGSMIAA    : EVXForm_1<1321, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegsmiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEGSMIAN    : EVXForm_1<1449, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegsmian $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEGUMIAA    : EVXForm_1<1320, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegumiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEGUMIAN    : EVXForm_1<1448, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhegumian $RT, $RA, $RB", IIC_VecFP, []>;

def EVMHESMF       : EVXForm_1<1035, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMFA      : EVXForm_1<1067, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMFAAW    : EVXForm_1<1291, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmfaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMFANW    : EVXForm_1<1419, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmfanw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMI       : EVXForm_1<1033, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMIA      : EVXForm_1<1065, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMIAAW    : EVXForm_1<1289, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESMIANW    : EVXForm_1<1417, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhesmianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSF       : EVXForm_1<1027, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSFA      : EVXForm_1<1059, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSFAAW    : EVXForm_1<1283, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessfaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSFANW    : EVXForm_1<1411, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessfanw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSIAAW    : EVXForm_1<1281, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHESSIANW    : EVXForm_1<1409, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhessianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUMI       : EVXForm_1<1032, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheumi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUMIA      : EVXForm_1<1064, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheumia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUMIAAW    : EVXForm_1<1288, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheumiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUMIANW    : EVXForm_1<1416, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheumianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUSIAAW    : EVXForm_1<1280, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheusiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHEUSIANW    : EVXForm_1<1408, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmheusianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGSMFAA    : EVXForm_1<1327, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogsmfaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGSMFAN    : EVXForm_1<1455, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogsmfan $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGSMIAA    : EVXForm_1<1325, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogsmiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGSMIAN    : EVXForm_1<1453, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogsmian $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGUMIAA    : EVXForm_1<1324, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogumiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOGUMIAN    : EVXForm_1<1452, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhogumian $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMF       : EVXForm_1<1039, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMFA      : EVXForm_1<1071, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMFAAW    : EVXForm_1<1295, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmfaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMFANW    : EVXForm_1<1423, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmfanw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMI       : EVXForm_1<1037, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMIA      : EVXForm_1<1069, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMIAAW    : EVXForm_1<1293, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSMIANW    : EVXForm_1<1421, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhosmianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSF       : EVXForm_1<1031, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSFA      : EVXForm_1<1063, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSFAAW    : EVXForm_1<1287, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossfaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSFANW    : EVXForm_1<1415, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossfanw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSIAAW    : EVXForm_1<1285, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOSSIANW    : EVXForm_1<1413, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhossianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUMI       : EVXForm_1<1036, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhoumi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUMIA      : EVXForm_1<1068, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhoumia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUMIAAW    : EVXForm_1<1292, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhoumiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUMIANW    : EVXForm_1<1420, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhoumianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUSIAAW    : EVXForm_1<1284, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhousiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMHOUSIANW    : EVXForm_1<1412, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmhousianw $RT, $RA, $RB", IIC_VecFP, []>;

def EVMRA          : EVXForm_2<1220, (outs sperc:$RT), (ins sperc:$RA),
                               "evmra $RT, $RA", IIC_VecFP, []>;

def EVMWHSMF       : EVXForm_1<1103, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhsmf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHSMFA      : EVXForm_1<1135, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhsmfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHSMI       : EVXForm_1<1101, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhsmi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHSMIA      : EVXForm_1<1133, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhsmia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHSSF       : EVXForm_1<1095, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhssf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHSSFA      : EVXForm_1<1127, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhssfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHUMI       : EVXForm_1<1100, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhumi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWHUMIA      : EVXForm_1<1132, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwhumia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLSMIAAW    : EVXForm_1<1353, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlsmiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLSMIANW    : EVXForm_1<1481, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlsmianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLSSIAAW    : EVXForm_1<1345, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlssiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLSSIANW    : EVXForm_1<1473, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlssianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUMI       : EVXForm_1<1096, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlumi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUMIA      : EVXForm_1<1128, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlumia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUMIAAW    : EVXForm_1<1352, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlumiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUMIANW    : EVXForm_1<1480, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlumianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUSIAAW    : EVXForm_1<1344, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlusiaaw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWLUSIANW    : EVXForm_1<1472, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwlusianw $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMF        : EVXForm_1<1115, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMFA       : EVXForm_1<1147, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMFAA      : EVXForm_1<1371, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmfaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMFAN      : EVXForm_1<1499, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmfan $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMI        : EVXForm_1<1113, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMIA       : EVXForm_1<1145, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMIAA      : EVXForm_1<1369, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSMIAN      : EVXForm_1<1497, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwsmian $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSSF        : EVXForm_1<1107, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwssf $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSSFA       : EVXForm_1<1139, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwssfa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSSFAA      : EVXForm_1<1363, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwssfaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWSSFAN      : EVXForm_1<1491, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwssfan $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWUMI        : EVXForm_1<1112, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwumi $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWUMIA       : EVXForm_1<1144, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwumia $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWUMIAA      : EVXForm_1<1368, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwumiaa $RT, $RA, $RB", IIC_VecFP, []>;
def EVMWUMIAN      : EVXForm_1<1496, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evmwumian $RT, $RA, $RB", IIC_VecFP, []>;


def EVNAND         : EVXForm_1<542, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evnand $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (not (and v2i32:$RA, v2i32:$RB)))]>;

def EVNEG          : EVXForm_2<521, (outs sperc:$RT), (ins sperc:$RA),
                               "evneg $RT, $RA", IIC_VecFP,
                               [(set v2i32:$RT, (ineg v2i32:$RA))]>;

def EVNOR          : EVXForm_1<536, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evnor $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (not (or v2i32:$RA, v2i32:$RB)))]>;
def EVOR           : EVXForm_1<535, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evor $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (or v2i32:$RA, v2i32:$RB))]>;
def EVORC          : EVXForm_1<539, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evorc $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (or v2i32:$RA, (not v2i32:$RB)))]>;

def EVRLWI         : EVXForm_1<554, (outs sperc:$RT), (ins sperc:$RA, u5imm:$RB),
                               "evrlwi $RT, $RA, $RB", IIC_VecFP, []>;
def EVRLW          : EVXForm_1<552, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evrlw $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (rotl v2i32:$RA, v2i32:$RB))]>;

def EVRNDW         : EVXForm_2<524, (outs sperc:$RT), (ins sperc:$RA),
                               "evrndw $RT, $RA", IIC_VecFP, []>;

def EVSEL          : EVXForm_4<79, (outs sperc:$RT),
                               (ins sperc:$RA, sperc:$RB, crrc:$crD),
                               "evsel crD,$RT,$RA,$RB", IIC_VecFP, []>;

def EVSLWI         : EVXForm_1<550, (outs sperc:$RT), (ins sperc:$RA, u5imm:$RB),
                               "evslwi $RT, $RA, $RB", IIC_VecFP, []>;
def EVSLW          : EVXForm_1<548, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evslw $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (shl v2i32:$RA, v2i32:$RB))]>;

def EVSPLATFI      : EVXForm_2<555, (outs sperc:$RT), (ins i32imm:$RA),
                               "evsplatfi $RT, $RA", IIC_VecFP, []>;
def EVSPLATI       : EVXForm_2<553, (outs sperc:$RT), (ins i32imm:$RA),
                               "evsplati $RT, $RA", IIC_VecFP, []>;

def EVSRWIS        : EVXForm_1<547, (outs sperc:$RT), (ins sperc:$RA, u5imm:$RB),
                               "evsrwis $RT, $RA, $RB", IIC_VecFP, []>;
def EVSRWIU        : EVXForm_1<546, (outs sperc:$RT), (ins sperc:$RA, u5imm:$RB),
                               "evsrwiu $RT, $RA, $RB", IIC_VecFP, []>;
def EVSRWS         : EVXForm_1<545, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evsrws $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (sra v2i32:$RA, v2i32:$RB))]>;
def EVSRWU         : EVXForm_1<544, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evsrwu $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (srl v2i32:$RA, v2i32:$RB))]>;

def EVSTDD         : EVXForm_D<801, (outs), (ins sperc:$RT, spe8dis:$dst),
                               "evstdd $RT, $dst", IIC_VecFP,
                               [(store f64:$RT, ixaddr:$dst)]>;
def EVSTDDX        : EVXForm_1<800, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstddx $RT, $dst", IIC_VecFP,
                               [(store f64:$RT, xaddr:$dst)]>;
def EVSTDH         : EVXForm_D<805, (outs), (ins sperc:$RT, spe8dis:$dst),
                               "evstdh $RT, $dst", IIC_VecFP, []>;
def EVSTDHX        : EVXForm_1<804, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstdhx $RT, $dst", IIC_VecFP, []>;
def EVSTDW         : EVXForm_D<803, (outs), (ins sperc:$RT, spe8dis:$dst),
                               "evstdw $RT, $dst", IIC_VecFP, []>;
def EVSTDWX        : EVXForm_1<802, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstdwx $RT, $dst", IIC_VecFP, []>;
def EVSTWHE        : EVXForm_D<817, (outs), (ins sperc:$RT, spe4dis:$dst),
                               "evstwhe $RT, $dst", IIC_VecFP, []>;
def EVSTWHEX       : EVXForm_1<816, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstwhex $RT, $dst", IIC_VecFP, []>;
def EVSTWHO        : EVXForm_D<821, (outs), (ins sperc:$RT, spe4dis:$dst),
                               "evstwho $RT, $dst", IIC_VecFP, []>;
def EVSTWHOX       : EVXForm_1<820, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstwhox $RT, $dst", IIC_VecFP, []>;
def EVSTWWE        : EVXForm_D<825, (outs), (ins sperc:$RT, spe4dis:$dst),
                               "evstwwe $RT, $dst", IIC_VecFP, []>;
def EVSTWWEX       : EVXForm_1<824, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstwwex $RT, $dst", IIC_VecFP, []>;
def EVSTWWO        : EVXForm_D<829, (outs), (ins sperc:$RT, spe4dis:$dst),
                               "evstwwo $RT, $dst", IIC_VecFP, []>;
def EVSTWWOX       : EVXForm_1<828, (outs), (ins sperc:$RT, memrr:$dst),
                               "evstwwox $RT, $dst", IIC_VecFP, []>;

def EVSUBFSSIAAW   : EVXForm_2<1219, (outs sperc:$RT), (ins sperc:$RA),
                               "evsubfssiaaw $RT, $RA", IIC_VecFP, []>;
def EVSUBFSMIAAW   : EVXForm_2<1227, (outs sperc:$RT), (ins sperc:$RA),
                               "evsubfsmiaaw $RT, $RA", IIC_VecFP, []>;
def EVSUBFUMIAAW   : EVXForm_2<1226, (outs sperc:$RT), (ins sperc:$RA),
                               "evsubfumiaaw $RT, $RA", IIC_VecFP, []>;
def EVSUBFUSIAAW   : EVXForm_2<1218, (outs sperc:$RT), (ins sperc:$RA),
                               "evsubfusiaaw $RT, $RA", IIC_VecFP, []>;
def EVSUBFW        : EVXForm_1<516, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evsubfw $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (sub v2i32:$RB, v2i32:$RA))]>;
def EVSUBIFW       : EVXForm_1<518, (outs sperc:$RT), (ins u5imm:$RA, sperc:$RB),
                               "evsubifw $RT, $RA, $RB", IIC_VecFP, []>;
def EVXOR          : EVXForm_1<534, (outs sperc:$RT), (ins sperc:$RA, sperc:$RB),
                               "evxor $RT, $RA, $RB", IIC_VecFP,
                               [(set v2i32:$RT, (xor v2i32:$RA, v2i32:$RB))]>;

let isAsmParserOnly = 1 in {
// Identical to the integer Load/Stores, but to handle floats
def SPELWZ        : DForm_1<32, (outs spe4rc:$rD), (ins memri:$src),
                            "lwz $rD, $src", IIC_LdStLoad,
                            [(set f32:$rD, (load iaddr:$src))]>;
def SPELWZX       : XForm_1<31,  23, (outs spe4rc:$rD), (ins memrr:$src),
                            "lwzx $rD, $src", IIC_LdStLoad,
                            [(set f32:$rD, (load xaddr:$src))]>;
def SPESTW        : DForm_1<36, (outs), (ins spe4rc:$rS, memri:$src),
                            "stw $rS, $src", IIC_LdStStore,
                            [(store f32:$rS, iaddr:$src)]>;
def SPESTWX       : XForm_8<31, 151, (outs), (ins spe4rc:$rS, memrr:$dst),
                           "stwx $rS, $dst", IIC_LdStStore,
                           [(store f32:$rS, xaddr:$dst)]>;
}

} // HasSPE

let Predicates = [HasSPE] in {
def : Pat<(f64 (extloadf32 iaddr:$src)),
          (COPY_TO_REGCLASS (SPELWZ iaddr:$src), SPERC)>;
def : Pat<(f64 (extloadf32 xaddr:$src)),
          (COPY_TO_REGCLASS (SPELWZX xaddr:$src), SPERC)>;

def : Pat<(f64 (fpextend f32:$src)),
          (COPY_TO_REGCLASS $src, SPERC)>;
}
