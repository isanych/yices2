(set-logic QF_BV)
(declare-fun _substvar_21_ () (_ BitVec 1))
(declare-fun _substvar_18_ () (_ BitVec 1))
(declare-fun _substvar_38_ () (_ BitVec 1))
(declare-fun _substvar_17_ () (_ BitVec 1))
(declare-fun _substvar_37_ () (_ BitVec 1))
(declare-fun _substvar_19_ () (_ BitVec 1))
(declare-fun _substvar_15_ () (_ BitVec 64))
(declare-fun _substvar_31_ () (_ BitVec 1))
(declare-fun _substvar_30_ () (_ BitVec 1))
(declare-fun _substvar_16_ () (_ BitVec 1))
(assert (= (= _substvar_16_ (_ bv1 1)) false))
(assert (= true (distinct _substvar_15_ (_ bv0 64))))
(assert (= _substvar_18_ _substvar_17_))
(assert (= _substvar_21_ _substvar_19_))
(assert (= _substvar_30_ (_ bv0 1)))
(assert (= (= _substvar_38_ (_ bv1 1)) (bvslt _substvar_15_ ((_ extract 63 0) (_ bv604800 64)))))
(assert (= _substvar_37_ _substvar_38_))
(assert (= (_ bv0 1) (bvand _substvar_31_ _substvar_37_)))
(check-sat)
(exit)
