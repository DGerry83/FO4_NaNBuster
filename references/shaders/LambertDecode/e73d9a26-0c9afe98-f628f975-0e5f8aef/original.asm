Shader hash e73d9a26-0c9afe98-f628f975-0e5f8aef

cs_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb0[4], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_uav_typed_texture2d (float,float,float,float) u0
      dcl_input vThreadID.xy
      dcl_temps 9
      dcl_thread_group 16, 16, 1
   0: utof r0.xy, vThreadID.xyxx
   1: add r0.zw, r0.xxxy, r0.xxxy
   2: ftou r1.xy, r0.zwzz
   3: mul r0.xy, r0.xyxx, cb0[2].zwzz
   4: mad r0.xy, cb0[2].zwzz, l(0.500000, 0.500000, 0.000000, 0.000000), r0.xyxx
   5: mul r0.zw, r0.xxxy, cb0[3].xxxy
   6: sample_l(texture2d)(float,float,float,float) r2.z, r0.zwzz, t0.yzxw, s0, l(0)
   7: itof r3.xy, vThreadID.xyxx
   8: add r3.xy, r3.xyxx, l(0.500000, 0.500000, 0.000000, 0.000000)
   9: mad r3.zw, r3.xxxy, cb0[0].xxxy, cb0[0].zzzw
  10: mul r2.xy, r2.zzzz, r3.zwzz
  11: mul_sat r2.w, r2.z, l(0.000143)
  12: mul r3.z, r2.w, l(256.000000)
  13: round_ni r3.z, r3.z
  14: mul r4.y, r3.z, l(0.003906)
  15: mad r4.z, r2.w, l(256.000000), -r3.z
  16: ge r3.z, l(0.500000), r2.w
  17: and r3.z, r3.z, cb0[1].x
  18: imul null, r3.w, vThreadID.x, l(3)
  19: imad r5.x, vThreadID.x, vThreadID.y, vThreadID.y
  20: xor r3.w, r3.w, r5.x
  21: imul null, r3.w, r3.w, l(10)
  22: utof r3.w, r3.w
  23: add r3.z, r3.z, r3.w
  24: mov r1.zw, l(0, 0, 0, 0)
  25: ld_indexable(texture2d)(float,float,float,float) r1.xy, r1.xyzw, t1.xyzw
  26: mad r1.xy, r1.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  27: dp2 r1.z, r1.xyxx, r1.xyxx
  28: mad r5.zw, -r1.zzzz, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)
  29: sqrt r1.z, r5.z
  30: mul r5.xy, r1.zzzz, r1.xyxx
  31: mul r1.xyz, r5.xywx, l(-1.000000, 1.000000, -1.000000, 0.000000)
  32: mul r1.w, cb0[1].y, l(100.000000)
  33: div r1.w, r1.w, r2.z
  34: mad r5.xy, r0.zwzz, l(2.000000, 2.000000, 0.000000, 0.000000), l(-1.000000, -1.000000, 0.000000, 0.000000)
  35: dp2 r3.w, abs(r5.xyxx), abs(r5.xyxx)
  36: add r5.x, r2.w, l(-0.300000)
  37: max r5.x, r5.x, l(0)
  38: mad r5.x, r5.x, l(10.000000), cb0[1].z
  39: mad r3.w, r3.w, l(5.000000), r5.x
  40: mul r5.x, cb0[1].y, cb0[1].y
  41: mov r5.yz, l(0, 0, 0, 0)
  42: loop
  43:   ige r5.w, r5.z, l(5)
  44:   breakc_nz r5.w
  45:   itof r5.w, r5.z
  46:   add r5.w, r5.w, l(0.500000)
  47:   mul r6.x, r1.w, r5.w
  48:   mad r5.w, r5.w, l(2.512000), r3.z
  49:   sincos r7.x, r8.x, r5.w
  50:   mul r5.w, r6.x, l(0.200000)
  51:   log r6.x, r5.w
  52:   round_ni r6.x, r6.x
  53:   ftoi r6.x, r6.x
  54:   iadd r6.x, r6.x, l(-3)
  55:   imax r6.x, r6.x, l(0)
  56:   imin r6.x, r6.x, l(4)
  57:   mov r8.y, r7.x
  58:   mul r6.yz, r5.wwww, r8.xxyx
  59:   ftoi r7.xy, r6.yzyy
  60:   iadd r7.xy, r7.xyxx, vThreadID.xyxx
  61:   round_z r6.yz, r6.yyzy
  62:   add r6.yz, r3.xxyx, r6.yyzy
  63:   mul r6.yz, r6.yyzy, cb0[2].zzwz
  64:   mul r6.yz, r6.yyzy, cb0[3].xxyx
  65:   itof r5.w, r6.x
  66:   sample_l(texture2d)(float,float,float,float) r6.z, r6.yzyy, t0.yzxw, s0, r5.w
  67:   itof r7.xy, r7.xyxx
  68:   add r7.xy, r7.xyxx, l(0.500000, 0.500000, 0.000000, 0.000000)
  69:   mad r7.xy, r7.xyxx, cb0[0].xyxx, cb0[0].zwzz
  70:   mul r6.xy, r6.zzzz, r7.xyxx
  71:   add r6.xyz, -r2.xyzx, r6.xyzx
  72:   dp3 r5.w, r6.xyzx, r6.xyzx
  73:   dp3 r6.x, r6.xyzx, r1.xyzx
  74:   mad r6.y, cb0[1].y, cb0[1].y, -r5.w
  75:   max r6.y, r6.y, l(0)
  76:   mul r6.z, r6.y, r6.y
  77:   mul r6.y, r6.y, r6.z
  78:   add r6.x, -r3.w, r6.x
  79:   add r5.w, r5.w, l(0.010000)
  80:   div r5.w, r6.x, r5.w
  81:   max r5.w, r5.w, l(0)
  82:   mad r5.y, r6.y, r5.w, r5.y
  83:   iadd r5.z, r5.z, l(1)
  84: endloop
  85: mul r1.x, r5.x, cb0[1].y
  86: mul r1.x, r1.x, r1.x
  87: div r1.x, r5.y, r1.x
  88: mad r1.x, -r1.x, cb0[1].w, l(1.000000)
  89: max r1.x, r1.x, l(0)
  90: sample_l(texture2d)(float,float,float,float) r1.yz, r0.zwzz, t3.zxyw, s3, l(0)
  91: dp2 r1.w, r1.yzyy, r1.yzyy
  92: rsq r1.w, r1.w
  93: mul r2.xy, r1.wwww, r1.yzyy
  94: mad r0.zw, r2.xxxy, l(0.000000, 0.000000, 0.055000, 0.055000), r0.zzzw
  95: sample_l(texture2d)(float,float,float,float) r0.zw, r0.zwzz, t3.zwxy, s3, l(0)
  96: mad r0.xy, cb0[3].xyxx, r0.xyxx, r1.yzyy
  97: mul r0.xy, r0.xyxx, cb0[3].zwzz
  98: sample_l(texture2d)(float,float,float,float) r2.xyz, r0.xyxx, t2.xyzw, s2, l(0)
  99: dp2 r0.x, r2.yzyy, l(0.996109, 0.003891, 0.000000, 0.000000)
 100: add r0.x, -r0.x, r2.w
 101: mad r0.x, -abs(r0.x), l(200.000000), l(1.000000)
 102: add r0.yz, -r0.zzwz, r1.yyzy
 103: dp2 r0.y, r0.yzyy, r0.yzyy
 104: sqrt r0.y, r0.y
 105: mad r0.y, -r0.y, l(200.000000), l(1.000000)
 106: max r0.xy, r0.xyxx, l(0.000000, 0.200000, 0.000000, 0.000000)
 107: mul r0.x, r0.x, r0.y
 108: mul r0.x, r0.x, l(0.990000)
 109: add r0.y, -r1.x, r2.x
 110: mad r0.x, r0.x, r0.y, r1.x
 111: ge r0.y, r1.x, l(0.950000)
 112: lt r0.z, r2.x, l(0.700000)
 113: and r0.y, r0.z, r0.y
 114: movc r4.x, r0.y, r1.x, r0.x
 115: mov r4.w, l(1.000000)
 116: store_uav_typed u0.xyzw, vThreadID.xyyy, r4.xyzw
 117: ret
