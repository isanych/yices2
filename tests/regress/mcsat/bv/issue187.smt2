(set-logic QF_UFBV)
(declare-const v3 Bool)
(declare-const v6 Bool)
(declare-const v9 Bool)
(declare-const bv_1-0 (_ BitVec 1))
(assert (= true true true (xor v9 true true v6 v3) true (bvslt bv_1-0 (_ bv0 1))))
(check-sat)