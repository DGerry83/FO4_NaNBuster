Shader hash 600e22f5-e1a79678-b183a8f0-99de971d

ps_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb12[30], immediateIndexed
      dcl_constantbuffer cb2[23], immediateIndexed
      dcl_sampler s0, mode_default
      dcl_sampler s1, mode_default
      dcl_sampler s2, mode_default
      dcl_sampler s3, mode_default
      dcl_sampler s5, mode_comparison
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t3
      dcl_resource_texture2darray (float,float,float,float) t5
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
  40: ge r2.w, l(0.001000), r0.w
  41: sample_indexable(texture2d)(float,float,float,float) r3.xyzw, r0.xyxx, t2.xyzw, s2
  42: sample_indexable(texture2d)(float,float,float,float) r4.x, r0.xyxx, t0.wxyz, s0
  43: sample_indexable(texture2d)(float,float,float,float) r0.xy, r0.xyxx, t1.xyzw, s1
  44: if_nz r2.w
  45:   mov o0.xyzw, l(0, 0, 0, 0)
  46:   mov o1.xyzw, l(0, 0, 0, 0)
  47:   ret
  48: endif
  49: rsq r0.z, r0.z
  50: mul r2.xyz, r0.zzzz, r2.xyzx
  51: mad r0.xy, r0.xyxx, l(4.000000, 4.000000, 0.000000, 0.000000), l(-2.000000, -2.000000, 0.000000, 0.000000)
  52: dp2 r0.z, r0.xyxx, r0.xyxx
  53: mad r4.yz, -r0.zzzz, l(0.000000, 0.250000, 0.500000, 0.000000), l(0.000000, 1.000000, 1.000000, 0.000000)
  54: sqrt r0.z, r4.y
  55: mul r0.xy, r0.zzzz, r0.xyxx
  56: mov r0.z, -r4.z
  57: add r2.w, -r3.x, l(1.000000)
  58: dp3 r4.y, -r1.xyzx, -r1.xyzx
  59: rsq r4.y, r4.y
  60: mul r5.xyz, -r1.xyzx, r4.yyyy
  61: mov r1.w, l(1.000000)
  62: dp4 r6.x, cb2[11].xyzw, r1.xyzw
  63: dp4 r6.y, cb2[12].xyzw, r1.xyzw
  64: dp4 r6.z, cb2[13].xyzw, r1.xyzw
  65: dp4 r1.w, cb2[14].xyzw, r1.xyzw
  66: mad r4.z, r6.z, l(0.500000), l(0.500000)
  67: div r6.xyz, r6.xyzx, r1.wwww
  68: dp3 r1.w, r6.xyzx, r6.xyzx
  69: sqrt r4.w, r1.w
  70: rsq r1.w, r1.w
  71: lt r5.w, r4.z, l(0)
  72: movc r7.xyz, r5.wwww, l(0.000000, 0.000000, -1.000000, 0.000000), l(0.000000, 0.000000, 1.000000, 0.000000)
  73: mad r6.xyz, r6.xyzx, r1.wwww, r7.xyzx
  74: dp3 r1.w, r6.xyzx, r6.xyzx
  75: rsq r1.w, r1.w
  76: mul r6.xyz, r1.wwww, r6.xyzx
  77: div r6.xy, r6.xyxx, r6.zzzz
  78: div_sat r1.w, r4.w, cb2[1].w
  79: mad r6.xy, r6.xyxx, l(0.500000, 0.500000, 0.000000, 0.000000), l(0.500000, 0.500000, 0.000000, 0.000000)
  80: ge r4.z, r4.z, l(0)
  81: add r4.w, -r6.y, l(1.000000)
  82: movc r6.z, r4.z, r4.w, r6.y
  83: mul r7.xy, r6.xzxx, cb2[20].zzzz
  84: mad r4.w, -r6.z, cb2[20].z, l(1.000000)
  85: movc r4.z, r4.z, r4.w, r7.y
  86: add r6.z, -r4.z, l(1.000000)
  87: mov r6.w, l(1.000000)
  88: mov r4.w, l(0)
  89: movc r7.zw, r5.wwww, r6.zzzw, r4.zzzw
  90: add r1.w, r1.w, -cb2[15].x
  91: sample_c_lz(texture2darray)(float,float,float,float) r1.w, r7.xzwx, t5.xxxx, s5, r1.w
  92: dp3 r4.z, r1.xyzx, r1.xyzx
  93: div_sat r4.z, r4.z, cb2[19].x
  94: mul r4.z, r4.z, r4.z
  95: mul r4.z, r4.z, r4.z
  96: mad r4.z, -r4.z, r4.z, l(1.000000)
  97: mul r1.w, r1.w, r4.z
  98: mul r0.w, r0.w, r1.w
  99: dp3 r1.w, r0.xyzx, r2.xyzx
 100: max r4.z, r1.w, l(0)
 101: min r4.w, r4.z, l(1.000000)
 102: mad r3.w, r3.w, l(255.000000), l(-1.000000)
 103: lt r3.w, abs(r3.w), l(0.250000)
 104: if_nz r3.w
 105:   dp3 r3.w, r3.xyzx, r2.xyzx
 106:   dp3 r3.z, r3.xyzx, r5.xyzx
 107:   mul r5.w, r3.w, r3.w
 108:   min r5.w, r5.w, l(1.000000)
 109:   add r5.w, -r5.w, l(1.000000)
 110:   sqrt r5.w, r5.w
 111:   mul r6.x, r3.z, r3.z
 112:   min r6.x, r6.x, l(1.000000)
 113:   add r6.x, -r6.x, l(1.000000)
 114:   sincos r7.x, r8.x, cb12[29].y
 115:   mul r6.y, r5.w, r7.x
 116:   mad r6.y, -r3.w, r8.x, -r6.y
 117:   mad r6.z, -r6.y, r6.y, l(1.000000)
 118:   sqrt r6.xz, r6.xxzx
 119:   mul r6.z, r6.x, r6.z
 120:   mad r6.y, r6.y, r3.z, r6.z
 121:   max r6.y, r6.y, l(0)
 122:   log r6.y, r6.y
 123:   mul r6.y, r6.y, cb12[28].w
 124:   exp r6.y, r6.y
 125:   mad_sat r6.y, cb12[28].z, r6.y, r4.z
 126:   min r4.x, r4.x, r6.y
 127:   sincos r7.x, r8.x, cb12[29].x
 128:   mul r5.w, r5.w, r7.x
 129:   mad r3.w, -r3.w, r8.x, -r5.w
 130:   mad r5.w, -r3.w, r3.w, l(1.000000)
 131:   sqrt r5.w, r5.w
 132:   mul r5.w, r6.x, r5.w
 133:   mad r3.z, r3.w, r3.z, r5.w
 134:   max r3.z, r3.z, l(0)
 135:   log r3.z, r3.z
 136:   mul r3.z, r3.z, cb12[28].y
 137:   exp r3.z, r3.z
 138:   mul r3.z, r3.z, cb12[28].x
 139:   mul r6.xyz, r3.zzzz, cb2[2].xyzx
 140:   mul r6.xyz, r4.wwww, r6.xyzx
 141: else
 142:   mad r3.x, r3.x, l(10.000000), l(1.000000)
 143:   exp r3.x, r3.x
 144:   dp3 r3.z, r5.xyzx, r0.xyzx
 145:   mad r7.xyz, -r0.xyzx, r3.zzzz, r5.xyzx
 146:   mad r8.xyz, -r0.xyzx, r1.wwww, r2.xyzx
 147:   dp3 r3.w, r7.xyzx, r8.xyzx
 148:   mul r5.w, r2.w, r2.w
 149:   mad r7.xy, r2.wwww, r2.wwww, l(0.570000, 0.090000, 0.000000, 0.000000)
 150:   div r7.xy, r5.wwww, r7.xyxx
 151:   mul r5.w, r7.y, l(0.450000)
 152:   mad r6.w, -r7.x, l(0.500000), l(1.000000)
 153:   mad r7.x, -r3.z, r3.z, l(1.000000)
 154:   mad r7.y, -r1.w, r1.w, l(1.000000)
 155:   mul_sat r7.x, r7.y, r7.x
 156:   sqrt r7.x, r7.x
 157:   max r1.w, r1.w, r3.z
 158:   div r1.w, r7.x, r1.w
 159:   max r3.w, r3.w, l(0)
 160:   mul r3.w, r3.w, r5.w
 161:   mad r1.w, r3.w, r1.w, r6.w
 162:   mul r4.x, r1.w, r4.z
 163:   mad r1.xyz, -r1.xyzx, r4.yyyy, r2.xyzx
 164:   dp3 r1.w, r1.xyzx, r1.xyzx
 165:   rsq r1.w, r1.w
 166:   mul r1.xyz, r1.wwww, r1.xyzx
 167:   mov_sat r3.z, r3.z
 168:   dp3_sat r1.w, r5.xyzx, r1.xyzx
 169:   dp3_sat r1.x, r1.xyzx, r0.xyzx
 170:   add r1.y, r3.x, l(2.000000)
 171:   mul r1.y, r1.y, l(0.159155)
 172:   log r1.z, r1.x
 173:   mul r1.z, r1.z, r3.x
 174:   exp r1.z, r1.z
 175:   mul r1.y, r1.z, r1.y
 176:   max r1.z, r1.w, l(0.000000)
 177:   min r3.x, r4.w, r3.z
 178:   add r1.x, r1.x, r1.x
 179:   mul r3.w, r1.x, r3.x
 180:   ge r3.w, r1.z, r3.w
 181:   eq r3.x, r3.z, r3.x
 182:   div r4.y, r4.w, r3.z
 183:   movc r3.x, r3.x, l(1.000000), r4.y
 184:   mul r1.x, r1.x, r3.x
 185:   div r1.x, r1.x, r1.z
 186:   div r1.z, l(1.000000, 1.000000, 1.000000, 1.000000), r3.z
 187:   movc r1.x, r3.w, r1.x, r1.z
 188:   add r1.z, -r1.w, l(1.000000)
 189:   mul r1.w, r1.z, r1.z
 190:   mul r1.w, r1.w, r1.w
 191:   mul r3.x, r1.w, r1.z
 192:   mad r1.z, -r1.z, r1.w, l(1.000000)
 193:   mad r1.z, r1.z, l(0.200000), r3.x
 194:   min r1.z, r1.z, l(1.000000)
 195:   mul r1.x, r1.x, r1.z
 196:   mul r1.x, r1.y, r1.x
 197:   mul r1.x, r1.x, l(0.250000)
 198:   min r1.x, r1.x, l(15.000000)
 199:   mul r1.x, r3.y, r1.x
 200:   mul r1.x, r1.x, l(3.141593)
 201:   mul r1.xyz, r1.xxxx, cb2[2].xyzx
 202:   mul r6.xyz, r4.wwww, r1.xyzx
 203: endif
 204: dp3_sat r0.x, r0.xyzx, r5.xyzx
 205: add r0.x, -r0.x, l(1.000000)
 206: log r0.x, r0.x
 207: mul r0.x, r0.x, l(0.010000)
 208: exp r0.x, r0.x
 209: dp3_sat r0.y, r5.xyzx, -r2.xyzx
 210: mul r0.x, r0.y, r0.x
 211: mul r0.x, r4.w, r0.x
 212: mul r0.x, r2.w, r0.x
 213: mul r0.xyz, r0.xxxx, cb2[2].xyzx
 214: mad r0.xyz, cb2[2].xyzx, r4.xxxx, r0.xyzx
 215: mul o1.xyz, r0.wwww, r6.xyzx
 216: mul r0.xyz, r0.wwww, r0.xyzx
 217: mov r0.w, l(0)
 218: div o0.xyzw, r0.xyzw, l(3.000000, 3.000000, 3.000000, 3.000000)
 219: mov o1.w, l(1.000000)
 220: ret
