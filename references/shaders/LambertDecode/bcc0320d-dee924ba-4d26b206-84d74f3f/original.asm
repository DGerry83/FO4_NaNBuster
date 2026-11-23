Shader hash bcc0320d-dee924ba-4d26b206-84d74f3f

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[30], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_input_ps_siv linear noperspective v0.xy, position
      dcl_output o0.xyzw
      dcl_output o1.xyzw
      dcl_temps 7
   0: mul r0.xy, v0.xyxx, cb2[22].xyxx
   1: mul r0.xy, r0.xyxx, cb2[0].xyxx
   2: mul r1.x, v0.x, cb2[0].x
   3: deriv_rtx_coarse r0.z, r0.x
   4: deriv_rty_coarse r0.w, r0.y
   5: sample_d(texture2d)(float,float,float,float) r0.z, r0.xyxx, t3.yzxw, s3, r0.zzzz, r0.wwww
   6: ge r0.w, l(0.010000), r0.z
   7: if_nz r0.w
   8:   mul r2.z, r0.z, l(100.000000)
   9:   mov r3.xyzw, cb12[24].xyzw
  10:   mov r4.xyzw, cb12[25].xyzw
  11:   mov r5.xyzw, cb12[26].xyzw
  12:   mov r6.xyzw, cb12[27].xyzw
  13: else
  14:   mad r2.z, r0.z, l(1.010000), l(-0.010000)
  15:   mov r3.xyzw, cb12[20].xyzw
  16:   mov r4.xyzw, cb12[21].xyzw
  17:   mov r5.xyzw, cb12[22].xyzw
  18:   mov r6.xyzw, cb12[23].xyzw
  19: endif
  20: mad r1.z, -v0.y, cb2[0].y, l(1.000000)
  21: mad r2.xy, r1.xzxx, l(2.000000, 2.000000, 0.000000, 0.000000), l(-1.000000, -1.000000, 0.000000, 0.000000)
  22: mov r2.w, l(1.000000)
  23: dp4 r1.x, r3.xyzw, r2.xyzw
  24: dp4 r1.y, r4.xyzw, r2.xyzw
  25: dp4 r1.z, r5.xyzw, r2.xyzw
  26: dp4 r0.z, r6.xyzw, r2.xyzw
  27: div r1.xyz, r1.xyzx, r0.zzzz
  28: add r2.xyz, -r1.xyzx, cb2[1].xyzx
  29: dp3 r0.z, r2.xyzx, r2.xyzx
  30: sqrt r0.w, r0.z
  31: div_sat r0.w, r0.w, cb2[1].w
  32: log r0.w, r0.w
  33: mul r0.w, r0.w, cb2[3].z
  34: exp r0.w, r0.w
  35: mad_sat r0.w, cb2[3].y, r0.w, cb2[3].x
  36: add r0.w, -r0.w, l(1.000000)
  37: log r0.w, r0.w
  38: mul r0.w, r0.w, l(2.200000)
  39: exp r0.w, r0.w
  40: ge r1.w, l(0.001000), r0.w
  41: sample_indexable(texture2d)(float,float,float,float) r3.xyzw, r0.xyxx, t2.xyzw, s2
  42: sample_indexable(texture2d)(float,float,float,float) r2.w, r0.xyxx, t0.xyzw, s0
  43: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  44: if_nz r1.w
  45:   mov o0.xyzw, l(0, 0, 0, 0)
  46:   mov o1.xyzw, l(0, 0, 0, 0)
  47:   ret
  48: endif
  49: rsq r0.z, r0.z
  50: mul r2.xyz, r0.zzzz, r2.xyzx
  51: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  52: dp2 r0.z, r0.xyxx, r0.xyxx
  53: mad r4.xy, -r0.zzzz, l(0.250000, 0.500000, 0.000000, 0.000000), l(1.000000, 1.000000, 0.000000, 0.000000)
  54: sqrt r0.z, r4.x
  55: mul r0.xy, r0.zzzz, r0.xyxx
  56: mov r0.z, -r4.y
  57: add r1.w, -r3.x, l(1.000000)
  58: dp3 r4.x, -r1.xyzx, -r1.xyzx
  59: rsq r4.x, r4.x
  60: mul r1.xyz, -r1.xyzx, r4.xxxx
  61: mad r3.w, r3.w, l(255.000000), l(-1.000000)
  62: lt r3.w, abs(r3.w), l(0.250000)
  63: if_nz r3.w
  64:   dp3 r3.w, r3.xyzx, r2.xyzx
  65:   dp3 r3.x, r3.xyzx, r1.xyzx
  66:   mul r3.yz, r3.wwxw, r3.wwxw
  67:   min r3.yz, r3.yyzy, l(0.000000, 1.000000, 1.000000, 0.000000)
  68:   add r3.yz, -r3.yyzy, l(0.000000, 1.000000, 1.000000, 0.000000)
  69:   sqrt r3.yz, r3.yyzy
  70:   dp3 r4.x, r0.xyzx, r2.xyzx
  71:   max r4.x, r4.x, l(0)
  72:   sincos r5.x, r6.x, cb12[29].y
  73:   mul r3.y, r3.y, r5.x
  74:   mad r3.y, -r3.w, r6.x, -r3.y
  75:   mad r3.w, -r3.y, r3.y, l(1.000000)
  76:   sqrt r3.w, r3.w
  77:   mul r3.z, r3.z, r3.w
  78:   mad r3.x, r3.y, r3.x, r3.z
  79:   max r3.x, r3.x, l(0)
  80:   log r3.x, r3.x
  81:   mul r3.x, r3.x, cb12[28].w
  82:   exp r3.x, r3.x
  83:   mad_sat r3.x, cb12[28].z, r3.x, r4.x
  84:   min r2.w, r2.w, r3.x
  85: else
  86:   dp3 r3.x, r1.xyzx, r0.xyzx
  87:   dp3 r3.y, r2.xyzx, r0.xyzx
  88:   mad r4.xyz, -r0.xyzx, r3.xxxx, r1.xyzx
  89:   mad r5.xyz, -r0.xyzx, r3.yyyy, r2.xyzx
  90:   dp3 r3.z, r4.xyzx, r5.xyzx
  91:   mul r3.w, r1.w, r1.w
  92:   mad r4.xy, r1.wwww, r1.wwww, l(0.570000, 0.090000, 0.000000, 0.000000)
  93:   div r4.xy, r3.wwww, r4.xyxx
  94:   mul r3.w, r4.y, l(0.450000)
  95:   mad r4.x, -r4.x, l(0.500000), l(1.000000)
  96:   mad r4.y, -r3.x, r3.x, l(1.000000)
  97:   mad r4.z, -r3.y, r3.y, l(1.000000)
  98:   mul_sat r4.y, r4.z, r4.y
  99:   sqrt r4.y, r4.y
 100:   max r3.x, r3.y, r3.x
 101:   div r3.x, r4.y, r3.x
 102:   max r3.yz, r3.yyzy, l(0, 0, 0, 0)
 103:   mul r3.z, r3.z, r3.w
 104:   mad r3.x, r3.z, r3.x, r4.x
 105:   mul r2.w, r3.x, r3.y
 106: endif
 107: dp3_sat r3.x, r0.xyzx, r2.xyzx
 108: dp3_sat r0.x, r0.xyzx, r1.xyzx
 109: add r0.x, -r0.x, l(1.000000)
 110: log r0.x, r0.x
 111: mul r0.x, r0.x, l(0.010000)
 112: exp r0.x, r0.x
 113: dp3_sat r0.y, r1.xyzx, -r2.xyzx
 114: mul r0.x, r0.y, r0.x
 115: mul r0.x, r3.x, r0.x
 116: mul r0.x, r1.w, r0.x
 117: mul r0.xyz, r0.xxxx, cb2[2].xyzx
 118: mad r0.xyz, cb2[2].xyzx, r2.wwww, r0.xyzx
 119: mul r0.xyz, r0.wwww, r0.xyzx
 120: mov r0.w, l(0)
 121: div o0.xyzw, r0.xyzw, l(3.000000, 3.000000, 3.000000, 3.000000)
 122: mov o1.xyzw, l(0.000000, 0.000000, 0.000000, 1.000000)
 123: ret
