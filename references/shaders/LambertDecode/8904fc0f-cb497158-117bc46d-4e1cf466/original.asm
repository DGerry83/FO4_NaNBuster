Shader hash 8904fc0f-cb497158-117bc46d-4e1cf466

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
      dcl_temps 9
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
  60: mul r4.yzw, -r1.xxyz, r4.xxxx
  61: dp3 r5.x, r0.xyzx, r2.xyzx
  62: max r5.y, r5.x, l(0)
  63: min r5.z, r5.y, l(1.000000)
  64: mad r3.w, r3.w, l(255.000000), l(-1.000000)
  65: lt r3.w, abs(r3.w), l(0.250000)
  66: if_nz r3.w
  67:   dp3 r3.w, r3.xyzx, r2.xyzx
  68:   dp3 r3.z, r3.xyzx, r4.yzwy
  69:   mul r5.w, r3.w, r3.w
  70:   min r5.w, r5.w, l(1.000000)
  71:   add r5.w, -r5.w, l(1.000000)
  72:   sqrt r5.w, r5.w
  73:   mul r6.x, r3.z, r3.z
  74:   min r6.x, r6.x, l(1.000000)
  75:   add r6.x, -r6.x, l(1.000000)
  76:   sincos r7.x, r8.x, cb12[29].y
  77:   mul r6.y, r5.w, r7.x
  78:   mad r6.y, -r3.w, r8.x, -r6.y
  79:   mad r6.z, -r6.y, r6.y, l(1.000000)
  80:   sqrt r6.xz, r6.xxzx
  81:   mul r6.z, r6.x, r6.z
  82:   mad r6.y, r6.y, r3.z, r6.z
  83:   max r6.y, r6.y, l(0)
  84:   log r6.y, r6.y
  85:   mul r6.y, r6.y, cb12[28].w
  86:   exp r6.y, r6.y
  87:   mad_sat r6.y, cb12[28].z, r6.y, r5.y
  88:   min r2.w, r2.w, r6.y
  89:   sincos r7.x, r8.x, cb12[29].x
  90:   mul r5.w, r5.w, r7.x
  91:   mad r3.w, -r3.w, r8.x, -r5.w
  92:   mad r5.w, -r3.w, r3.w, l(1.000000)
  93:   sqrt r5.w, r5.w
  94:   mul r5.w, r6.x, r5.w
  95:   mad r3.z, r3.w, r3.z, r5.w
  96:   max r3.z, r3.z, l(0)
  97:   log r3.z, r3.z
  98:   mul r3.z, r3.z, cb12[28].y
  99:   exp r3.z, r3.z
 100:   mul r3.z, r3.z, cb12[28].x
 101:   mul r6.xyz, r3.zzzz, cb2[2].xyzx
 102:   mul r6.xyz, r5.zzzz, r6.xyzx
 103: else
 104:   mad r3.x, r3.x, l(10.000000), l(1.000000)
 105:   exp r3.x, r3.x
 106:   dp3 r3.z, r4.yzwy, r0.xyzx
 107:   mad r7.xyz, -r0.xyzx, r3.zzzz, r4.yzwy
 108:   mad r8.xyz, -r0.xyzx, r5.xxxx, r2.xyzx
 109:   dp3 r3.w, r7.xyzx, r8.xyzx
 110:   mul r5.w, r1.w, r1.w
 111:   mad r7.xy, r1.wwww, r1.wwww, l(0.570000, 0.090000, 0.000000, 0.000000)
 112:   div r7.xy, r5.wwww, r7.xyxx
 113:   mul r5.w, r7.y, l(0.450000)
 114:   mad r6.w, -r7.x, l(0.500000), l(1.000000)
 115:   mad r7.x, -r3.z, r3.z, l(1.000000)
 116:   mad r7.y, -r5.x, r5.x, l(1.000000)
 117:   mul_sat r7.x, r7.y, r7.x
 118:   sqrt r7.x, r7.x
 119:   max r5.x, r5.x, r3.z
 120:   div r5.x, r7.x, r5.x
 121:   max r3.w, r3.w, l(0)
 122:   mul r3.w, r3.w, r5.w
 123:   mad r3.w, r3.w, r5.x, r6.w
 124:   mul r2.w, r3.w, r5.y
 125:   mad r1.xyz, -r1.xyzx, r4.xxxx, r2.xyzx
 126:   dp3 r3.w, r1.xyzx, r1.xyzx
 127:   rsq r3.w, r3.w
 128:   mul r1.xyz, r1.xyzx, r3.wwww
 129:   mov_sat r3.z, r3.z
 130:   dp3_sat r3.w, r4.yzwy, r1.xyzx
 131:   dp3_sat r1.x, r1.xyzx, r0.xyzx
 132:   add r1.y, r3.x, l(2.000000)
 133:   mul r1.y, r1.y, l(0.159155)
 134:   log r1.z, r1.x
 135:   mul r1.z, r1.z, r3.x
 136:   exp r1.z, r1.z
 137:   mul r1.y, r1.z, r1.y
 138:   max r1.z, r3.w, l(0.000000)
 139:   min r3.x, r5.z, r3.z
 140:   add r1.x, r1.x, r1.x
 141:   mul r4.x, r1.x, r3.x
 142:   ge r4.x, r1.z, r4.x
 143:   eq r3.x, r3.z, r3.x
 144:   div r5.x, r5.z, r3.z
 145:   movc r3.x, r3.x, l(1.000000), r5.x
 146:   mul r1.x, r1.x, r3.x
 147:   div r1.x, r1.x, r1.z
 148:   div r1.z, l(1.000000, 1.000000, 1.000000, 1.000000), r3.z
 149:   movc r1.x, r4.x, r1.x, r1.z
 150:   add r1.z, -r3.w, l(1.000000)
 151:   mul r3.x, r1.z, r1.z
 152:   mul r3.x, r3.x, r3.x
 153:   mul r3.z, r1.z, r3.x
 154:   mad r1.z, -r1.z, r3.x, l(1.000000)
 155:   mad r1.z, r1.z, l(0.200000), r3.z
 156:   min r1.z, r1.z, l(1.000000)
 157:   mul r1.x, r1.x, r1.z
 158:   mul r1.x, r1.y, r1.x
 159:   mul r1.x, r1.x, l(0.250000)
 160:   min r1.x, r1.x, l(15.000000)
 161:   mul r1.x, r3.y, r1.x
 162:   mul r1.x, r1.x, l(3.141593)
 163:   mul r1.xyz, r1.xxxx, cb2[2].xyzx
 164:   mul r6.xyz, r5.zzzz, r1.xyzx
 165: endif
 166: dp3_sat r0.x, r0.xyzx, r4.yzwy
 167: add r0.x, -r0.x, l(1.000000)
 168: log r0.x, r0.x
 169: mul r0.x, r0.x, l(0.010000)
 170: exp r0.x, r0.x
 171: dp3_sat r0.y, r4.yzwy, -r2.xyzx
 172: mul r0.x, r0.y, r0.x
 173: mul r0.x, r5.z, r0.x
 174: mul r0.x, r1.w, r0.x
 175: mul r0.xyz, r0.xxxx, cb2[2].xyzx
 176: mad r0.xyz, cb2[2].xyzx, r2.wwww, r0.xyzx
 177: mul o1.xyz, r0.wwww, r6.xyzx
 178: mul r0.xyz, r0.wwww, r0.xyzx
 179: mov r0.w, l(0)
 180: div o0.xyzw, r0.xyzw, l(3.000000, 3.000000, 3.000000, 3.000000)
 181: mov o1.w, l(1.000000)
 182: ret
