Shader hash be21279d-733505d8-43f98a86-09502d65

cs_5_0
      dcl_globalFlags refactoringAllowed
      dcl_constantbuffer cb0[4], immediateIndexed
      dcl_constantbuffer cb12[30], immediateIndexed
      dcl_resource_texture2d (float,float,float,float) t0
      dcl_resource_texture2d (float,float,float,float) t1
      dcl_resource_texture2d (float,float,float,float) t2
      dcl_resource_texture2d (float,float,float,float) t4
      dcl_resource_structured t6, 48
      dcl_resource_structured t7, 512
      dcl_uav_typed_texture2d (float,float,float,float) u0
      dcl_uav_typed_texture2d (float,float,float,float) u1
      dcl_input vThreadGroupID.xy
      dcl_input vThreadIDInGroup.xy
      dcl_temps 22
      dcl_thread_group 8, 8, 1
   0: ftou r0.x, cb0[2].z
   1: imad r0.x, vThreadGroupID.y, r0.x, vThreadGroupID.x
   2: imad r0.yz, vThreadGroupID.xxyx, l(0, 8, 8, 0), vThreadIDInGroup.xxyx
   3: ld_structured_indexable(structured_buffer, stride=512)(mixed,mixed,mixed,mixed) r0.w, r0.x, l(0), t7.xxxx
   4: mul r1.x, cb0[2].y, l(0.333333)
   5: umin r1.y, r0.w, l(127)
   6: sincos r2.xy, r3.xy, cb12[29].yxyy
   7: mov r4.x, l(0)
   8: mov r5.w, l(0)
   9: mov r6.zw, l(0, 0, 0, 0)
  10: mov r7.w, l(1.000000)
  11: mov r8.w, l(0)
  12: mov r4.y, l(0)
  13: loop
  14:   uge r1.z, r4.y, l(1)
  15:   breakc_nz r1.z
  16:   iadd r6.xy, r0.yzyy, r4.xyxx
  17:   if_nz r0.w
  18:     ld_indexable(texture2d)(float,float,float,float) r1.z, r6.xyww, t0.yzxw
  19:     ge r1.w, l(0.010000), r1.z
  20:     if_nz r1.w
  21:       mul r7.z, r1.z, l(100.000000)
  22:       mov r9.xyzw, cb12[24].xyzw
  23:       mov r10.xyzw, cb12[25].xyzw
  24:       mov r11.xyzw, cb12[26].xyzw
  25:       mov r12.xyzw, cb12[27].xyzw
  26:     else
  27:       mad r7.z, r1.z, l(1.010000), l(-0.010000)
  28:       mov r9.xyzw, cb12[20].xyzw
  29:       mov r10.xyzw, cb12[21].xyzw
  30:       mov r11.xyzw, cb12[22].xyzw
  31:       mov r12.xyzw, cb12[23].xyzw
  32:     endif
  33:     utof r1.zw, r6.xxxy
  34:     mul r13.x, r1.z, cb0[3].x
  35:     mad r13.z, -cb0[3].y, r1.w, l(1.000000)
  36:     mad r7.xy, r13.xzxx, l(2.000000, 2.000000, 0.000000, 0.000000), l(-1.000000, -1.000000, 0.000000, 0.000000)
  37:     dp4 r9.x, r9.xyzw, r7.xyzw
  38:     dp4 r9.y, r10.xyzw, r7.xyzw
  39:     dp4 r9.z, r11.xyzw, r7.xyzw
  40:     dp4 r1.z, r12.xyzw, r7.xyzw
  41:     div r7.xyz, r9.xyzx, r1.zzzz
  42:     ld_indexable(texture2d)(float,float,float,float) r1.zw, r6.xyww, t2.zwxy
  43:     mad r1.zw, r1.zzzw, l(0.000000, 0.000000, 4.000000, 4.000000), l(0.000000, 0.000000, -2.000000, -2.000000)
  44:     dp2 r2.z, r1.zwzz, r1.zwzz
  45:     mad r2.zw, -r2.zzzz, l(0.000000, 0.000000, 0.250000, 0.500000), l(0.000000, 0.000000, 1.000000, 1.000000)
  46:     sqrt r2.z, r2.z
  47:     mul r9.xy, r1.zwzz, r2.zzzz
  48:     mov r9.z, -r2.w
  49:     dp3 r1.z, -r7.xyzx, -r7.xyzx
  50:     rsq r1.z, r1.z
  51:     mul r10.xyz, r1.zzzz, -r7.xyzx
  52:     ld_indexable(texture2d)(float,float,float,float) r11.xyzw, r6.xyww, t4.xyzw
  53:     add r1.w, -r11.x, l(1.000000)
  54:     mul r2.z, r11.y, l(3.141593)
  55:     mad r3.zw, r11.wwwx, l(0.000000, 0.000000, 255.000000, 10.000000), l(0.000000, 0.000000, -1.000000, 1.000000)
  56:     lt r2.w, abs(r3.z), l(0.250000)
  57:     ld_indexable(texture2d)(float,float,float,float) r3.z, r6.xyzw, t1.xywz
  58:     exp r3.w, r3.w
  59:     dp3 r4.z, r11.xyzx, r10.xyzx
  60:     mul r4.w, r4.z, r4.z
  61:     min r4.w, r4.w, l(1.000000)
  62:     add r4.w, -r4.w, l(1.000000)
  63:     sqrt r4.w, r4.w
  64:     dp3 r9.w, r9.xyzx, r10.xyzx
  65:     mad r12.xyz, -r9.xyzx, r9.wwww, r10.xyzx
  66:     mad r10.w, -r9.w, r9.w, l(1.000000)
  67:     mov_sat r11.w, r9.w
  68:     add r12.w, r3.w, l(2.000000)
  69:     mul r12.w, r12.w, l(0.159155)
  70:     div r13.x, l(1.000000, 1.000000, 1.000000, 1.000000), r11.w
  71:     add r13.y, -r11.w, l(1.000000)
  72:     log r13.y, r13.y
  73:     mul r13.y, r13.y, l(0.010000)
  74:     exp r13.y, r13.y
  75:     mov r14.xyz, l(0, 0, 0, 0)
  76:     mov r8.xyz, l(0, 0, 0, 0)
  77:     mov r13.z, l(0)
  78:     loop
  79:       uge r13.w, r13.z, r1.y
  80:       breakc_nz r13.w
  81:       ishl r13.w, r13.z, l(2)
  82:       iadd r13.w, r13.w, l(4)
  83:       ld_structured_indexable(structured_buffer, stride=512)(mixed,mixed,mixed,mixed) r13.w, r0.x, r13.w, t7.xxxx
  84:       ld_structured_indexable(structured_buffer, stride=48)(mixed,mixed,mixed,mixed) r15.xyzw, r13.w, l(0), t6.xyzw
  85:       ld_structured_indexable(structured_buffer, stride=48)(mixed,mixed,mixed,mixed) r16.xyzw, r13.w, l(16), t6.yzwx
  86:       ld_structured_indexable(structured_buffer, stride=48)(mixed,mixed,mixed,mixed) r17.xyz, r13.w, l(32), t6.xyzx
  87:       add r15.yzw, -r7.xxyz, r15.yyzw
  88:       dp3 r13.w, r15.yzwy, r15.yzwy
  89:       and r14.w, r15.x, l(8)
  90:       if_z r14.w
  91:         and r14.w, r15.x, l(4)
  92:         rsq r17.w, r13.w
  93:         mul r15.yzw, r15.yyzw, r17.wwww
  94:         dp3 r17.w, r9.xyzx, r15.yzwy
  95:         max r18.x, r17.w, l(0)
  96:         min r18.yzw, r18.xxxx, l(0.000000, 1.000000, 1.000000, 1.000000)
  97:         if_nz r2.w
  98:           dp3 r19.x, r11.xyzx, r15.yzwy
  99:           mul r19.y, r19.x, r19.x
 100:           min r19.y, r19.y, l(1.000000)
 101:           add r19.y, -r19.y, l(1.000000)
 102:           sqrt r19.y, r19.y
 103:           mul r19.z, r2.x, r19.y
 104:           mad r19.z, -r19.x, r3.x, -r19.z
 105:           mad r19.w, -r19.z, r19.z, l(1.000000)
 106:           sqrt r19.w, r19.w
 107:           mul r19.w, r4.w, r19.w
 108:           mad r19.z, r19.z, r4.z, r19.w
 109:           max r19.z, r19.z, l(0)
 110:           log r19.z, r19.z
 111:           mul r19.z, r19.z, cb12[28].w
 112:           exp r19.z, r19.z
 113:           mad_sat r19.z, cb12[28].z, r19.z, r18.x
 114:           min r19.z, r3.z, r19.z
 115:           mul r20.xyz, r16.xyzx, r19.zzzz
 116:           and r19.z, r15.x, l(16)
 117:           if_nz r19.z
 118:             mul r19.y, r2.y, r19.y
 119:             mad r19.x, -r19.x, r3.y, -r19.y
 120:             mad r19.y, -r19.x, r19.x, l(1.000000)
 121:             sqrt r19.y, r19.y
 122:             mul r19.y, r4.w, r19.y
 123:             mad r19.x, r19.x, r4.z, r19.y
 124:             max r19.x, r19.x, l(0)
 125:             log r19.x, r19.x
 126:             mul r19.x, r19.x, cb12[28].y
 127:             exp r19.x, r19.x
 128:             mul r19.x, r19.x, cb12[28].x
 129:             mul r19.xyz, r16.xyzx, r19.xxxx
 130:             mul r19.xyz, r18.yzwy, r19.xyzx
 131:           else
 132:             mov r19.xyz, l(0, 0, 0, 0)
 133:           endif
 134:         else
 135:           and r21.xy, r15.xxxx, l(2, 16, 0, 0)
 136:           movc r15.x, r21.x, l(0), r1.w
 137:           ne r19.w, r15.x, l(0)
 138:           if_nz r19.w
 139:             mad r21.xzw, -r9.xxyz, r17.wwww, r15.yyzw
 140:             dp3 r19.w, r12.xyzx, r21.xzwx
 141:             mul r20.w, r15.x, r15.x
 142:             mad r21.xz, r15.xxxx, r15.xxxx, l(0.570000, 0.000000, 0.090000, 0.000000)
 143:             div r21.xz, r20.wwww, r21.xxzx
 144:             mul r15.x, r21.z, l(0.450000)
 145:             mad r20.w, -r21.x, l(0.500000), l(1.000000)
 146:             mad r21.x, -r17.w, r17.w, l(1.000000)
 147:             mul_sat r21.x, r10.w, r21.x
 148:             sqrt r21.x, r21.x
 149:             max r17.w, r9.w, r17.w
 150:             div r17.w, r21.x, r17.w
 151:             max r19.w, r19.w, l(0)
 152:             mul r15.x, r15.x, r19.w
 153:             mad r15.x, r15.x, r17.w, r20.w
 154:             mul r21.xzw, r15.xxxx, r18.xxxx
 155:           else
 156:             mov r21.xzw, r18.yyzw
 157:           endif
 158:           mul r20.xyz, r16.xyzx, r21.xzwx
 159:           if_nz r21.y
 160:             mad r18.xyz, -r7.xyzx, r1.zzzz, r15.yzwy
 161:             dp3 r15.x, r18.xyzx, r18.xyzx
 162:             rsq r15.x, r15.x
 163:             mul r18.xyz, r15.xxxx, r18.xyzx
 164:             dp3_sat r15.x, r10.xyzx, r18.xyzx
 165:             dp3_sat r17.w, r18.xyzx, r9.xyzx
 166:             log r18.x, r17.w
 167:             mul r18.x, r3.w, r18.x
 168:             exp r18.x, r18.x
 169:             mul r18.x, r12.w, r18.x
 170:             max r18.y, r15.x, l(0.000000)
 171:             min r18.z, r11.w, r18.w
 172:             add r17.w, r17.w, r17.w
 173:             mul r19.w, r17.w, r18.z
 174:             ge r19.w, r18.y, r19.w
 175:             eq r18.z, r11.w, r18.z
 176:             div r20.w, r18.w, r11.w
 177:             movc r18.z, r18.z, l(1.000000), r20.w
 178:             mul r17.w, r17.w, r18.z
 179:             div r17.w, r17.w, r18.y
 180:             movc r17.w, r19.w, r17.w, r13.x
 181:             add r15.x, -r15.x, l(1.000000)
 182:             mul r18.y, r15.x, r15.x
 183:             mul r18.y, r18.y, r18.y
 184:             mul r18.z, r15.x, r18.y
 185:             mad r15.x, -r15.x, r18.y, l(1.000000)
 186:             mad r15.x, r15.x, l(0.200000), r18.z
 187:             min r15.x, r15.x, l(1.000000)
 188:             mul r15.x, r17.w, r15.x
 189:             mul r15.x, r18.x, r15.x
 190:             mul r15.x, r15.x, l(0.250000)
 191:             min r15.x, r15.x, l(15.000000)
 192:             mul r15.x, r2.z, r15.x
 193:             mul r18.xyz, r15.xxxx, r16.xyzx
 194:             mul r19.xyz, r18.wwww, r18.xyzx
 195:           else
 196:             mov r19.xyz, l(0, 0, 0, 0)
 197:           endif
 198:         endif
 199:         if_z r14.w
 200:           dp3_sat r14.w, r10.xyzx, -r15.yzwy
 201:           mul r14.w, r13.y, r14.w
 202:           mul r14.w, r18.w, r14.w
 203:           mul r14.w, r1.w, r14.w
 204:           mad r16.xyz, r16.xyzx, r14.wwww, r20.xyzx
 205:         else
 206:           mov r16.xyz, r20.xyzx
 207:         endif
 208:       else
 209:         mov r19.xyz, l(0, 0, 0, 0)
 210:       endif
 211:       sqrt r13.w, r13.w
 212:       div_sat r13.w, r13.w, r16.w
 213:       log r13.w, r13.w
 214:       mul r13.w, r13.w, r17.z
 215:       exp r13.w, r13.w
 216:       mad_sat r13.w, r17.y, r13.w, r17.x
 217:       add r13.w, -r13.w, l(1.000000)
 218:       log r13.w, r13.w
 219:       mul r13.w, r13.w, l(2.200000)
 220:       exp r13.w, r13.w
 221:       mad r14.xyz, r16.xyzx, r13.wwww, r14.xyzx
 222:       mad r8.xyz, r19.xyzx, r13.wwww, r8.xyzx
 223:       iadd r13.z, r13.z, l(1)
 224:     endloop
 225:   else
 226:     mov r14.xyz, l(0, 0, 0, 0)
 227:     mov r8.xyz, l(0, 0, 0, 0)
 228:   endif
 229:   mul r5.xyz, r1.xxxx, r14.xyzx
 230:   store_uav_typed u0.xyzw, r6.xyyy, r5.xyzw
 231:   store_uav_typed u1.xyzw, r6.xyyy, r8.xyzw
 232:   iadd r4.y, r4.y, l(1)
 233: endloop
 234: ret
