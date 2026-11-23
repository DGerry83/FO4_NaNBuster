Shader hash def41f6e-9dbcf63b-4c43cf22-4eea0bb4

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[28], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
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
  28: sample_indexable(texture2d)(float,float,float,float) r2.xyz, r0.xyxx, t2.xywz, s2
  29: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  30: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  31: dp2 r0.z, r0.xyxx, r0.xyxx
  32: mad r0.zw, -r0.zzzz, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)
  33: sqrt r0.z, r0.z
  34: mul r3.xy, r0.zzzz, r0.xyxx
  35: mov r3.z, -r0.w
  36: dp3 r0.x, -r1.xyzx, -r1.xyzx
  37: rsq r0.x, r0.x
  38: mul r0.xyz, r0.xxxx, -r1.xyzx
  39: mov r3.w, l(1.000000)
  40: dp4 r0.w, cb2[6].xyzw, r3.xyzw
  41: dp4 r1.x, cb2[7].xyzw, r3.xyzw
  42: dp4 r1.y, cb2[8].xyzw, r3.xyzw
  43: log r4.x, r0.w
  44: log r4.y, r1.x
  45: log r4.z, r1.y
  46: mul r1.xyz, r4.xyzx, l(2.200000, 2.200000, 2.200000, 0.000000)
  47: exp r1.xyz, r1.xyzx
  48: dp3 r0.w, r3.xyzx, r0.xyzx
  49: add r1.w, r0.w, r0.w
  50: mad r3.xyz, r1.wwww, r3.xyzx, -r0.xyzx
  51: mov_sat r0.w, r0.w
  52: add r0.x, -r0.w, l(1.000000)
  53: add r0.y, -r2.x, l(3.000000)
  54: log r0.x, r0.x
  55: mul r0.x, r0.x, r0.y
  56: exp r0.x, r0.x
  57: mul r0.x, r0.x, l(0.250000)
  58: mov r3.w, l(1.000000)
  59: dp4 r0.y, cb2[6].xyzw, r3.xyzw
  60: dp4 r0.z, cb2[7].xyzw, r3.xyzw
  61: dp4 r0.w, cb2[8].xyzw, r3.xyzw
  62: log r3.x, r0.y
  63: log r3.y, r0.z
  64: log r3.z, r0.w
  65: mul r0.yzw, r3.xxyz, l(0.000000, 2.200000, 2.200000, 2.200000)
  66: exp r0.yzw, r0.yyzw
  67: mul r0.xyz, r0.xxxx, r0.yzwy
  68: mad r0.w, r2.z, l(255.000000), l(-1.000000)
  69: lt r0.w, abs(r0.w), l(0.250000)
  70: movc r0.w, r0.w, l(0), r2.y
  71: mul o1.xyz, r0.wwww, r0.xyzx
  72: mov o1.w, l(1.000000)
  73: mul o0.xyz, r1.xyzx, l(0.333333, 0.333333, 0.333333, 0.000000)
  74: mov o0.w, l(0)
  75: ret
