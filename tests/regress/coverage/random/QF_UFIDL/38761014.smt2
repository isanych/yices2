(set-info :source |fuzzsmt|)
(set-info :smt-lib-version 2.0)
(set-info :category "random")
(set-info :status unknown)
(set-logic QF_UFIDL)
(declare-fun v0 () Int)
(declare-fun v1 () Int)
(declare-fun v2 () Int)
(declare-fun v3 () Int)
(declare-fun f0 ( Int Int Int) Int)
(declare-fun p0 ( Int Int) Bool)
(assert (let ((e4 4))
(let ((e5 0))
(let ((e6 0))
(let ((e7 1))
(let ((e8 (> (- v0 v2) (- e5))))
(let ((e9 (>= v2 v3)))
(let ((e10 (> v0 v1)))
(let ((e11 (<= v3 v0)))
(let ((e12 (> v0 v1)))
(let ((e13 (<= (- v0 v1) e7)))
(let ((e14 (= v0 v2)))
(let ((e15 (<= (- v1 v2) e4)))
(let ((e16 (<= v3 v3)))
(let ((e17 (> v0 v0)))
(let ((e18 (< (- v1 v0) e4)))
(let ((e19 (distinct v3 v1)))
(let ((e20 (> (- v3 v0) e4)))
(let ((e21 (distinct v3 v0)))
(let ((e22 (= v0 v1)))
(let ((e23 (= v2 v1)))
(let ((e24 (< (- v3 v3) e4)))
(let ((e25 (distinct v2 v2)))
(let ((e26 (= (- v2 v1) (- e5))))
(let ((e27 (>= (- v1 v2) (- e5))))
(let ((e28 (> (- v3 v2) (- e6))))
(let ((e29 (> v0 v2)))
(let ((e30 (>= (- v1 v2) (- e4))))
(let ((e31 (>= v1 v3)))
(let ((e32 (distinct (- v3 v2) e6)))
(let ((e33 (< (- v2 v0) e4)))
(let ((e34 (>= (- v2 v1) (- e5))))
(let ((e35 (distinct v0 v3)))
(let ((e36 (> (- v0 v0) (- e7))))
(let ((e37 (f0 v0 v2 v3)))
(let ((e38 (f0 v2 v0 v3)))
(let ((e39 (f0 v2 v2 v3)))
(let ((e40 (f0 v3 v0 e38)))
(let ((e41 (f0 v2 v0 v1)))
(let ((e42 (f0 v3 e39 e37)))
(let ((e43 (f0 e42 v2 v3)))
(let ((e44 (f0 e37 e37 e41)))
(let ((e45 (f0 v1 v3 e38)))
(let ((e46 (f0 v0 e40 e43)))
(let ((e47 (f0 v1 v1 v1)))
(let ((e48 (= e41 e41)))
(let ((e49 (p0 e46 v1)))
(let ((e50 (distinct e41 v1)))
(let ((e51 (p0 e47 e39)))
(let ((e52 (p0 e42 e37)))
(let ((e53 (p0 v0 v2)))
(let ((e54 (= e46 e47)))
(let ((e55 (p0 v3 v1)))
(let ((e56 (p0 v0 e37)))
(let ((e57 (p0 e39 v2)))
(let ((e58 (distinct e46 e40)))
(let ((e59 (= v0 e37)))
(let ((e60 (p0 e40 e46)))
(let ((e61 (distinct e42 v2)))
(let ((e62 (= e47 e47)))
(let ((e63 (= v0 v1)))
(let ((e64 (= e43 e44)))
(let ((e65 (distinct e38 e45)))
(let ((e66 (p0 e44 e45)))
(let ((e67 (p0 v3 e46)))
(let ((e68 (p0 e41 v1)))
(let ((e69 (distinct e43 e42)))
(let ((e70 (= v3 e44)))
(let ((e71 (p0 v1 e45)))
(let ((e72 (distinct e42 e40)))
(let ((e73 (= v3 e44)))
(let ((e74 (= e43 e45)))
(let ((e75 (p0 e45 e40)))
(let ((e76 (p0 e40 v2)))
(let ((e77 (p0 e39 e40)))
(let ((e78 (distinct e39 e42)))
(let ((e79 (= e39 e42)))
(let ((e80 (p0 e39 e42)))
(let ((e81 (p0 e39 e38)))
(let ((e82 (p0 v1 e44)))
(let ((e83 (distinct e47 e47)))
(let ((e84 (distinct v2 v2)))
(let ((e85 (= e38 v0)))
(let ((e86 (p0 e46 v1)))
(let ((e87 (p0 v2 e47)))
(let ((e88 (p0 e45 e37)))
(let ((e89 (p0 e47 e37)))
(let ((e90 (distinct e38 e43)))
(let ((e91 (= e43 v0)))
(let ((e92 (p0 e41 e38)))
(let ((e93 (p0 e41 v3)))
(let ((e94 (<= e47 v3)))
(let ((e95 (>= v0 e40)))
(let ((e96 (>= e45 e43)))
(let ((e97 (<= v0 e43)))
(let ((e98 (>= e39 e45)))
(let ((e99 (distinct e41 e38)))
(let ((e100 (<= e47 v0)))
(let ((e101 (= v0 e38)))
(let ((e102 (distinct e43 e43)))
(let ((e103 (distinct e40 v0)))
(let ((e104 (distinct v1 v1)))
(let ((e105 (= e43 e46)))
(let ((e106 (>= e39 e45)))
(let ((e107 (> e47 v0)))
(let ((e108 (> e43 e40)))
(let ((e109 (distinct e44 e44)))
(let ((e110 (distinct e39 e40)))
(let ((e111 (>= v0 v3)))
(let ((e112 (p0 e46 v3)))
(let ((e113 (>= e45 v2)))
(let ((e114 (< e39 e46)))
(let ((e115 (distinct e37 e44)))
(let ((e116 (>= e46 e46)))
(let ((e117 (= v1 e47)))
(let ((e118 (distinct e37 v0)))
(let ((e119 (<= v3 e37)))
(let ((e120 (distinct e42 e43)))
(let ((e121 (p0 e41 v1)))
(let ((e122 (> v3 e41)))
(let ((e123 (= v2 e39)))
(let ((e124 (>= v2 e41)))
(let ((e125 (p0 e46 e47)))
(let ((e126 (p0 e42 v1)))
(let ((e127 (= e37 e38)))
(let ((e128 (<= v1 e43)))
(let ((e129 (>= e41 v0)))
(let ((e130 (= e38 e38)))
(let ((e131 (= e40 e44)))
(let ((e132 (p0 e37 e44)))
(let ((e133 (< e45 e38)))
(let ((e134 (distinct v2 v0)))
(let ((e135 (< e42 e40)))
(let ((e136 (p0 e42 v3)))
(let ((e137 (< v2 v0)))
(let ((e138 (= v2 e38)))
(let ((e139 (distinct e47 e44)))
(let ((e140 (<= e42 e37)))
(let ((e141 (= e115 e137)))
(let ((e142 (ite e69 e29 e128)))
(let ((e143 (or e123 e9)))
(let ((e144 (not e81)))
(let ((e145 (ite e15 e107 e76)))
(let ((e146 (= e112 e90)))
(let ((e147 (not e111)))
(let ((e148 (or e54 e58)))
(let ((e149 (or e95 e62)))
(let ((e150 (or e130 e31)))
(let ((e151 (and e108 e134)))
(let ((e152 (and e87 e88)))
(let ((e153 (= e26 e35)))
(let ((e154 (xor e71 e129)))
(let ((e155 (and e57 e23)))
(let ((e156 (not e113)))
(let ((e157 (=> e101 e147)))
(let ((e158 (=> e97 e106)))
(let ((e159 (ite e102 e157 e94)))
(let ((e160 (= e28 e120)))
(let ((e161 (or e104 e8)))
(let ((e162 (xor e149 e121)))
(let ((e163 (not e86)))
(let ((e164 (or e142 e109)))
(let ((e165 (=> e154 e61)))
(let ((e166 (or e33 e124)))
(let ((e167 (or e22 e55)))
(let ((e168 (=> e162 e126)))
(let ((e169 (=> e14 e132)))
(let ((e170 (not e103)))
(let ((e171 (= e140 e72)))
(let ((e172 (=> e68 e155)))
(let ((e173 (= e30 e119)))
(let ((e174 (xor e166 e79)))
(let ((e175 (or e127 e146)))
(let ((e176 (xor e158 e36)))
(let ((e177 (= e11 e80)))
(let ((e178 (and e172 e177)))
(let ((e179 (= e85 e144)))
(let ((e180 (=> e21 e116)))
(let ((e181 (not e51)))
(let ((e182 (and e64 e171)))
(let ((e183 (ite e159 e20 e143)))
(let ((e184 (or e183 e139)))
(let ((e185 (and e174 e138)))
(let ((e186 (not e173)))
(let ((e187 (=> e105 e163)))
(let ((e188 (= e122 e164)))
(let ((e189 (= e60 e56)))
(let ((e190 (and e169 e25)))
(let ((e191 (xor e12 e131)))
(let ((e192 (or e99 e67)))
(let ((e193 (=> e75 e100)))
(let ((e194 (and e170 e53)))
(let ((e195 (not e27)))
(let ((e196 (xor e178 e50)))
(let ((e197 (and e161 e141)))
(let ((e198 (= e150 e77)))
(let ((e199 (and e18 e152)))
(let ((e200 (and e189 e89)))
(let ((e201 (xor e32 e125)))
(let ((e202 (xor e185 e135)))
(let ((e203 (ite e63 e199 e98)))
(let ((e204 (ite e78 e70 e73)))
(let ((e205 (not e74)))
(let ((e206 (=> e24 e136)))
(let ((e207 (=> e10 e114)))
(let ((e208 (or e193 e34)))
(let ((e209 (not e194)))
(let ((e210 (and e110 e204)))
(let ((e211 (= e93 e186)))
(let ((e212 (not e52)))
(let ((e213 (not e59)))
(let ((e214 (= e66 e82)))
(let ((e215 (xor e180 e196)))
(let ((e216 (or e188 e84)))
(let ((e217 (= e197 e83)))
(let ((e218 (xor e209 e200)))
(let ((e219 (= e217 e156)))
(let ((e220 (ite e207 e181 e96)))
(let ((e221 (xor e215 e208)))
(let ((e222 (ite e167 e203 e92)))
(let ((e223 (= e201 e175)))
(let ((e224 (xor e213 e13)))
(let ((e225 (not e205)))
(let ((e226 (not e184)))
(let ((e227 (ite e220 e182 e133)))
(let ((e228 (= e145 e212)))
(let ((e229 (xor e216 e153)))
(let ((e230 (= e117 e117)))
(let ((e231 (and e230 e226)))
(let ((e232 (ite e151 e221 e223)))
(let ((e233 (=> e118 e231)))
(let ((e234 (=> e148 e65)))
(let ((e235 (=> e224 e190)))
(let ((e236 (or e179 e227)))
(let ((e237 (or e165 e225)))
(let ((e238 (=> e222 e232)))
(let ((e239 (=> e187 e206)))
(let ((e240 (ite e238 e198 e211)))
(let ((e241 (xor e218 e176)))
(let ((e242 (not e219)))
(let ((e243 (=> e49 e234)))
(let ((e244 (ite e214 e214 e202)))
(let ((e245 (xor e237 e239)))
(let ((e246 (xor e19 e243)))
(let ((e247 (or e168 e242)))
(let ((e248 (and e160 e16)))
(let ((e249 (xor e244 e240)))
(let ((e250 (not e192)))
(let ((e251 (not e235)))
(let ((e252 (or e229 e91)))
(let ((e253 (xor e228 e250)))
(let ((e254 (ite e252 e252 e191)))
(let ((e255 (=> e17 e246)))
(let ((e256 (not e210)))
(let ((e257 (and e241 e245)))
(let ((e258 (= e251 e257)))
(let ((e259 (or e256 e236)))
(let ((e260 (= e48 e248)))
(let ((e261 (= e247 e233)))
(let ((e262 (or e259 e253)))
(let ((e263 (xor e258 e249)))
(let ((e264 (not e255)))
(let ((e265 (not e254)))
(let ((e266 (xor e264 e262)))
(let ((e267 (=> e263 e195)))
(let ((e268 (and e261 e261)))
(let ((e269 (xor e268 e268)))
(let ((e270 (=> e260 e266)))
(let ((e271 (ite e270 e270 e265)))
(let ((e272 (not e269)))
(let ((e273 (not e267)))
(let ((e274 (=> e272 e271)))
(let ((e275 (not e274)))
(let ((e276 (= e273 e275)))
e276
))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))

(check-sat)