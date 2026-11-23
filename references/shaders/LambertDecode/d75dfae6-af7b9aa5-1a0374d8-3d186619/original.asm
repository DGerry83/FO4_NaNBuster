Shader hash d75dfae6-af7b9aa5-1a0374d8-3d186619

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[28], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s1, mode_default
      dcl_sampler s3, mode_default
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_resource_texture2d (float,float,float,float) t8
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
  28: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  29: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  30: dp2 r0.z, r0.xyxx, r0.xyxx
  31: mad r0.zw, -r0.zzzz, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)
  32: sqrt r0.z, r0.z
  33: mul r2.xy, r0.zzzz, r0.xyxx
  34: mov r2.z, -r0.w
  35: dp3 r0.x, -r1.xyzx, -r1.xyzx
  36: rsq r0.x, r0.x
  37: mov r2.w, l(1.000000)
  38: dp4 r0.y, cb2[6].xyzw, r2.xyzw
  39: dp4 r0.z, cb2[7].xyzw, r2.xyzw
  40: dp4 r0.w, cb2[8].xyzw, r2.xyzw
  41: log r3.x, r0.y
  42: log r3.y, r0.z
  43: log r3.z, r0.w
  44: mul r0.yzw, r3.xxyz, l(0.000000, 2.200000, 2.200000, 2.200000)
  45: exp r0.yzw, r0.yyzw
  46: mad r3.xyz, r1.xyzx, r0.xxxx, l(-1.500000, 1.000000, 0.000000, 0.000000)
  47: dp3_sat r1.w, r3.xyzx, r2.xyzx
  48: mad r1.xyz, r1.xyzx, r0.xxxx, l(1.000000, 1.000000, 0.000000, 0.000000)
  49: dp3_sat r0.x, r1.xyzx, r2.xyzx
  50: dp2_sat r1.x, l(0.125000, -0.750000, 0.000000, 0.000000), r2.yzyy
  51: mul r0.x, r0.x, cb2[21].x
  52: mad r0.x, r1.w, cb2[21].x, r0.x
  53: mad r0.x, r1.x, cb2[21].y, r0.x
  54: dp3 r1.x, r0.yzwy, r0.yzwy
  55: rsq r1.x, r1.x
  56: mul r0.yzw, r0.yyzw, r1.xxxx
  57: mul r0.xyz, r0.yzwy, r0.xxxx
  58: ld_indexable(texture2d)(float,float,float,float) r0.w, l(0, 0, 0, 0), t8.yzwx
  59: mad r0.w, r0.w, l(96.000000), l(1.500000)
  60: mul o0.xyz, r0.wwww, r0.xyzx
  61: mov o0.w, l(1.000000)
  62: mov o1.xyzw, l(0, 0, 0, 0)
  63: ret
