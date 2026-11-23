Shader hash 325742ce-45bf17d0-f4879324-3637b6aa

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[30], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_sampler s7, mode_default
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_resource_texture2d (float,float,float,float) t7
      dcl_input_ps_siv linear noperspective v0.xy, position
      dcl_output o0.xyzw
      dcl_output o1.xyzw
      dcl_temps 10
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
  40: ge r2.w, l(0.001000), r0.w
  41: sample_indexable(texture2d)(float,float,float,float) r3.xyzw, r0.xyxx, t2.xyzw, s2
  42: sample_indexable(texture2d)(float,float,float,float) r4.x, r0.xyxx, t0.wxyz, s0
  43: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  44: mov r1.w, l(1.000000)
  45: dp4 r5.x, cb2[11].xyzw, r1.xyzw
  46: dp4 r5.y, cb2[12].xyzw, r1.xyzw
  47: dp4 r5.z, cb2[13].xyzw, r1.xyzw
  48: dp4 r1.w, cb2[14].xyzw, r1.xyzw
  49: mad r4.y, r5.z, l(0.500000), l(0.500000)
  50: div r5.xyz, r5.xyzx, r1.wwww
  51: dp3 r1.w, r5.xyzx, r5.xyzx
  52: rsq r1.w, r1.w
  53: lt r4.y, r4.y, l(0)
  54: movc r6.xyz, r4.yyyy, l(0.000000, 0.000000, -1.000000, 0.000000), l(0.000000, 0.000000, 1.000000, 0.000000)
  55: mad r5.xyz, r5.xyzx, r1.wwww, r6.xyzx
  56: dp3 r1.w, r5.xyzx, r5.xyzx
  57: rsq r1.w, r1.w
  58: mul r5.xyz, r1.wwww, r5.xyzx
  59: div r4.zw, r5.xxxy, r5.zzzz
  60: mad r5.xy, r4.zwzz, l(0.500000, 0.500000, 0.000000, 0.000000), l(0.500000, 0.500000, 0.000000, 0.000000)
  61: add r1.w, -r5.y, l(1.000000)
  62: mad r1.w, r1.w, l(0.500000), l(0.500000)
  63: mul r4.z, r5.y, l(0.500000)
  64: movc r5.z, r4.y, r1.w, r4.z
  65: sample_indexable(texture2d)(float,float,float,float) r4.yzw, r5.xzxx, t7.wxyz, s7
  66: if_nz r2.w
  67:   mov o0.xyzw, l(0, 0, 0, 0)
  68:   mov o1.xyzw, l(0, 0, 0, 0)
  69:   ret
  70: endif
  71: rsq r0.z, r0.z
  72: mul r2.xyz, r0.zzzz, r2.xyzx
  73: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  74: dp2 r0.z, r0.xyxx, r0.xyxx
  75: mad r5.xy, -r0.zzzz, l(0.250000, 0.500000, 0.000000, 0.000000), l(1.000000, 1.000000, 0.000000, 0.000000)
  76: sqrt r0.z, r5.x
  77: mul r0.xy, r0.zzzz, r0.xyxx
  78: mov r0.z, -r5.y
  79: add r1.w, -r3.x, l(1.000000)
  80: dp3 r2.w, -r1.xyzx, -r1.xyzx
  81: rsq r2.w, r2.w
  82: mul r5.xyz, -r1.xyzx, r2.wwww
  83: dp3 r5.w, r0.xyzx, r2.xyzx
  84: max r6.x, r5.w, l(0)
  85: min r6.y, r6.x, l(1.000000)
  86: mad r3.w, r3.w, l(255.000000), l(-1.000000)
  87: lt r3.w, abs(r3.w), l(0.250000)
  88: if_nz r3.w
  89:   dp3 r3.w, r3.xyzx, r2.xyzx
  90:   dp3 r3.z, r3.xyzx, r5.xyzx
  91:   mul r6.zw, r3.wwwz, r3.wwwz
  92:   min r6.zw, r6.zzzw, l(0.000000, 0.000000, 1.000000, 1.000000)
  93:   add r6.zw, -r6.zzzw, l(0.000000, 0.000000, 1.000000, 1.000000)
  94:   sqrt r6.zw, r6.zzzw
  95:   sincos r7.x, r8.x, cb12[29].y
  96:   mul r7.x, r6.z, r7.x
  97:   mad r7.x, -r3.w, r8.x, -r7.x
  98:   mad r7.y, -r7.x, r7.x, l(1.000000)
  99:   sqrt r7.y, r7.y
 100:   mul r7.y, r6.w, r7.y
 101:   mad r7.x, r7.x, r3.z, r7.y
 102:   max r7.x, r7.x, l(0)
 103:   log r7.x, r7.x
 104:   mul r7.x, r7.x, cb12[28].w
 105:   exp r7.x, r7.x
 106:   mad_sat r7.x, cb12[28].z, r7.x, r6.x
 107:   min r4.x, r4.x, r7.x
 108:   sincos r7.x, r8.x, cb12[29].x
 109:   mul r6.z, r6.z, r7.x
 110:   mad r3.w, -r3.w, r8.x, -r6.z
 111:   mad r6.z, -r3.w, r3.w, l(1.000000)
 112:   sqrt r6.z, r6.z
 113:   mul r6.z, r6.w, r6.z
 114:   mad r3.z, r3.w, r3.z, r6.z
 115:   max r3.z, r3.z, l(0)
 116:   log r3.z, r3.z
 117:   mul r3.z, r3.z, cb12[28].y
 118:   exp r3.z, r3.z
 119:   mul r3.z, r3.z, cb12[28].x
 120:   mul r7.xyz, r3.zzzz, cb2[2].xyzx
 121:   mul r7.xyz, r6.yyyy, r7.xyzx
 122: else
 123:   mad r3.x, r3.x, l(10.000000), l(1.000000)
 124:   exp r3.x, r3.x
 125:   dp3 r3.z, r5.xyzx, r0.xyzx
 126:   mad r8.xyz, -r0.xyzx, r3.zzzz, r5.xyzx
 127:   mad r9.xyz, -r0.xyzx, r5.wwww, r2.xyzx
 128:   dp3 r3.w, r8.xyzx, r9.xyzx
 129:   mul r6.z, r1.w, r1.w
 130:   mad r8.xy, r1.wwww, r1.wwww, l(0.570000, 0.090000, 0.000000, 0.000000)
 131:   div r6.zw, r6.zzzz, r8.xxxy
 132:   mul r6.w, r6.w, l(0.450000)
 133:   mad r6.z, -r6.z, l(0.500000), l(1.000000)
 134:   mad r7.w, -r3.z, r3.z, l(1.000000)
 135:   mad r8.x, -r5.w, r5.w, l(1.000000)
 136:   mul_sat r7.w, r7.w, r8.x
 137:   sqrt r7.w, r7.w
 138:   max r5.w, r5.w, r3.z
 139:   div r5.w, r7.w, r5.w
 140:   max r3.w, r3.w, l(0)
 141:   mul r3.w, r3.w, r6.w
 142:   mad r3.w, r3.w, r5.w, r6.z
 143:   mul r4.x, r3.w, r6.x
 144:   mad r1.xyz, -r1.xyzx, r2.wwww, r2.xyzx
 145:   dp3 r2.w, r1.xyzx, r1.xyzx
 146:   rsq r2.w, r2.w
 147:   mul r1.xyz, r1.xyzx, r2.wwww
 148:   mov_sat r3.z, r3.z
 149:   dp3_sat r2.w, r5.xyzx, r1.xyzx
 150:   dp3_sat r1.x, r1.xyzx, r0.xyzx
 151:   add r1.y, r3.x, l(2.000000)
 152:   mul r1.y, r1.y, l(0.159155)
 153:   log r1.z, r1.x
 154:   mul r1.z, r1.z, r3.x
 155:   exp r1.z, r1.z
 156:   mul r1.y, r1.z, r1.y
 157:   max r1.z, r2.w, l(0.000000)
 158:   min r3.x, r6.y, r3.z
 159:   add r1.x, r1.x, r1.x
 160:   mul r3.w, r1.x, r3.x
 161:   ge r3.w, r1.z, r3.w
 162:   eq r3.x, r3.z, r3.x
 163:   div r5.w, r6.y, r3.z
 164:   movc r3.x, r3.x, l(1.000000), r5.w
 165:   mul r1.x, r1.x, r3.x
 166:   div r1.x, r1.x, r1.z
 167:   div r1.z, l(1.000000, 1.000000, 1.000000, 1.000000), r3.z
 168:   movc r1.x, r3.w, r1.x, r1.z
 169:   add r1.z, -r2.w, l(1.000000)
 170:   mul r2.w, r1.z, r1.z
 171:   mul r2.w, r2.w, r2.w
 172:   mul r3.x, r1.z, r2.w
 173:   mad r1.z, -r1.z, r2.w, l(1.000000)
 174:   mad r1.z, r1.z, l(0.200000), r3.x
 175:   min r1.z, r1.z, l(1.000000)
 176:   mul r1.x, r1.x, r1.z
 177:   mul r1.x, r1.y, r1.x
 178:   mul r1.x, r1.x, l(0.250000)
 179:   min r1.x, r1.x, l(15.000000)
 180:   mul r1.x, r3.y, r1.x
 181:   mul r1.x, r1.x, l(3.141593)
 182:   mul r1.xyz, r1.xxxx, cb2[2].xyzx
 183:   mul r7.xyz, r6.yyyy, r1.xyzx
 184: endif
 185: dp3_sat r0.x, r0.xyzx, r5.xyzx
 186: add r0.x, -r0.x, l(1.000000)
 187: log r0.x, r0.x
 188: mul r0.x, r0.x, l(0.010000)
 189: exp r0.x, r0.x
 190: dp3_sat r0.y, r5.xyzx, -r2.xyzx
 191: mul r0.x, r0.y, r0.x
 192: mul r0.x, r6.y, r0.x
 193: mul r0.x, r1.w, r0.x
 194: mul r0.xyz, r0.xxxx, cb2[2].xyzx
 195: mad r0.xyz, cb2[2].xyzx, r4.xxxx, r0.xyzx
 196: mul r0.xyz, r4.yzwy, r0.xyzx
 197: mul r1.xyz, r4.yzwy, r7.xyzx
 198: mul o1.xyz, r0.wwww, r1.xyzx
 199: mul r0.xyz, r0.wwww, r0.xyzx
 200: mov r0.w, l(0)
 201: div o0.xyzw, r0.xyzw, l(3.000000, 3.000000, 3.000000, 3.000000)
 202: mov o1.w, l(1.000000)
 203: ret
